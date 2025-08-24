#include <SDL3/SDL.h>
#include <SDLx_model/SDL_model.h>

#include <filesystem>
#include <limits>

#include "internal.hpp"

SDLx_Model* SDLx_ModelLoad(SDL_GPUDevice* device, SDL_GPUCopyPass* copy_pass, const char* path, SDLx_ModelType type)
{
    if (!device)
    {
        SDL_InvalidParamError("device");
        return nullptr;
    }
    if (!copy_pass)
    {
        SDL_InvalidParamError("copy_pass");
        return nullptr;
    }
    if (!path)
    {
        SDL_InvalidParamError("path");
        return nullptr;
    }
    std::filesystem::path file = path;
    if (type == SDLX_MODELTYPE_INVALID)
    {
        if (std::filesystem::exists(file.replace_extension(".gltf")) ||
            std::filesystem::exists(file.replace_extension(".glb")))
        {
            type = SDLX_MODELTYPE_GLTF;
        }
        else if (std::filesystem::exists(file.replace_extension(".vox")))
        {
            if (std::filesystem::exists(file.replace_extension(".obj")) &&
                std::filesystem::exists(file.replace_extension(".png")) &&
                std::filesystem::exists(file.replace_extension(".mtl")))
            {
                type = SDLX_MODELTYPE_VOXOBJ;
            }
            else
            {
                type = SDLX_MODELTYPE_VOXRAW;
            }
        }
        else
        {
            SDL_Log("Failed to deduce type: %s", path);
            return nullptr;
        }
    }
    SDLx_Model* model = new SDLx_Model();
    if (!model)
    {
        SDL_SetError("Failed to allocate model: %s", path);
        return nullptr;
    }
    model->min.x = std::numeric_limits<float>::max();
    model->min.y = std::numeric_limits<float>::max();
    model->min.z = std::numeric_limits<float>::max();
    model->max.x = std::numeric_limits<float>::lowest();
    model->max.y = std::numeric_limits<float>::lowest();
    model->max.z = std::numeric_limits<float>::lowest();
    bool success = false;
    switch (type)
    {
    case SDLX_MODELTYPE_GLTF:
        success = LoadGltf(model, device, copy_pass, file);
        break;
    case SDLX_MODELTYPE_VOXOBJ:
        success = LoadVoxObj(model, device, copy_pass, file);
        break;
    case SDLX_MODELTYPE_VOXRAW:
        success = LoadVoxRaw(model, device, copy_pass, file);
        break;
    }
    if (!success)
    {
        SDL_Log("Failed to create model: %s", path);
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
        SDL_InvalidParamError("device");
        return;
    }
    if (!model)
    {
        return;
    }
    switch (model->type)
    {
    case SDLX_MODELTYPE_GLTF:
        for (int i = 0; i < model->gltf.num_meshes; i++)
        {
            SDLx_ModelMesh& mesh = model->gltf.meshes[i];
            for (int j = 0; j < mesh.num_primitives; j++)
            {
                SDLx_ModelPrimitive& primitive = mesh.primitives[j];
                SDL_ReleaseGPUBuffer(device, primitive.position_buffer);
                SDL_ReleaseGPUBuffer(device, primitive.texcoord_buffer);
                SDL_ReleaseGPUBuffer(device, primitive.normal_buffer);
                SDL_ReleaseGPUBuffer(device, primitive.index_buffer);
                SDL_ReleaseGPUTexture(device, primitive.color_texture);
                SDL_ReleaseGPUTexture(device, primitive.normal_texture);
            }
            delete mesh.primitives;
        }
        delete model->gltf.meshes;
        break;
    case SDLX_MODELTYPE_VOXOBJ:
        SDL_ReleaseGPUBuffer(device, model->vox_obj.vertex_buffer);
        SDL_ReleaseGPUBuffer(device, model->vox_obj.index_buffer);
        SDL_ReleaseGPUTexture(device, model->vox_obj.palette_texture);
        break;
    case SDLX_MODELTYPE_VOXRAW:
        SDL_ReleaseGPUBuffer(device, model->vox_raw.vertex_buffer);
        SDL_ReleaseGPUBuffer(device, model->vox_raw.index_buffer);
        SDL_ReleaseGPUBuffer(device, model->vox_raw.instance_buffer);
        break;
    }
    delete model;
}