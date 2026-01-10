/**
 * @file vr_avatar_main.c
 * @brief VR Avatar Project - Camera Viewfinder (Double Buffered)
 */

#include "tal_api.h"
#include "tkl_output.h"
#include "netmgr.h"
#include "netconn_wifi.h"

// --- DISPLAY & GRAPHICS INCLUDES ---
#include "tdl_display_manage.h"
#include "tdl_display_draw.h"
#include "board_com_api.h"
#include "tkl_dma2d.h"
#include <string.h>

// --- APP INCLUDES ---
#include "emotion_manager.h"
#include "camera_manager.h"

// --- CONFIGURATION ---
#define WIFI_SSID     "JJ Lake"
#define WIFI_PASSWORD "20220315"

#ifndef DISPLAY_NAME
#define DISPLAY_NAME "lcd_disp"
#endif

// --- GLOBALS ---
TDL_DISP_HANDLE_T   sg_tdl_disp_hdl = NULL;
TDL_DISP_DEV_INFO_T sg_display_info;

// Double Buffering (Ping-Pong)
TDL_DISP_FRAME_BUFF_T *sg_p_display_fb_1 = NULL;
TDL_DISP_FRAME_BUFF_T *sg_p_display_fb_2 = NULL;

// FIX: Renamed back to 'sg_p_display_fb' so emotion_manager.c can find it!
TDL_DISP_FRAME_BUFF_T *sg_p_display_fb = NULL;

// DMA2D Globals
static TKL_DMA2D_FRAME_INFO_T sg_dma_in  = {0};
static TKL_DMA2D_FRAME_INFO_T sg_dma_out = {0};
static SEM_HANDLE             sg_dma_sem = NULL;

// State Flags
static volatile bool sg_show_camera   = false;
static uint16_t      sg_screen_width  = 0;
static uint16_t      sg_screen_height = 0;

// --- DMA2D SETUP ---
static void __dma2d_irq_cb(TUYA_DMA2D_IRQ_E type, VOID_T *args)
{
    if (sg_dma_sem)
        tal_semaphore_post(sg_dma_sem);
}

static int init_dma2d(void)
{
    OPERATE_RET ret = tal_semaphore_create_init(&sg_dma_sem, 0, 1);
    if (ret != OPRT_OK)
        return ret;

    TUYA_DMA2D_BASE_CFG_T dma_cfg = {.cb = __dma2d_irq_cb, .arg = NULL};
    return tkl_dma2d_init(&dma_cfg);
}

// --- CAMERA CALLBACK ---
OPERATE_RET camera_to_screen_cb(TDL_CAMERA_HANDLE_T hdl, TDL_CAMERA_FRAME_T *frame)
{
    if (!sg_show_camera)
        return OPRT_OK;
    if (sg_p_display_fb == NULL)
        return OPRT_COM_ERROR;

    // 1. Setup Input (Camera YUV422)
    sg_dma_in.type   = TUYA_FRAME_FMT_YUV422;
    sg_dma_in.width  = frame->width;
    sg_dma_in.height = frame->height;
    sg_dma_in.pbuf   = frame->data;

    // 2. Setup Output (Active Framebuffer RGB565)
    // We draw to whichever buffer sg_p_display_fb is currently pointing to
    sg_dma_out.type   = TUYA_FRAME_FMT_RGB565;
    sg_dma_out.width  = sg_p_display_fb->width;
    sg_dma_out.height = sg_p_display_fb->height;
    sg_dma_out.pbuf   = sg_p_display_fb->frame;

    // 3. Run Hardware Conversion
    tkl_dma2d_convert(&sg_dma_in, &sg_dma_out);
    tal_semaphore_wait(sg_dma_sem, 100);

    // 4. Swap Bytes (Endianness Fix)
    if (sg_display_info.is_swap) {
        tdl_disp_dev_rgb565_swap((uint16_t *)sg_p_display_fb->frame, sg_p_display_fb->len / 2);
    }

    // 5. Blast to Screen
    tdl_disp_dev_flush(sg_tdl_disp_hdl, sg_p_display_fb);

    // 6. Swap Buffers (Ping-Pong)
    // Toggle the pointer between fb_1 and fb_2
    sg_p_display_fb = (sg_p_display_fb == sg_p_display_fb_1) ? sg_p_display_fb_2 : sg_p_display_fb_1;

    return OPRT_OK;
}

