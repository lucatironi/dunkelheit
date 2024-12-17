cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_STANDARD 20)

project(dunkelheit)

add_subdirectory(deps)
include_directories(inc)

file(GLOB PROJECT_HEADERS inc/*.hpp)
file(GLOB PROJECT_SOURCES src/*.cpp)
file(GLOB PROJECT_SHADERS src/shaders/*.vs
                          src/shaders/*.fs)

configure_file(config/root_directory.hpp.in config/root_directory.hpp)
include_directories(${CMAKE_BINARY_DIR}/config)

add_executable(${PROJECT_NAME} ${PROJECT_HEADERS} ${PROJECT_SOURCES} ${PROJECT_SHADERS})

target_link_libraries(${PROJECT_NAME} PRIVATE glfw glad glm assimp stb freetype)

# Include the IrrKlang headers
set(IRRKLANG_DIR ${CMAKE_SOURCE_DIR}/deps/irrklang)

if (WIN32)
    set(ARCH_DIR /Winx64-visualStudio)
    set(ARCH_LIB_EXT dll)
    set(IRRKLANG_LIB_NAME irrklang)
    target_link_libraries(${PROJECT_NAME} PUBLIC ${IRRKLANG_DIR}/lib/Winx64-visualStudio/irrKlang.lib)
elseif(APPLE)
    set(ARCH_DIR /macosx-gcc)
    set(ARCH_LIB_EXT dylib)
    set(IRRKLANG_LIB_NAME libirrklang)
    target_link_libraries(${PROJECT_NAME} PUBLIC ${IRRKLANG_DIR}/bin/macosx-gcc/libirrKlang.dylib)
endif()

include_directories(${IRRKLANG_DIR}/include)

target_include_directories(${PROJECT_NAME} PUBLIC ${IRRKLANG_DIR}/include)

add_custom_command(
    TARGET ${PROJECT_NAME}
    POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${IRRKLANG_DIR}/bin${ARCH_DIR}/${IRRKLANG_LIB_NAME}.${ARCH_LIB_EXT}
    $<TARGET_FILE_DIR:${PROJECT_NAME}>/${IRRKLANG_LIB_NAME}.${ARCH_LIB_EXT}
)
add_custom_command(
    TARGET ${PROJECT_NAME}
    POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${IRRKLANG_DIR}/bin${ARCH_DIR}/ikpFlac.${ARCH_LIB_EXT}
    $<TARGET_FILE_DIR:${PROJECT_NAME}>/ikpFlac.${ARCH_LIB_EXT}
)
add_custom_command(
    TARGET ${PROJECT_NAME}
    POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${IRRKLANG_DIR}/bin${ARCH_DIR}/ikpMP3.${ARCH_LIB_EXT}
    $<TARGET_FILE_DIR:${PROJECT_NAME}>/ikpMP3.${ARCH_LIB_EXT}
)
