#include <SDL3/SDL.h>
#include <SDLx_model/SDL_model.h>

#include <filesystem>
#include <limits>
#include <vector>

#include "internal.hpp"

SDLx_Model* SDLx_ModelLoad(SDL_GPUDevice* device, SDL_GPUCopyPass* copy_pass, const char* name, SDLx_ModelType type)
{
    if (!device)
    {
        SDL_InvalidParamError(device);
        return nullptr;
    }
    if (!copy_pass)
    {
        SDL_InvalidParamError(copy_pass);
        return nullptr;
    }
    if (!name)
    {
        SDL_InvalidParamError(name);
        return nullptr;
    }
    std::filesystem::path path = name;
    if (type == SDLX_MODELTYPE_INVALID)
    {
        static std::vector<std::filesystem::path> Extensions[SDLX_MODELTYPE_COUNT] =
        {
            {".vox", ".obj", ".png"},
            {".vox"},
        };
        for (int i = 0; i < SDLX_MODELTYPE_COUNT && type == SDLX_MODELTYPE_INVALID; i++)
        {
            type = SDLx_ModelType(i);
            for (const std::filesystem::path& extension : Extensions[i])
            {
                if (!std::filesystem::exists(path.replace_extension(extension)))
                {
                    type = SDLX_MODELTYPE_INVALID;
                    break;
                }
            }
        }
        if (type == SDLX_MODELTYPE_INVALID)
        {
            SDL_Log("Failed to deduce type: %s", name);
            return nullptr;
        }
    }
    SDLx_Model* model = new SDLx_Model();
    if (!model)
    {
        SDL_SetError("Failed to allocate model: %s", name);
        return nullptr;
    }
    model->min.x = std::numeric_limits<float>::max();
    model->min.y = std::numeric_limits<float>::max();
    model->min.z = std::numeric_limits<float>::max();
    model->max.x = std::numeric_limits<float>::min();
    model->max.y = std::numeric_limits<float>::min();
    model->max.z = std::numeric_limits<float>::min();
    bool success = false;
    switch (type)
    {
    case SDLX_MODELTYPE_VOXOBJ:
        success = LoadVoxObj(model, device, copy_pass, path);
        break;
    case SDLX_MODELTYPE_VOXRAW:
        success = LoadVoxRaw(model, device, copy_pass, path);
        break;
    }
    if (!success)
    {
        SDL_Log("Failed to create model: %s", name);
        delete model;
        return nullptr;
    }
    model->type = type;
    return model;
}

void SDLx_ModelDestroy(SDL_GPUDevice* device, SDLx_Model* model)
{
    if (!device)
    {
        SDL_InvalidParamError(device);
        return;
    }
    if (!model)
    {
        return;
    }
    switch (model->type)
    {
    case SDLX_MODELTYPE_VOXOBJ:
        SDL_ReleaseGPUBuffer(device, model->vox_obj.vertex_buffer);
        SDL_ReleaseGPUBuffer(device, model->vox_obj.index_buffer);
        SDL_ReleaseGPUTexture(device, model->vox_obj.palette_texture);
        break;
    case SDLX_MODELTYPE_VOXRAW:
        break;
    }
    delete model;
}