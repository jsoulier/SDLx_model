#include <SDL3/SDL.h>
#include <SDLx_model/SDL_model.h>

#include <cstdint>
#include <cstring>
#include <filesystem>
#include <type_traits>

#include "cgltf.h"
#include "internal.hpp"

template<typename T>
SDL_GPUBuffer* CreateVertexBuffer(SDL_GPUDevice* device, SDL_GPUCopyPass* copy_pass, const cgltf_accessor* accessor)
{
    SDL_GPUTransferBuffer* transfer_buffer;
    SDL_GPUBuffer* buffer;
    {
        SDL_GPUTransferBufferCreateInfo info{};
        info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
        info.size = accessor->count * sizeof(T);
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
        info.size = accessor->count * sizeof(T);
        buffer = SDL_CreateGPUBuffer(device, &info);
        if (!buffer)
        {
            SDL_Log("Failed to create buffer: %s", SDL_GetError());
            return nullptr;
        }
    }
    float* data = static_cast<float*>(SDL_MapGPUTransferBuffer(device, transfer_buffer, false));
    if (!data)
    {
        SDL_Log("Failed to map transfer buffer: %s", SDL_GetError());
        return nullptr;
    }
    for (uint32_t i = 0; i < accessor->count; i++)
    {
        if constexpr (std::is_same_v<T, SDLx_ModelVec2>)
        {
            cgltf_accessor_read_float(accessor, i, data + i * 2, 2);
        }
        else
        {
            cgltf_accessor_read_float(accessor, i, data + i * 3, 3);
        }
    }
    SDL_GPUTransferBufferLocation location{};
    SDL_GPUBufferRegion region{};
    location.transfer_buffer = transfer_buffer;
    region.buffer = buffer;
    region.size = accessor->count * sizeof(T);
    SDL_UnmapGPUTransferBuffer(device, transfer_buffer);
    SDL_UploadToGPUBuffer(copy_pass, &location, &region, false);
    SDL_ReleaseGPUTransferBuffer(device, transfer_buffer);
    return buffer;
}

bool CreateIndexBuffer(SDLx_ModelPrimitive& primitive, SDL_GPUDevice* device, SDL_GPUCopyPass* copy_pass, const cgltf_accessor* accessor)
{
    int stride;
    primitive.num_indices = accessor->count;
    if (accessor->component_type == cgltf_component_type_r_16 || accessor->component_type == cgltf_component_type_r_16u)
    {
        primitive.index_element_size = SDL_GPU_INDEXELEMENTSIZE_16BIT;
        stride = 2;
    }
    else
    {
        primitive.index_element_size = SDL_GPU_INDEXELEMENTSIZE_32BIT;
        stride = 4;
    }
    SDL_GPUTransferBuffer* transfer_buffer;
    {
        SDL_GPUTransferBufferCreateInfo info{};
        info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
        info.size = accessor->count * stride;
        transfer_buffer = SDL_CreateGPUTransferBuffer(device, &info);
        if (!transfer_buffer)
        {
            SDL_Log("Failed to create transfer buffer: %s", SDL_GetError());
            return false;
        }
    }
    {
        SDL_GPUBufferCreateInfo info{};
        info.usage = SDL_GPU_BUFFERUSAGE_INDEX;
        info.size = accessor->count * stride;
        primitive.index_buffer = SDL_CreateGPUBuffer(device, &info);
        if (!primitive.index_buffer)
        {
            SDL_Log("Failed to create buffer: %s", SDL_GetError());
            return false;
        }
    }
    void* data = SDL_MapGPUTransferBuffer(device, transfer_buffer, false);
    if (!data)
    {
        SDL_Log("Failed to map transfer buffer: %s", SDL_GetError());
        return false;
    }
    if (primitive.index_element_size == SDL_GPU_INDEXELEMENTSIZE_16BIT)
    {
        uint16_t* u16_data = static_cast<uint16_t*>(data);
        for (uint32_t i = 0; i < accessor->count; i++)
        {
            u16_data[i] = cgltf_accessor_read_index(accessor, i);
        }
    }
    else
    {
        uint32_t* u32_data = static_cast<uint32_t*>(data);
        for (uint32_t i = 0; i < accessor->count; i++)
        {
            u32_data[i] = cgltf_accessor_read_index(accessor, i);
        }
    }
    SDL_GPUTransferBufferLocation location{};
    SDL_GPUBufferRegion region{};
    location.transfer_buffer = transfer_buffer;
    region.buffer = primitive.index_buffer;
    region.size = accessor->count * stride;
    SDL_UnmapGPUTransferBuffer(device, transfer_buffer);
    SDL_UploadToGPUBuffer(copy_pass, &location, &region, false);
    SDL_ReleaseGPUTransferBuffer(device, transfer_buffer);
    return true;
}

