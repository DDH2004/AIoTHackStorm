#ifndef __EMOTION_SAD_H__
#define __EMOTION_SAD_H__

#include "emotion_manager.h"

// Sad: Blue face, drooping eyebrows, frown
static const emotion_face_params_t EMOTION_PARAMS_SAD = {
    .bg_color      = 0x0000,
    .face_color    = 0x001F, // Blue
    .eye_color     = 0xFFFF, // White eyes (teary?)
    .eyebrow_color = 0xFFFF,
    .mouth_color   = 0xFFFF,

    .eye_offset_y = 10, // Eyes lower
    .eye_squint   = 2,

    .eyebrow_offset_y = -5,  // Eyebrows low
    .eyebrow_angle    = -20, // Drooping OUT
    // .eyebrow_length REMOVED

    .mouth_offset_y = 40,
    .mouth_width    = 40,
    .mouth_height   = 5,
    .mouth_curve    = -15 // Big frown
};

#endif