/**
 * @file emotion_manager.h
 * @brief Emotion management header
 */

#ifndef __EMOTION_MANAGER_H__
#define __EMOTION_MANAGER_H__

#include "tuya_cloud_types.h"

// Enum for supported emotions
typedef enum {
    EMOTION_NEUTRAL = 0,
    EMOTION_HAPPY,
    EMOTION_SAD,
    EMOTION_ANGRY,
    EMOTION_CONFUSED,
    EMOTION_SURPRISED,
    EMOTION_COUNT // Total number of emotions
} emotion_type_t;

// Structure for facial parameters
typedef struct {
    uint32_t bg_color;
    uint32_t face_color;
    uint32_t eye_color;
    uint32_t eyebrow_color;
    uint32_t mouth_color;
    int16_t  eye_offset_y;     // Vertical offset for eyes from center
    int16_t  eye_squint;       // How much to squint eyes (subtract from radius)
    int16_t  eyebrow_offset_y; // Distance above eyes
    int16_t  eyebrow_angle;    // Angle multiplier for eyebrows
    int16_t  mouth_offset_y;   // Vertical offset for mouth
    int16_t  mouth_width;      // Width multiplier
    int16_t  mouth_height;     // Height/Thickness
    int16_t  mouth_curve;      // Curve factor (+ for smile, - for frown)
} emotion_face_params_t;

// --- API FUNCTIONS ---

/**
 * @brief Initialize the emotion manager
 * @return 0 on success
 */
int emotion_manager_init(void);

/**
 * @brief Get the currently active emotion
 */
emotion_type_t emotion_get_current(void);

/**
 * @brief Set the current emotion manually
 * @param emotion The emotion ID to set
 */
void emotion_set_current(emotion_type_t emotion); // <--- THIS WAS MISSING

/**
 * @brief Cycle to the next emotion (for testing)
 */
emotion_type_t emotion_next(void);

/**
 * @brief Get the string name of an emotion
 */
const char *emotion_get_name(emotion_type_t emotion);

/**
 * @brief Draw the current emotion at the center of the screen
 */
void emotion_draw_current(uint16_t screen_width, uint16_t screen_height);

/**
 * @brief Draw the current emotion at specific X/Y coordinates
 * @param x Center X coordinate
 * @param y Center Y coordinate
 */
void emotion_draw_at(int x, int y); // <--- ADDED THIS TOO

#endif // __EMOTION_MANAGER_H__