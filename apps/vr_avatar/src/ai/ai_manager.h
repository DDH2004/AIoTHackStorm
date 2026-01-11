/**
 * @file ai_manager.h
 * @brief Face Tracking and Attribute Detection
 */

#ifndef __AI_MANAGER_H__
#define __AI_MANAGER_H__

#include "tal_api.h"
#include "tdl_camera_manage.h"

// MATCH THE HARDWARE: 480x480
#define AI_MODEL_WIDTH  480
#define AI_MODEL_HEIGHT 480

typedef struct {
    int x;
    int y;
    int width;
    int height;
    int emotion_id; // 0=Neutral, 1=Happy, 2=Sad
} ai_face_result_t;

/**
 * @brief Initialize the AI Engine
 */
int ai_init(void);

/**
 * @brief Run detection on a video frame
 */
int ai_process_frame(TDL_CAMERA_FRAME_T *frame, ai_face_result_t *result);

#endif