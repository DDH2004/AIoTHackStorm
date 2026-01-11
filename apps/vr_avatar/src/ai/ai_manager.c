/**
 * @file ai_manager.c
 * @brief AI Simulation (Skin Tone Tracking)
 */

#include "ai_manager.h"
#include "emotion_manager.h"

static bool sg_ai_inited    = false;
static int  sg_base_x       = 0;
static int  sg_base_y       = 0;
static int  sg_calib_frames = 0;

int ai_init(void)
{
    PR_NOTICE("--- AI Smart Simulation Mode (Skin Tracking) ---");
    sg_ai_inited = true;
    return OPRT_OK;
}

int ai_process_frame(TDL_CAMERA_FRAME_T *frame, ai_face_result_t *result)
{
    if (!sg_ai_inited || frame == NULL || frame->data == NULL)
        return OPRT_COM_ERROR;

    long     sum_x = 0, sum_y = 0, count = 0;
    uint8_t *data = (uint8_t *)frame->data;

    // We scan in steps of 4 pixels to be fast.
    // YUV422 (YUYV) Macro-pixel is 4 bytes = 2 pixels.
    // [Y0, U0, Y1, V0]
    int w = frame->width;
    int h = frame->height;

    // Debug printer
    static int debug_timer = 0;
    bool       show_debug  = (debug_timer++ % 20 == 0); // ~1 sec

    for (int y = 0; y < h; y += 8) {
        for (int x = 0; x < w; x += 8) {

            // Calculate index for the pixel pair
            // Each pixel is 2 bytes effective (average), but YUYV structure is 4 bytes per 2 pixels.
            // We align x to even numbers to make math easy.
            int aligned_x = x & ~1;
            int index     = (y * w * 2) + (aligned_x * 2);

            if (index + 3 >= w * h * 2)
                continue;

            // Extract YUV components (YUYV format)
            // Byte 0: Y0
            // Byte 1: U (Shared by pixel 0 and 1)
            // Byte 2: Y1
            // Byte 3: V (Shared by pixel 0 and 1)

            uint8_t Y = data[index];
            uint8_t U = data[index + 1];
            uint8_t V = data[index + 3];

            // --- SKIN TONE DETECTION LOGIC ---
            // These thresholds are empirical for standard webcams.
            // Skin is usually "Reddish" (High V) and "Not Blue" (Low U).
            // U (Cb): 77 ~ 127
            // V (Cr): 133 ~ 173

            bool is_skin = false;

            // Adjusted Ranges:
            // U: 100 - 200 (Catches 175)
            // V: 130 - 200 (Catches 176)
            if ((U > 100 && U < 200) && (V > 130 && V < 200)) {
                // Ensure it's not too dark
                if (Y > 40) {
                    is_skin = true;
                }
            }

            // Print one debug pixel to help you calibrate
            if (show_debug && x == 240 && y == 240) {
                PR_NOTICE("Center Pixel: Y=%d U=%d V=%d -> %s", Y, U, V, is_skin ? "SKIN" : "BG");
            }

            if (is_skin) {
                sum_x += x;
                sum_y += y;
                count++;
            }
        }
    }

    // 2. DECISION LOGIC
    // We need at least 5 "skin" blocks to consider it a face.
    // (Prevents noise triggers)
    if (count > 5) {
        int curr_x = sum_x / count;
        int curr_y = sum_y / count;

        if (show_debug) {
            PR_NOTICE("Skin Blob Found! Count:%ld Center:(%d, %d)", count, curr_x, curr_y);
        }

        // Auto-Calibration:
        // On startup, assume the first face seen is "Center"
        if (sg_calib_frames < 20) {
            sg_base_x = curr_x;
            sg_base_y = curr_y;
            sg_calib_frames++;
            result->x = 0;
            result->y = 0;
            return OPRT_OK;
        }

        // Relative Movement
        // Multiply by 4 for sensitivity
        result->x = -(curr_x - sg_base_x) * 4;
        result->y = (curr_y - sg_base_y) * 4;

        // Deadzone
        if (result->x > -10 && result->x < 10)
            result->x = 0;
        if (result->y > -10 && result->y < 10)
            result->y = 0;

        return OPRT_OK;

    } else {
        // No skin found -> Return to center
        if (show_debug && sg_calib_frames > 0) {
            PR_NOTICE("No Skin detected.");
        }

        // Decay to center (Smooth return)
        result->x = result->x / 2;
        result->y = result->y / 2;

        return OPRT_COM_ERROR;
    }
}