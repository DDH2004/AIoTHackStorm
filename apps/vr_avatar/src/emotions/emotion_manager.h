/**
 * @file emotion_manager.h
 * @brief Header for Emotion Manager and Data Structures
 */

#ifndef __EMOTION_MANAGER_H__
#define __EMOTION_MANAGER_H__

#include "tal_api.h"

// --- TYPES ---

// List of available emotions
typedef enum {
    EMOTION_NEUTRAL = 0,
    EMOTION_HAPPY,
    EMOTION_SAD,
    EMOTION_CONFUSED,
    EMOTION_ANGRY,
    EMOTION_COUNT // Helper to know how many emotions exist
} emotion_type_t;

// Parameters defining how a face looks
typedef struct {
    // Eyes
    int16_t eye_offset_x;
    int16_t eye_offset_y;
    int16_t eye_radius; // 0 = default calculated size
    int16_t eye_squint; // 0 = open, higher = narrower

    // Eyebrows
    int16_t eyebrow_offset_y;
    int16_t eyebrow_angle;  // degrees
    int16_t eyebrow_length; // 0 = default

    // Mouth
    int16_t mouth_offset_y;
    int16_t mouth_width; // 0 = default
    int16_t mouth_height;
    int16_t mouth_curve; // Positive = Smile, Negative = Frown

    // Colors (RGB565)
    uint32_t face_color;
    uint32_t eye_color;
    uint32_t mouth_color;
    uint32_t eyebrow_color;
    uint32_t bg_color;

} emotion_face_params_t;

// --- PUBLIC FUNCTIONS ---

/**
 * @brief Initialize the emotion manager
 */
int emotion_manager_init(void);

/**
 * @brief Cycle to the next emotion in the list
 * @return The new current emotion type
 */
emotion_type_t emotion_next(void);

/**
 * @brief Get the currently active emotion
 */
emotion_type_t emotion_get_current(void);

/**
 * @brief Get the string name of an emotion (e.g., "HAPPY")
 */
const char *emotion_get_name(emotion_type_t type);

/**
 * @brief Draw the current emotion to the screen
 * @param width Screen width
 * @param height Screen height
 */
void emotion_draw_current(uint16_t width, uint16_t height);

#endif /* __EMOTION_MANAGER_H__ */