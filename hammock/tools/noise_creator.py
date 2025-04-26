import streamlit as st
import noise
import numpy as np
import matplotlib.pyplot as plt
import os
import time
import threading
from scipy.spatial import Voronoi, distance
from PIL import Image
from itertools import product

# Move control elements to sidebar
st.sidebar.title("🌥️ Cloud Noise Editor")

# Channel selection in sidebar
st.sidebar.subheader("Channel Configuration")
red_noise = st.sidebar.selectbox("Red Channel", ["None", "Perlin", "Worley", "Curl", "Perlin-Worley"], key="red")
green_noise = st.sidebar.selectbox("Green Channel", ["None", "Perlin", "Worley", "Curl", "Perlin-Worley"], key="green")
blue_noise = st.sidebar.selectbox("Blue Channel", ["None", "Perlin", "Worley", "Curl", "Perlin-Worley"], key="blue")
alpha_noise = st.sidebar.selectbox("Alpha Channel", ["None", "Perlin", "Worley", "Curl", "Perlin-Worley"], key="alpha")

# Display settings in sidebar
st.sidebar.subheader("Display Settings")
show_channels = st.sidebar.multiselect("Show Channels", ["Red", "Green", "Blue", "Alpha"], default=["Red"])
dimensionality = st.sidebar.radio("Noise Dimension", ["2D", "3D"])
size = st.sidebar.slider("Resolution", 16, 512, 64, step=16)

if dimensionality == "3D":
    z_slice = st.sidebar.slider("Z-Slice", 0, size - 1, 0)

seed = st.sidebar.slider("Seed", 0, 1000, 42)

# Channel-specific parameters
def show_noise_params(noise_type, channel):
    if noise_type in ["Perlin", "Curl", ]:
        return {
            'scale': st.sidebar.slider(f"Scale ({channel})", 1.0, 100.0, 30.0, step=1.0, key=f"scale_{channel}"),
            'octaves': st.sidebar.slider(f"Octaves ({channel})", 1, 8, 5, key=f"octaves_{channel}"),
            'persistence': st.sidebar.slider(f"Persistence ({channel})", 0.1, 1.0, 0.5, step=0.05, key=f"persistence_{channel}"),
            'lacunarity': st.sidebar.slider(f"Lacunarity ({channel})", 1.0, 4.0, 2.0, step=0.1, key=f"lacunarity_{channel}")
        }
    elif noise_type in ["Perlin-Worley"]:
         return {
            'num_points': st.sidebar.slider(f"Number of Points ({channel})", 1, 1000, 50, key=f"points_{channel}"),
            'distance_func': st.sidebar.selectbox(f"Distance Function ({channel})", ['Euclidean', 'Manhattan', 'Chebyshev'], key=f"dist_{channel}"),
            'combination': st.sidebar.selectbox(f"Combination Method ({channel})", ['Nearest', 'Second Nearest', 'Difference'], key=f"comb_{channel}"),
            'scale': st.sidebar.slider(f"Scale ({channel})", 1.0, 100.0, 30.0, step=1.0, key=f"scale_{channel}"),
            'octaves': st.sidebar.slider(f"Octaves ({channel})", 1, 8, 5, key=f"octaves_{channel}"),
            'persistence': st.sidebar.slider(f"Persistence ({channel})", 0.1, 1.0, 0.5, step=0.05, key=f"persistence_{channel}"),
            'lacunarity': st.sidebar.slider(f"Lacunarity ({channel})", 1.0, 4.0, 2.0, step=0.1, key=f"lacunarity_{channel}")
        }
    elif noise_type == "Worley":
        return {
            'num_points': st.sidebar.slider(f"Number of Points ({channel})", 1, 1000, 50, key=f"points_{channel}"),
            'distance_func': st.sidebar.selectbox(f"Distance Function ({channel})", ['Euclidean', 'Manhattan', 'Chebyshev'], key=f"dist_{channel}"),
            'combination': st.sidebar.selectbox(f"Combination Method ({channel})", ['Nearest', 'Second Nearest', 'Difference'], key=f"comb_{channel}")
        }
    return {}

# Channel parameters in sidebar
st.sidebar.subheader("Channel Parameters")
params = {}
if red_noise != "None":
    st.sidebar.markdown("### 🔴 Red Channel")
    params['red'] = show_noise_params(red_noise, 'red')
if green_noise != "None":
    st.sidebar.markdown("### 🟢 Green Channel")
    params['green'] = show_noise_params(green_noise, 'green')
