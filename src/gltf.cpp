#include <SDL3/SDL.h>
#include <SDLx_model/SDL_model.h>

#include <filesystem>

#include "cgltf.h"
#include "internal.hpp"

bool LoadGltf(SDLx_Model* model, SDL_GPUDevice* device, SDL_GPUCopyPass* copy_pass, std::filesystem::path& path)
{
    cgltf_options options{};
    cgltf_data* data = nullptr;
    if (cgltf_parse_file(&options, path.replace_extension(".gltf").string().data(), &data) != cgltf_result_success)
    {
        SDL_Log("Failed to parse gltf: %s", path.string().data());
        return false;
    }
    cgltf_free(data);
    return true;
}