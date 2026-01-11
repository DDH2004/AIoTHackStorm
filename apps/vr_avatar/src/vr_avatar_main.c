/**
 * @file vr_avatar_main.c
 * @brief VR Avatar Project - Camera + Face Emotion Detection
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
#include "face_emotion.h"

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

// Face Emotion State
static volatile detected_emotion_t sg_detected_emotion = DETECTED_EMOTION_UNKNOWN;
static volatile BOOL_T sg_emotion_changed = FALSE;
static volatile BOOL_T sg_network_ready = FALSE;

// --- MAP DETECTED EMOTION TO AVATAR EMOTION ---
static emotion_type_t map_detected_to_avatar(detected_emotion_t detected)
{
    switch (detected) {
        case DETECTED_EMOTION_HAPPY:     return EMOTION_HAPPY;
        case DETECTED_EMOTION_SAD:       return EMOTION_SAD;
        case DETECTED_EMOTION_ANGRY:     return EMOTION_ANGRY;
        case DETECTED_EMOTION_SURPRISED: return EMOTION_CONFUSED;  // Map to confused
        case DETECTED_EMOTION_NEUTRAL:   return EMOTION_NEUTRAL;
        case DETECTED_EMOTION_FEARFUL:   return EMOTION_SAD;
        case DETECTED_EMOTION_DISGUSTED: return EMOTION_ANGRY;
        default:                         return EMOTION_NEUTRAL;
    }
}

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

// --- WIFI STATUS CALLBACK ---
static int wifi_status_callback(void *data)
{
    netmgr_status_e status = (netmgr_status_e)(uintptr_t)data;
    
    switch (status) {
        case NETMGR_LINK_UP:
            PR_NOTICE(">>> WiFi CONNECTED <<<");
            sg_network_ready = TRUE;
            // Start face emotion detection now that we have network
            face_emotion_start();
            break;
            
        case NETMGR_LINK_DOWN:
            PR_NOTICE(">>> WiFi DISCONNECTED <<<");
            sg_network_ready = FALSE;
            face_emotion_stop();
            break;
            
        default:
            PR_DEBUG("WiFi status: %d", status);
            break;
    }
    
    return 0;
}

// --- FACE EMOTION CALLBACK ---
static void face_emotion_callback(const face_emotion_result_t *result)
{
    if (result == NULL) return;
    
    PR_NOTICE("Emotion detected: %s (confidence: %d%%, face: %d,%d %dx%d)",
              face_emotion_get_name(result->emotion),
              result->confidence,
              result->face_x, result->face_y,
              result->face_width, result->face_height);
    
    sg_detected_emotion = result->emotion;
    sg_emotion_changed = TRUE;
}

// --- UPDATE AVATAR DISPLAY ---
static void update_avatar_display(emotion_type_t emotion)
{
    emotion_set_current(emotion);
    emotion_draw_current(sg_screen_width, sg_screen_height);
    tdl_disp_dev_flush(sg_tdl_disp_hdl, sg_p_display_fb);
}

// --- MAIN ---
void user_main(void)
{
    tal_log_init(TAL_LOG_LEVEL_DEBUG, 4096, (TAL_LOG_OUTPUT_CB)tkl_log_output);
    
    PR_NOTICE("========================================");
    PR_NOTICE("    VR AVATAR - EMOTION MIRROR");
    PR_NOTICE("========================================");

    if (init_display() != 0) {
        PR_ERR("Display Init Failed");
        return;
    }
    init_dma2d();
    emotion_manager_init();

    if (camera_init(camera_to_screen_cb) != 0) {
        PR_ERR("Camera Init Failed");
    }

    // Show initial neutral face
    PR_NOTICE("Showing initial neutral face...");
    update_avatar_display(EMOTION_NEUTRAL);

    // Initialize system services
    tal_kv_init(&(tal_kv_cfg_t){.seed = "vmlkasdh93dlvlcy", .key = "dflfuap134ddlduq"});
    tal_sw_timer_init();
    tal_workq_init();

    // Initialize network with callback
    netmgr_init(NETCONN_WIFI);
    tal_event_subscribe(EVENT_LINK_STATUS_CHG, "wifi_cb", 
                        wifi_status_callback, SUBSCRIBE_TYPE_NORMAL);

    netconn_wifi_info_t wifi_info = {0};
    strcpy(wifi_info.ssid, WIFI_SSID);
    strcpy(wifi_info.pswd, WIFI_PASSWORD);
    PR_NOTICE("Connecting to WiFi: %s", WIFI_SSID);
    netmgr_conn_set(NETCONN_WIFI, NETCONN_CMD_SSID_PSWD, &wifi_info);

    // Initialize face emotion detection (will start after WiFi connects)
    if (face_emotion_init(face_emotion_callback) != 0) {
        PR_ERR("Face emotion init failed!");
    }

    PR_NOTICE("========================================");
    PR_NOTICE("  Waiting for WiFi + emotion server...");
    PR_NOTICE("========================================");

    // Main loop - respond to emotion changes
    emotion_type_t current_avatar_emotion = EMOTION_NEUTRAL;
    
    while (1) {
        // Check for emotion changes from face detection
        if (sg_emotion_changed) {
            sg_emotion_changed = FALSE;
            
            emotion_type_t new_emotion = map_detected_to_avatar(sg_detected_emotion);
            
            if (new_emotion != current_avatar_emotion) {
                PR_NOTICE("Avatar: %s -> %s",
                          emotion_get_name(current_avatar_emotion),
                          emotion_get_name(new_emotion));
                
                current_avatar_emotion = new_emotion;
                update_avatar_display(current_avatar_emotion);
            }
        }
        
        tal_system_sleep(50);
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