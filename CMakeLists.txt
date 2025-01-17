cmake_minimum_required(VERSION 3.16)

# Project Setup
project(dunkelheit LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add macros for build configurations
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_definitions(DEBUG_BUILD)
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    add_compile_definitions(RELEASE_BUILD)
endif()

# Include Dependencies
add_subdirectory(deps)

set(SOURCE_FILES
    src/main.cpp
)

set(HEADER_FILES
    inc/cube_model.hpp
    inc/entity.hpp
    inc/fps_camera.hpp
    inc/item.hpp
    inc/json_file.hpp
    inc/level.hpp
    inc/mesh.hpp
    inc/model.hpp
    inc/object.hpp
    inc/player_audio_system.hpp
    inc/random_generator.hpp
    inc/settings.hpp
    inc/shader.hpp
    inc/text_renderer.hpp
    inc/texture2D.hpp
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
target_link_libraries(${PROJECT_NAME} PRIVATE glfw glad glm assimp stb freetype nlohmann_json::nlohmann_json)

# Set the IrrKlang variables
set(IRRKLANG_DIR ${CMAKE_SOURCE_DIR}/deps/irrklang)

# Include the IrrKlang headers
target_include_directories(${PROJECT_NAME} PUBLIC ${IRRKLANG_DIR}/include)

if (WIN32)
    # Link IrrKlang Library
    target_link_libraries(${PROJECT_NAME} PUBLIC ${IRRKLANG_DIR}/lib/winx64/irrKlang.lib)
    # Copy IrrKlang Library to Output Directory
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${IRRKLANG_DIR}/bin/winx64/irrKlang.dll
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/irrKlang.dll)
elseif(APPLE)
    # Link IrrKlang Library
    target_link_libraries(${PROJECT_NAME} PUBLIC ${IRRKLANG_DIR}/lib/macosx/libirrKlang.dylib)
    # Copy IrrKlang Library to Output Directory
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${IRRKLANG_DIR}/bin/macosx/libirrKlang.dylib
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/libirrKlang.dylib)
endif(WIN32)

# Final Output Message
message(STATUS "Building project ${PROJECT_NAME}")