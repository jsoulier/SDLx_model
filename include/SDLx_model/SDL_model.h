#pragma once

#include <SDL3/SDL.h>

typedef enum SDLx_ModelType
{
    SDLX_MODELTYPE_INVALID,
    SDLX_MODELTYPE_VOXOBJ, /* MagicaVoxel Obj */ 
    SDLX_MODELTYPE_VOXRAW, /* MagicaVoxel Vox */ 
    SDLX_MODELTYPE_COUNT,
} SDLx_ModelType;

typedef struct SDLx_ModelVec3
{
    float x;
    float y;
    float z;
} SDLx_ModelVec3;

/*
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
    SDL_GPUBuffer* vertex_buffer;    /* SDLx_ModelVoxObjVertex */
    SDL_GPUBuffer* index_buffer;     /* Uint16 or Uint32 */
    SDL_GPUTexture* palette_texture;
    Uint16 num_indices;
    SDL_GPUIndexElementSize index_element_size;
} SDLx_ModelVoxObj;

typedef struct SDLx_ModelVoxRawInstance
{
    SDLx_ModelVec3 position;

    /*
     * 00-07: a (8 bits)
     * 08-15: b (8 bits)
     * 16-23: g (8 bits)
     * 24-31: r (8 bits)
     */
    Uint32 color;
} SDLx_ModelVoxRawInstance;

typedef struct SDLx_ModelVoxRaw
{
    SDL_GPUBuffer* vertex_buffer;   /* SDLx_ModelVec3 */
    SDL_GPUBuffer* index_buffer;    /* Uint16 or Uint32 */
    SDL_GPUBuffer* instance_buffer; /* SDLx_ModelVoxRawInstance */
    Uint16 num_indices;
    Uint32 num_instances;
    SDL_GPUIndexElementSize index_element_size;
} SDLx_ModelVoxRaw;

typedef struct SDLx_Model
{
    SDLx_ModelType type;
    union
    {
        SDLx_ModelVoxObj vox_obj;
        SDLx_ModelVoxRaw vox_raw;
    };
    SDLx_ModelVec3 min;
    SDLx_ModelVec3 max;
} SDLx_Model;

SDLx_Model* SDLx_ModelLoad(SDL_GPUDevice* device,
    SDL_GPUCopyPass* copy_pass, const char* path, SDLx_ModelType type);
void SDLx_ModelDestroy(SDL_GPUDevice* device, SDLx_Model* model);