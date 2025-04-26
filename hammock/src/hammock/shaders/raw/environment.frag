#version 450

#include "common/global_binding.glsl"
#include "common/projection_binding.glsl"


layout(location = 0) in vec2 in_uv;

layout(location = 0) out vec4 outPosition;
layout(location = 1) out vec4 outFragColor;
layout(location = 2) out vec4 outNormal;
layout(location = 3) out vec4 outMaterial;


// From http://filmicworlds.com/blog/filmic-tonemapping-operators/
vec3 Uncharted2Tonemap(vec3 color)
{
	float A = 0.15;
	float B = 0.50;
	float C = 0.10;
	float D = 0.20;
	float E = 0.02;
	float F = 0.30;
	float W = 11.2;
	return ((color*(A*color+C*B)+D*E)/(color*(A*color+B)+D*F))-E/F;
}

void main() 
{
    float exposure = projection.exposureGammaWhitePoint.x;
    float gamma = projection.exposureGammaWhitePoint.y;
    float whitePoint = projection.exposureGammaWhitePoint.z;

    // Transform UV coordinates to a direction in view space
    vec4 direction_view = inverse(projection.projection * projection.view) * vec4(in_uv * 2.0 - 1.0, 1.0, 1.0);
    vec3 direction = normalize(direction_view.xyz / direction_view.w);
    
    // Convert 3D direction to 2D UV coordinates using equirectangular projection
    vec2 uv;
    uv.x = 0.5 + 0.5 * atan(direction.z, direction.x) / PI;
    uv.y = 0.5 - asin(clamp(direction.y, -1.0, 1.0)) / PI;

    // Ensure UV wrapping
    uv = mod(uv, 1.0);

    // Sample the environment map in the calculated direction
    vec3 color = textureLod(environmentSampler, uv, 0.0).rgb;
    // Tone mapping
	color = Uncharted2Tonemap(color * exposure);
	color = color * (1.0f / Uncharted2Tonemap(vec3(whitePoint)));
	// Gamma correction
	color = pow(color, vec3(1.0f / gamma));

    // Output the environment color as fragment color
    outFragColor = vec4(color, 1.0);

    outPosition = vec4(0);
    outNormal = vec4(direction, 1.0);
    outMaterial = vec4(-1);
}
