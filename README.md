# SDLx_model

> [!NOTE]
> Not affiliated with [SDL](https://github.com/libsdl-org). API is WIP and may change

Lightweight model loader for SDL3 GPU to ease creation of GPU resources

### Building

```shell
git submodule add https://github.com/jsoulier/SDLx_model
```

```cmake
add_subdirectory(SDL)
add_subdirectory(SDLx_model)

target_link_libraries(<target>
    SDL3::SDL3
    SDLx_model::SDLx_model
)
```

### Usage

```c
#include <SDL3/SDL.h>
#include <SDLx_model/SDL_model.h>

int main()
{
    SDLx_Model* model = SDLx_ModelLoad(<device>, <copy_pass>, <path>, SDLX_MODELTYPE_INVALID);
    if (!model)
    {
        SDL_Log("Failed to load model: %s", SDL_GetError());
        return 1;
    }

    switch (model->type)
    {
    case SDLX_MODELTYPE_GLTF:
        SDLx_ModelGltf& gltf = model->gltf;
        SDL_BindGPUGraphicsPipeline(<render_pass>, <pipeline>);
        SDL_PushGPUVertexUniformData(<command_buffer>, 0, <view_proj_matrix>, 64);
        for (int i = 0; i < gltf.num_nodes; i++)
        {
            SDLx_ModelMesh* mesh = gltf.nodes[i].mesh;
            SDLx_ModelMatrix& transform = gltf.nodes[i].transform;
            SDL_PushGPUVertexUniformData(<command_buffer>, 1, transform, 64);
            for (int k = 0; k < mesh->num_primitives; k++)
            {
                SDLx_ModelPrimitive& primitive = mesh->primitives[k];
                SDL_GPUBufferBinding vertex_buffers[3]{};
                SDL_GPUBufferBinding index_buffer{};
                SDL_GPUTextureSamplerBinding textures[2]{};
                vertex_buffers[0].buffer = primitive.position_buffer;
                vertex_buffers[1].buffer = primitive.texcoord_buffer;
                vertex_buffers[2].buffer = primitive.normal_buffer;
                index_buffer.buffer = primitive.index_buffer;
                textures[0].texture = primitive.color_texture;
                textures[0].sampler = <sampler>;
                textures[1].texture = primitive.normal_texture;
                textures[1].sampler = <sampler>;
                SDL_BindGPUVertexBuffers(<render_pass>, 0, vertex_buffers, 3);
                SDL_BindGPUIndexBuffer(<render_pass>, &index_buffer, primitive.index_element_size);
                SDL_BindGPUFragmentSamplers(<render_pass>, 0, textures, 2);
                SDL_DrawGPUIndexedPrimitives(<render_pass>, primitive.num_indices, 1, 0, 0, 0);
            }
        }
        break;
    }

    SDLx_ModelDestroy(device, model);
}
```

### Examples

You can build the examples [here](test/main.cpp) with the following commands

```shell
git clone https://github.com/jsoulier/SDLx_model --recurse-submodules
cd SDLx_model
mkdir build
cd build
cmake ..
cmake --build . --parallel 8
cd bin
./SDLx_model_test <path>
```