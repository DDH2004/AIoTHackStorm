/**
 * @file vr_avatar_main.c
 * @brief VR Avatar Project - WiFi + Red Screen Test
 */

#include "tal_api.h"
#include "tkl_output.h"
#include "netmgr.h"
#include "netconn_wifi.h"

// --- DISPLAY INCLUDES (From Example) ---
#include "tdl_display_manage.h"
#include "tdl_display_draw.h"
#include "board_com_api.h"

// --- CONFIGURATION ---
#define WIFI_SSID     "NETGEAR26"
#define WIFI_PASSWORD "fluffyteapot856"

// Fallback if not defined in board_com_api.h
#ifndef DISPLAY_NAME
#define DISPLAY_NAME "lcd_disp"
#endif

// --- GLOBALS ---
static TDL_DISP_HANDLE_T      sg_tdl_disp_hdl = NULL;
static TDL_DISP_DEV_INFO_T    sg_display_info;
static TDL_DISP_FRAME_BUFF_T *sg_p_display_fb = NULL;

// --- DISPLAY FUNCTION ---
void init_screen_red(void)
{
    OPERATE_RET rt = OPRT_OK;
    PR_NOTICE("--- Initializing Screen (TDL Method) ---");

    // 1. Register Hardware (Crucial Step!)
    // This loads the drivers defined in the Board Support Package
    board_register_hardware();

    // 2. Find the Display Device
    sg_tdl_disp_hdl = tdl_disp_find_dev(DISPLAY_NAME);
    if (NULL == sg_tdl_disp_hdl) {
        PR_ERR("Display Device '%s' NOT FOUND!", DISPLAY_NAME);
        return;
    }

    // 3. Get Info & Open
    rt = tdl_disp_dev_get_info(sg_tdl_disp_hdl, &sg_display_info);
    if (rt != OPRT_OK) {
        PR_ERR("Get Display Info Failed: %d", rt);
        return;
    }

    rt = tdl_disp_dev_open(sg_tdl_disp_hdl);
    if (rt != OPRT_OK) {
        PR_ERR("Open Display Failed: %d", rt);
        return;
    }

    // 4. Set Brightness
    tdl_disp_set_brightness(sg_tdl_disp_hdl, 100);

    // 5. Calculate Buffer Size
    uint32_t frame_len       = 0;
    uint8_t  bytes_per_pixel = (tdl_disp_get_fmt_bpp(sg_display_info.fmt) + 7) / 8;
    frame_len                = sg_display_info.width * sg_display_info.height * bytes_per_pixel;

    // 6. Create Frame Buffer
    sg_p_display_fb = tdl_disp_create_frame_buff(DISP_FB_TP_PSRAM, frame_len);
    if (NULL == sg_p_display_fb) {
        PR_ERR("Failed to Allocate Frame Buffer");
        return;
    }

    // 7. Setup Buffer Metadata
    sg_p_display_fb->x_start = 0;
    sg_p_display_fb->y_start = 0;
    sg_p_display_fb->fmt     = sg_display_info.fmt;
    sg_p_display_fb->width   = sg_display_info.width;
    sg_p_display_fb->height  = sg_display_info.height;

    // 8. FILL RED (0xF800 is Red in RGB565, 0xFF0000FF in ARGB)
    // The example uses a helper to fill. We will use a solid color.
    // We assume RGB565 for now, but let's use a safe Red value.
    uint32_t red_color = 0xF800;
    if (bytes_per_pixel > 2)
        red_color = 0xFFFF0000; // ARGB Red

    PR_NOTICE("Drawing RED to Screen...");
    tdl_disp_draw_fill_full(sg_p_display_fb, red_color, sg_display_info.is_swap);

    // 9. Flush to Hardware
    tdl_disp_dev_flush(sg_tdl_disp_hdl, sg_p_display_fb);

    PR_NOTICE("Screen Initialization Complete.");
}

// --- WIFI CALLBACK ---
static int link_status_cb(void *data)
{
    netmgr_status_e status = (netmgr_status_e)data;
    if (status == NETMGR_LINK_UP) {
        PR_NOTICE(">>> SUCCESS: Network Link UP! IP Acquired. <<<");
    } else {
        PR_NOTICE(">>> Network Link Status Changed: %d <<<", status);
    }
    return 0;
}

// --- MAIN ---
void user_main(void)
{
    // 1. Init Logging
    tal_log_init(TAL_LOG_LEVEL_DEBUG, 4096, (TAL_LOG_OUTPUT_CB)tkl_log_output);
    PR_NOTICE("--- VR Avatar App Starting ---");

    // 2. Init Screen (TDL)
    init_screen_red();

    // 3. Init System & Network
    tal_kv_init(&(tal_kv_cfg_t){.seed = "vmlkasdh93dlvlcy", .key = "dflfuap134ddlduq"});
    tal_sw_timer_init();
    tal_workq_init();

    netmgr_init(NETCONN_WIFI);
    tal_event_subscribe(EVENT_LINK_STATUS_CHG, "wifi_test", link_status_cb, SUBSCRIBE_TYPE_NORMAL);

    // 4. Connect to WiFi
    netconn_wifi_info_t wifi_info = {0};
    strcpy(wifi_info.ssid, WIFI_SSID);
    strcpy(wifi_info.pswd, WIFI_PASSWORD);

    PR_NOTICE("Attempting to connect to SSID: %s", WIFI_SSID);
    netmgr_conn_set(NETCONN_WIFI, NETCONN_CMD_SSID_PSWD, &wifi_info);
}

#if OPERATING_SYSTEM == SYSTEM_LINUX
void main(int argc, char *argv[])
{
    user_main();
    while (1)
        tal_system_sleep(500);
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