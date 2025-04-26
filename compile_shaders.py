# This script checks if the system has correct Vulkan version that includes slangc compiler
# Supports both Windows and Linux, MacOS is not supported

import os
import subprocess
import platform

print("Running doctor ...")

# Determine the operating system
if platform.system() == 'Windows':
    compiler = os.environ['VULKAN_SDK'] + '\\Bin\\slangc.exe'
    print("Target system Windows detected")
elif platform.system() == 'Linux':
    compiler = 'slangc'
    print("Target system Linux detected")
else:
    raise EnvironmentError("Unsupported OS. Supported operating systems are: Linux, Windows")

# Check if vulkan env var is set
if not os.path.exists(os.environ['VULKAN_SDK']):
    print("VULKAN_SDK environment variable is not set. Please install Vulkan SDK, relaunch the shell for new changes to take effect and try again.")
    exit(1)
else:
    print("Found VULKAN_SDK env var")

# Check if the slang compiler exits
if platform.system() == 'Windows' and not os.path.exists(compiler):
    print("Failed to compile the shaders. Slang compiler missing! Make sure Vulkan SDK 1.3.296.0 or newer is installed or install it separately.")
    exit(1)
else:
    print("Found slangc compiler")

print("OK")
print("Creating output dir...")
os.makedirs('spv', exist_ok=True)
print("OK")

# Compile the shaders 
print("Compiling shaders, this may take few seconds...")
# Participating medium scene
subprocess.check_call([compiler , "shaders/medium.slang", '-o', 'spv/medium.vert.spv', '-target', 'spirv', '-entry', 'vertexMain'], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
subprocess.check_call([compiler , "shaders/medium.slang", '-o', 'spv/medium.frag.spv', '-target', 'spirv', '-entry', 'pixelMain'], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)

# Main renderer
subprocess.check_call([compiler , "shaders/clouds.slang", '-o', 'spv/clouds.comp.spv', '-target', 'spirv', '-entry', 'computeMain'], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
subprocess.check_call([compiler , "shaders/clouds.slang", '-o', 'spv/clouds_repr.comp.spv', '-target', 'spirv', '-entry', 'computeMain', '-DCLOUD_RENDER_SUBSAMPLE'], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
subprocess.check_call([compiler , "shaders/transmittance.slang", '-o', 'spv/transmittance.comp.spv', '-target', 'spirv', '-entry', 'computeMain'], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
subprocess.check_call([compiler , "shaders/multiplescattering.slang", '-o', 'spv/multiplescattering.comp.spv', '-target', 'spirv', '-entry', 'computeMain'], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
subprocess.check_call([compiler , "shaders/skyview.slang", '-o', 'spv/skyview.comp.spv', '-target', 'spirv', '-entry', 'computeMain'], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
subprocess.check_call([compiler , "shaders/aerialperspective.slang", '-o', 'spv/aerialperspective.comp.spv', '-target', 'spirv', '-entry', 'computeMain'], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
subprocess.check_call([compiler , "shaders/depth.slang", '-o', 'spv/depth.vert.spv', '-target', 'spirv', '-entry', 'vertexMain'], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
subprocess.check_call([compiler , "shaders/depth.slang", '-o', 'spv/depth.frag.spv', '-target', 'spirv', '-entry', 'pixelDepth'], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
subprocess.check_call([compiler , "shaders/depth.slang", '-o', 'spv/linear_depth.frag.spv', '-target', 'spirv', '-entry', 'pixelLinearDepth'], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
subprocess.check_call([compiler , "shaders/terrain.slang", '-o', 'spv/terrain.vert.spv', '-target', 'spirv', '-entry', 'vertexMain'], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
subprocess.check_call([compiler , "shaders/terrain.slang", '-o', 'spv/terrain.frag.spv', '-target', 'spirv', '-entry', 'pixelMain'], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
subprocess.check_call([compiler , "shaders/sky.slang", '-o', 'spv/sky.vert.spv', '-target', 'spirv', '-entry', 'vertexMain'], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
subprocess.check_call([compiler , "shaders/sky.slang", '-o', 'spv/sky.frag.spv', '-target', 'spirv', '-entry', 'pixelMain'], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
subprocess.check_call([compiler , "shaders/occlusion.slang", '-o', 'spv/godrays.vert.spv', '-target', 'spirv', '-entry', 'vertexMain'], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
subprocess.check_call([compiler , "shaders/occlusion.slang", '-o', 'spv/mask.frag.spv', '-target', 'spirv', '-entry', 'pixelMask'], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
subprocess.check_call([compiler , "shaders/occlusion.slang", '-o', 'spv/blur.frag.spv', '-target', 'spirv', '-entry', 'pixelBlur'], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
subprocess.check_call([compiler , "shaders/compose.slang", '-o', 'spv/compose.vert.spv', '-target', 'spirv', '-entry', 'vertexMain'], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
subprocess.check_call([compiler , "shaders/compose.slang", '-o', 'spv/compose.frag.spv', '-target', 'spirv', '-entry', 'pixelMain'], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
subprocess.check_call([compiler , "shaders/postprocess.slang", '-o', 'spv/postprocess.vert.spv', '-target', 'spirv', '-entry', 'vertexMain'], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
subprocess.check_call([compiler , "shaders/postprocess.slang", '-o', 'spv/postprocess.frag.spv', '-target', 'spirv', '-entry', 'pixelMain'], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)

print("OK")