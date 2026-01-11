#ifndef __AI_MANAGER_H__
#define __AI_MANAGER_H__

#include "tal_api.h"
#include "tdl_camera_manage.h"

// FIX: Must match Camera Resolution
#define AI_MODEL_WIDTH  640
#define AI_MODEL_HEIGHT 480w

typedef struct {
    int x;
    int y;
    int width;
    int height;
    int emotion_id;
} ai_face_result_t;

int ai_init(void);
int ai_process_frame(TDL_CAMERA_FRAME_T *frame, ai_face_result_t *result);

#endif