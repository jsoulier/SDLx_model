#pragma once

#include <SDLx_model/SDL_model.h>

bool LoadVoxObj(SDLx_Model* model, SDL_GPUDevice* device, SDL_GPUCopyPass* copy_pass, const char* path);
bool LoadVoxRaw(SDLx_Model* model, SDL_GPUDevice* device, SDL_GPUCopyPass* copy_pass, const char* path);
SDL_GPUTexture* LoadTexture(SDL_GPUDevice* device, SDL_GPUCopyPass* copy_pass, const char* path);