// --- DISPLAY INIT ---
static int init_display(void)
{
    OPERATE_RET rt = OPRT_OK;
    PR_NOTICE("--- Initializing Display (Double Buffered) ---");

    board_register_hardware();

    sg_tdl_disp_hdl = tdl_disp_find_dev(DISPLAY_NAME);
    if (NULL == sg_tdl_disp_hdl)
        return -1;

    rt = tdl_disp_dev_get_info(sg_tdl_disp_hdl, &sg_display_info);
    if (rt != OPRT_OK)
        return -1;

    rt = tdl_disp_dev_open(sg_tdl_disp_hdl);
    if (rt != OPRT_OK)
        return -1;

    sg_screen_width  = sg_display_info.width;
    sg_screen_height = sg_display_info.height;
    tdl_disp_set_brightness(sg_tdl_disp_hdl, 100);

    // Create TWO Frame Buffers
    uint32_t frame_len = CAM_WIDTH * CAM_HEIGHT * 2;

    sg_p_display_fb_1 = tdl_disp_create_frame_buff(DISP_FB_TP_PSRAM, frame_len);
    if (sg_p_display_fb_1) {
        sg_p_display_fb_1->fmt    = sg_display_info.fmt;
        sg_p_display_fb_1->width  = CAM_WIDTH;
        sg_p_display_fb_1->height = CAM_HEIGHT;
    }

    sg_p_display_fb_2 = tdl_disp_create_frame_buff(DISP_FB_TP_PSRAM, frame_len);
    if (sg_p_display_fb_2) {
        sg_p_display_fb_2->fmt    = sg_display_info.fmt;
        sg_p_display_fb_2->width  = CAM_WIDTH;
        sg_p_display_fb_2->height = CAM_HEIGHT;
    }

    sg_p_display_fb = sg_p_display_fb_1; // Initialize the global pointer

    return 0;
}

// --- MAIN ---
void user_main(void)
{
    tal_log_init(TAL_LOG_LEVEL_DEBUG, 4096, (TAL_LOG_OUTPUT_CB)tkl_log_output);
    PR_NOTICE("%s", "=== VR Avatar App Starting ===");

    if (init_display() != 0) {
        PR_ERR("Display Init Failed");
        return;
    }
    init_dma2d();
    emotion_manager_init();

    if (camera_init(camera_to_screen_cb) != 0) {
        PR_ERR("Camera Init Failed");
    }

    // Connect WiFi
    tal_kv_init(&(tal_kv_cfg_t){.seed = "vmlkasdh93dlvlcy", .key = "dflfuap134ddlduq"});
    tal_sw_timer_init();
    tal_workq_init();
    netmgr_init(NETCONN_WIFI);
    netconn_wifi_info_t wifi_info = {0};
    strcpy(wifi_info.ssid, WIFI_SSID);
    strcpy(wifi_info.pswd, WIFI_PASSWORD);
    netmgr_conn_set(NETCONN_WIFI, NETCONN_CMD_SSID_PSWD, &wifi_info);

    // SHOW CAMERA
    PR_NOTICE(">>> CAMERA STARTING: Look at the screen! <<<");
    sg_show_camera = true;

    for (int i = 0; i < 20; i++) {
        PR_NOTICE("Camera Viewfinder: %d/20 seconds...", i + 1);
        tal_system_sleep(1000);
    }

    PR_NOTICE(">>> Camera Done. Switching to Avatar. <<<");
    sg_show_camera = false;

    // CLEAR SCREEN MANUALLY
    if (sg_p_display_fb != NULL) {
        memset(sg_p_display_fb->frame, 0x00, sg_p_display_fb->len);
        tdl_disp_dev_flush(sg_tdl_disp_hdl, sg_p_display_fb);
    }

    while (1) {
        emotion_type_t new_emotion = emotion_next();
        PR_NOTICE("Emotion: %s", emotion_get_name(new_emotion));
        emotion_draw_current(sg_screen_width, sg_screen_height);
        tal_system_sleep(3000);
    }
}

#if OPERATING_SYSTEM == SYSTEM_LINUX
void main(int argc, char *argv[])
{
    user_main();
}
#else
static THREAD_HANDLE ty_app_thread = NULL;
static void          tuya_app_thread(void *arg)
{
    user_main();
    tal_thread_delete(ty_app_thread);
    ty_app_thread = NULL;
}
void tuya_app_main(void)
{
    THREAD_CFG_T thrd_param = {4096, 4, "tuya_app_main"};
    tal_thread_create_and_start(&ty_app_thread, NULL, NULL, tuya_app_thread, NULL, &thrd_param);
}
#endif