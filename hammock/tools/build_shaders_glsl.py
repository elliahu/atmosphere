import os
import subprocess
import platform

# Determine the operating system
if platform.system() == 'Windows':
    compiler = os.environ['VULKAN_SDK'] + '\\Bin\\glslc.exe'
elif platform.system() == 'Linux':
    compiler = 'glslc'
else:
    raise EnvironmentError("Unsupported OS")

raw_shaders = r'../src/hammock/shaders/raw'
compiled_shaders = r'../src/hammock/shaders/compiled'

# Ensure the compiled shaders directory exists
os.makedirs(compiled_shaders, exist_ok=True)

# Iterate shaders
for path in os.listdir(raw_shaders):
    shader_in = os.path.abspath(os.path.join(raw_shaders, path))
    shader_out = os.path.abspath(os.path.join(compiled_shaders, path)) + '.spv'

    # Check if current path is a file
    if os.path.isfile(shader_in):
        # Compile
        subprocess.check_call([compiler, "-I"+os.path.abspath(os.path.join(raw_shaders, "include")) , shader_in, '-o', shader_out, '--target-env=vulkan1.3', '--target-spv=spv1.4'])
        print(path + ' compiled successfully')
