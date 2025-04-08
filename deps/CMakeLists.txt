include(FetchContent)

# glfw
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.4
    GIT_PROGRESS TRUE
    GIT_SHALLOW TRUE)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(glfw)

# glad
FetchContent_Declare(
    glad
    GIT_REPOSITORY https://github.com/Dav1dde/glad.git
    GIT_TAG v0.1.36
    GIT_PROGRESS TRUE
    GIT_SHALLOW TRUE)

FetchContent_MakeAvailable(glad)

# glm
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 1.0.1
    GIT_PROGRESS TRUE
    GIT_SHALLOW TRUE)

FetchContent_MakeAvailable(glm)

# assimp
FetchContent_Declare(
    assimp
    GIT_REPOSITORY https://github.com/assimp/assimp.git
    GIT_TAG v5.4.2
    GIT_PROGRESS TRUE
    GIT_SHALLOW TRUE)

set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_FBX_IMPORTER ON CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_GLTF_IMPORTER ON CACHE BOOL "" FORCE)
set(ASSIMP_NO_EXPORT ON CACHE BOOL "" FORCE)
set(DASSIMP_WARNINGS_AS_ERRORS OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(assimp)

# stb
FetchContent_Declare(
    stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG master
    GIT_PROGRESS TRUE
    GIT_SHALLOW TRUE)

FetchContent_MakeAvailable(stb)

add_library(stb INTERFACE ${stb_SOURCE_DIR}/stb_image.h)
target_include_directories(stb INTERFACE ${stb_SOURCE_DIR})

# freetype2
FetchContent_Declare(
    freetype
    GIT_REPOSITORY https://gitlab.freedesktop.org/freetype/freetype
    GIT_TAG VER-2-13-2
    GIT_PROGRESS TRUE)

FetchContent_MakeAvailable(freetype)

# nlohmann/json
FetchContent_Declare(
    json
    URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz)

FetchContent_MakeAvailable(json)

# miniaudio
FetchContent_Declare(
    miniaudio
    GIT_REPOSITORY https://github.com/mackron/miniaudio.git
    GIT_TAG 0.11.22
    GIT_PROGRESS TRUE
    GIT_SHALLOW TRUE)

FetchContent_MakeAvailable(miniaudio)

target_include_directories(miniaudio INTERFACE ${miniaudio_SOURCE_DIR})