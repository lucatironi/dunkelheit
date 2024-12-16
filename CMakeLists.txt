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

include_directories(${IRRKLANG_DIR}/include)

target_link_libraries(${PROJECT_NAME} PUBLIC ${IRRKLANG_DIR}/lib/Winx64-visualStudio/irrKlang.lib)

target_include_directories(${PROJECT_NAME} PUBLIC ${IRRKLANG_DIR}/include)

add_custom_command(
    TARGET ${PROJECT_NAME}
    POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${IRRKLANG_DIR}/bin/Winx64-visualStudio/irrKlang.dll
    $<TARGET_FILE_DIR:${PROJECT_NAME}>/irrKlang.dll
)
add_custom_command(
    TARGET ${PROJECT_NAME}
    POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${IRRKLANG_DIR}/bin/Winx64-visualStudio/ikpFlac.dll
    $<TARGET_FILE_DIR:${PROJECT_NAME}>/ikpFlac.dll
)
add_custom_command(
    TARGET ${PROJECT_NAME}
    POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${IRRKLANG_DIR}/bin/Winx64-visualStudio/ikpMP3.dll
    $<TARGET_FILE_DIR:${PROJECT_NAME}>/ikpMP3.dll
)
