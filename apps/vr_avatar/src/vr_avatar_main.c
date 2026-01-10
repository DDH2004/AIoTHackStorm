/**
 * @file vr_avatar_main.c
 * @brief VR Avatar Project - Emotion Display with Voice Detection
 */

#include "tal_api.h"
#include "tkl_output.h"
#include "netmgr.h"
#include "netconn_wifi.h"

// --- DISPLAY INCLUDES ---
#include "tdl_display_manage.h"
#include "tdl_display_draw.h"
#include "board_com_api.h"

// --- EMOTION MANAGER ---
#include "emotion_manager.h"

// --- AUDIO CAPTURE ---
#include "audio_capture.h"

// --- CONFIGURATION ---
#define WIFI_SSID     "JJ Lake"
#define WIFI_PASSWORD "20220315"

#ifndef DISPLAY_NAME
#define DISPLAY_NAME "lcd_disp"
#endif

// --- GLOBALS (shared with emotion_manager.c) ---
TDL_DISP_HANDLE_T      sg_tdl_disp_hdl = NULL;
TDL_DISP_DEV_INFO_T    sg_display_info;
TDL_DISP_FRAME_BUFF_T *sg_p_display_fb = NULL;
static uint16_t        sg_screen_width = 0;
static uint16_t        sg_screen_height = 0;

// --- VOICE STATE ---
static volatile BOOL_T sg_is_speaking = FALSE;
static volatile BOOL_T sg_voice_changed = FALSE;

// --- DISPLAY INITIALIZATION ---
static int init_display(void)
{
    OPERATE_RET rt = OPRT_OK;
    PR_NOTICE("--- Initializing Display ---");

    board_register_hardware();

    sg_tdl_disp_hdl = tdl_disp_find_dev(DISPLAY_NAME);
    if (NULL == sg_tdl_disp_hdl) {
        PR_ERR("Display Device '%s' NOT FOUND!", DISPLAY_NAME);
        return -1;
    }

    rt = tdl_disp_dev_get_info(sg_tdl_disp_hdl, &sg_display_info);
    if (rt != OPRT_OK) {
        PR_ERR("Get Display Info Failed: %d", rt);
        return -1;
    }

    rt = tdl_disp_dev_open(sg_tdl_disp_hdl);
    if (rt != OPRT_OK) {
        PR_ERR("Open Display Failed: %d", rt);
        return -1;
    }

    sg_screen_width = sg_display_info.width;
    sg_screen_height = sg_display_info.height;
    PR_NOTICE("Display: %dx%d", sg_screen_width, sg_screen_height);

    tdl_disp_set_brightness(sg_tdl_disp_hdl, 100);

    uint8_t bytes_per_pixel = (tdl_disp_get_fmt_bpp(sg_display_info.fmt) + 7) / 8;
    uint32_t frame_len = sg_display_info.width * sg_display_info.height * bytes_per_pixel;

    sg_p_display_fb = tdl_disp_create_frame_buff(DISP_FB_TP_PSRAM, frame_len);
    if (NULL == sg_p_display_fb) {
        PR_ERR("Failed to Allocate Frame Buffer");
        return -1;
    }

    sg_p_display_fb->x_start = 0;
    sg_p_display_fb->y_start = 0;
    sg_p_display_fb->fmt = sg_display_info.fmt;
    sg_p_display_fb->width = sg_display_info.width;
    sg_p_display_fb->height = sg_display_info.height;

    PR_NOTICE("Display Initialization Complete.");
    return 0;
}

// --- WIFI CALLBACK ---
static int link_status_cb(void *data)
{
    netmgr_status_e status = (netmgr_status_e)(uintptr_t)data;
    if (status == NETMGR_LINK_UP) {
        PR_NOTICE(">>> SUCCESS: Network Link UP! <<<");
    } else {
        PR_NOTICE(">>> Network Status: %d <<<", status);
    }
    return 0;
}

