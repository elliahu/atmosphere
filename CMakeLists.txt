cmake_minimum_required(VERSION 3.29)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
project(atmosphere)

# Add the hammock library (builds it, examples, and tools too!)
add_subdirectory(hammock)

# Add the executable
add_executable(app
        main.cpp
        # medium
        medium/IScene.h
        medium/ParticipatingMediumScene.cpp
        medium/ParticipatingMediumScene.h
        medium/SignedDistanceField.h

        # renderer
        renderer/Renderer.h
        renderer/Renderer.cpp
        renderer/Types.h
        renderer/Camera.h
        renderer/ui/UserInterface.h
        renderer/ui/UserInterface.cpp
        renderer/IRenderGroup.h
        renderer/geometry/GeometryPass.h
        renderer/geometry/GeometryPass.cpp
        renderer/clouds/CloudsPass.cpp
        renderer/clouds/CloudsPass.h
        renderer/composition/CompositionPass.cpp
        renderer/composition/CompositionPass.cpp
        renderer/composition/CompositionPass.h
        renderer/composition/PostProcessingPass.cpp
        renderer/composition/PostProcessingPass.h
        renderer/atmosphere/AtmospherePass.cpp
        renderer/atmosphere/AtmospherePass.h
        renderer/atmosphere/ILookUpTable.h
        renderer/atmosphere/Transmittance.cpp
        renderer/atmosphere/Transmittance.h
        renderer/atmosphere/MultipleScattering.cpp
        renderer/atmosphere/MultipleScattering.h
        renderer/atmosphere/SkyView.cpp
        renderer/atmosphere/SkyView.h
        renderer/depth/DepthPass.cpp
        renderer/depth/DepthPass.h
        renderer/atmosphere/AerialPerspective.cpp
        renderer/atmosphere/AerialPerspective.h
        renderer/composition/GodRaysPass.cpp
        renderer/composition/GodRaysPass.h
        renderer/Profiler.h
        renderer/BenchmarkResult.h

)

# Link the engine library
target_link_libraries(app PRIVATE hammock)
target_include_directories(app PRIVATE hammock/include)
target_include_directories(app PRIVATE renderer)
target_include_directories(app PRIVATE medium)
#target_compile_definitions(app PRIVATE CLOUD_RENDER_SUBSAMPLE)
#target_compile_definitions(app PRIVATE HIGH_QUALITY_CLOUDS)