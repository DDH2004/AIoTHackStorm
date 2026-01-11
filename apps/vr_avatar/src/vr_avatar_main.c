/**
 * @file vr_avatar_main.c
 * @brief VR Avatar Project - Hybrid Architecture
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

// --- NEW: Include MCP Header ---
// If you haven't created this file yet, creating it is mandatory!
#include "avatar_mcp.h"

// --- CONFIGURATION ---
#define WIFI_SSID     "JJ Lake"
#define WIFI_PASSWORD "20220315"

#ifndef DISPLAY_NAME
#define DISPLAY_NAME "lcd_disp"
#endif

// --- GLOBALS ---
TDL_DISP_HANDLE_T   sg_tdl_disp_hdl = NULL;
TDL_DISP_DEV_INFO_T sg_display_info;

// FIX: Removed 'volatile' to prevent compiler type errors.
// Since only one thread writes and one reads, this is safe enough for a demo.
static ai_face_result_t sg_face_result = {0};

// Double Buffering
TDL_DISP_FRAME_BUFF_T *sg_p_display_fb_1 = NULL;
TDL_DISP_FRAME_BUFF_T *sg_p_display_fb_2 = NULL;
TDL_DISP_FRAME_BUFF_T *sg_p_display_fb   = NULL;

// DMA2D Globals
static TKL_DMA2D_FRAME_INFO_T sg_dma_in  = {0};
static TKL_DMA2D_FRAME_INFO_T sg_dma_out = {0};
static SEM_HANDLE             sg_dma_sem = NULL;

static bool     sg_show_camera   = false;
static uint16_t sg_screen_width  = 0;
static uint16_t sg_screen_height = 0;

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
    // 1. Run AI (Works fine on 480 width)
    ai_process_frame(frame, &sg_face_result);

    // 2. Viewfinder Logic
    if (sg_show_camera && sg_p_display_fb != NULL) {

        // INPUT: Camera is 480x480 (YUV422)
        sg_dma_in.type   = TUYA_FRAME_FMT_YUV422;
        sg_dma_in.width  = frame->width;  // 480
        sg_dma_in.height = frame->height; // 480
        sg_dma_in.pbuf   = frame->data;

        // OUTPUT: Buffer is ALSO 480x480 (RGB565)
        // This is a direct format conversion. No scaling = NO STRIPES.
        sg_dma_out.type   = TUYA_FRAME_FMT_RGB565;
        sg_dma_out.width  = sg_p_display_fb->width;  // 480 (Matches Camera!)
        sg_dma_out.height = sg_p_display_fb->height; // 480
        sg_dma_out.pbuf   = sg_p_display_fb->frame;

        tkl_dma2d_convert(&sg_dma_in, &sg_dma_out);
        tal_semaphore_wait(sg_dma_sem, 100);

        if (sg_display_info.is_swap) {
            tdl_disp_dev_rgb565_swap((uint16_t *)sg_p_display_fb->frame, sg_p_display_fb->len / 2);
        }

        tdl_disp_dev_flush(sg_tdl_disp_hdl, sg_p_display_fb);
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

    // ACTUAL Physical Screen Size
    sg_screen_width  = sg_display_info.width;  // 320
    sg_screen_height = sg_display_info.height; // 480
    tdl_disp_set_brightness(sg_tdl_disp_hdl, 100);

    // FIX: BUFFER SIZE MUST MATCH CAMERA (480) TO PREVENT STRIPES
    // We waste some memory (pixels 321-480 are off-screen), but it fixes the DMA.
    uint32_t buf_width  = 480;
    uint32_t buf_height = 480;
    uint32_t frame_len  = buf_width * buf_height * 2;

    sg_p_display_fb_1 = tdl_disp_create_frame_buff(DISP_FB_TP_PSRAM, frame_len);
    if (sg_p_display_fb_1) {
        sg_p_display_fb_1->fmt    = sg_display_info.fmt;
        sg_p_display_fb_1->width  = buf_width;  // 480
        sg_p_display_fb_1->height = buf_height; // 480
    }
    sg_p_display_fb_2 = tdl_disp_create_frame_buff(DISP_FB_TP_PSRAM, frame_len);
    if (sg_p_display_fb_2) {
        sg_p_display_fb_2->fmt    = sg_display_info.fmt;
        sg_p_display_fb_2->width  = buf_width;  // 480
        sg_p_display_fb_2->height = buf_height; // 480
    }
    sg_p_display_fb = sg_p_display_fb_1;

    return 0;
}

// --- MAIN ---
void user_main(void)
{
    tal_log_init(TAL_LOG_LEVEL_DEBUG, 4096, (TAL_LOG_OUTPUT_CB)tkl_log_output);
    PR_NOTICE("%s", "=== VR Avatar App Starting (Hybrid Mode) ===");

    if (init_display() != 0) {
        PR_ERR("Display Fail");
        return;
    }
    init_dma2d();

    // 1. Init Managers
    emotion_manager_init();
    ai_init();

    // 2. Init Remote Control (MCP)
    avatar_mcp_init();

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

    // PHASE 1: CAMERA (10s Viewfinder)
    PR_NOTICE(">>> PHASE 1: VIEWFINDER <<<");
    sg_show_camera = true;
    for (int i = 0; i < 10; i++) {
        PR_NOTICE("Viewfinder: %d/10 - X:%d Y:%d", i + 1, sg_face_result.x, sg_face_result.y);
        tal_system_sleep(1000);
    }

    PR_NOTICE(">>> PHASE 2: HYBRID AVATAR MODE <<<");
    sg_show_camera = false;

    // Debug
    int loop_count = 0;

    // PHASE 2: AVATAR LOOP
    while (1) {
        // A. TRACKING
        // Center of the VISIBLE screen (320x480) is roughly (160, 240)
        int draw_x = 160 + sg_face_result.x;
        int draw_y = 240 + sg_face_result.y;

        // --- FIX: STRICT CLAMPING ---
        // The face radius is roughly 100px.
        // So we must keep the center between 100 and (Width - 100).

        // Clamp X (Screen Width 320)
        if (draw_x < 110)
            draw_x = 110;
        if (draw_x > 210)
            draw_x = 210; // 320 - 110

        // Clamp Y (Screen Height 480)
        // This fixes the "y:517" error!
        if (draw_y < 110)
            draw_y = 110;
        if (draw_y > 370)
            draw_y = 370; // 480 - 110

        // DEBUG: Print status once per second (approx every 20 frames)
        if (loop_count++ % 20 == 0) {
            PR_NOTICE("Avatar Loop: DrawX:%d DrawY:%d Emotion:%d", draw_x, draw_y, emotion_get_current());
        }

        if (sg_p_display_fb != NULL) {
            // Clear screen
            memset(sg_p_display_fb->frame, 0x00, sg_p_display_fb->len);

            // FORCE TEST: Draw a red box at the top left to prove display works
            // If you see this but no face, your face colors are Black (0x0000).
            for (int i = 0; i < 50; i++) {
                for (int j = 0; j < 50; j++) {
                    tdl_disp_draw_point(sg_p_display_fb, i, j, 0xF800, sg_display_info.is_swap);
                }
            }

            emotion_draw_at(draw_x, draw_y);

            tdl_disp_dev_flush(sg_tdl_disp_hdl, sg_p_display_fb);
            sg_p_display_fb = (sg_p_display_fb == sg_p_display_fb_1) ? sg_p_display_fb_2 : sg_p_display_fb_1;
        }

        tal_system_sleep(50); // 20 FPS
    }
}

// ... (OS Wrappers) ...
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