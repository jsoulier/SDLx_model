#include <SDL3/SDL.h>
#include <SDLx_model/SDL_model.h>

#include <cmath>
#include <cstdint>
#include <filesystem>
#include <limits>
#include <unordered_map>

#include "internal.hpp"
#include "tiny_obj_loader.h"

static SDLx_ModelVoxObjVertex Parse(SDLx_Model* model,
    const tinyobj::attrib_t& attrib, const tinyobj::index_t& index)
{
    static constexpr int PositionScale = 10;
    static constexpr int TexcoordScale = 255;
    int position_x = attrib.vertices[index.vertex_index * 3 + 0] * PositionScale;
    int position_y = attrib.vertices[index.vertex_index * 3 + 1] * PositionScale;
    int position_z = attrib.vertices[index.vertex_index * 3 + 2] * PositionScale;
    int normal_x = attrib.normals[index.normal_index * 3 + 0];
    int normal_y = attrib.normals[index.normal_index * 3 + 1];
    int normal_z = attrib.normals[index.normal_index * 3 + 2];
    uint64_t texcoord = attrib.texcoords[index.texcoord_index * 2 + 0] * TexcoordScale;
    uint64_t magnitude_x = std::abs(position_x);
    uint64_t direction_x = position_x < 0 ? 1 : 0;
    uint64_t magnitude_y = std::abs(position_y);
    uint64_t direction_y = position_y < 0 ? 1 : 0;
    uint64_t magnitude_z = std::abs(position_z);
    uint64_t direction_z = position_z < 0 ? 1 : 0;
    uint64_t normal;
    if (normal_x < 0)
    {
        normal = 0;
    }
    else if (normal_x > 0)
    {
        normal = 1;
    }
    else if (normal_y < 0)
    {
        normal = 2;
    }
    else if (normal_y > 0)
    {
        normal = 3;
    }
    else if (normal_z < 0)
    {
        normal = 4;
    }
    else if (normal_z > 0)
    {
        normal = 5;
    }
    else
    {
        SDL_assert(false);
    }
    SDL_assert(magnitude_x < 256);
    SDL_assert(magnitude_y < 256);
    SDL_assert(magnitude_z < 256);
    SDL_assert(texcoord < 256);
    model->min.x = std::min(float(position_x), model->min.x);
    model->min.y = std::min(float(position_y), model->min.y);
    model->min.z = std::min(float(position_z), model->min.z);
    model->max.x = std::max(float(position_x), model->max.x);
    model->max.y = std::max(float(position_y), model->max.y);
    model->max.z = std::max(float(position_z), model->max.z);
    SDLx_ModelVoxObjVertex vertex{};
    vertex |= (magnitude_x & 0xFF) << 0;
    vertex |= (direction_x & 0x01) << 8;
    vertex |= (magnitude_y & 0xFF) << 9;
    vertex |= (direction_y & 0x01) << 17;
    vertex |= (magnitude_z & 0xFF) << 18;
    vertex |= (direction_z & 0x01) << 26;
    vertex |= (normal & 0x07) << 32;
    vertex |= (texcoord & 0xFF) << 35;
    return vertex;
}

