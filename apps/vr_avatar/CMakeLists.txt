# @file CMakeLists.txt
# @brief VR Avatar Application (Hybrid MCP)

set(APP_PATH ${CMAKE_CURRENT_LIST_DIR})

# 1. Collect Source Files
aux_source_directory(${APP_PATH}/src APP_SRCS)
aux_source_directory(${APP_PATH}/src/emotions EMOTION_SRCS)
aux_source_directory(${APP_PATH}/src/camera CAMERA_SRCS)
aux_source_directory(${APP_PATH}/src/ai AI_SRCS)      # <--- IMPORTANT
aux_source_directory(${APP_PATH}/src/mcp MCP_SRCS)    # <--- MCP Sources

# 2. Configure Target
add_library(tuyaapp STATIC
    ${APP_SRCS}
    ${EMOTION_SRCS}
    ${CAMERA_SRCS}
    ${AI_SRCS}
    ${MCP_SRCS}
)

# 3. Set Include Directories
target_include_directories(tuyaapp
    PRIVATE
    ${APP_PATH}/src
    ${APP_PATH}/src/emotions
    ${APP_PATH}/src/camera
    ${APP_PATH}/src/ai
    ${APP_PATH}/src/mcp

    # Driver Includes
    ${CMAKE_SOURCE_DIR}/src/peripherals/display/tdl_display/include
    ${CMAKE_SOURCE_DIR}/src/peripherals/media/tdl_camera/include
    ${CMAKE_SOURCE_DIR}/include/components/board
)

# 4. Link Dependencies
target_link_libraries(tuyaapp
    tal_system
    tal_media
    tal_wifi
    tal_bluetooth
    board_init
    tdl_display
    tdl_media
    tkl_media
    wukong_mcp   # <--- Link MCP Library
    
)