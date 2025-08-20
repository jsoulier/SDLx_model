#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDLx_model/SDL_model.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cstdint>

#include "pipeline.hpp"

static constexpr float Fov = 1.0f;
static constexpr float Near = 0.1f;
static constexpr float Far = 1000.0f;
static constexpr float Pan = 0.002f;
static constexpr float Zoom = 20.0f;
static constexpr glm::vec3 Up = glm::vec3{0.0f, 1.0f, 0.0f};
static constexpr glm::vec4 Light = glm::vec4{-0.33f, -1.0f, -0.66f, 0.33f};

static SDL_Window* window;
static SDL_GPUDevice* device;
static SDL_GPUGraphicsPipeline* pipelines[SDLX_MODELTYPE_COUNT];
static SDL_GPUTexture* depth_texture;
static SDL_GPUSampler* nearest_sampler;
static SDLx_Model* model;
static uint32_t old_width;
static uint32_t old_height;
static float pitch;
static float yaw;
static float distance{256.0f};

static bool Init()
{
    SDL_SetLogPriorities(SDL_LOG_PRIORITY_VERBOSE);
    SDL_SetAppMetadata("SDLx_model_test", nullptr, nullptr);
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        SDL_Log("Failed to initialize SDL: %s", SDL_GetError());
        return false;
    }
    window = SDL_CreateWindow("SDLx_model_test", 960, 720, SDL_WINDOW_RESIZABLE);
    if (!window)
    {
        SDL_Log("Failed to create window: %s", SDL_GetError());
        return false;
    }
#if SDL_PLATFORM_APPLE
    device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_MSL, true, nullptr);
#else
    device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, true, nullptr);
#endif
    if (!device)
    {
        SDL_Log("Failed to create device: %s", SDL_GetError());
        return false;
    }
    if (!SDL_ClaimWindowForGPUDevice(device, window))
    {
        SDL_Log("Failed to claim window: %s", SDL_GetError());
        return false;
    }
    {
        SDL_GPUSamplerCreateInfo info{};
        info.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
        info.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
        info.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
        info.min_filter = SDL_GPU_FILTER_NEAREST;
        info.mag_filter = SDL_GPU_FILTER_NEAREST;
        info.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST;
        nearest_sampler = SDL_CreateGPUSampler(device, &info);
        if (!nearest_sampler)
        {
            SDL_Log("Failed to create sampler: %s", SDL_GetError());
            return false;
        }
    }
    pipelines[SDLX_MODELTYPE_VOXOBJ] = CreateVoxObjPipeline(device, window);
    pipelines[SDLX_MODELTYPE_VOXRAW] = CreateVoxRawPipeline(device, window);
    for (int i = SDLX_MODELTYPE_INVALID + 1; i < SDLX_MODELTYPE_COUNT; i++)
    {
        if (!pipelines[i])
        {
            SDL_Log("Failed to create pipeline(s)");
            return false;
        }
    }
    return true;
}

static bool Load(const char* path)
{
    if (model)
    {
        SDLx_ModelDestroy(device, model);
        model = nullptr;
    }
    SDL_GPUCommandBuffer* command_buffer = SDL_AcquireGPUCommandBuffer(device);
    if (!command_buffer)
    {
        SDL_Log("Failed to acquire command buffer: %s", SDL_GetError());
        return false;
    }
    SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass(command_buffer);
    if (!copy_pass)
    {
        SDL_Log("Failed to begin copy pass: %s", SDL_GetError());
        SDL_CancelGPUCommandBuffer(command_buffer);
        return false;
    }
    model = SDLx_ModelLoad(device, copy_pass, path, SDLX_MODELTYPE_INVALID);
    if (!model)
    {
        SDL_Log("Failed to load model");
        SDL_EndGPUCopyPass(copy_pass);
        SDL_CancelGPUCommandBuffer(command_buffer);
        return false;
    }
    SDL_EndGPUCopyPass(copy_pass);
    SDL_SubmitGPUCommandBuffer(command_buffer);
    return true;
}

