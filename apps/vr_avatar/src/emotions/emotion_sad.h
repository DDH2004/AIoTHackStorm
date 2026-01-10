/**
 * @file emotion_sad.h
 * @brief Sad emotion face parameters
 */

#ifndef __EMOTION_SAD_H__
#define __EMOTION_SAD_H__

#include "emotion_manager.h"

// Sad face: frown, droopy eyebrows, downcast eyes
static const emotion_face_params_t EMOTION_PARAMS_SAD = {
    // Eyes - normal but lower position suggests downcast
    .eye_offset_x = 0,
    .eye_offset_y = 5,      // Slightly lower
    .eye_radius = 0,
    .eye_squint = 0,
    
    // Eyebrows - inner edges raised (worried look)
    .eyebrow_offset_y = 12,
    .eyebrow_angle = -20,   // Inner edges up, outer edges down
    .eyebrow_length = 0,
    
    // Mouth - frown
    .mouth_offset_y = 5,
    .mouth_width = 0,
    .mouth_height = 8,
    .mouth_curve = -25,     // Downward curve
    
    // Colors
    .face_color = 0xFFE0,   // Yellow
    .eye_color = 0x0000,    // Black
    .mouth_color = 0x0000,  // Black
    .eyebrow_color = 0x0000,// Black
    .bg_color = 0x001F      // Blue (sad vibes)
};

#endif /* __EMOTION_SAD_H__ */