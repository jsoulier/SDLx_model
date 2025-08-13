#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDLx_model/SDL_model.h>
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "pipeline.hpp"
#include "shader.hpp"

static SDL_Window* window;
static SDL_GPUDevice* device;
static SDL_GPUGraphicsPipeline* pipelines[SDLX_MODELTYPE_COUNT];
static SDL_GPUTexture* depth_texture;
static SDL_GPUSampler* nearest_sampler;
static SDLx_Model* model;
static uint32_t old_width;
static uint32_t old_height;

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
    SDL_PropertiesID properties = SDL_CreateProperties();
#if SDL_PLATFORM_APPLE
    SDL_SetBooleanProperty(properties, SDL_PROP_GPU_DEVICE_CREATE_SHADERS_METAL_BOOLEAN, true);
#else
    SDL_SetBooleanProperty(properties, SDL_PROP_GPU_DEVICE_CREATE_SHADERS_SPIRV_BOOLEAN, true);
#endif
    device = SDL_CreateGPUDeviceWithProperties(properties);
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

static bool LoadModel(const char* path)
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
    if (!SDL_AcquireGPUSwapchainTexture(command_buffer, window, &color_texture, &width, &height))
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
    SDL_SubmitGPUCommandBuffer(command_buffer);
}

int main(int argc, char** argv)
{
    if (!Init())
    {
        return 1;
    }
    if (argc > 1 && !LoadModel(argv[1]))
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
            }
        }
        Draw();
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