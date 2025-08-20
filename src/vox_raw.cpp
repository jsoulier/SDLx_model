#include <SDL3/SDL.h>
#include <SDLx_model/SDL_model.h>

#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <limits>
#include <vector>

#include "internal.hpp"

template<typename T>
T Read(std::ifstream& file)
{
#if SDL_BYTEORDER != SDL_LIL_ENDIAN
#error "Big endian currently unsupported"
#endif
    T data;
    file.read(reinterpret_cast<char*>(&data), sizeof(data));
    return data;
}

bool LoadVoxRaw(SDLx_Model* model, SDL_GPUDevice* device,
    SDL_GPUCopyPass* copy_pass, std::filesystem::path& path)
{
    std::ifstream file(path.replace_extension(".vox"), std::ios::binary);
    if (!file)
    {
        SDL_Log("Failed to open vox: %s", path.string().data());
        return false;
    }
    char magic[4];
    file.read(magic, 4);
    if (std::strncmp(magic, "VOX ", 4))
    {
        SDL_Log("Failed to parse vox: %s", path.string().data());
        return false;
    }
    uint32_t version = Read<uint32_t>(file);
    struct Voxel
    {
        uint8_t x;
        uint8_t y;
        uint8_t z;
        uint8_t palette_index;
    };
    std::vector<Voxel> voxels;
    std::vector<uint32_t> palette;
    while (file.good())
    {
        char chunk_id[4];
        file.read(chunk_id, 4);
        uint32_t chunk_size = Read<uint32_t>(file);
        uint32_t child_chunk_size = Read<uint32_t>(file);
        if (!std::strncmp(chunk_id, "XYZI", 4))
        {
            voxels.resize(Read<uint32_t>(file));
            for (uint32_t i = 0; i < voxels.size(); i++)
            {
                voxels[i] = Read<Voxel>(file);
            }
        }
        else if (!std::strncmp(chunk_id, "RGBA", 4))
        {
            palette.resize(256);
            for (int i = 0; i < 254; i++)
            {
                palette[i + 1] = Read<uint32_t>(file);
            }
        }
        else
        {
            file.seekg(chunk_size, std::ios::cur);
        }
    }
    SDL_GPUTransferBuffer* transfer_buffer;
    {
        SDL_GPUTransferBufferCreateInfo info{};
        info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
        info.size = voxels.size() * sizeof(SDLx_ModelVoxRawInstance);
        transfer_buffer = SDL_CreateGPUTransferBuffer(device, &info);
        if (!transfer_buffer)
        {
            SDL_Log("Failed to create transfer buffer: %s", SDL_GetError());
            return false;
        }
    }
    {
        SDL_GPUBufferCreateInfo info{};
        info.usage = SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_READ |
            SDL_GPU_BUFFERUSAGE_VERTEX | SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_WRITE;
        info.size = voxels.size() * sizeof(SDLx_ModelVoxRawInstance);
        model->vox_raw.instance_buffer = SDL_CreateGPUBuffer(device, &info);
        if (!model->vox_raw.instance_buffer)
        {
            SDL_Log("Failed to create buffer: %s", SDL_GetError());
            return false;
        }
    }
    SDLx_ModelVoxRawInstance* instance_data = static_cast<SDLx_ModelVoxRawInstance*>(
        SDL_MapGPUTransferBuffer(device, transfer_buffer, false));
    if (!instance_data)
    {
        SDL_Log("Failed to map transfer buffer: %s", SDL_GetError());
        return false;
    }
    for (uint32_t i = 0; i < voxels.size(); i++)
    {
        model->max.x = std::max(float(voxels[i].x + 1.0f), model->max.x);
        model->max.y = std::max(float(voxels[i].z + 1.0f), model->max.y);
        model->max.z = std::max(float(voxels[i].y + 1.0f), model->max.z);
    }
    float center_x = model->max.x * 0.5f;
    float center_y = model->max.y * 0.5f;
    float center_z = model->max.z * 0.5f;
    for (uint32_t i = 0; i < voxels.size(); i++)
    {
        SDL_assert(voxels[i].palette_index < palette.size());
        SDLx_ModelVoxRawInstance instance;
        instance.position.x = voxels[i].x - center_x;
        instance.position.y = voxels[i].z - center_y;
        instance.position.z = model->max.z - voxels[i].y - center_z - 1.0f;
        instance.color = SDL_Swap32(palette[voxels[i].palette_index]);
        instance_data[i] = instance;
    }
    SDL_GPUTransferBufferLocation location{};
    SDL_GPUBufferRegion region{};
    location.transfer_buffer = transfer_buffer;
    region.buffer = model->vox_raw.instance_buffer;
    region.size = voxels.size() * sizeof(SDLx_ModelVoxRawInstance);
    SDL_UnmapGPUTransferBuffer(device, transfer_buffer);
    SDL_UploadToGPUBuffer(copy_pass, &location, &region, false);
    SDL_ReleaseGPUTransferBuffer(device, transfer_buffer);
    model->vox_raw.vertex_buffer = CreateCubeVertexBuffer(device, copy_pass);
    model->vox_raw.index_buffer = CreateCubeIndexBuffer(device, copy_pass);
    if (!model->vox_raw.vertex_buffer || !model->vox_raw.index_buffer)
    {
        SDL_Log("Failed to create buffer(s)");
        return false;
    }
    model->vox_raw.num_indices = 36;
    model->vox_raw.num_instances = voxels.size();
    model->vox_raw.index_element_size = SDL_GPU_INDEXELEMENTSIZE_16BIT;
    model->min = {-center_x, -center_y, -center_z};
    model->max = { center_x,  center_y,  center_z};
    return true;
}