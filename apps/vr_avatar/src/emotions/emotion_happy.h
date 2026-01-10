/**
 * @file emotion_happy.h
 * @brief Happy emotion face parameters
 */

#ifndef __EMOTION_HAPPY_H__
#define __EMOTION_HAPPY_H__

#include "emotion_manager.h"

// Happy face: big smile, raised eyebrows, slightly squinted eyes
static const emotion_face_params_t EMOTION_PARAMS_HAPPY = {
    // Eyes - slightly squinted from smiling
    .eye_offset_x = 0,
    .eye_offset_y = 0,
    .eye_radius   = 0,
    .eye_squint   = 3, // Slight squint

    // Eyebrows - raised, slight arch
    .eyebrow_offset_y = 20,
    .eyebrow_angle    = 10, // Slight raise on outer edges
    .eyebrow_length   = 0,

    // Mouth - wide smile
    .mouth_offset_y = 0,
    .mouth_width    = 0,
    .mouth_height   = 12, // Taller for smile
    .mouth_curve    = 30, // Strong upward curve

    // Colors
    .face_color    = 0xFFE0, // Yellow
    .eye_color     = 0x0000, // Black
    .mouth_color   = 0x0000, // Black
    .eyebrow_color = 0x0000, // Black
    .bg_color      = 0x07E0  // Green (happy vibes)
};

#endif /* __EMOTION_HAPPY_H__ */