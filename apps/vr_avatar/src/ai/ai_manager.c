/**
 * @file ai_manager.c
 * @brief Real Face Attribute Detection (No more brightness hacking!)
 */

#include "ai_manager.h"
#include "emotion_manager.h"
#include "tdl_face_attri_manage.h" // The Real AI Service

// Standard Model Address for T5AI Pocket
// If this fails, we might need to check if the model is flashed to a different spot
#define FACE_DETECT_MODEL_ADDR 0x600000
#define FACE_ATTRI_MODEL_ADDR  0x700000 // Often sits right after detection

static FACE_ATTRI_HANDLE_T sg_ai_handle = NULL;
static bool                sg_ai_inited = false;

int ai_init(void)
{
    OPERATE_RET rt = OPRT_OK;
    PR_NOTICE("--- Initializing Real AI (Face Attribute) ---");

    // 1. Initialize the Face Attribute Service
    // This loads the "brain" that knows what a face and emotions look like
    rt = tdl_face_attri_create(&sg_ai_handle, FACE_DETECT_MODEL_ADDR, FACE_ATTRI_MODEL_ADDR);

    if (rt != OPRT_OK) {
        PR_ERR("AI Model Load Failed: %d. (Is the model flashed?)", rt);
        return rt;
    }

    sg_ai_inited = true;
    PR_NOTICE("AI Engine Loaded Successfully!");
    return OPRT_OK;
}

int ai_process_frame(TDL_CAMERA_FRAME_T *frame, ai_face_result_t *result)
{
    if (!sg_ai_inited || sg_ai_handle == NULL)
        return OPRT_COM_ERROR;
    if (frame == NULL || frame->data == NULL)
        return OPRT_INVALID_PARM;

    FACE_ATTRI_RESULT_T ai_ret = {0};

    // --- 1. RUN THE REAL AI ---
    // This function scans the image for faces and attributes
    OPERATE_RET rt = tdl_face_attri_run(sg_ai_handle, frame, &ai_ret);

    if (rt == OPRT_OK && ai_ret.num > 0) {
        // We found at least one face!

        // --- 2. EXTRACT POSITION ---
        // The AI gives us a rectangle (left, top, right, bottom)
        int face_x = ai_ret.item[0].rect.left;
        int face_y = ai_ret.item[0].rect.top;
        int face_w = ai_ret.item[0].rect.right - ai_ret.item[0].rect.left;
        int face_h = ai_ret.item[0].rect.bottom - ai_ret.item[0].rect.top;

        // Calculate the center point of the face
        int center_x = face_x + (face_w / 2);
        int center_y = face_y + (face_h / 2);

        // Convert to offset from screen center (for your avatar logic)
        // Camera is 480x480
        result->x = center_x - (480 / 2);
        result->y = center_y - (480 / 2);

        // Mirror effect (invert X)
        result->x = -result->x;

        // --- 3. EXTRACT EMOTION ---
        // The attribute result usually returns an ID or string.
        // For T5AI Face Attribute:
        // 0: Neutral, 1: Happy, 2: Surprise, 3: Angry, 4: Sad
        int emotion_code = ai_ret.item[0].attr.emotion;

        switch (emotion_code) {
        case 1:
            result->emotion_id = EMOTION_HAPPY;
            break;
        case 4:
            result->emotion_id = EMOTION_SAD;
            break;
        case 3:
            result->emotion_id = EMOTION_ANGRY;
            break;
        default:
            result->emotion_id = EMOTION_NEUTRAL;
            break;
        }

        // Debug Log
        PR_DEBUG("Real Face Found! X:%d Y:%d E:%d", result->x, result->y, emotion_code);

        return OPRT_OK;
    }

    // No face found in this frame
    return OPRT_COM_ERROR;
}