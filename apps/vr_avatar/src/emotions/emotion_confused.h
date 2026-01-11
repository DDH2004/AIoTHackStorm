#ifndef __EMOTION_CONFUSED_H__
#define __EMOTION_CONFUSED_H__

#include "emotion_manager.h"

// Confused: One eyebrow up, one down, crooked mouth
static const emotion_face_params_t EMOTION_PARAMS_CONFUSED = {
    .bg_color      = 0x0000, // Black Background
    .face_color    = 0xFFE0, // Yellow
    .eye_color     = 0x0000, // Black Eyes
    .eyebrow_color = 0x0000, // Black Eyebrows
    .mouth_color   = 0x0000, // Black Mouth

    .eye_offset_y = 0,
    .eye_squint   = 0, // Normal eyes

    .eyebrow_offset_y = 10,
    .eyebrow_angle    = -15, // Asymmetrical slant
    // .eyebrow_length = ... REMOVED (Calculated automatically)

    .mouth_offset_y = 20,
    .mouth_width    = 40,
    .mouth_height   = 5,
    .mouth_curve    = 5 // Slightly crooked/wobbly
};

#endif