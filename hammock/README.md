![Hammock Engine Logo](https://raw.githubusercontent.com/elliahu/HammockEngine/master/docs/img/hammock-engine-logo.png)

# Hammock Engine 🔥

Custom c++23 game engine using Vulkan - the next generation graphics API.

## Features 🚀

The following points are features currently implemented:

- [x] Both Windows and Linux supported
- [x] UI system
- [x] Physically-based deferred rendering pipeline
- [x] glTF 2.0 load format
- [x] Cubemaps, Spherical maps and 3D Textures
- [x] Volumetric rendering and physically accurate clouds
- [x] Full IBL support
- [x] Runtime mipmap generation
- [x] Runtime/Offline environment, generation (prefiltered, irradiance, BRDF lut)

## Working on ⏱

- [ ] Indirect drawing
- [ ] SSAO
- [ ] Headless rendering
- [ ] Optional RTX features
- [ ] Multithreaded command-buffer generation

## Tools

During the development I've also created some tools that are using Hammock underneath. These tools can be found in
`tools/` directory.

- `generate_environment_maps` - generates high-def irradiance map and high-res BRDF look-up table from source
  environment
  map.
- `build_shaders_glsl.py` - python script that compiles shaders using vulkan-shipped glslc utility. Windows and Linux compatible.

## Gallery

![img](https://raw.githubusercontent.com/elliahu/HammockEngine/master/docs/img/IBL.png)
![img](https://raw.githubusercontent.com/elliahu/HammockEngine/master/docs/img/sponza.png)
![img](https://raw.githubusercontent.com/elliahu/HammockEngine/master/docs/img/helmet.png)
![img](https://raw.githubusercontent.com/elliahu/HammockEngine/master/docs/img/cloud.png)

About licencing: [https://choosealicense.com/no-permission/](https://choosealicense.com/no-permission/)


