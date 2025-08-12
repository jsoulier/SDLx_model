#include <SDLx_model/SDL_model.h>

#include <cstring>

#include "internal.hpp"
#include "stb_image.h"

SDL_GPUTexture* LoadTexture(SDL_GPUDevice* device, SDL_GPUCopyPass* copy_pass, const char* path)
{
    int width;
    int height;
    int channels;
    void* src_data = stbi_load(path, &width, &height, &channels, 4);
    if (!src_data)
    {
        SDL_Log("Failed to load image: %s, %s", path, stbi_failure_reason());
        return nullptr;
    }
    SDL_GPUTexture* texture;
    SDL_GPUTransferBuffer* transfer_buffer;
    {
        SDL_GPUTextureCreateInfo info{};
        info.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;
        info.type = SDL_GPU_TEXTURETYPE_2D;
        info.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
        info.width = width;
        info.height = height;
        info.layer_count_or_depth = 1;
        info.num_levels = 1;
        texture = SDL_CreateGPUTexture(device, &info);
        if (!texture)
        {
            SDL_Log("Failed to create texture: %s, %s", path, SDL_GetError());
            stbi_image_free(src_data);
            return nullptr;
        }
    }
    {
        SDL_GPUTransferBufferCreateInfo info{};
        info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
        info.size = width * height * 4;
        transfer_buffer = SDL_CreateGPUTransferBuffer(device, &info);
        if (!transfer_buffer)
        {
            SDL_Log("Failed to create transfer buffer: %s, %s", path, SDL_GetError());
            stbi_image_free(src_data);
            SDL_ReleaseGPUTexture(device, texture);
            return nullptr;
        }
    }
    void* dst_data = SDL_MapGPUTransferBuffer(device, transfer_buffer, false);
    if (!dst_data)
    {
        SDL_Log("Failed to map transfer buffer: %s, %s", path, SDL_GetError());
        stbi_image_free(src_data);
        SDL_ReleaseGPUTexture(device, texture);
        return nullptr;
    }
    std::memcpy(dst_data, src_data, width * height * 4);
    stbi_image_free(src_data);
    SDL_UnmapGPUTransferBuffer(device, transfer_buffer);
    SDL_GPUTextureTransferInfo info{};
    SDL_GPUTextureRegion region{};
    info.transfer_buffer = transfer_buffer;
    region.texture = texture;
    region.w = width;
    region.h = height;
    region.d = 1;
    SDL_UploadToGPUTexture(copy_pass, &info, &region, true);
    SDL_ReleaseGPUTransferBuffer(device, transfer_buffer);
    return texture;
}