// --- VOICE STATUS CALLBACK ---
static void voice_status_callback(voice_status_t status)
{
    sg_is_speaking = (status == VOICE_STATUS_SPEAKING) ? TRUE : FALSE;
    sg_voice_changed = TRUE;
    PR_NOTICE(">>> Voice: %s <<<", sg_is_speaking ? "SPEAKING" : "SILENT");
}

// --- MAIN ---
void user_main(void)
{
    tal_log_init(TAL_LOG_LEVEL_DEBUG, 4096, (TAL_LOG_OUTPUT_CB)tkl_log_output);
    
    PR_NOTICE("========================================");
    PR_NOTICE("    VR AVATAR - AUDIO DEBUG MODE");
    PR_NOTICE("========================================");

    // Initialize display
    if (init_display() != 0) {
        PR_ERR("Display init failed!");
        return;
    }

    // Initialize emotion manager
    emotion_manager_init();

    // Draw initial emotion
    PR_NOTICE("Drawing initial emotion: %s", emotion_get_name(emotion_get_current()));
    emotion_draw_current(sg_screen_width, sg_screen_height);
    
    // Flush initial display
    tdl_disp_dev_flush(sg_tdl_disp_hdl, sg_p_display_fb);

    // Initialize system & network
    tal_kv_init(&(tal_kv_cfg_t){.seed = "vmlkasdh93dlvlcy", .key = "dflfuap134ddlduq"});
    tal_sw_timer_init();
    tal_workq_init();

    netmgr_init(NETCONN_WIFI);
    tal_event_subscribe(EVENT_LINK_STATUS_CHG, "wifi_cb", link_status_cb, SUBSCRIBE_TYPE_NORMAL);

    netconn_wifi_info_t wifi_info = {0};
    strcpy(wifi_info.ssid, WIFI_SSID);
    strcpy(wifi_info.pswd, WIFI_PASSWORD);
    PR_NOTICE("Connecting to: %s", WIFI_SSID);
    netmgr_conn_set(NETCONN_WIFI, NETCONN_CMD_SSID_PSWD, &wifi_info);

    // Initialize audio capture
    PR_NOTICE("--- Initializing Audio ---");
    if (audio_capture_init() == 0) {
        audio_capture_start(voice_status_callback);
        PR_NOTICE("Audio capture started - listening for voice!");
    } else {
        PR_WARN("Audio init failed - continuing without voice detection");
    }

    // Main loop - only redraw when emotion changes
    BOOL_T was_speaking = FALSE;
    
    PR_NOTICE("=== Main loop started ===");
    
    while (1) {
        // Handle voice status changes - only redraw when status changes
        if (sg_voice_changed) {
            sg_voice_changed = FALSE;
            
            if (sg_is_speaking && !was_speaking) {
                // Started speaking - show HAPPY
                emotion_set_current(EMOTION_HAPPY);
                emotion_draw_current(sg_screen_width, sg_screen_height);
                tdl_disp_dev_flush(sg_tdl_disp_hdl, sg_p_display_fb);
                PR_NOTICE("Speaking: Showing HAPPY emotion");
            } else if (!sg_is_speaking && was_speaking) {
                // Stopped speaking - return to NEUTRAL
                emotion_set_current(EMOTION_NEUTRAL);
                emotion_draw_current(sg_screen_width, sg_screen_height);
                tdl_disp_dev_flush(sg_tdl_disp_hdl, sg_p_display_fb);
                PR_NOTICE("Silent: Showing NEUTRAL emotion");
            }
            was_speaking = sg_is_speaking;
        }
        
        tal_system_sleep(100);  // 100ms polling, no constant redraw
    }
}

#if OPERATING_SYSTEM == SYSTEM_LINUX
void main(int argc, char *argv[])
{
    user_main();
}
#else
static THREAD_HANDLE ty_app_thread = NULL;
static void tuya_app_thread(void *arg)
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