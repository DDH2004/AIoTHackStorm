# @file CMakeLists.txt
# @brief VR Avatar App CMake

set(APP_PATH ${CMAKE_CURRENT_LIST_DIR})
aux_source_directory(${APP_PATH}/src APP_SRCS)

add_library(tuyaapp STATIC ${APP_SRCS})

# 1. Include Paths
target_include_directories(tuyaapp
    PRIVATE
    ${APP_PATH}/src
    # Include TDL (Driver Layer) headers
    ${CMAKE_SOURCE_DIR}/src/peripherals/display/tdl_display/include
    # Include Board headers (for board_com_api.h)
    ${CMAKE_SOURCE_DIR}/include/components/board
)

# 2. Link Libraries
target_link_libraries(tuyaapp
    tal_system
    tal_media
    tal_wifi
    tal_bluetooth
    # Link the Display Driver Layer
    tdl_display 
    # Link the Board Init library (crucial for board_register_hardware)
    board_init  
)