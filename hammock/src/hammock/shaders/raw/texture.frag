#version 450

layout (binding = 0) uniform sampler2D cloudsSampler;
layout (binding = 1) uniform sampler2D skySampler;

layout (binding = 2) uniform PostProcessUBO {
    vec4 colorTint;

    // Tonemapping parameters
    float exposure;
    float gamma;
    int tonemapOperator;  // 0: Linear, 1: Reinhard, 2: ACES, 3: Uncharted 2

    // Color grading parameters
    float contrast;
    float brightness;
    float saturation;


    float vignetteStrength;
    float vignetteSoftness;

    // Color temperature/white balance
    float temperature; // -1.0 to 1.0, cool to warm

    // Film grain
    float grainAmount;

    // Adjust cloud blending
    int cloudBlendMode; // 0: Normal, 1: Screen, 2: Soft Light

    float time;
};

layout (location = 0) in vec2 uv;
layout (location = 0) out vec4 outFragColor;

// Utility functions
vec3 rgb2hsv(vec3 c) {
    vec4 K = vec4(0.0, -1.0/3.0, 2.0/3.0, -1.0);
    vec4 p = mix(vec4(c.bg, K.wz), vec4(c.gb, K.xy), step(c.b, c.g));
    vec4 q = mix(vec4(p.xyw, c.r), vec4(c.r, p.yzx), step(p.x, c.r));

    float d = q.x - min(q.w, q.y);
    float e = 1.0e-10;
    return vec3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);
}

vec3 hsv2rgb(vec3 c) {
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

// Random hash function for film grain
float hash(vec2 p) {
    p = 50.0 * fract(p * 0.3183099);
    return fract(p.x * p.y * (p.x + p.y));
}

// ACES Tonemapping function
vec3 ACESFilm(vec3 x) {
    float a = 2.51;
    float b = 0.03;
    float c = 2.43;
    float d = 0.59;
    float e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

// Uncharted 2 tonemapping (modified version)
vec3 Uncharted2Tonemap(vec3 color) {
    float A = 0.15;
    float B = 0.50;
    float C = 0.10;
    float D = 0.20;
    float E = 0.02;
    float F = 0.30;
    float W = 11.2;
    return ((color*(A*color+C*B)+D*E)/(color*(A*color+B)+D*F))-E/F;
}

// Color temperature adjustment
vec3 adjustTemperature(vec3 rgb, float temperature) {
    // Apply temperature shift (blue to orange)
    vec3 warm = vec3(1.0, 0.6, 0.4); // Warm color bias
    vec3 cool = vec3(0.4, 0.6, 1.0); // Cool color bias

    vec3 tempAdjust = mix(cool, warm, (temperature + 1.0) * 0.5);
    return rgb * tempAdjust;
}

// Apply vignette effect
float vignette(vec2 uv, float strength, float softness) {
    uv = uv * 2.0 - 1.0;
    float dist = length(uv);
    return smoothstep(strength, strength - softness, dist);
}

// Cloud blending modes
vec3 blendNormal(vec3 base, vec3 blend, float opacity) {
    return mix(base, blend, opacity);
}

vec3 blendScreen(vec3 base, vec3 blend) {
    return 1.0 - (1.0 - base) * (1.0 - blend);
}

vec3 blendSoftLight(vec3 base, vec3 blend) {
    return (max(blend.x,max(blend.y, blend.z)) < 0.5) ?
    base - (1.0 - 2.0 * blend) * base * (1.0 - base) :
    base + (2.0 * blend - 1.0) * (sqrt(base) - base);
}


void main()
{
    // Sample clouds and sky
    vec4 clouds = texture(cloudsSampler, vec2(uv.x, 1.0 - uv.y));
    vec4 sky = texture(skySampler, vec2(uv.x, 1.0 - uv.y));

    sky = vec4(pow(sky.xyz, vec3(2.2)),sky.a);

    // Cloud blending based on selected mode
    vec3 cloudBlended;
    if (cloudBlendMode < 0.5) {
        // Normal blending
        cloudBlended = sky.rgb * (1.0 - clouds.a) + clouds.rgb;
    } else if (cloudBlendMode < 1.5) {
        // Screen blending
        cloudBlended = mix(sky.rgb, blendScreen(sky.rgb, clouds.rgb), clouds.a);
    } else {
        // Soft light blending
        cloudBlended = mix(sky.rgb, blendSoftLight(sky.rgb, clouds.rgb), clouds.a);
    }

    // Apply exposure
    vec3 hdrColor = cloudBlended* pow(2.0, exposure);

    // Apply tonemapping based on selected operator
    vec3 tonemapped;
    if (tonemapOperator < 0.5) {
        // Linear
        tonemapped = hdrColor;
    } else if (tonemapOperator < 1.5) {
        // Reinhard
        tonemapped = hdrColor / (1.0 + hdrColor);
    } else if (tonemapOperator < 2.5) {
        // ACES
        tonemapped = ACESFilm(hdrColor);
    } else {
        // Uncharted 2
        float W = 11.2;
        vec3 whiteScale = 1.0 / Uncharted2Tonemap(vec3(W));
        tonemapped = Uncharted2Tonemap(hdrColor) * whiteScale;
    }

    // Apply color grading

    // Convert to HSV for easier color adjustments
    vec3 hsv = rgb2hsv(tonemapped);

    // Adjust saturation
    hsv.y *= saturation;

    // Back to RGB
    vec3 graded = hsv2rgb(hsv);

    // Apply contrast and brightness
    graded = (graded - 0.5) * contrast + 0.5 + brightness;

    // Apply color tint
    graded *= colorTint.rgb;

    // Apply temperature adjustment
    graded = adjustTemperature(graded, temperature);


    // Apply vignette
    float vig = vignette(uv, vignetteStrength, vignetteSoftness);
    graded *= vig;

    // Apply film grain
    if (grainAmount > 0.0) {
        float grain = hash((uv * vec2(1920,1080)) + time * 10.0);
        graded += (grain - 0.5) * grainAmount;
    }

    // Apply gamma correction
    graded = pow(graded, vec3(1.0 / gamma));

    // Output final color
    outFragColor = vec4(graded, 1.0);
}