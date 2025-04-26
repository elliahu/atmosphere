struct VertexOutput {
    float2 uv;
    float4 position : SV_Position; // Screen-space position
};

struct PixelOutput {
    float4 color : SV_Target;
};

struct PostProcessingData{
    float4 colorTint;
    float contrast ;
    float brightness;
    float vignetteStrength;
    float vignetteSoftness;
    float grainAmount;
    float time;
};

[vk::binding(0)] Sampler2D compositionImage;
[vk::push_constant] PostProcessingData post;

// Fullscreen vertex shader: Outputs a unit square (quad) in normalized device coordinates
[shader("vertex")]
VertexOutput vertexMain(uint vertexID : SV_VertexID) {
    VertexOutput output;
    output.uv = float2((vertexID << 1) & 2, vertexID & 2);
    output.position = float4(output.uv * 2.0 - 1.0, 0.0, 1.0);
    return output;
}


// Random hash function for film grain
float hash(float2 p) {
    p = 50.0 * fract(p * 0.3183099);
    return fract(p.x * p.y * (p.x + p.y));
}

// Apply vignette effect
float vignette(float2 uv, float strength, float softness) {
    uv = uv * 2.0 - 1.0;
    float dist = length(uv);
    return smoothstep(strength, strength - softness, dist);
}

// Post processing constants, 
// looks like dark magic but it is not
// Better performance than ACCESS tonemap and nicer results in context of atmospheric medium
#define POSTCOLOR_A 2.51
#define POSTCOLOR_B 0.03
#define POSTCOLOR_C 2.43
#define POSTCOLOR_D 0.59
#define POSTCOLOR_E 0.14

// The actual tonemap function
float3 tonemap(float3 input)
{
    return (input * (POSTCOLOR_A * input + POSTCOLOR_B))
         / (input * (POSTCOLOR_C * input + POSTCOLOR_D) + POSTCOLOR_E);
}

// Pseudo random tonemap
float3 postProcessColor(float2 seed, float3 input)
{
    input = tonemap(input);

    float rand = frac(sin(dot(
        seed, float2(12.9898, 78.233) * 2.0)) * 43758.5453);
    input = 255 * saturate(pow(input, 1 / 2.2));
    input = rand.xxx < (input - floor(input)) ? ceil(input) : floor(input);

    return input / 255;
}


// Pixel shader
[shader("pixel")]
PixelOutput pixelMain(VertexOutput input, float4 fragCoord : SV_Position){
    PixelOutput output;
    float3 color = compositionImage.SampleLevel(float2(input.uv.x, 1.0 - input.uv.y), 0.0).rgb;
    float3 graded =  postProcessColor(input.uv, color);

     // Apply contrast and brightness
    graded = (graded - 0.5) * post.contrast + 0.5 + post.brightness;

    
    // Apply color tint
    graded *= post.colorTint.rgb;


    // Apply vignette
    float vig = vignette(input.uv, post.vignetteStrength, post.vignetteSoftness);
    graded *= vig;

    // Apply film grain
    if (post.grainAmount > 0.0) {
        float grain = hash((input.uv * float2(1920,1080)) + post.time * 100.0);
        graded += (grain - 0.5) * post.grainAmount;
    }

    // Output final color=
    output.color = float4(graded, 1.0);

    return output;
}

