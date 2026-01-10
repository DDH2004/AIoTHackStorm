/**
 * @file vr_avatar_main.c
 * @brief VR Avatar Project - Step 1: Simple Face Display
 */

#include "tal_api.h"
#include "tkl_output.h"
#include "netmgr.h"
#include "netconn_wifi.h"

// --- DISPLAY INCLUDES ---
#include "tdl_display_manage.h"
#include "tdl_display_draw.h"
#include "board_com_api.h"

// --- CONFIGURATION ---
#define WIFI_SSID     "JJ Lake"
#define WIFI_PASSWORD "20220315"

#ifndef DISPLAY_NAME
#define DISPLAY_NAME "lcd_disp"
#endif

// --- COLOR DEFINITIONS (RGB565) ---
#define COLOR_BLACK     0x0000
#define COLOR_WHITE     0xFFFF
#define COLOR_YELLOW    0xFFE0
#define COLOR_CYAN      0x07FF

// --- GLOBALS ---
static TDL_DISP_HANDLE_T      sg_tdl_disp_hdl = NULL;
static TDL_DISP_DEV_INFO_T    sg_display_info;
static TDL_DISP_FRAME_BUFF_T *sg_p_display_fb = NULL;
static uint16_t               sg_screen_width = 0;
static uint16_t               sg_screen_height = 0;

// --- HELPER: Draw a filled circle using SDK draw_point ---
static void draw_filled_circle(int16_t cx, int16_t cy, int16_t r, uint32_t color)
{
    for (int16_t y = -r; y <= r; y++) {
        for (int16_t x = -r; x <= r; x++) {
            if (x * x + y * y <= r * r) {
                int16_t px = cx + x;
                int16_t py = cy + y;
                if (px >= 0 && px < sg_screen_width && py >= 0 && py < sg_screen_height) {
                    tdl_disp_draw_point(sg_p_display_fb, px, py, color, sg_display_info.is_swap);
                }
            }
        }
    }
}

// --- HELPER: Draw a filled rectangle using SDK ---
static void draw_filled_rect(int16_t x, int16_t y, int16_t w, int16_t h, uint32_t color)
{
    TDL_DISP_RECT_T rect;
    
    // Clamp starting coordinates
    int16_t x0 = (x < 0) ? 0 : x;
    int16_t y0 = (y < 0) ? 0 : y;
    int16_t x1 = x + w;
    int16_t y1 = y + h;
    
    // Clamp ending coordinates to screen bounds
    if (x1 > sg_screen_width) x1 = sg_screen_width;
    if (y1 > sg_screen_height) y1 = sg_screen_height;
    
    // Set rect using x0, y0, x1, y1 format
    rect.x0 = x0;
    rect.y0 = y0;
    rect.x1 = x1;
    rect.y1 = y1;
    
    tdl_disp_draw_fill(sg_p_display_fb, &rect, color, sg_display_info.is_swap);
}

// --- DRAW AVATAR FACE ---
static void draw_avatar_face(int is_happy)
{
    int16_t cx = sg_screen_width / 2;
    int16_t cy = sg_screen_height / 2;
    int16_t face_radius = (sg_screen_width < sg_screen_height ? sg_screen_width : sg_screen_height) / 3;

    // 1. Clear background
    tdl_disp_draw_fill_full(sg_p_display_fb, COLOR_CYAN, sg_display_info.is_swap);

    // 2. Draw face (yellow circle)
    draw_filled_circle(cx, cy, face_radius, COLOR_YELLOW);

    // 3. Draw eyes
    int16_t eye_offset_x = face_radius / 3;
    int16_t eye_offset_y = face_radius / 4;
    int16_t eye_radius = face_radius / 8;
    
    draw_filled_circle(cx - eye_offset_x, cy - eye_offset_y, eye_radius, COLOR_BLACK);
    draw_filled_circle(cx + eye_offset_x, cy - eye_offset_y, eye_radius, COLOR_BLACK);

    // 4. Draw mouth
    int16_t mouth_y = cy + face_radius / 3;
    int16_t mouth_width = face_radius / 2;
    
    if (is_happy) {
        // Happy mouth (wider rectangle)
        draw_filled_rect(cx - mouth_width / 2, mouth_y, mouth_width, face_radius / 8, COLOR_BLACK);
    } else {
        // Neutral mouth (thin line)
        draw_filled_rect(cx - mouth_width / 2, mouth_y, mouth_width, 4, COLOR_BLACK);
    }

    // 5. Flush to display
    tdl_disp_dev_flush(sg_tdl_disp_hdl, sg_p_display_fb);
}

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

    if (init_display() != 0) {
        PR_ERR("Display init failed!");
        return;
    }

    PR_NOTICE("Drawing avatar face...");
    draw_avatar_face(1);

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

    int happy = 1;
    while (1) {
        tal_system_sleep(2000);
        happy = !happy;
        draw_avatar_face(happy);
        PR_DEBUG("Expression: %s", happy ? "HAPPY" : "NEUTRAL");
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