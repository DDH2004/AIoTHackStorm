/**
 * @file face_emotion.c
 * @brief Face emotion detection via HTTP server
 */

#include "face_emotion.h"
#include "tal_api.h"
#include "tal_network.h"
#include "tal_memory.h"

#include <string.h>
#include <stdio.h>

// --- LOCAL SERVER CONFIGURATION ---
#define EMOTION_SERVER_HOST "192.168.34.249"
#define EMOTION_SERVER_PORT 5000
#define EMOTION_SERVER_PATH "/detect_simple"

#define HTTP_TIMEOUT_MS     10000
#define CAPTURE_INTERVAL_MS 2000
#define MAX_RESPONSE_SIZE   256

// Frame size for emotion detection
#define CAPTURE_WIDTH       160
#define CAPTURE_HEIGHT      120
#define CAPTURE_BPP         3  // RGB888

// --- MODULE STATE ---
static face_emotion_cb_t sg_callback = NULL;
static volatile BOOL_T sg_is_running = FALSE;
static THREAD_HANDLE sg_detect_thread = NULL;
static uint8_t* sg_frame_buffer = NULL;

// --- EMOTION NAME LOOKUP ---
static const char* sg_emotion_names[] = {
    "happy", "sad", "angry", "neutral", "surprise", "fear", "disgust", "none", "unknown", "error"
};

const char* face_emotion_get_name(detected_emotion_t emotion)
{
    if (emotion >= 0 && emotion < (int)(sizeof(sg_emotion_names)/sizeof(sg_emotion_names[0]))) {
        return sg_emotion_names[emotion];
    }
    return "unknown";
}

// --- PARSE SERVER RESPONSE ---
static detected_emotion_t parse_emotion_response(const char* response, face_emotion_result_t* result)
{
    if (!response || !result) {
        return DETECTED_EMOTION_ERROR;
    }
    
    memset(result, 0, sizeof(face_emotion_result_t));
    
    char emotion_str[32] = {0};
    int x = 0, y = 0, w = 0, h = 0, conf = 0;
    
    if (sscanf(response, "%31[^:]:%d:%d:%d:%d:%d", emotion_str, &x, &y, &w, &h, &conf) >= 1) {
        result->face_x = x;
        result->face_y = y;
        result->face_width = w;
        result->face_height = h;
        result->confidence = conf;
        result->face_detected = (w > 0 && h > 0) ? TRUE : FALSE;
        
        if (strcmp(emotion_str, "happy") == 0) result->emotion = DETECTED_EMOTION_HAPPY;
        else if (strcmp(emotion_str, "sad") == 0) result->emotion = DETECTED_EMOTION_SAD;
        else if (strcmp(emotion_str, "angry") == 0) result->emotion = DETECTED_EMOTION_ANGRY;
        else if (strcmp(emotion_str, "neutral") == 0) result->emotion = DETECTED_EMOTION_NEUTRAL;
        else if (strcmp(emotion_str, "surprise") == 0) result->emotion = DETECTED_EMOTION_SURPRISED;
        else if (strcmp(emotion_str, "fear") == 0) result->emotion = DETECTED_EMOTION_FEARFUL;
        else if (strcmp(emotion_str, "disgust") == 0) result->emotion = DETECTED_EMOTION_DISGUSTED;
        else result->emotion = DETECTED_EMOTION_NONE;
        
        return result->emotion;
    }
    
    result->emotion = DETECTED_EMOTION_NONE;
    return DETECTED_EMOTION_NONE;
}

// --- HTTP POST ---
static int send_http_post(const char* host, int port, const char* path, 
                          const uint8_t* data, size_t data_len,
                          char* response, size_t response_size)
{
    int sockfd = -1;
    int ret = -1;
    TUYA_IP_ADDR_T server_ip;
    
    ret = tal_net_gethostbyname(host, &server_ip);
    if (ret != OPRT_OK) {
        PR_ERR("DNS failed: %d", ret);
        return -1;
    }
    
    sockfd = tal_net_socket_create(PROTOCOL_TCP);
    if (sockfd < 0) {
        PR_ERR("Socket failed");
        return -1;
    }
    
    tal_net_set_timeout(sockfd, HTTP_TIMEOUT_MS, TRANS_SEND);
    tal_net_set_timeout(sockfd, HTTP_TIMEOUT_MS, TRANS_RECV);
    
    ret = tal_net_connect(sockfd, server_ip, port);
    if (ret != OPRT_OK) {
        PR_ERR("Connect failed: %d", ret);
        tal_net_close(sockfd);
        return -1;
    }
    
    char header[512];
    int header_len = snprintf(header, sizeof(header),
        "POST %s HTTP/1.1\r\n"
        "Host: %s:%d\r\n"
        "Content-Type: application/octet-stream\r\n"
        "Content-Length: %d\r\n"
        "Connection: close\r\n"
        "\r\n",
        path, host, port, (int)data_len);
    
    ret = tal_net_send(sockfd, header, header_len);
    if (ret != header_len) {
        tal_net_close(sockfd);
        return -1;
    }
    
    if (data && data_len > 0) {
        size_t sent = 0;
        while (sent < data_len) {
            size_t chunk = (data_len - sent > 4096) ? 4096 : (data_len - sent);
            ret = tal_net_send(sockfd, (void*)(data + sent), chunk);
            if (ret <= 0) {
                tal_net_close(sockfd);
                return -1;
            }
            sent += ret;
        }
    }
    
    memset(response, 0, response_size);
    int total = 0;
    while (total < (int)response_size - 1) {
        ret = tal_net_recv(sockfd, response + total, response_size - 1 - total);
        if (ret <= 0) break;
        total += ret;
    }
    
    tal_net_close(sockfd);
    
    char* body = strstr(response, "\r\n\r\n");
    if (body) {
        body += 4;
        memmove(response, body, strlen(body) + 1);
    }
    
    return total > 0 ? 0 : -1;
}

