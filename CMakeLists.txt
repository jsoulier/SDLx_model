cmake_minimum_required(VERSION 3.24)
project(SDLx_model)

set(BINARY_DIR ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${BINARY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${BINARY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${BINARY_DIR})
make_directory(${BINARY_DIR})

if(PROJECT_IS_TOP_LEVEL)
    add_subdirectory(lib/SDL)
    set(GLM_BUILD_LIBRARY OFF)
    add_subdirectory(lib/glm)
endif()

add_library(SDLx_model
    src/SDL_model.cpp
    src/internal.cpp
    src/stb_image.c
    src/tiny_obj_loader.cpp
    src/vox_obj.cpp
    src/vox_raw.cpp
)
target_include_directories(SDLx_model PUBLIC include)
target_link_libraries(SDLx_model PRIVATE SDL3::SDL3)
set_target_properties(SDLx_model PROPERTIES CXX_STANDARD 23)
add_library(SDLx_model::SDLx_model ALIAS SDLx_model)

if(PROJECT_IS_TOP_LEVEL)
    add_executable(SDLx_model_test WIN32 test/test.cpp)
    target_link_libraries(SDLx_model_test PRIVATE SDL3::SDL3 SDLx_model::SDLx_model glm)
    set_target_properties(SDLx_model_test PROPERTIES CXX_STANDARD 23)
endif()