#ifndef __EMOTION_NEUTRAL_H__
#define __EMOTION_NEUTRAL_H__

#include "emotion_manager.h"

// Neutral: Yellow face, flat features
static const emotion_face_params_t EMOTION_PARAMS_NEUTRAL = {
    .bg_color      = 0x0000,
    .face_color    = 0xFFE0, // Yellow
    .eye_color     = 0x0000,
    .eyebrow_color = 0x0000,
    .mouth_color   = 0x0000,

    .eye_offset_y = 0,
    .eye_squint   = 0,

    .eyebrow_offset_y = 10,
    .eyebrow_angle    = 0, // Flat
    // .eyebrow_length REMOVED

    .mouth_offset_y = 30,
    .mouth_width    = 50,
    .mouth_height   = 5,
    .mouth_curve    = 0 // Flat line
};

#endif