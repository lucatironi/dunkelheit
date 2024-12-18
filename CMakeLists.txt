cmake_minimum_required(VERSION 3.16)

# Project Setup
project(dunkelheit LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include Dependencies
add_subdirectory(deps)

set(SOURCE_FILES
    src/main.cpp
)

set(HEADER_FILES
    inc/cube_model.hpp
    inc/file_system.hpp
    inc/footsteps_system.hpp
    inc/fps_camera.hpp
    inc/level.hpp
    inc/mesh.hpp
    inc/model.hpp
    inc/object.hpp
    inc/random_generator.hpp
    inc/shader.hpp
    inc/text_renderer.hpp
    inc/texture2D.hpp
    inc/weapon.hpp
)

# Configure Root Directory Header
configure_file(config/root_directory.hpp.in ${CMAKE_BINARY_DIR}/config/root_directory.hpp)

# Add Executable First
add_executable(${PROJECT_NAME} ${SOURCE_FILES} ${HEADER_FILES})

if (WIN32)
    set_target_properties(${PROJECT_NAME} PROPERTIES
    LINK_FLAGS_RELEASE "/ENTRY:mainCRTStartup /SUBSYSTEM:WINDOWS"
)
endif()

# Include Directories
target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_SOURCE_DIR}/inc
    ${CMAKE_BINARY_DIR}/config
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
target_link_libraries(${PROJECT_NAME} PRIVATE glfw glad glm assimp stb freetype)

# Set the IrrKlang variables
set(IRRKLANG_DIR ${CMAKE_SOURCE_DIR}/deps/irrklang)
if (WIN32)
    set(ARCH_DIR /winx64)
    set(ARCH_LIB_EXT dll)
    set(IRRKLANG_LIB_NAME irrklang)
    # Link IrrKlang Library
    target_link_libraries(${PROJECT_NAME} PUBLIC ${IRRKLANG_DIR}/lib/winx64/irrKlang.lib)
elseif(APPLE)
    set(ARCH_DIR /macosx)
    set(ARCH_LIB_EXT dylib)
    set(IRRKLANG_LIB_NAME libirrklang)
    # Link IrrKlang Library
    target_link_libraries(${PROJECT_NAME} PUBLIC ${IRRKLANG_DIR}/lib/macosx/libirrKlang.dylib)
endif(WIN32)

# Include the IrrKlang headers
target_include_directories(${PROJECT_NAME} PUBLIC ${IRRKLANG_DIR}/include)

set(IRRKLANG_LIBS
    ${IRRKLANG_LIB_NAME}.${ARCH_LIB_EXT}
    ikpFlac.${ARCH_LIB_EXT}
    ikpMP3.${ARCH_LIB_EXT}
)

foreach(IRRKLANG_LIB ${IRRKLANG_LIBS})
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${IRRKLANG_DIR}/bin${ARCH_DIR}/${IRRKLANG_LIB}
    $<TARGET_FILE_DIR:${PROJECT_NAME}>/${IRRKLANG_LIB}
)
endforeach()

# Final Output Message
message(STATUS "Building project ${PROJECT_NAME}")