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

### Friday
- 3D audio + audio components
- UI utils to drag/drop entities and assets into the inspector
- Rendering 3D models (static GLTF) + mesh component
- Material asset
- Simple HDR forward pass + tonemap via PassIO + debug renderer
- Physics system (implement different types of colliders)

### Saturday
- Save/Load dialog for scenes
- Asset compression and packing on export
- Project system
- Runtime renders to backbuffer
- Prefabs

## Sunday
- Bindless scene buffer setup
- Microfacet BRDF
- Skybox
- IBL
- CSM
- TAA
- Bloom
- Shadow atlas

## After
- Vulkan and Metal TLAS/BLAS Build
- RT shadows
- GPU driven frustum cull
- DOF
- Batched 2D sprite renderer
- Text renderer
