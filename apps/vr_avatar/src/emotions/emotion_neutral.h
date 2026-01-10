/**
 * @file emotion_neutral.h
 * @brief Neutral emotion face parameters
 */

#ifndef __EMOTION_NEUTRAL_H__
#define __EMOTION_NEUTRAL_H__

#include "emotion_manager.h"

// Neutral face: relaxed, default expression
static const emotion_face_params_t EMOTION_PARAMS_NEUTRAL = {
    // Eyes - normal, centered
    .eye_offset_x = 0,      // Will be calculated as fraction of face
    .eye_offset_y = 0,
    .eye_radius = 0,        // Will be calculated
    .eye_squint = 0,        // No squint
    
    // Eyebrows - flat, neutral
    .eyebrow_offset_y = 15,
    .eyebrow_angle = 0,     // Flat
    .eyebrow_length = 0,    // Will be calculated
    
    // Mouth - straight line
    .mouth_offset_y = 0,
    .mouth_width = 0,       // Will be calculated
    .mouth_height = 4,      // Thin line
    .mouth_curve = 0,       // Straight
    
    // Colors
    .face_color = 0xFFE0,   // Yellow
    .eye_color = 0x0000,    // Black
    .mouth_color = 0x0000,  // Black
    .eyebrow_color = 0x0000,// Black
    .bg_color = 0x07FF      // Cyan
};

#endif /* __EMOTION_NEUTRAL_H__ */