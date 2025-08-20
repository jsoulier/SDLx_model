#include <SDL3/SDL.h>
#include <SDLx_model/SDL_model.h>

#include <cstring>
#include <filesystem>

#include "internal.hpp"
#include "stb_image.h"

SDL_GPUTexture* LoadTexture(SDL_GPUDevice* device,
    SDL_GPUCopyPass* copy_pass, std::filesystem::path& path)
{
    int width;
    int height;
    int channels;
    void* src_data = stbi_load(path.string().data(), &width, &height, &channels, 4);
    if (!src_data)
    {
        SDL_Log("Failed to load image: %s, %s", path.string().data(), stbi_failure_reason());
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
            SDL_Log("Failed to create texture: %s, %s", path.string().data(), SDL_GetError());
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
            SDL_Log("Failed to create transfer buffer: %s, %s", path.string().data(), SDL_GetError());
            stbi_image_free(src_data);
            SDL_ReleaseGPUTexture(device, texture);
            return nullptr;
        }
    }
    void* dst_data = SDL_MapGPUTransferBuffer(device, transfer_buffer, false);
    if (!dst_data)
    {
        SDL_Log("Failed to map transfer buffer: %s, %s", path.string().data(), SDL_GetError());
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

SDL_GPUBuffer* CreateCubeVertexBuffer(SDL_GPUDevice* device, SDL_GPUCopyPass* copy_pass)
{
    static const SDLx_ModelVec3 Vertices[8] =
    {
       {0.0f, 0.0f, 1.0f},
       {1.0f, 0.0f, 1.0f},
       {1.0f, 1.0f, 1.0f},
       {0.0f, 1.0f, 1.0f},
       {0.0f, 0.0f, 0.0f},
       {1.0f, 0.0f, 0.0f},
       {1.0f, 1.0f, 0.0f},
       {0.0f, 1.0f, 0.0f},
    };
    SDL_GPUTransferBuffer* transfer_buffer;
    SDL_GPUBuffer* buffer;
    {
        SDL_GPUTransferBufferCreateInfo info{};
        info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
        info.size = sizeof(Vertices);
        transfer_buffer = SDL_CreateGPUTransferBuffer(device, &info);
        if (!transfer_buffer)
        {
            SDL_Log("Failed to create transfer buffer: %s", SDL_GetError());
            return nullptr;
        }
    }
    {
        SDL_GPUBufferCreateInfo info{};
        info.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
        info.size = sizeof(Vertices);
        buffer = SDL_CreateGPUBuffer(device, &info);
        if (!buffer)
        {
            SDL_Log("Failed to create buffer: %s", SDL_GetError());
            return nullptr;
        }
    }
    void* vertex_data = SDL_MapGPUTransferBuffer(device, transfer_buffer, false);
    if (!vertex_data)
    {
        SDL_Log("Failed to map transfer buffer: %s", SDL_GetError());
        return nullptr;
    }
    std::memcpy(vertex_data, Vertices, sizeof(Vertices));
    SDL_GPUTransferBufferLocation location{};
    SDL_GPUBufferRegion region{};
    location.transfer_buffer = transfer_buffer;
    region.buffer = buffer;
    region.size = sizeof(Vertices);
    SDL_UnmapGPUTransferBuffer(device, transfer_buffer);
    SDL_UploadToGPUBuffer(copy_pass, &location, &region, false);
    SDL_ReleaseGPUTransferBuffer(device, transfer_buffer);
    return buffer;
}

SDL_GPUBuffer* CreateCubeIndexBuffer(SDL_GPUDevice* device, SDL_GPUCopyPass* copy_pass)
{
    static const uint16_t Indices[36] =
    {
        0, 1, 2,
        0, 2, 3,
        5, 4, 7,
        5, 7, 6,
        4, 0, 3,
        4, 3, 7,
        1, 5, 6,
        1, 6, 2,
        3, 2, 6,
        3, 6, 7,
        4, 5, 1,
        4, 1, 0,
    };
    SDL_GPUTransferBuffer* transfer_buffer;
    SDL_GPUBuffer* buffer;
    {
        SDL_GPUTransferBufferCreateInfo info{};
        info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
        info.size = sizeof(Indices);
        transfer_buffer = SDL_CreateGPUTransferBuffer(device, &info);
        if (!transfer_buffer)
        {
            SDL_Log("Failed to create transfer buffer: %s", SDL_GetError());
            return nullptr;
        }
    }
    {
        SDL_GPUBufferCreateInfo info{};
        info.usage = SDL_GPU_BUFFERUSAGE_INDEX;
        info.size = sizeof(Indices);
        buffer = SDL_CreateGPUBuffer(device, &info);
        if (!buffer)
        {
            SDL_Log("Failed to create buffer: %s", SDL_GetError());
            return nullptr;
        }
    }
    void* index_data = SDL_MapGPUTransferBuffer(device, transfer_buffer, false);
    if (!index_data)
    {
        SDL_Log("Failed to map transfer buffer: %s", SDL_GetError());
        return nullptr;
    }
    std::memcpy(index_data, Indices, sizeof(Indices));
    SDL_GPUTransferBufferLocation location{};
    SDL_GPUBufferRegion region{};
    location.transfer_buffer = transfer_buffer;
    region.buffer = buffer;
    region.size = sizeof(Indices);
    SDL_UnmapGPUTransferBuffer(device, transfer_buffer);
    SDL_UploadToGPUBuffer(copy_pass, &location, &region, false);
    SDL_ReleaseGPUTransferBuffer(device, transfer_buffer);
    return buffer;
}