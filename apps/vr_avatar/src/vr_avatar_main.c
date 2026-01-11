/**
 * @file vr_avatar_main.c
 * @brief VR Avatar Project - Final Integration
 */

#include "tal_api.h"
#include "tkl_output.h"
#include "netmgr.h"
#include "netconn_wifi.h"

#include "tdl_display_manage.h"
#include "tdl_display_draw.h"
#include "board_com_api.h"
#include "tkl_dma2d.h"
#include <string.h>

#include "emotion_manager.h"
#include "camera_manager.h"
#include "ai_manager.h"

// --- CONFIGURATION ---
#define WIFI_SSID     "JJ Lake"
#define WIFI_PASSWORD "20220315"

#ifndef DISPLAY_NAME
#define DISPLAY_NAME "lcd_disp"
#endif

// --- GLOBALS ---
TDL_DISP_HANDLE_T   sg_tdl_disp_hdl = NULL;
TDL_DISP_DEV_INFO_T sg_display_info;

// CRITICAL FIX: 'volatile' ensures the main loop sees updates from the camera thread!
static volatile ai_face_result_t sg_face_result = {0};

// Double Buffering
TDL_DISP_FRAME_BUFF_T *sg_p_display_fb_1 = NULL;
TDL_DISP_FRAME_BUFF_T *sg_p_display_fb_2 = NULL;
TDL_DISP_FRAME_BUFF_T *sg_p_display_fb   = NULL; // The active buffer

// DMA2D Globals
static TKL_DMA2D_FRAME_INFO_T sg_dma_in  = {0};
static TKL_DMA2D_FRAME_INFO_T sg_dma_out = {0};
static SEM_HANDLE             sg_dma_sem = NULL;

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
    // 1. Run AI (Updates the volatile global variable)
    // We cast away volatile locally just to pass it to the function
    ai_process_frame(frame, (ai_face_result_t *)&sg_face_result);

    // 2. If Phase 1 (Viewfinder), Draw Camera to Screen
    if (sg_show_camera && sg_p_display_fb != NULL) {
        sg_dma_in.type   = TUYA_FRAME_FMT_YUV422;
        sg_dma_in.width  = frame->width;
        sg_dma_in.height = frame->height;
        sg_dma_in.pbuf   = frame->data;

        sg_dma_out.type   = TUYA_FRAME_FMT_RGB565;
        sg_dma_out.width  = sg_p_display_fb->width;
        sg_dma_out.height = sg_p_display_fb->height;
        sg_dma_out.pbuf   = sg_p_display_fb->frame;

        tkl_dma2d_convert(&sg_dma_in, &sg_dma_out);
        tal_semaphore_wait(sg_dma_sem, 100);

        if (sg_display_info.is_swap) {
            tdl_disp_dev_rgb565_swap((uint16_t *)sg_p_display_fb->frame, sg_p_display_fb->len / 2);
        }

        tdl_disp_dev_flush(sg_tdl_disp_hdl, sg_p_display_fb);

        // Swap Buffers for next frame (Ping-Pong)
        sg_p_display_fb = (sg_p_display_fb == sg_p_display_fb_1) ? sg_p_display_fb_2 : sg_p_display_fb_1;
    }

    return OPRT_OK;
}

// --- DISPLAY INIT ---
static int init_display(void)
{
    OPERATE_RET rt = OPRT_OK;
    PR_NOTICE("--- Initializing Display ---");

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

    // Create 2 Buffers
    uint32_t frame_len = CAM_WIDTH * CAM_HEIGHT * 2;
    sg_p_display_fb_1  = tdl_disp_create_frame_buff(DISP_FB_TP_PSRAM, frame_len);
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
    sg_p_display_fb = sg_p_display_fb_1;

    return 0;
}

// --- MAIN ---
void user_main(void)
{
    tal_log_init(TAL_LOG_LEVEL_DEBUG, 4096, (TAL_LOG_OUTPUT_CB)tkl_log_output);
    PR_NOTICE("%s", "=== VR Avatar App Starting ===");

    if (init_display() != 0) {
        PR_ERR("Display Fail");
        return;
    }
    init_dma2d();
    emotion_manager_init();
    ai_init();

    if (camera_init(camera_to_screen_cb) != 0) {
        PR_ERR("Camera Fail");
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

    // PHASE 1: CAMERA (10s)
    PR_NOTICE(">>> PHASE 1: VIEWFINDER <<<");
    sg_show_camera = true;
    for (int i = 0; i < 10; i++) {
        PR_NOTICE("Viewfinder: %d/10 - X:%d Y:%d", i + 1, sg_face_result.x, sg_face_result.y);
        tal_system_sleep(1000);
    }

    PR_NOTICE(">>> PHASE 2: AVATAR MODE STARTING <<<");
    sg_show_camera = false;

    // PHASE 2: AVATAR LOOP
    while (1) {
        // 1. Calculate Draw Position
        // Screen center + AI offset
        int draw_x = (sg_screen_width / 2) + sg_face_result.x;
        int draw_y = (sg_screen_height / 2) + sg_face_result.y;

        // Clamp to edges
        if (draw_x < 60)
            draw_x = 60;
        if (draw_x > sg_screen_width - 60)
            draw_x = sg_screen_width - 60;
        if (draw_y < 60)
            draw_y = 60;
        if (draw_y > sg_screen_height - 60)
            draw_y = sg_screen_height - 60;

        // 2. Update Emotion
        if (sg_face_result.emotion_id != 0) {
            emotion_set_current((emotion_type_t)sg_face_result.emotion_id);
        }

        // 3. DRAW & FLUSH (With Double Buffering!)
        if (sg_p_display_fb != NULL) {
            // A. Clear current buffer
            memset(sg_p_display_fb->frame, 0x00, sg_p_display_fb->len);

            // B. Draw Face
            emotion_draw_at(draw_x, draw_y);

            // C. Log (Helpful for debugging!)
            PR_NOTICE("Avatar: X%d Y%d E%d", draw_x, draw_y, sg_face_result.emotion_id);

            // D. Flush
            tdl_disp_dev_flush(sg_tdl_disp_hdl, sg_p_display_fb);

            // E. Swap Buffers (Fixes flickering)
            sg_p_display_fb = (sg_p_display_fb == sg_p_display_fb_1) ? sg_p_display_fb_2 : sg_p_display_fb_1;
        }

        tal_system_sleep(50); // 20 FPS
    }
}

// ... (Linux/Thread wrappers remain the same) ...
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