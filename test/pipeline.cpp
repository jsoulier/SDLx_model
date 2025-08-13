#include <SDL3/SDL.h>
#include <SDLx_model/SDL_model.h>

#include "shader.hpp"

SDL_GPUGraphicsPipeline* CreateVoxObjPipeline(SDL_GPUDevice* device, SDL_Window* window)
{
    SDL_GPUShader* frag_shader = LoadShader(device, "vox_obj.frag");
    SDL_GPUShader* vert_shader = LoadShader(device, "vox_obj.vert");
    if (!frag_shader || !vert_shader)
    {
        SDL_Log("Failed to load shader(s)");
        return nullptr;
    }
    SDL_GPUColorTargetDescription targets[1]{};
    SDL_GPUVertexBufferDescription buffers[1]{};
    SDL_GPUVertexAttribute attribs[1]{};
    targets[0].format = SDL_GetGPUSwapchainTextureFormat(device, window);
    buffers[0].pitch = sizeof(SDLx_ModelVoxObjVertex);
    attribs[0].format = SDL_GPU_VERTEXELEMENTFORMAT_UINT2;
    SDL_GPUGraphicsPipelineCreateInfo info{};
    info.vertex_shader = vert_shader;
    info.fragment_shader = frag_shader;
    info.target_info.color_target_descriptions = targets;
    info.target_info.num_color_targets = 1;
    info.target_info.depth_stencil_format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT;
    info.target_info.has_depth_stencil_target = true;
    info.vertex_input_state.vertex_buffer_descriptions = buffers;
    info.vertex_input_state.num_vertex_buffers = 1;
    info.vertex_input_state.vertex_attributes = attribs;
    info.vertex_input_state.num_vertex_attributes = 1;
    info.depth_stencil_state.enable_depth_test = true;
    info.depth_stencil_state.enable_depth_write = true;
    info.depth_stencil_state.compare_op = SDL_GPU_COMPAREOP_LESS;
    info.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_BACK;
    info.rasterizer_state.front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE;
    SDL_GPUGraphicsPipeline* pipeline = SDL_CreateGPUGraphicsPipeline(device, &info);
    if (!pipeline)
    {
        SDL_Log("Failed to create graphics pipeline: %s", SDL_GetError());
        return nullptr;
    }
    SDL_ReleaseGPUShader(device, frag_shader);
    SDL_ReleaseGPUShader(device, vert_shader);
    return pipeline;
}

SDL_GPUGraphicsPipeline* CreateVoxRawPipeline(SDL_GPUDevice* device, SDL_Window* window)
{
    /* TODO: */
    return CreateVoxObjPipeline(device, window);
}