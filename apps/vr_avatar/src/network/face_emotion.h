/**
 * @file face_emotion.h
 * @brief Face emotion detection via HTTP server
 */

#ifndef __FACE_EMOTION_H__
#define __FACE_EMOTION_H__

#include "tuya_cloud_types.h"

// --- DETECTED EMOTION TYPES (from face detection server) ---
typedef enum {
    DETECTED_EMOTION_HAPPY = 0,
    DETECTED_EMOTION_SAD,
    DETECTED_EMOTION_ANGRY,
    DETECTED_EMOTION_NEUTRAL,
    DETECTED_EMOTION_SURPRISED,
    DETECTED_EMOTION_FEARFUL,
    DETECTED_EMOTION_DISGUSTED,
    DETECTED_EMOTION_NONE,
    DETECTED_EMOTION_UNKNOWN,
    DETECTED_EMOTION_ERROR
} detected_emotion_t;

// --- FACE DETECTION RESULT ---
typedef struct {
    detected_emotion_t emotion;
    BOOL_T face_detected;
    int face_x;
    int face_y;
    int face_width;
    int face_height;
    int confidence;  // 0-100
} face_emotion_result_t;

// --- CALLBACK TYPE ---
typedef void (*face_emotion_cb_t)(const face_emotion_result_t *result);

// --- API FUNCTIONS ---

/**
 * @brief Initialize face emotion detection
 * @param callback Function to call when emotion is detected
 * @return 0 on success, -1 on failure
 */
int face_emotion_init(face_emotion_cb_t callback);

/**
 * @brief Start emotion detection thread
 * @return 0 on success, -1 on failure
 */
int face_emotion_start(void);

/**
 * @brief Stop emotion detection
 * @return 0 on success, -1 on failure
 */
int face_emotion_stop(void);

/**
 * @brief Deinitialize and cleanup
 */
void face_emotion_deinit(void);

/**
 * @brief Get emotion name string
 * @param emotion The emotion type
 * @return Emotion name string
 */
const char* face_emotion_get_name(detected_emotion_t emotion);

#endif /* __FACE_EMOTION_H__ */
