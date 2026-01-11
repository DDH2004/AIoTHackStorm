#ifndef __EMOTION_ANGRY_H__
#define __EMOTION_ANGRY_H__

#include "emotion_manager.h"

// Angry: Red face, sharp eyebrows down
static const emotion_face_params_t EMOTION_PARAMS_ANGRY = {
    .bg_color      = 0x0000,
    .face_color    = 0xF800, // Red
    .eye_color     = 0x0000,
    .eyebrow_color = 0x0000,
    .mouth_color   = 0x0000,

    .eye_offset_y = 0,
    .eye_squint   = 5, // Squinting eyes

    .eyebrow_offset_y = 5,  // Lower eyebrows
    .eyebrow_angle    = 25, // Sharp angle IN
    // .eyebrow_length REMOVED

    .mouth_offset_y = 30,
    .mouth_width    = 50,
    .mouth_height   = 5,
    .mouth_curve    = -10 // Frown
};

#endif