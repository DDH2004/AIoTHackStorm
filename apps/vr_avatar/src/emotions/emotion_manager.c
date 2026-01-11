/**
 * @file emotion_manager.c
 * @brief Emotion management implementation (Refactored for Tracking)
 */

#include "emotion_manager.h"
#include "tdl_display_manage.h"
#include "tdl_display_draw.h"
#include "tal_api.h"

// Include all emotion definitions
#include "emotion_neutral.h"
#include "emotion_happy.h"
#include "emotion_sad.h"
#include "emotion_angry.h"
#include "emotion_confused.h"

// --- EXTERNAL REFERENCES ---
extern TDL_DISP_HANDLE_T      sg_tdl_disp_hdl;
extern TDL_DISP_DEV_INFO_T    sg_display_info;
extern TDL_DISP_FRAME_BUFF_T *sg_p_display_fb;

// --- MODULE STATE ---
static emotion_type_t sg_current_emotion = EMOTION_NEUTRAL;

// --- EMOTION PARAMS ---
static const char                  *sg_emotion_names[]  = {"NEUTRAL", "HAPPY", "SAD", "ANGRY", "CONFUSED"};
static const emotion_face_params_t *sg_emotion_params[] = {&EMOTION_PARAMS_NEUTRAL, &EMOTION_PARAMS_HAPPY,
                                                           &EMOTION_PARAMS_SAD, &EMOTION_PARAMS_ANGRY,
                                                           &EMOTION_PARAMS_CONFUSED};

// ==========================================
// DRAWING HELPERS (Pixels)
// ==========================================

static void draw_circle(int16_t cx, int16_t cy, int16_t r, uint32_t color)
{
    for (int16_t y = -r; y <= r; y++) {
        for (int16_t x = -r; x <= r; x++) {
            if (x * x + y * y <= r * r) {
                int16_t px = cx + x;
                int16_t py = cy + y;
                if (px >= 0 && py >= 0) {
                    tdl_disp_draw_point(sg_p_display_fb, px, py, color, sg_display_info.is_swap);
                }
            }
        }
    }
}

static void draw_ellipse(int16_t cx, int16_t cy, int16_t rx, int16_t ry, uint32_t color)
{
    for (int16_t y = -ry; y <= ry; y++) {
        for (int16_t x = -rx; x <= rx; x++) {
            if ((x * x * ry * ry + y * y * rx * rx) <= (rx * rx * ry * ry)) {
                int16_t px = cx + x;
                int16_t py = cy + y;
                if (px >= 0 && py >= 0) {
                    tdl_disp_draw_point(sg_p_display_fb, px, py, color, sg_display_info.is_swap);
                }
            }
        }
    }
}

static void draw_mouth(int16_t cx, int16_t cy, int16_t width, int16_t height, int16_t curve, uint32_t color)
{
    int16_t half_width = width / 2;
    for (int16_t x = -half_width; x <= half_width; x++) {
        int16_t curve_offset = 0;
        if (curve != 0) {
            curve_offset = (curve * x * x) / (half_width * half_width);
        }
        int16_t px = cx + x;
        int16_t py = cy + curve_offset;
        for (int16_t dy = 0; dy < height / 2; dy++) {
            if (px >= 0 && py + dy >= 0)
                tdl_disp_draw_point(sg_p_display_fb, px, py + dy, color, sg_display_info.is_swap);
            if (px >= 0 && py - dy >= 0 && dy > 0)
                tdl_disp_draw_point(sg_p_display_fb, px, py - dy, color, sg_display_info.is_swap);
        }
    }
}

static void draw_eyebrow(int16_t cx, int16_t cy, int16_t length, int16_t angle, int side, uint32_t color)
{
    int16_t half_len = length / 2;
    for (int16_t i = -half_len; i <= half_len; i++) {
        int16_t y_offset = (angle * i * side) / half_len;
        int16_t px       = cx + i;
        int16_t py       = cy + y_offset;
        for (int16_t dy = -1; dy <= 1; dy++) {
            if (px >= 0 && py + dy >= 0) {
                tdl_disp_draw_point(sg_p_display_fb, px, py + dy, color, sg_display_info.is_swap);
            }
        }
    }
}

