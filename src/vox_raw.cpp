#include <SDL3/SDL.h>
#include <SDLx_model/SDL_model.h>

#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
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

bool LoadVoxRaw(SDLx_Model* model, SDL_GPUDevice* device, SDL_GPUCopyPass* copy_pass, std::filesystem::path& path)
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
        if (!std::strncmp(chunk_id, "SIZE", 4))
        {
            model->max.x = Read<uint32_t>(file);
            model->max.y = Read<uint32_t>(file);
            model->max.z = Read<uint32_t>(file);
        }
        else if (!std::strncmp(chunk_id, "XYZI", 4))
        {
            uint32_t num_voxels = Read<uint32_t>(file);
            voxels.reserve(num_voxels);
            for (uint32_t i = 0; i < num_voxels; i++)
            {
                voxels.emplace_back(Read<Voxel>(file));
            }
        }
        else if (std::strncmp(chunk_id, "RGBA", 4) == 0)
        {
            palette.reserve(256);
            for (int i = 0; i < 256; i++)
            {
                palette.emplace_back(Read<uint32_t>(file));
            }
        }
        else
        {
            file.seekg(chunk_size, std::ios::cur);
        }
    }
    return true;
}