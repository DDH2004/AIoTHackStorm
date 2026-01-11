/**
 * @file vr_avatar_main.c
 * @brief VR Avatar Project - Minimal Client Mode
 */

#include "tal_api.h"
#include "tkl_output.h"
#include "netmgr.h"
#include "netconn_wifi.h"
#include "tdl_display_manage.h"
#include "tdl_display_draw.h"
#include "board_com_api.h"
#include <string.h>

#include "emotion_manager.h"
// #include "camera_manager.h" // REMOVED
// #include "ai_manager.h"     // REMOVED
#include "avatar_mcp.h"
#include "avatar_mcp.h"
// System headers are included via tal_api.h

// --- CONFIGURATION ---
#define WIFI_SSID     "JJ Lake"
#define WIFI_PASSWORD "20220315"

#ifndef DISPLAY_NAME
#define DISPLAY_NAME "lcd_disp"
#endif

// --- GLOBALS ---
TDL_DISP_HANDLE_T   sg_tdl_disp_hdl = NULL;
TDL_DISP_DEV_INFO_T sg_display_info;

// Double Buffering
TDL_DISP_FRAME_BUFF_T *sg_p_display_fb_1 = NULL;
TDL_DISP_FRAME_BUFF_T *sg_p_display_fb_2 = NULL;
TDL_DISP_FRAME_BUFF_T *sg_p_display_fb   = NULL;

static uint16_t      sg_screen_width  = 0;
static uint16_t      sg_screen_height = 0;
static THREAD_HANDLE sg_http_thread   = NULL;

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

    // Use screen size for buffer now since we don't need to match camera
    uint32_t buf_width  = sg_screen_width;
    uint32_t buf_height = sg_screen_height;
    uint32_t frame_len  = buf_width * buf_height * 2;

    sg_p_display_fb_1 = tdl_disp_create_frame_buff(DISP_FB_TP_PSRAM, frame_len);
    if (sg_p_display_fb_1) {
        sg_p_display_fb_1->fmt    = sg_display_info.fmt;
        sg_p_display_fb_1->width  = buf_width;
        sg_p_display_fb_1->height = buf_height;
    }
    sg_p_display_fb_2 = tdl_disp_create_frame_buff(DISP_FB_TP_PSRAM, frame_len);
    if (sg_p_display_fb_2) {
        sg_p_display_fb_2->fmt    = sg_display_info.fmt;
        sg_p_display_fb_2->width  = buf_width;
        sg_p_display_fb_2->height = buf_height;
    }
    sg_p_display_fb = sg_p_display_fb_1;

    return 0;
}

static void http_update_thread(void *arg)
{
    while (1) {
        GW_WIFI_STAT_E wf_stat = get_wf_status();
        if (wf_stat >= 4) {
            update_emotion_from_server();
        }
        tal_system_sleep(1000); // Poll every 1 second
    }
}

// --- MAIN ---
void user_main(void)
{
    tal_log_init(TAL_LOG_LEVEL_DEBUG, 4096, (TAL_LOG_OUTPUT_CB)tkl_log_output);
    PR_NOTICE("--- User Main Start (Minimal) ---");

    // Give system a moment to breathe
    tal_system_sleep(1000);

    // 0. System Service Init (CRITICAL for NetMgr)
    tal_kv_init(&(tal_kv_cfg_t){
        .seed = "vmlkasdh93dlvlcy",
        .key  = "dflfuap134ddlduq",
    });
    tal_sw_timer_init();
    tal_workq_init();
    tal_time_service_init();
    tal_cli_init();

    // 1. Connect WiFi FIRST
    PR_NOTICE("Initializing NetMgr...");
    netmgr_init(NETCONN_WIFI);
    PR_NOTICE("NetMgr Initialized");

    netconn_wifi_info_t wifi_info = {0};
    strcpy(wifi_info.ssid, WIFI_SSID);
    strcpy(wifi_info.pswd, WIFI_PASSWORD);

    PR_NOTICE("Setting WiFi Credentials...");
    netmgr_conn_set(NETCONN_WIFI, NETCONN_CMD_SSID_PSWD, &wifi_info);
    PR_NOTICE("WiFi Credentials Set");

    // Give WiFi some time to start up before starting HTTP thread
    tal_system_sleep(2000);

    // 2. Start HTTP Thread
    THREAD_CFG_T http_cfg = {4096, 3, "http_sync"};
    PR_NOTICE("Starting HTTP Thread...");
    tal_thread_create_and_start(&sg_http_thread, NULL, NULL, http_update_thread, NULL, &http_cfg);
    PR_NOTICE("HTTP Thread Started");

    // 3. Initialize Display
    if (init_display() != 0) {
        PR_ERR("Display Init Failed");
        return;
    }
    PR_NOTICE("Display Initialized");

    // 4. Initialize Emotion Manager
    emotion_manager_init();
    PR_NOTICE("Emotion Manager Initialized");

    // 5. Main Loop: Draw Emotion
    while (1) {
        if (sg_p_display_fb != NULL) {
            // Clear background (Black)
            memset(sg_p_display_fb->frame, 0x00, sg_p_display_fb->len);

            // Get tracking position (0.0 - 1.0)
            face_position_t pos = avatar_get_position();

            // Map to screen coordinates
            // Screen is 320x480 (Portrait) or 480x320 (Landscape)
            // Assuming 320x480 based on previous code
            int draw_x = (int)(pos.x * sg_screen_width);
            int draw_y = (int)(pos.y * sg_screen_height);

            // Clamp to keep face somewhat on screen
            // Face radius is 100, so keep center within bounds
            if (draw_x < 50)
                draw_x = 50;
            if (draw_x > sg_screen_width - 50)
                draw_x = sg_screen_width - 50;
            if (draw_y < 50)
                draw_y = 50;
            if (draw_y > sg_screen_height - 50)
                draw_y = sg_screen_height - 50;

            // Draw current emotion at tracked position
            emotion_draw_at(draw_x, draw_y);

            // Flush display
            tdl_disp_dev_flush(sg_tdl_disp_hdl, sg_p_display_fb);

            // Swap buffers
            sg_p_display_fb = (sg_p_display_fb == sg_p_display_fb_1) ? sg_p_display_fb_2 : sg_p_display_fb_1;
        }
        tal_system_sleep(50); // 20 FPS for smoother tracking
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
    THREAD_CFG_T thrd_param = {8192, 4, "tuya_app_main"};
    tal_thread_create_and_start(&ty_app_thread, NULL, NULL, tuya_app_thread, NULL, &thrd_param);
}
#endif