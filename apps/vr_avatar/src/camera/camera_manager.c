/**
 * @file camera_manager.c
 * @brief TDL Camera Implementation (5-Second Log Test)
 */

#include "camera_manager.h"
#include "tdl_camera_manage.h"
#include "board_com_api.h"

// Fallback name
#ifndef CAMERA_NAME
#define CAMERA_NAME "camera"
#endif

static TDL_CAMERA_HANDLE_T sg_camera_handle = NULL;

// Default logger if no callback is provided
OPERATE_RET __default_camera_cb(TDL_CAMERA_HANDLE_T hdl, TDL_CAMERA_FRAME_T *frame)
{
    // Do nothing, just consume the frame
    return OPRT_OK;
}

int camera_init(camera_frame_cb_t cb)
{
    OPERATE_RET      rt  = OPRT_OK;
    TDL_CAMERA_CFG_T cfg = {0};

    PR_NOTICE("--- Initializing Camera (TDL) ---");

    sg_camera_handle = tdl_camera_find_dev(CAMERA_NAME);
    if (NULL == sg_camera_handle) {
        PR_ERR("Camera Device '%s' NOT FOUND!", CAMERA_NAME);
        return OPRT_NOT_FOUND;
    }

    cfg.fps     = CAM_FPS;
    cfg.width   = CAM_WIDTH;
    cfg.height  = CAM_HEIGHT;
    cfg.out_fmt = TDL_CAMERA_FMT_YUV422;

    // USE THE PASSED CALLBACK (or default if NULL)
    if (cb != NULL) {
        cfg.get_frame_cb = cb;
    } else {
        cfg.get_frame_cb = __default_camera_cb;
    }

    rt = tdl_camera_dev_open(sg_camera_handle, &cfg);
    if (rt != OPRT_OK) {
        PR_ERR("Failed to Open Camera: %d", rt);
        return rt;
    }

    PR_NOTICE("Camera Init Success!");
    return OPRT_OK;
}

// TDL auto-starts, so this is just a placeholder
int camera_start(void)
{
    return OPRT_OK;
}