bool LoadGltf(SDLx_Model* model, SDL_GPUDevice* device, SDL_GPUCopyPass* copy_pass, std::filesystem::path& path)
{
    cgltf_options options{};
    cgltf_data* data = nullptr;
    if (cgltf_parse_file(&options, path.replace_extension(".gltf").string().data(), &data) &&
        cgltf_parse_file(&options, path.replace_extension(".glb").string().data(), &data))
    {
        SDL_Log("Failed to parse gltf: %s", path.string().data());
        return false;
    }
    if (cgltf_load_buffers(&options, data, path.string().data()))
    {
        SDL_Log("Failed to load buffers: %s", path.string().data());
        return false;
    }
    model->gltf.num_meshes = data->meshes_count;
    model->gltf.meshes = new SDLx_ModelMesh[model->gltf.num_meshes];
    if (!model->gltf.meshes)
    {
        SDL_Log("Failed to create meshes");
        return false;
    }
    for (int i = 0; i < model->gltf.num_meshes; i++)
    {
        const cgltf_mesh& src_mesh = data->meshes[i];
        SDLx_ModelMesh& mesh = model->gltf.meshes[i];
        mesh.num_primitives = src_mesh.primitives_count;
        mesh.primitives = new SDLx_ModelPrimitive[mesh.num_primitives];
        std::memset(mesh.primitives, 0, mesh.num_primitives * sizeof(SDLx_ModelPrimitive));
        if (!mesh.primitives)
        {
            SDL_Log("Failed to create primitives");
            return false;
        }
        for (int j = 0; j < mesh.num_primitives; j++)
        {
            const cgltf_primitive& src_primitive = src_mesh.primitives[j];
            SDLx_ModelPrimitive& primitive = mesh.primitives[j];
            for (int k = 0; k < src_primitive.attributes_count; k++)
            {
                const cgltf_attribute& attribute = src_primitive.attributes[k];
                const cgltf_accessor* accessor = attribute.data;
                switch (attribute.type)
                {
                case cgltf_attribute_type_position:
                    primitive.position_buffer = CreateVertexBuffer<SDLx_ModelVec3>(device, copy_pass, accessor);
                    if (!primitive.position_buffer)
                    {
                        SDL_Log("Failed to create position buffer");
                        return false;
                    }
                    break;
                case cgltf_attribute_type_texcoord:
                    primitive.texcoord_buffer = CreateVertexBuffer<SDLx_ModelVec2>(device, copy_pass, accessor);
                    if (!primitive.texcoord_buffer)
                    {
                        SDL_Log("Failed to create texcoord buffer");
                        return false;
                    }
                    break;
                case cgltf_attribute_type_normal:
                    primitive.normal_buffer = CreateVertexBuffer<SDLx_ModelVec3>(device, copy_pass, accessor);
                    if (!primitive.normal_buffer)
                    {
                        SDL_Log("Failed to create normal buffer");
                        return false;
                    }
                    break;
                }
            }
            const cgltf_accessor* accessor = src_primitive.indices;
            if (accessor)
            {
                if (!CreateIndexBuffer(primitive, device, copy_pass, accessor))
                {
                    SDL_Log("Failed to create index buffer");
                    return false;
                }
            }
            const cgltf_material* material = src_primitive.material;
            if (material)
            {
                if (material->has_pbr_metallic_roughness)
                {
                    const cgltf_texture_view& view = material->pbr_metallic_roughness.base_color_texture;
                    if (view.texture && view.texture->image && view.texture->image->uri)
                    {
                        const cgltf_image* image = view.texture->image;
                        path.replace_filename(image->uri);
                        primitive.color_texture = LoadTexture(device, copy_pass, path);
                        if (!primitive.color_texture)
                        {
                            SDL_Log("Failed to load color texture");
                            return false;
                        }
                    }
                }
                const cgltf_texture_view& view = material->normal_texture;
                if (view.texture && view.texture->image && view.texture->image->uri)
                {
                    cgltf_image* image = view.texture->image;
                    path.replace_filename(image->uri);
                    primitive.normal_texture = LoadTexture(device, copy_pass, path);
                    if (!primitive.normal_texture)
                    {
                        SDL_Log("Failed to load color texture");
                        return false;
                    }
                }
            }
            if (!primitive.color_texture)
            {
                primitive.color_texture = Create1x1Texture(device, copy_pass, 0xFFFFFFFF);
                if (!primitive.color_texture)
                {
                    SDL_Log("Failed to create color texture");
                    return false;
                }
            }
            if (!primitive.normal_texture)
            {
                primitive.normal_texture = Create1x1Texture(device, copy_pass, 0x01010101);
                if (!primitive.normal_texture)
                {
                    SDL_Log("Failed to create normal texture");
                    return false;
                }
            }
        }
    }
    model->gltf.num_nodes = 0;
    for (int i = 0; i < data->nodes_count; i++)
    {
        const cgltf_node& src_node = data->nodes[i];
        if (src_node.mesh)
        {
            model->gltf.num_nodes++;
        }
    }
    model->gltf.nodes = new SDLx_ModelNode[model->gltf.num_nodes];
    if (!model->gltf.nodes)
    {
        SDL_Log("Failed to create nodes");
        return false;
    }
    model->gltf.num_nodes = 0;
    for (int i = 0; i < data->nodes_count; i++)
    {
        const cgltf_node& src_node = data->nodes[i];
        if (!src_node.mesh)
        {
            continue;
        }
        SDLx_ModelNode& node = model->gltf.nodes[model->gltf.num_nodes++];
        SDLx_ModelMatrix& transform = node.transform;
        node.mesh = &model->gltf.meshes[src_node.mesh - data->meshes];
        cgltf_node_transform_world(&src_node, transform);
        const cgltf_mesh* src_mesh = src_node.mesh;
        for (int j = 0; j < src_mesh->primitives_count; j++)
        {
            const cgltf_primitive* primitive = &src_mesh->primitives[j];
            for (int k = 0; k < primitive->attributes_count; k++)
            {
                const cgltf_attribute* attribute = &primitive->attributes[k];
                if (attribute->type != cgltf_attribute_type_position)
                {
                    continue;
                }
                const cgltf_accessor* accessor = attribute->data;
                if (!accessor->has_min || !accessor->has_max)
                {
                    continue;
                }
                for (int l = 0; l < 8; l++)
                {
                    float world_bounds[4]{};
                    float local_bounds[4] =
                    {
                        (l & 1) ? accessor->max[0] : accessor->min[0],
                        (l & 2) ? accessor->max[1] : accessor->min[1],
                        (l & 4) ? accessor->max[2] : accessor->min[2],
                        1.0f
                    };
                    for (int r = 0; r < 4; r++)
                    {
                        for (int c = 0; c < 4; c++)
                        {
                            world_bounds[r] += transform[r * 4 + c] * local_bounds[c];
                        }
                    }
                    model->min.x = std::min(model->min.x, world_bounds[0]);
                    model->min.y = std::min(model->min.y, world_bounds[1]);
                    model->min.z = std::min(model->min.z, world_bounds[2]);
                    model->max.x = std::max(model->max.x, world_bounds[0]);
                    model->max.y = std::max(model->max.y, world_bounds[1]);
                    model->max.z = std::max(model->max.z, world_bounds[2]);
                }
            }
        }
    }
    cgltf_free(data);
    return true;
}