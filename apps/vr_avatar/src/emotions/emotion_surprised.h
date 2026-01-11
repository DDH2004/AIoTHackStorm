#ifndef __EMOTION_SURPRISED_H__
#define __EMOTION_SURPRISED_H__

#include "emotion_manager.h"

// Surprised: Yellow face, wide eyes, open O mouth
static const emotion_face_params_t EMOTION_PARAMS_SURPRISED = {
    .bg_color      = 0x0000,
    .face_color    = 0xFFE0, // Yellow
    .eye_color     = 0x0000, // Black
    .eyebrow_color = 0x0000, // Black
    .mouth_color   = 0x0000, // Black

    .eye_offset_y = 0,
    .eye_squint   = -5, // Wide eyes (Negative squint = larger radius)

    .eyebrow_offset_y = 15, // Eyebrows raised high
    .eyebrow_angle    = 0,  // Flat eyebrows

    .mouth_offset_y = 30,
    .mouth_width    = 30, // Narrower
    .mouth_height   = 30, // Tall (Open O)
    .mouth_curve    = 0   // No curve (Circle/Oval)
};

#endif
