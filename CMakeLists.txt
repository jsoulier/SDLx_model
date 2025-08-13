cmake_minimum_required(VERSION 3.24)
project(SDLx_model)

if(PROJECT_IS_TOP_LEVEL)
    set(BINARY_DIR ${CMAKE_BINARY_DIR}/bin)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${BINARY_DIR})
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${BINARY_DIR})
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${BINARY_DIR})
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${BINARY_DIR})
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${BINARY_DIR})
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${BINARY_DIR})
    make_directory(${BINARY_DIR})
    set(GLM_BUILD_LIBRARY OFF)
    add_subdirectory(test/lib/SDL)
    add_subdirectory(test/lib/glm)
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
    add_executable(SDLx_model_test WIN32 test/main.cpp test/shader.cpp test/pipeline.cpp)
    target_include_directories(SDLx_model_test PUBLIC test/lib/json)
    target_link_libraries(SDLx_model_test PRIVATE SDL3::SDL3 SDLx_model::SDLx_model glm)
    set_target_properties(SDLx_model_test PROPERTIES CXX_STANDARD 23)
    function(add_shader FILE)
        set(DEPENDS ${ARGN})
        set(HLSL ${CMAKE_SOURCE_DIR}/test/shaders/${FILE})
        set(SPV ${CMAKE_SOURCE_DIR}/test/shaders/bin/${FILE}.spv)
        set(DXIL ${CMAKE_SOURCE_DIR}/test/shaders/bin/${FILE}.dxil)
        set(MSL ${CMAKE_SOURCE_DIR}/test/shaders/bin/${FILE}.msl)
        set(JSON ${CMAKE_SOURCE_DIR}/test/shaders/bin/${FILE}.json)
        if(MSVC)
            set(SHADERCROSS test/lib/SDL_shadercross/msvc/shadercross.exe)
        endif()
        function(compile OUTPUT)
            add_custom_command(
                OUTPUT ${OUTPUT}
                COMMAND ${SHADERCROSS} ${HLSL} -s hlsl -o ${OUTPUT} -I src
                WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
                DEPENDS ${HLSL} ${DEPENDS}
                COMMENT ${OUTPUT}
            )
            get_filename_component(NAME ${OUTPUT} NAME)
            string(REPLACE . _ NAME ${NAME})
            set(NAME compile_${NAME})
            add_custom_target(${NAME} DEPENDS ${OUTPUT})
            add_dependencies(SDLx_model_test ${NAME})
        endfunction()
        if (MSVC)
            compile(${SPV})
            compile(${DXIL})
            compile(${MSL})
            compile(${JSON})
        endif()
        function(package OUTPUT)
            get_filename_component(NAME ${OUTPUT} NAME)
            set(BINARY ${BINARY_DIR}/${NAME})
            add_custom_command(
                OUTPUT ${BINARY}
                COMMAND ${CMAKE_COMMAND} -E copy ${OUTPUT} ${BINARY}
                WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
                DEPENDS ${OUTPUT}
                COMMENT ${BINARY}
            )
            string(REPLACE . _ NAME ${NAME})
            set(NAME package_${NAME})
            add_custom_target(${NAME} DEPENDS ${BINARY})
            add_dependencies(SDLx_model_test ${NAME})
        endfunction()
        package(${SPV})
        if(WIN32)
            package(${DXIL})
        elseif(APPLE)
            package(${MSL})
        endif()
        package(${JSON})
    endfunction()
    add_shader(vox_obj.frag)
    add_shader(vox_obj.vert)
endif()