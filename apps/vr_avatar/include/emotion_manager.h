/**
 * @file emotion_manager.h
 * @brief Emotion management for VR Avatar - defines emotion types and drawing functions
 */

#ifndef __EMOTION_MANAGER_H__
#define __EMOTION_MANAGER_H__

#include <stdint.h>

// --- EMOTION TYPES ---
typedef enum {
    EMOTION_NEUTRAL = 0,
    EMOTION_HAPPY,
    EMOTION_SAD,
    EMOTION_ANGRY,
    EMOTION_CONFUSED,
    EMOTION_MAX
} emotion_type_t;

// --- FACE COMPONENT PARAMETERS ---
typedef struct {
    // Eye parameters
    int16_t eye_offset_x;       // Horizontal distance from center
    int16_t eye_offset_y;       // Vertical distance from center
    int16_t eye_radius;         // Size of eyes
    int16_t eye_squint;         // 0 = normal, positive = squint (smaller height)
    
    // Eyebrow parameters
    int16_t eyebrow_offset_y;   // Distance above eyes
    int16_t eyebrow_angle;      // Angle in degrees (-45 to 45)
    int16_t eyebrow_length;     // Length of eyebrow
    
    // Mouth parameters
    int16_t mouth_offset_y;     // Distance below center
    int16_t mouth_width;        // Width of mouth
    int16_t mouth_height;       // Height of mouth
    int16_t mouth_curve;        // Positive = smile, negative = frown, 0 = straight
    
    // Colors (RGB565)
    uint32_t face_color;
    uint32_t eye_color;
    uint32_t mouth_color;
    uint32_t eyebrow_color;
    uint32_t bg_color;
} emotion_face_params_t;

// --- API FUNCTIONS ---

/**
 * @brief Initialize the emotion manager
 * @return 0 on success, -1 on failure
 */
int emotion_manager_init(void);

/**
 * @brief Get the current emotion
 * @return Current emotion type
 */
emotion_type_t emotion_get_current(void);

/**
 * @brief Set the current emotion
 * @param emotion The emotion to set
 */
void emotion_set_current(emotion_type_t emotion);

/**
 * @brief Cycle to the next emotion
 * @return The new emotion type
 */
emotion_type_t emotion_next(void);

/**
 * @brief Get the name of an emotion as a string
 * @param emotion The emotion type
 * @return String name of the emotion
 */
const char* emotion_get_name(emotion_type_t emotion);

/**
 * @brief Get the face parameters for a specific emotion
 * @param emotion The emotion type
 * @return Pointer to the face parameters (do not free)
 */
const emotion_face_params_t* emotion_get_params(emotion_type_t emotion);

/**
 * @brief Draw the face for the current emotion
 * @param screen_width Display width
 * @param screen_height Display height
 */
void emotion_draw_current(uint16_t screen_width, uint16_t screen_height);

/**
 * @brief Draw the face for a specific emotion
 * @param emotion The emotion to draw
 * @param screen_width Display width
 * @param screen_height Display height
 */
void emotion_draw(emotion_type_t emotion, uint16_t screen_width, uint16_t screen_height);

#endif /* __EMOTION_MANAGER_H__ */