if blue_noise != "None":
    st.sidebar.markdown("### 🔵 Blue Channel")
    params['blue'] = show_noise_params(blue_noise, 'blue')
if alpha_noise != "None":
    st.sidebar.markdown("### ⚪ Alpha Channel")
    params['alpha'] = show_noise_params(alpha_noise, 'alpha')

# Main content area for preview and export
st.title("Preview & Export")

def generate_perlin_2d(params):
    world = np.zeros((size, size))
    freq = 1.0 / params['scale']
    for y in range(size):
        for x in range(size):
            # Use proper wrapping by using modulo of size
            world[y, x] = noise.pnoise2(
                x * freq, y * freq,
                octaves=params['octaves'],
                persistence=params['persistence'],
                lacunarity=params['lacunarity'],
                repeatx=size,  # Set repeat to size for perfect tiling
                repeaty=size,
                base=seed
            )
    return (world - world.min()) / (world.max() - world.min())

def generate_perlin_3d(params):
    world = np.zeros((size, size, size))
    freq = 1.0 / params['scale']
    for z in range(size):
        for y in range(size):
            for x in range(size):
                world[z, y, x] = noise.pnoise3(
                    x * freq, y * freq, z * freq,
                    octaves=params['octaves'],
                    persistence=params['persistence'],
                    lacunarity=params['lacunarity'],
                    repeatx=size,
                    repeaty=size,
                    repeatz=size,
                    base=seed
                )
    return (world - world.min()) / (world.max() - world.min())

def generate_worley_2d(params):
    np.random.seed(seed)
    num_points = params.get('num_points', 20)
    points = np.random.rand(num_points, 2) * size
    
    # Create coordinate grids
    x_coords, y_coords = np.meshgrid(np.arange(size), np.arange(size))
    coords = np.stack([x_coords, y_coords], axis=-1)
    
    # Initialize distances array for all periodic copies
    min_distances = np.full((size, size), np.inf)
    second_min_distances = np.full((size, size), np.inf)
    
    # Process periodic copies efficiently
    for ox in [-size, 0, size]:
        for oy in [-size, 0, size]:
            # Shift points
            shifted_points = points + np.array([ox, oy])
            
            # Calculate distances using broadcasting
            if params.get('distance_func') == 'Manhattan':
                distances = np.abs(coords[:, :, np.newaxis] - shifted_points).sum(axis=-1)
            elif params.get('distance_func') == 'Chebyshev':
                distances = np.max(np.abs(coords[:, :, np.newaxis] - shifted_points), axis=-1)
            else:  # Euclidean
                distances = np.sqrt(((coords[:, :, np.newaxis] - shifted_points) ** 2).sum(axis=-1))
            
            # Update min and second min distances
            prev_min = min_distances.copy()
            min_distances = np.minimum(min_distances, np.min(distances, axis=-1))
            mask = min_distances < prev_min
            second_min_distances[mask] = prev_min[mask]
            second_min_distances[~mask] = np.minimum(
                second_min_distances[~mask],
                np.partition(distances[~mask], 1, axis=-1)[..., 1]
            )
    
    # Normalize distances
    min_distances /= size
    second_min_distances /= size
    
    # Apply combination method
    combination = params.get('combination', 'Nearest')
    if combination == 'Nearest':
        world = 1 - min_distances
    elif combination == 'Second Nearest':
        world = 1 - second_min_distances
    else:  # Difference
        world = 1 - (second_min_distances - min_distances)
    
    return (world - world.min()) / (world.max() - world.min())