bool LoadVoxObj(SDLx_Model* model, SDL_GPUDevice* device,
    SDL_GPUCopyPass* copy_pass, std::filesystem::path& path)
{
    tinyobj::ObjReader reader;
    if (!reader.ParseFromFile(path.replace_extension(".obj").string()))
    {
        SDL_Log("Failed to parse obj: %s", path.string().data());
        return false;
    }
    const tinyobj::attrib_t& attrib = reader.GetAttrib();
    const tinyobj::shape_t& shape = reader.GetShapes()[0];
    uint32_t max_num_indices = shape.mesh.num_face_vertices.size() * 3;
    SDL_assert(max_num_indices <= std::numeric_limits<uint16_t>::max());
    SDL_GPUTransferBuffer* vertex_transfer_buffer;
    SDL_GPUTransferBuffer* index_transfer_buffer;
    {
        SDL_GPUTransferBufferCreateInfo info{};
        info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
        info.size = max_num_indices * sizeof(SDLx_ModelVoxObjVertex);
        vertex_transfer_buffer = SDL_CreateGPUTransferBuffer(device, &info);
        info.size = max_num_indices * sizeof(uint16_t);
        index_transfer_buffer = SDL_CreateGPUTransferBuffer(device, &info);
        if (!vertex_transfer_buffer || !index_transfer_buffer)
        {
            SDL_Log("Failed to create transfer buffer(s): %s, %s", path.string().data(), SDL_GetError());
            return false;
        }
    }
    SDLx_ModelVoxObjVertex* vertex_data = static_cast<SDLx_ModelVoxObjVertex*>(
        SDL_MapGPUTransferBuffer(device, vertex_transfer_buffer, false));
    uint16_t* index_data = static_cast<uint16_t*>(
        SDL_MapGPUTransferBuffer(device, index_transfer_buffer, false));
    if (!vertex_data || !index_data)
    {
        SDL_Log("Failed to map transfer buffer(s): %s, %s", path.string().data(), SDL_GetError());
        return false;
    }
    uint32_t num_vertices = 0;
    model->vox_obj.num_indices = 0;
    std::unordered_map<SDLx_ModelVoxObjVertex, uint16_t> vertex_to_index;
    for (uint16_t i = 0; i < max_num_indices; i++)
    {
        tinyobj::index_t index = shape.mesh.indices[i];
        SDLx_ModelVoxObjVertex vertex = Parse(model, attrib, index);
        auto [it, inserted] = vertex_to_index.try_emplace(vertex, num_vertices);
        if (inserted)
        {
            vertex_data[num_vertices] = vertex;
            index_data[model->vox_obj.num_indices++] = num_vertices++;
        }
        else
        {
            index_data[model->vox_obj.num_indices++] = it->second;
        }
    }
    SDL_UnmapGPUTransferBuffer(device, vertex_transfer_buffer);
    SDL_UnmapGPUTransferBuffer(device, index_transfer_buffer);
    {
        SDL_GPUBufferCreateInfo info{};
        info.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
        info.size = num_vertices * sizeof(SDLx_ModelVoxObjVertex);
        model->vox_obj.vertex_buffer = SDL_CreateGPUBuffer(device, &info);
        info.usage = SDL_GPU_BUFFERUSAGE_INDEX;
        info.size = model->vox_obj.num_indices * sizeof(uint16_t);
        model->vox_obj.index_buffer = SDL_CreateGPUBuffer(device, &info);
        if (!model->vox_obj.vertex_buffer || !model->vox_obj.index_buffer)
        {
            SDL_Log("Failed to create buffer(s): %s, %s", path.string().data(), SDL_GetError());
            return false;
        }
    }
    {
        SDL_GPUTransferBufferLocation location{};
        SDL_GPUBufferRegion region{};
        location.transfer_buffer = vertex_transfer_buffer;
        region.buffer = model->vox_obj.vertex_buffer;
        region.size = num_vertices * sizeof(SDLx_ModelVoxObjVertex);
        SDL_UploadToGPUBuffer(copy_pass, &location, &region, false);
        location.transfer_buffer = index_transfer_buffer;
        region.buffer = model->vox_obj.index_buffer;
        region.size = model->vox_obj.num_indices * sizeof(uint16_t);
        SDL_UploadToGPUBuffer(copy_pass, &location, &region, false);
    }
    SDL_ReleaseGPUTransferBuffer(device, vertex_transfer_buffer);
    SDL_ReleaseGPUTransferBuffer(device, index_transfer_buffer);
    model->vox_obj.palette_texture = LoadTexture(device, copy_pass, path.replace_extension(".png"));
    if (!model->vox_obj.palette_texture)
    {
        SDL_Log("Failed to load texture: %s", path.string().data());
        return false;
    }
    model->vox_obj.index_element_size = SDL_GPU_INDEXELEMENTSIZE_16BIT;
    return true;
}