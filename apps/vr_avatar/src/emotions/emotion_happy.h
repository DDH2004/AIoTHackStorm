#ifndef __EMOTION_HAPPY_H__
#define __EMOTION_HAPPY_H__

#include "emotion_manager.h"

// Happy: Yellow face, high eyebrows, big smile
static const emotion_face_params_t EMOTION_PARAMS_HAPPY = {
    .bg_color      = 0x0000,
    .face_color    = 0xFFE0, // Yellow
    .eye_color     = 0x0000, // Black
    .eyebrow_color = 0x0000, // Black
    .mouth_color   = 0x0000, // Black

    .eye_offset_y = -5, // Eyes slightly higher
    .eye_squint   = 0,

    .eyebrow_offset_y = 15, // Eyebrows raised
    .eyebrow_angle    = 10, // Slight arch
    // .eyebrow_length REMOVED

    .mouth_offset_y = 20,
    .mouth_width    = 60,
    .mouth_height   = 10, // Open mouth
    .mouth_curve    = -20 // Big smile (Negative = Smile)
};

#endif