static void Draw()
{
    SDL_GPUCommandBuffer* command_buffer = SDL_AcquireGPUCommandBuffer(device);
    if (!command_buffer)
    {
        SDL_Log("Failed to acquire command buffer: %s", SDL_GetError());
        return;
    }
    SDL_GPUTexture* color_texture;
    uint32_t width;
    uint32_t height;
    if (!SDL_WaitAndAcquireGPUSwapchainTexture(command_buffer, window, &color_texture, &width, &height))
    {
        SDL_Log("Failed to acquire swapchain texture: %s", SDL_GetError());
        SDL_CancelGPUCommandBuffer(command_buffer);
        return;
    }
    if (!color_texture || !width || !height)
    {
        /* NOTE: not an error */
        SDL_SubmitGPUCommandBuffer(command_buffer);
        return;
    }
    if (width != old_width || height != old_height)
    {
        SDL_ReleaseGPUTexture(device, depth_texture);
        SDL_GPUTextureCreateInfo info{};
        info.type = SDL_GPU_TEXTURETYPE_2D;
        info.usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET;
        info.format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT;
        info.width = width;
        info.height = height;
        info.layer_count_or_depth = 1;
        info.num_levels = 1;
        depth_texture = SDL_CreateGPUTexture(device, &info);
        if (!depth_texture)
        {
            SDL_Log("Failed to create texture: %s", SDL_GetError());
            return;
        }
        old_width = width;
        old_height = height;
    }
    SDL_GPUColorTargetInfo color_info{};
    color_info.texture = color_texture;
    color_info.load_op = SDL_GPU_LOADOP_CLEAR;
    color_info.store_op = SDL_GPU_STOREOP_STORE;
    color_info.cycle = true;
    SDL_GPUDepthStencilTargetInfo depth_info{};
    depth_info.texture = depth_texture;
    depth_info.load_op = SDL_GPU_LOADOP_CLEAR;
    depth_info.stencil_load_op = SDL_GPU_LOADOP_CLEAR;
    depth_info.store_op = SDL_GPU_STOREOP_STORE;
    depth_info.clear_depth = 1.0f;
    depth_info.cycle = true;
    SDL_GPURenderPass* render_pass = SDL_BeginGPURenderPass(command_buffer, &color_info, 1, &depth_info);
    if (!render_pass)
    {
        SDL_Log("Failed to begin render pass: %s", SDL_GetError());
        SDL_SubmitGPUCommandBuffer(command_buffer);
        return;
    }
    if (!model)
    {
        SDL_EndGPURenderPass(render_pass);
        SDL_SubmitGPUCommandBuffer(command_buffer);
        return;
    }
    glm::vec3 vector;
    vector.x = std::cos(pitch) * std::cos(yaw);
    vector.y = std::sin(pitch);
    vector.z = std::cos(pitch) * std::sin(yaw);
    glm::vec3 center;
    center.x = (model->max.x + model->min.x) / 2.0f;
    center.y = (model->max.y + model->min.y) / 2.0f;
    center.z = (model->max.z + model->min.z) / 2.0f;
    glm::vec3 position = center - vector * distance;
    glm::mat4 view = glm::lookAt(position, position + vector, Up);
    glm::mat4 proj = glm::perspective(Fov, float(width) / height, Near, Far);
    glm::mat4 view_proj_matrix = proj * view;
    switch (model->type)
    {
    case SDLX_MODELTYPE_VOXOBJ:
        {
            SDLx_ModelVoxObj& vox_obj = model->vox_obj;
            SDL_GPUBufferBinding vertex_buffer{};
            SDL_GPUBufferBinding index_buffer{};
            SDL_GPUTextureSamplerBinding palette_texture{};
            vertex_buffer.buffer = vox_obj.vertex_buffer;
            index_buffer.buffer = vox_obj.index_buffer;
            palette_texture.sampler = nearest_sampler;
            palette_texture.texture = vox_obj.palette_texture;
            SDL_BindGPUGraphicsPipeline(render_pass, pipelines[SDLX_MODELTYPE_VOXOBJ]);
            SDL_PushGPUVertexUniformData(command_buffer, 0, &view_proj_matrix, sizeof(view_proj_matrix));
            SDL_PushGPUFragmentUniformData(command_buffer, 0, &Light, sizeof(Light));
            SDL_BindGPUVertexBuffers(render_pass, 0, &vertex_buffer, 1);
            SDL_BindGPUIndexBuffer(render_pass, &index_buffer, vox_obj.index_element_size);
            SDL_BindGPUFragmentSamplers(render_pass, 0, &palette_texture, 1);
            SDL_DrawGPUIndexedPrimitives(render_pass, vox_obj.num_indices, 1, 0, 0, 0);
        }
        break;
    case SDLX_MODELTYPE_VOXRAW:
        {
            SDLx_ModelVoxRaw& vox_raw = model->vox_raw;
            SDL_GPUBufferBinding vertex_buffers[2]{};
            SDL_GPUBufferBinding index_buffer{};
            vertex_buffers[0].buffer = vox_raw.vertex_buffer;
            vertex_buffers[1].buffer = vox_raw.instance_buffer;
            index_buffer.buffer = vox_raw.index_buffer;
            SDL_BindGPUGraphicsPipeline(render_pass, pipelines[SDLX_MODELTYPE_VOXRAW]);
            SDL_PushGPUVertexUniformData(command_buffer, 0, &view_proj_matrix, sizeof(view_proj_matrix));
            SDL_BindGPUVertexBuffers(render_pass, 0, vertex_buffers, 2);
            SDL_BindGPUIndexBuffer(render_pass, &index_buffer, vox_raw.index_element_size);
            SDL_DrawGPUIndexedPrimitives(render_pass, vox_raw.num_indices, vox_raw.num_instances, 0, 0, 0);
        }
        break;
    }
    SDL_EndGPURenderPass(render_pass);
    SDL_SubmitGPUCommandBuffer(command_buffer);
}

int main(int argc, char** argv)
{
    if (!Init())
    {
        return 1;
    }
    if (argc > 1 && !Load(argv[1]))
    {
        return 1;
    }
    bool running = true;
    while (running)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
            case SDL_EVENT_QUIT:
                running = false;
                break;
            case SDL_EVENT_MOUSE_MOTION:
                if (event.motion.state & SDL_BUTTON_LMASK)
                {
                    static constexpr float Pitch = glm::pi<float>() / 2.0f - 0.01f;
                    yaw += event.motion.xrel * Pan;
                    pitch = std::clamp(pitch - event.motion.yrel * Pan, -Pitch, Pitch);
                }
                break;
            case SDL_EVENT_MOUSE_WHEEL:
                distance = std::max(1.0f, distance - event.wheel.y * Zoom);
                break;
            case SDL_EVENT_DROP_FILE:
                Load(event.drop.data);
                break;
            }
        }
        Draw();
    }
    SDL_HideWindow(window);
    if (model)
    {
        SDLx_ModelDestroy(device, model);
    }
    for (int i = SDLX_MODELTYPE_INVALID + 1; i < SDLX_MODELTYPE_COUNT; i++)
    {
        SDL_ReleaseGPUGraphicsPipeline(device, pipelines[i]);
    }
    SDL_ReleaseGPUSampler(device, nearest_sampler);
    SDL_ReleaseGPUTexture(device, depth_texture);
    SDL_ReleaseWindowFromGPUDevice(device, window);
    SDL_DestroyGPUDevice(device);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
