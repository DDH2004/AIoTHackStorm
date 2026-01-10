#ifndef __CAMERA_MANAGER_H__
#define __CAMERA_MANAGER_H__

#include "tal_api.h"
#include "tdl_camera_manage.h"

// Resolution must match QVGA to align with 320px screen width
#define CAM_WIDTH  480
#define CAM_HEIGHT 480
#define CAM_FPS    30

typedef OPERATE_RET (*camera_frame_cb_t)(TDL_CAMERA_HANDLE_T hdl, TDL_CAMERA_FRAME_T *frame);

int camera_init(camera_frame_cb_t cb);
int camera_start(void);

#endif