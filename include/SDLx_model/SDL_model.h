#pragma once

#include <SDL3/SDL.h>

typedef enum SDLx_ModelType
{
    SDLX_MODELTYPE_INVALID,
    SDLX_MODELTYPE_VOXOBJ, /* MagicaVoxel Obj */ 
    SDLX_MODELTYPE_VOXRAW, /* MagicaVoxel Vox */ 
    SDLX_MODELTYPE_COUNT,
} SDLx_ModelType;

/*
 * LSB to MSB
 * 00-07: x magnitude (8 bits)
 * 08-08: x direction (1 bits)
 * 09-16: y magnitude (8 bits)
 * 17-17: y direction (1 bits)
 * 18-25: z magnitude (8 bits)
 * 26-26: z direction (1 bits)
 * 27-31: unused (5 bits)
 * 32-34: normal (3 bits)
 * 35-42: x texcoord (8 bits)
 * 43-63: unused (21 bits)
 */
typedef Uint64 SDLx_ModelVoxObjVertex;

typedef struct SDLx_ModelVoxObj
{
    SDL_GPUBuffer* vertex_buffer;
    SDL_GPUBuffer* index_buffer;
    SDL_GPUTexture* palette_texture;
    Uint16 num_indices;
    SDL_GPUIndexElementSize index_element_size;
} SDLx_ModelVoxObj;

typedef struct SDLx_ModelVoxRaw
{
} SDLx_ModelVoxRaw;

typedef struct SDLx_Model
{
    SDLx_ModelType type;
    union
    {
        SDLx_ModelVoxObj vox_obj;
        SDLx_ModelVoxRaw vox_raw;
    };
    struct
    {
        float x;
        float y;
        float z;
    } min, max;
} SDLx_Model;

SDLx_Model* SDLx_ModelLoad(SDL_GPUDevice* device, SDL_GPUCopyPass* copy_pass, const char* name, SDLx_ModelType type);
void SDLx_ModelDestroy(SDL_GPUDevice* device, SDLx_Model* model);