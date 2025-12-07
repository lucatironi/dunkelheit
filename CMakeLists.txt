cmake_minimum_required(VERSION 4.0)

# Project Setup
project(dunkelheit LANGUAGES CXX C)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_COMPILER_LAUNCHER ccache)
set(CMAKE_CXX_COMPILER_LAUNCHER ccache)

# Add macros for build configurations
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_definitions(DEBUG_BUILD)
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    add_compile_definitions(RELEASE_BUILD)
endif()

# Include Dependencies
add_subdirectory(deps)

set(SOURCE_FILES
    src/audio_engine.cpp
    src/main.cpp
)

set(HEADER_FILES
    inc/animated_model.hpp
    inc/audio_engine.hpp
    inc/basic_model.hpp
    inc/cube_model.hpp
    inc/entity.hpp
    inc/fps_camera.hpp
    inc/item.hpp
    inc/json_file.hpp
    inc/level.hpp
    inc/mesh.hpp
    inc/model.hpp
    inc/model_loader.hpp
    inc/object.hpp
    inc/pixelator.hpp
    inc/player_audio_system.hpp
    inc/random_generator.hpp
    inc/settings.hpp
    inc/shader.hpp
    inc/text_renderer.hpp
    inc/texture_2D.hpp
    inc/torch.hpp
    inc/working_directory.hpp
)

# Add Executable First
add_executable(${PROJECT_NAME} ${SOURCE_FILES} ${HEADER_FILES})

if (WIN32)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        LINK_FLAGS_RELEASE "/ENTRY:mainCRTStartup /SUBSYSTEM:WINDOWS")
    set_target_properties(${PROJECT_NAME} PROPERTIES
        LINK_FLAGS_MINSIZEREL "/ENTRY:mainCRTStartup /SUBSYSTEM:WINDOWS")
endif()

# Include Directories
target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_SOURCE_DIR}/inc
)

# Copy configuration file to Output Directory
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
    ${CMAKE_CURRENT_SOURCE_DIR}/config/settings.json
    $<TARGET_FILE_DIR:${PROJECT_NAME}>/config/settings.json
)

# Copy Shaders to Output Directory
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
    ${CMAKE_CURRENT_SOURCE_DIR}/src/shaders
    $<TARGET_FILE_DIR:${PROJECT_NAME}>/shaders
)

# Copy Assets to Output Directory
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
    ${CMAKE_CURRENT_SOURCE_DIR}/assets
    $<TARGET_FILE_DIR:${PROJECT_NAME}>/assets
)

# Link External Libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    glfw
    glad
    glm
    assimp
    stb
    freetype
    nlohmann_json::nlohmann_json
    miniaudio
    ozz_animation_offline
    ozz_animation
)

# Final Output Message
message(STATUS "Building project ${PROJECT_NAME}")