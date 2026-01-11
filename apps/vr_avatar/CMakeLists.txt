# @file CMakeLists.txt
# @brief VR Avatar Application

# 1. Set App Path
set(APP_PATH ${CMAKE_CURRENT_LIST_DIR})

# 2. Collect Source Files
aux_source_directory(${APP_PATH}/src APP_SRCS)
aux_source_directory(${APP_PATH}/src/emotions EMOTION_SRCS)
aux_source_directory(${APP_PATH}/src/camera CAMERA_SRCS)
aux_source_directory(${APP_PATH}/src/ai AI_SRCS)  # <--- NEW: Add AI Sources

# --- DRIVER SOURCES (TDL) ---
aux_source_directory(${CMAKE_SOURCE_DIR}/src/peripherals/display/tdl_display/src TDL_DISP_SRCS)
aux_source_directory(${CMAKE_SOURCE_DIR}/src/peripherals/media/tdl_camera/src TDL_CAM_SRCS)
aux_source_directory(${CMAKE_SOURCE_DIR}/src/peripherals/ai/tdl_face_attri/src AI_LIB_SRCS)


########################################
# 3. Configure Target
########################################
add_library(tuyaapp STATIC
    ${APP_SRCS}
    ${EMOTION_SRCS}
    ${CAMERA_SRCS}
    ${AI_SRCS}       
    ${AI_LIB_SRCS}
)

# 4. Set Include Directories
target_include_directories(tuyaapp
    PRIVATE
    ${APP_PATH}/src
    ${APP_PATH}/src/emotions
    ${APP_PATH}/src/camera
    ${APP_PATH}/src/ai   

    # ADD THIS: Include path for the real AI header
    ${CMAKE_SOURCE_DIR}/src/peripherals/tdl_face_attri/include
    
    # Display Includes
    ${CMAKE_SOURCE_DIR}/src/peripherals/display/tdl_display/include
    # Camera Includes
    ${CMAKE_SOURCE_DIR}/src/peripherals/media/tdl_camera/include
    # Board Includes
    ${CMAKE_SOURCE_DIR}/include/components/board
)

# 5. Link Dependencies
target_link_libraries(tuyaapp
    tal_system
    tal_media
    tal_wifi
    tal_bluetooth
    
    # Libraries
    board_init
    tdl_display
    tdl_media
    tkl_media
    tdl_face_attri
)