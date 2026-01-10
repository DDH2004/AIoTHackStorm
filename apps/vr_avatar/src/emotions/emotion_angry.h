/**
 * @file emotion_angry.h
 * @brief Angry emotion face parameters
 */

#ifndef __EMOTION_ANGRY_H__
#define __EMOTION_ANGRY_H__

#include "emotion_manager.h"

// Angry face: furrowed brows, intense eyes, tight mouth
static const emotion_face_params_t EMOTION_PARAMS_ANGRY = {
    // Eyes - intense, slightly squinted
    .eye_offset_x = 0,
    .eye_offset_y = 0,
    .eye_radius = 0,
    .eye_squint = 5,        // Squinted/intense
    
    // Eyebrows - sharply angled down toward center
    .eyebrow_offset_y = 8,  // Lower, closer to eyes
    .eyebrow_angle = 30,    // Angled down toward center
    .eyebrow_length = 0,
    
    // Mouth - tight, slightly downturned
    .mouth_offset_y = 0,
    .mouth_width = 0,
    .mouth_height = 6,
    .mouth_curve = -10,     // Slight frown
    
    // Colors
    .face_color = 0xFBE0,   // Orange-yellow
    .eye_color = 0x0000,    // Black
    .mouth_color = 0x0000,  // Black
    .eyebrow_color = 0x0000,// Black
    .bg_color = 0xF800      // Red (angry vibes)
};

#endif /* __EMOTION_ANGRY_H__ */