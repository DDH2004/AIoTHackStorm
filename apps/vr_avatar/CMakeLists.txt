# @file CMakeLists.txt
# @brief VR Avatar Application

# 1. Set App Path
set(APP_PATH ${CMAKE_CURRENT_LIST_DIR})

# 2. Collect Source Files
aux_source_directory(${APP_PATH}/src APP_SRCS)
aux_source_directory(${APP_PATH}/src/emotions EMOTION_SRCS)
aux_source_directory(${APP_PATH}/src/camera CAMERA_SRCS)
aux_source_directory(${APP_PATH}/src/network NETWORK_SRCS)

# --- DRIVER SOURCES ---
# Add Display Driver Sources (TDL)
aux_source_directory(${CMAKE_SOURCE_DIR}/src/peripherals/display/tdl_display/src TDL_DISP_SRCS)
# Add Camera Driver Sources (TDL) - CRITICAL FIX
aux_source_directory(${CMAKE_SOURCE_DIR}/src/peripherals/media/tdl_camera/src TDL_CAM_SRCS)


########################################
# 3. Configure Target
########################################
add_library(tuyaapp STATIC
    ${APP_SRCS}
    ${EMOTION_SRCS}
    ${CAMERA_SRCS}
    ${NETWORK_SRCS}
)

# 4. Set Include Directories
target_include_directories(tuyaapp
    PRIVATE
    ${APP_PATH}/src
    ${APP_PATH}/src/emotions
    ${APP_PATH}/src/camera
    ${APP_PATH}/src/network
    # Display Includes
    ${CMAKE_SOURCE_DIR}/src/peripherals/display/tdl_display/include
    # Camera Includes
    ${CMAKE_SOURCE_DIR}/src/peripherals/media/tdl_camera/include
    # HTTP Client
    ${CMAKE_SOURCE_DIR}/src/libhttp/include
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
    tdl_media    # Often contains the TDL Camera logic
    tkl_media
)