# Minimum features
- GPU driven culling
- Raytraced shadows
- CSM
- Simple post processing
- Skybox rendering
- IBL
- C++ scripting (xmake compilation)
- Physics (jolt)
- Audio (miniaudio)
- Editor/serialization/scenes blablabla (ImGui)
- Actor model for entities
- GLTF model loading and image loading with STB
- Simple asset export pipeline (BC7, compressed mesh format)
- Simple in-game UI

## Would be good to have

- Animations
- LOD selection
- Mesh shaders
- SSR
- RTAO
- AngelScript support

## TODO

### Thursday
- Scripting system (how tho)
- Asset system (ref counting/caching)

### Friday
- Rendering 3D models (static GLTF) + mesh components
- 3D audio + audio components
- Simple HDR forward pass + tonemap via PassIO + debug renderer
- Physics system (implement different types of colliders)
- Project system (simple)

### Saturday
- Microfacet BRDF
- Skybox
- IBL
- CSM
- TAA
- Vulkan TLAS/BLAS/AS Build
- RT shadows
- Runtime renders to backbuffer

## Sunday
- GPU driven frustum cull
- Shadow atlas
- DOF
- Bloom
- Batched 2D sprite renderer
- Text renderer
