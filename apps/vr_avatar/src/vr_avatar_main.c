/**
 * @file vr_avatar_main.c
 * @brief VR Avatar Project - Emotion Display Demo
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
    netmgr_status_e status = (netmgr_status_e)data;
    if (status == NETMGR_LINK_UP) {
        PR_NOTICE(">>> SUCCESS: Network Link UP! <<<");
    } else {
        PR_NOTICE(">>> Network Status: %d <<<", status);
    }
    return 0;
}

// --- MAIN ---
void user_main(void)
{
    tal_log_init(TAL_LOG_LEVEL_DEBUG, 4096, (TAL_LOG_OUTPUT_CB)tkl_log_output);
    PR_NOTICE("=== VR Avatar App Starting ===");

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

    // Demo loop - cycle through all emotions
    while (1) {
        tal_system_sleep(3000);  // Wait 3 seconds
        
        emotion_type_t new_emotion = emotion_next();
        PR_NOTICE("Emotion changed to: %s", emotion_get_name(new_emotion));
        emotion_draw_current(sg_screen_width, sg_screen_height);
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