// ==========================================
// CORE DRAWING LOGIC (Internal)
// ==========================================

// This function does the heavy lifting using the parameters
static void __draw_face_impl(int16_t cx, int16_t cy, int16_t face_radius, const emotion_face_params_t *params)
{
    // 1. Calculate dynamic sizes
    int16_t eye_offset_x   = face_radius / 3;
    int16_t eye_offset_y   = face_radius / 4 + params->eye_offset_y;
    int16_t eye_radius     = face_radius / 8;
    int16_t mouth_width    = face_radius / 2;
    int16_t eyebrow_length = face_radius / 3;

    // 2. Draw Face Circle
    draw_circle(cx, cy, face_radius, params->face_color);

    // 3. Draw Eyes
    int16_t eye_ry = eye_radius - params->eye_squint;
    if (eye_ry < 3)
        eye_ry = 3;

    draw_ellipse(cx - eye_offset_x, cy - eye_offset_y, eye_radius, eye_ry, params->eye_color);
    draw_ellipse(cx + eye_offset_x, cy - eye_offset_y, eye_radius, eye_ry, params->eye_color);

    // 4. Draw Eyebrows
    int16_t eyebrow_y = cy - eye_offset_y - eye_radius - params->eyebrow_offset_y;
    draw_eyebrow(cx - eye_offset_x, eyebrow_y, eyebrow_length, params->eyebrow_angle, -1, params->eyebrow_color);
    draw_eyebrow(cx + eye_offset_x, eyebrow_y, eyebrow_length, params->eyebrow_angle, 1, params->eyebrow_color);

    // 5. Draw Mouth
    int16_t mouth_y = cy + face_radius / 3 + params->mouth_offset_y;
    draw_mouth(cx, mouth_y, mouth_width, params->mouth_height, params->mouth_curve, params->mouth_color);
}

// ==========================================
// PUBLIC API
// ==========================================

int emotion_manager_init(void)
{
    sg_current_emotion = EMOTION_NEUTRAL;
    PR_NOTICE("Emotion Manager initialized");
    return 0;
}

emotion_type_t emotion_get_current(void)
{
    return sg_current_emotion;
}
void emotion_set_current(emotion_type_t emotion)
{
    if (emotion < EMOTION_COUNT)
        sg_current_emotion = emotion;
}
emotion_type_t emotion_next(void)
{
    sg_current_emotion = (sg_current_emotion + 1) % EMOTION_COUNT;
    return sg_current_emotion;
}
const char *emotion_get_name(emotion_type_t emotion)
{
    return (emotion < EMOTION_COUNT) ? sg_emotion_names[emotion] : "UNKNOWN";
}

// This is the function called by MAIN LOOP to draw at AI coordinates
void emotion_draw_at(int x, int y)
{
    // Get current emotion parameters
    const emotion_face_params_t *params = sg_emotion_params[sg_current_emotion];

    // Fixed radius for the tracking face (100px)
    int16_t radius = 100;

    // Use our internal helper
    __draw_face_impl(x, y, radius, params);
}

// Legacy function (draws at center of screen)
void emotion_draw_current(uint16_t screen_width, uint16_t screen_height)
{
    const emotion_face_params_t *params = sg_emotion_params[sg_current_emotion];

    int16_t cx     = screen_width / 2;
    int16_t cy     = screen_height / 2;
    int16_t radius = (screen_width < screen_height ? screen_width : screen_height) / 3;

    // Clear background first (optional, usually done by double buffer clear in main)
    tdl_disp_draw_fill_full(sg_p_display_fb, params->bg_color, sg_display_info.is_swap);

    __draw_face_impl(cx, cy, radius, params);

    tdl_disp_dev_flush(sg_tdl_disp_hdl, sg_p_display_fb);
}