def generate_worley_3d(params):
    np.random.seed(seed)
    num_points = params.get('num_points', 20)
    points = np.random.rand(num_points, 3) * size

    # Create coordinate grids more efficiently
    coords = np.stack(np.meshgrid(
        np.arange(size),
        np.arange(size),
        np.arange(size),
        indexing='ij'
    ), axis=-1)

    # Pre-compute distance function
    if params.get('distance_func') == 'Manhattan':
        dist_func = lambda x: np.abs(x).sum(axis=-1)
    elif params.get('distance_func') == 'Chebyshev':
        dist_func = lambda x: np.max(np.abs(x), axis=-1)
    else:  # Euclidean
        dist_func = lambda x: np.sqrt((x * x).sum(axis=-1))

    # Initialize with nearest neighbor search for periodic boundaries
    min_distances = np.full((size, size, size), np.inf)
    if params.get('combination') in ['Second Nearest', 'Difference']:
        second_min_distances = np.full((size, size, size), np.inf)

    # Process only necessary periodic copies
    shifts = np.array(list(product([-size, 0, size], repeat=3)))
    
    # Batch process all shifts
    for shift in shifts:
        shifted_points = points + shift
        diff = coords[:, :, :, np.newaxis] - shifted_points
        distances = dist_func(diff)
        
        # Update distances more efficiently
        new_mins = np.min(distances, axis=-1)
        if params.get('combination') in ['Second Nearest', 'Difference']:
            mask = new_mins < min_distances
            second_min_distances[mask] = min_distances[mask]
            min_distances = np.minimum(min_distances, new_mins)
            # Update second minimum only where needed
            needs_second = ~mask
            if needs_second.any():
                partitioned = np.partition(distances[needs_second], 1, axis=-1)
                second_min_distances[needs_second] = np.minimum(
                    second_min_distances[needs_second],
                    partitioned[..., 1]
                )
        else:
            min_distances = np.minimum(min_distances, new_mins)

    # Normalize distances
    min_distances /= size
    if params.get('combination') in ['Second Nearest', 'Difference']:
        second_min_distances /= size

    # Apply combination method
    combination = params.get('combination', 'Nearest')
    if combination == 'Nearest':
        world = 1 - min_distances
    elif combination == 'Second Nearest':
        world = 1 - second_min_distances
    else:  # Difference
        world = 1 - (second_min_distances - min_distances)

    return (world - world.min()) / (world.max() - world.min())

def generate_curl_2d(params):
    perlin_x = generate_perlin_2d(params)
    perlin_y = generate_perlin_2d(params)
    curl_noise = np.gradient(perlin_x, axis=1) - np.gradient(perlin_y, axis=0)
    return (curl_noise - curl_noise.min()) / (curl_noise.max() - curl_noise.min())

def generate_curl_3d(params):
    perlin_x = generate_perlin_3d(params)
    perlin_y = generate_perlin_3d(params)
    curl_noise = np.gradient(perlin_x, axis=1) - np.gradient(perlin_y, axis=0)
    return (curl_noise - curl_noise.min()) / (curl_noise.max() - curl_noise.min())

def generate_perlin_worley_2d(params):
    # Generate both noises in parallel
    perlin = generate_perlin_2d(params)
    worley = generate_worley_2d(params)
    # Blend using vectorized operations
    return (perlin + worley) / 2

def generate_perlin_worley_3d(params):
    perlin = generate_perlin_3d(params)
    worley = generate_worley_3d(params)
    return (perlin + worley) / 2

@st.cache_data
def cached_noise_generation(noise_type, is_3d, seed_val, size_val, channel_id=None, **params):
    """Cached wrapper for noise generation with channel-specific caching"""
    if noise_type == "None":
        return np.zeros((size_val, size_val, size_val) if is_3d else (size_val, size_val))
    
    noise_generators = {
        "Perlin": (generate_perlin_3d, generate_perlin_2d),
        "Worley": (generate_worley_3d, generate_worley_2d),
        "Curl": (generate_curl_3d, generate_curl_2d),
        "Perlin-Worley": (generate_perlin_worley_3d, generate_perlin_worley_2d)
    }
    
    if noise_type not in noise_generators:
        return np.zeros((size_val, size_val, size_val) if is_3d else (size_val, size_val))
        
    generator_3d, generator_2d = noise_generators[noise_type]
    return generator_3d(params) if is_3d else generator_2d(params)

def generate_channel_noise(noise_type, channel_params, is_3d=False, channel_id=None):
    params_dict = dict(channel_params) if channel_params else {}
    return cached_noise_generation(
        noise_type=noise_type,
        is_3d=is_3d,
        seed_val=seed,
        size_val=size,
        channel_id=channel_id,
        **params_dict
    )

# Generate noise for each channel
is_3d = dimensionality == "3D"
noise_data = {
    'red': generate_channel_noise(red_noise, params.get('red', {}), is_3d, 'red'),
    'green': generate_channel_noise(green_noise, params.get('green', {}), is_3d, 'green'),
    'blue': generate_channel_noise(blue_noise, params.get('blue', {}), is_3d, 'blue'),
    'alpha': generate_channel_noise(alpha_noise, params.get('alpha', {}), is_3d, 'alpha')
}

