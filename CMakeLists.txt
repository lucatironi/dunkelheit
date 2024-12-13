cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_STANDARD 20)

project(dunkelheit)

add_subdirectory(deps)
include_directories(inc)

file(GLOB PROJECT_HEADERS inc/*.hpp)
file(GLOB PROJECT_SOURCES src/*.cpp)
file(GLOB PROJECT_SHADERS src/shaders/*.vs
                          src/shaders/*.fs)


add_executable(${PROJECT_NAME} ${PROJECT_HEADERS} ${PROJECT_SOURCES} ${PROJECT_SHADERS})

target_link_libraries(${PROJECT_NAME} PRIVATE glfw glad glm assimp stb freetype)