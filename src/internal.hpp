#pragma once

#include <SDL3/SDL.h>
#include <SDLx_model/SDL_model.h>

#include <filesystem>

bool LoadVoxObj(SDLx_Model* model, SDL_GPUDevice* device,
    SDL_GPUCopyPass* copy_pass, std::filesystem::path& path);
bool LoadVoxRaw(SDLx_Model* model, SDL_GPUDevice* device,
    SDL_GPUCopyPass* copy_pass, std::filesystem::path& path);
SDL_GPUTexture* LoadTexture(SDL_GPUDevice* device,
    SDL_GPUCopyPass* copy_pass, std::filesystem::path& path);
SDL_GPUBuffer* CreateCubeVertexBuffer(SDL_GPUDevice* device, SDL_GPUCopyPass* copy_pass);
SDL_GPUBuffer* CreateCubeIndexBuffer(SDL_GPUDevice* device, SDL_GPUCopyPass* copy_pass);