// --- GENERATE TEST PATTERN (simulates camera) ---
static void generate_test_frame(uint8_t* buffer, int frame_num)
{
    // Create a varying pattern so server can detect "faces"
    // This creates a simple gradient that changes over time
    for (int y = 0; y < CAPTURE_HEIGHT; y++) {
        for (int x = 0; x < CAPTURE_WIDTH; x++) {
            int idx = (y * CAPTURE_WIDTH + x) * 3;
            
            // Create a "face-like" bright region in center
            int cx = CAPTURE_WIDTH / 2;
            int cy = CAPTURE_HEIGHT / 2;
            int dx = x - cx;
            int dy = y - cy;
            int dist = (dx * dx + dy * dy);
            
            if (dist < 900) {  // ~30 pixel radius "face"
                // Skin-like color
                buffer[idx + 0] = 200;  // R
                buffer[idx + 1] = 150;  // G
                buffer[idx + 2] = 120;  // B
            } else {
                // Background - slight variation
                buffer[idx + 0] = 50 + (frame_num % 50);
                buffer[idx + 1] = 50;
                buffer[idx + 2] = 50;
            }
        }
    }
}

// --- DETECTION THREAD ---
static void detection_thread(void* arg)
{
    char response[MAX_RESPONSE_SIZE];
    face_emotion_result_t result;
    size_t frame_size = CAPTURE_WIDTH * CAPTURE_HEIGHT * CAPTURE_BPP;
    int frame_num = 0;
    
    PR_NOTICE("Emotion detection started");
    PR_NOTICE("Server: %s:%d%s", EMOTION_SERVER_HOST, EMOTION_SERVER_PORT, EMOTION_SERVER_PATH);
    PR_NOTICE("Frame: %dx%d RGB (%d bytes)", CAPTURE_WIDTH, CAPTURE_HEIGHT, (int)frame_size);
    
    // Allocate frame buffer
    sg_frame_buffer = tal_malloc(frame_size);
    if (!sg_frame_buffer) {
        PR_ERR("Failed to allocate frame buffer");
        return;
    }
    
    while (sg_is_running) {
        tal_system_sleep(CAPTURE_INTERVAL_MS);
        if (!sg_is_running) break;
        
        // Generate test frame (simulated camera)
        generate_test_frame(sg_frame_buffer, frame_num++);
        
        // Send to server
        int ret = send_http_post(EMOTION_SERVER_HOST, EMOTION_SERVER_PORT,
                                  EMOTION_SERVER_PATH,
                                  sg_frame_buffer, frame_size,
                                  response, sizeof(response));
        
        if (ret == 0 && strlen(response) > 0) {
            parse_emotion_response(response, &result);
            
            if (result.face_detected) {
                PR_NOTICE(">>> %s (conf: %d%%) @ [%d,%d %dx%d]",
                          face_emotion_get_name(result.emotion),
                          result.confidence,
                          result.face_x, result.face_y,
                          result.face_width, result.face_height);
            } else {
                PR_DEBUG("No face detected");
            }
            
            if (sg_callback) {
                sg_callback(&result);
            }
        }
    }
    
    if (sg_frame_buffer) {
        tal_free(sg_frame_buffer);
        sg_frame_buffer = NULL;
    }
    
    PR_NOTICE("Emotion detection stopped");
}

// --- PUBLIC API ---
int face_emotion_init(face_emotion_cb_t callback)
{
    sg_callback = callback;
    PR_NOTICE("Face emotion init: http://%s:%d%s", 
              EMOTION_SERVER_HOST, EMOTION_SERVER_PORT, EMOTION_SERVER_PATH);
    return 0;
}

int face_emotion_start(void)
{
    if (sg_is_running) return 0;
    
    sg_is_running = TRUE;
    
    THREAD_CFG_T cfg = {
        .stackDepth = 8192,
        .priority = THREAD_PRIO_2,
        .thrdname = "emotion"
    };
    
    int ret = tal_thread_create_and_start(&sg_detect_thread, NULL, NULL,
                                          detection_thread, NULL, &cfg);
    if (ret != OPRT_OK) {
        PR_ERR("Thread create failed: %d", ret);
        sg_is_running = FALSE;
        return -1;
    }
    
    return 0;
}

int face_emotion_stop(void)
{
    if (!sg_is_running) return 0;
    
    sg_is_running = FALSE;
    
    if (sg_detect_thread) {
        tal_thread_delete(sg_detect_thread);
        sg_detect_thread = NULL;
    }
    
    return 0;
}

void face_emotion_deinit(void)
{
    face_emotion_stop();
}
