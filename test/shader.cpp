#include <SDL3/SDL.h>

#include <cstdint>
#include <cstring>
#include <exception>
#include <format>
#include <fstream>
#include <iterator>
#include <string>

#include "shader.hpp"
#include "json.hpp"

static void* Load(SDL_GPUDevice* device, const char* name)
{
    SDL_GPUShaderFormat shader_format = SDL_GetGPUShaderFormats(device);
    const char* entrypoint;
    const char* file_extension;
    if (shader_format & SDL_GPU_SHADERFORMAT_SPIRV)
    {
        shader_format = SDL_GPU_SHADERFORMAT_SPIRV;
        entrypoint = "main";
        file_extension = "spv";
    }
    else if (shader_format & SDL_GPU_SHADERFORMAT_DXIL)
    {
        shader_format = SDL_GPU_SHADERFORMAT_DXIL;
        entrypoint = "main";
        file_extension = "dxil";
    }
    else if (shader_format & SDL_GPU_SHADERFORMAT_MSL)
    {
        shader_format = SDL_GPU_SHADERFORMAT_MSL;
        entrypoint = "main0";
        file_extension = "msl";
    }
    else
    {
        SDL_assert(false);
    }
    std::string shader_path = std::format("{}.{}", name, file_extension);
    std::ifstream shader_file(shader_path, std::ios::binary);
    if (shader_file.fail())
    {
        SDL_Log("Failed to open shader: %s", shader_path.data());
        return nullptr;
    }
    std::string json_path = std::format("{}.json", name);
    std::ifstream json_file(json_path, std::ios::binary);
    if (json_file.fail())
    {
        SDL_Log("Failed to open json: %s", json_path.data());
        return nullptr;
    }
    std::string shader_data(std::istreambuf_iterator<char>(shader_file), {});
    nlohmann::json json;
    try
    {
        json_file >> json;
    }
    catch (const std::exception& exception)
    {
        SDL_Log("Failed to parse json: %s, %s", json_path.data(), exception.what());
        return nullptr;
    }
    void* shader = nullptr;
    if (std::strstr(name, ".comp"))
    {
        SDL_GPUComputePipelineCreateInfo info{};
        info.num_samplers = json["samplers"];
        info.num_readonly_storage_textures = json["readonly_storage_textures"];
        info.num_readonly_storage_buffers = json["readonly_storage_buffers"];
        info.num_readwrite_storage_textures = json["readwrite_storage_textures"];
        info.num_readwrite_storage_buffers = json["readwrite_storage_buffers"];
        info.num_uniform_buffers = json["uniform_buffers"];
        info.threadcount_x = json["threadcount_x"];
        info.threadcount_y = json["threadcount_y"];
        info.threadcount_z = json["threadcount_z"];
        info.code = reinterpret_cast<Uint8*>(shader_data.data());
        info.code_size = shader_data.size();
        info.entrypoint = entrypoint;
        info.format = shader_format;
        shader = SDL_CreateGPUComputePipeline(device, &info);
    }
    else
    {
        SDL_GPUShaderCreateInfo info{};
        info.num_samplers = json["samplers"];
        info.num_storage_textures = json["storage_textures"];
        info.num_storage_buffers = json["storage_buffers"];
        info.num_uniform_buffers = json["uniform_buffers"];
        info.code = reinterpret_cast<Uint8*>(shader_data.data());
        info.code_size = shader_data.size();
        info.entrypoint = entrypoint;
        info.format = shader_format;
        if (std::strstr(name, ".frag"))
        {
            info.stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
        }
        else
        {
            info.stage = SDL_GPU_SHADERSTAGE_VERTEX;
        }
        shader = SDL_CreateGPUShader(device, &info);
    }
    if (!shader)
    {
        SDL_Log("Failed to create shader: %s, %s", name, SDL_GetError());
        return nullptr;
    }
    return shader;
}

SDL_GPUShader* LoadShader(SDL_GPUDevice* device, const char* name)
{
    return static_cast<SDL_GPUShader*>(Load(device, name));
}

SDL_GPUComputePipeline* LoadComputePipeline(SDL_GPUDevice* device, const char* name)
{
    return static_cast<SDL_GPUComputePipeline*>(Load(device, name));
}