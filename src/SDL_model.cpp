#include <SDLx_model/SDL_model.h>

#include "internal.hpp"

SDLx_Model* SDLx_ModelLoad(SDL_GPUDevice* device, SDL_GPUCopyPass* copy_pass, const char* path, SDLx_ModelType type)
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
    if (!path)
    {
        SDL_InvalidParamError(path);
        return nullptr;
    }
    if (type == SDLX_MODELTYPE_INVALID)
    {
        /* TODO: deduce automatically */
        SDL_InvalidParamError(type);
        return nullptr;
    }
    SDLx_Model* model = new SDLx_Model();
    if (!model)
    {
        SDL_SetError("Failed to allocate model");
        return nullptr;
    }
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
        SDL_Log("Failed to create model");
        delete model;
        return nullptr;
    }
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
        break;
    case SDLX_MODELTYPE_VOXRAW:
        break;
    }
    delete model;
}