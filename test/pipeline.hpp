#pragma once

#include <SDL3/SDL.h>

SDL_GPUGraphicsPipeline* CreateGltfPipeline(SDL_GPUDevice* device, SDL_Window* window);
SDL_GPUGraphicsPipeline* CreateVoxObjPipeline(SDL_GPUDevice* device, SDL_Window* window);
SDL_GPUGraphicsPipeline* CreateVoxRawPipeline(SDL_GPUDevice* device, SDL_Window* window);