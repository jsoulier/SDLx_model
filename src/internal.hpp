#pragma once

#include <SDL3/SDL.h>
#include <SDLx_model/SDL_model.h>

#include <cstdint>
#include <filesystem>

bool LoadGltf(SDLx_Model* model, SDL_GPUDevice* device, SDL_GPUCopyPass* copy_pass, std::filesystem::path& path);
bool LoadVoxObj(SDLx_Model* model, SDL_GPUDevice* device, SDL_GPUCopyPass* copy_pass, std::filesystem::path& path);
bool LoadVoxRaw(SDLx_Model* model, SDL_GPUDevice* device, SDL_GPUCopyPass* copy_pass, std::filesystem::path& path);
SDL_GPUTexture* LoadTexture(SDL_GPUDevice* device, SDL_GPUCopyPass* copy_pass, const std::filesystem::path& path);
SDL_GPUTexture* Create1x1Texture(SDL_GPUDevice* device, SDL_GPUCopyPass* copy_pass, uint32_t color);
SDL_GPUTexture* Create1x1Texture(SDL_GPUDevice* device, SDL_GPUCopyPass* copy_pass, uint32_t color);
SDL_GPUBuffer* CreateCubeVertexBuffer(SDL_GPUDevice* device, SDL_GPUCopyPass* copy_pass);
SDL_GPUBuffer* CreateCubeIndexBuffer(SDL_GPUDevice* device, SDL_GPUCopyPass* copy_pass);