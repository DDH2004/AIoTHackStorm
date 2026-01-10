/**
 * @file camera_manager.h
 * @brief Handles Camera Initialization and Frame Capture
 */

#ifndef __CAMERA_MANAGER_H__
#define __CAMERA_MANAGER_H__

#include "tal_api.h"
#include "tdl_camera_manage.h"

#define CAM_WIDTH  320
#define CAM_HEIGHT 480
#define CAM_FPS    30

// We define a type for the callback so we can pass it around
typedef OPERATE_RET (*camera_frame_cb_t)(TDL_CAMERA_HANDLE_T hdl, TDL_CAMERA_FRAME_T *frame);

/**
 * @brief Initialize the Camera
 * @param cb The function to call when a new frame arrives (Can be NULL)
 */
int camera_init(camera_frame_cb_t cb);

int camera_start(void);

#endif