# Combine channels for display
def combine_channels(noise_data, show_channels, z_slice=None):
    # For single channel display, return grayscale
    if len(show_channels) == 1:
        channel = show_channels[0]
        key = channel.lower()
        if noise_data[key] is not None:
            if z_slice is not None and len(noise_data[key].shape) == 3:
                return np.stack([noise_data[key][z_slice]] * 3, axis=-1)
            else:
                return np.stack([noise_data[key]] * 3, axis=-1)
        return np.zeros((size, size, 3))
    
    # For multiple channels, combine them
    rgba = np.zeros((size, size, 4))
    channel_mapping = {
        'Red': (0, 'red'), 
        'Green': (1, 'green'), 
        'Blue': (2, 'blue'), 
        'Alpha': (3, 'alpha')
    }
    
    for channel in channel_mapping:
        idx, key = channel_mapping[channel]
        if channel in show_channels and noise_data[key] is not None:
            if z_slice is not None and len(noise_data[key].shape) == 3:
                rgba[:, :, idx] = noise_data[key][z_slice]
            else:
                rgba[:, :, idx] = noise_data[key]
    
    # If alpha is not selected, set it to 1
    if 'Alpha' not in show_channels:
        rgba[:, :, 3] = 1.0
    
    # Convert RGBA to RGB for display
    rgb = np.zeros((size, size, 3))
    rgb = rgba[:, :, :3] * rgba[:, :, 3:4] + (1 - rgba[:, :, 3:4])
    
    return rgb

# Display combined noise
combined_noise = combine_channels(noise_data, show_channels, z_slice if is_3d else None)

fig, ax = plt.subplots()
if len(show_channels) == 1:
    ax.imshow(combined_noise, cmap='gray')
else:
    ax.imshow(combined_noise)
ax.axis("off")
st.pyplot(fig)


# Export status
if "export_status" in st.session_state:
    st.info(st.session_state["export_status"])

def export_noise(file_path):
    start_time = time.time()
    st.session_state["export_status"] = f"🔄 Exporting {dimensionality} noise..."
    
    os.makedirs(os.path.dirname(file_path), exist_ok=True)
    
    if dimensionality == "2D":
        # Convert to RGBA for export
        rgba = np.zeros((size, size, 4))
        for i, (channel, key) in enumerate([('Red', 'red'), ('Green', 'green'), ('Blue', 'blue'), ('Alpha', 'alpha')]):
            if channel in show_channels and noise_data[key] is not None:
                rgba[:, :, i] = noise_data[key]
            elif i < 3:  # RGB channels
                rgba[:, :, i] = 0
            else:  # Alpha channel
                rgba[:, :, i] = 1
        
        image = Image.fromarray((rgba * 255).astype(np.uint8), 'RGBA')
        image.save(file_path)
    else:
        for z in range(size):
            rgba = np.zeros((size, size, 4))
            for i, (channel, key) in enumerate([('Red', 'red'), ('Green', 'green'), ('Blue', 'blue'), ('Alpha', 'alpha')]):
                if channel in show_channels and noise_data[key] is not None:
                    rgba[:, :, i] = noise_data[key][z]
                elif i < 3:  # RGB channels
                    rgba[:, :, i] = 0
                else:  # Alpha channel
                    rgba[:, :, i] = 1
            
            image = Image.fromarray((rgba * 255).astype(np.uint8), 'RGBA')
            image.save(file_path.replace(".png", f"_slice_{z:03d}.png"))
            
            progress = (z + 1) / size * 100
            elapsed = time.time() - start_time
            estimated_total = elapsed / (z + 1) * size
            remaining = estimated_total - elapsed
            st.session_state["export_status"] = f"Exporting slice {z+1}/{size} ({progress:.1f}%) - ⏳ {remaining:.1f}s remaining..."
    
    st.session_state["export_status"] = f"✅ Export completed in {time.time() - start_time:.2f} sec."


# Export controls in columns
col1, col2, col3 = st.columns([2, 1, 1])
with col1:
    save_path = st.text_input("📁 Export file path", "noise_exports/noise.png")
with col2:
    if st.button("📤 Export as PNG") and save_path.lower().endswith(".png"):
        st.session_state["export_status"] = "Initializing..."
        threading.Thread(target=export_noise, args=(save_path,), daemon=True).start()
with col3:
    if os.path.exists(save_path):
        with open(save_path, "rb") as file:
            st.download_button(
                label="⬇️ Download",
                data=file,
                file_name=os.path.basename(save_path),
                mime="image/png"
            )