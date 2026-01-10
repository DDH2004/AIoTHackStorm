##
# @file CMakeLists.txt
# @brief VR Avatar Application
##

# Get the app name from folder
set(APP_PATH ${CMAKE_CURRENT_LIST_DIR})
get_filename_component(APP_NAME ${APP_PATH} NAME)

# Collect source files from src/ and src/emotions/
aux_source_directory(${APP_PATH}/src APP_SRCS)
aux_source_directory(${APP_PATH}/src/emotions EMOTION_SRCS)

# Combine all sources
list(APPEND APP_SRCS ${EMOTION_SRCS})

# Include directories
set(APP_INC 
    ${APP_PATH}/include
    ${APP_PATH}/src
    ${APP_PATH}/src/emotions
)

########################################
# Build library
########################################
add_library(${EXAMPLE_LIB})

target_sources(${EXAMPLE_LIB}
    PRIVATE
        ${APP_SRCS}
)

target_include_directories(${EXAMPLE_LIB}
    PRIVATE
        ${APP_INC}
)