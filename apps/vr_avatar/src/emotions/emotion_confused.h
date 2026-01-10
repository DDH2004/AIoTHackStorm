/**
 * @file emotion_confused.h
 * @brief Confused emotion face parameters
 */

#ifndef __EMOTION_CONFUSED_H__
#define __EMOTION_CONFUSED_H__

#include "emotion_manager.h"

// Confused face: one eyebrow raised, asymmetric, wavy mouth
static const emotion_face_params_t EMOTION_PARAMS_CONFUSED = {
    // Eyes - one slightly larger (raised eyebrow side)
    .eye_offset_x = 0,
    .eye_offset_y = 0,
    .eye_radius = 0,
    .eye_squint = -2,       // Negative = slightly wider
    
    // Eyebrows - asymmetric (one raised)
    .eyebrow_offset_y = 18,
    .eyebrow_angle = -15,   // Tilted
    .eyebrow_length = 0,
    
    // Mouth - wavy/uncertain
    .mouth_offset_y = 0,
    .mouth_width = 0,
    .mouth_height = 6,
    .mouth_curve = 5,       // Slight uncertain curve
    
    // Colors
    .face_color = 0xFFE0,   // Yellow
    .eye_color = 0x0000,    // Black
    .mouth_color = 0x0000,  // Black
    .eyebrow_color = 0x0000,// Black
    .bg_color = 0xF81F      // Magenta (confused vibes)
};

#endif /* __EMOTION_CONFUSED_H__ */