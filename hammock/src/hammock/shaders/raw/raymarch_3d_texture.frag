#version 450

// Inputs
layout (location = 0) in vec2 inUv;

// Outputs
layout (location = 0) out vec4 outColor;

// Uniforms
layout (set = 0, binding = 0) uniform SceneUbo {
    mat4 inverseProjection;
    mat4 view;
    mat4 inverseView;
    vec4 textureDim;
    vec4 lightPosition;
    vec4 baseSkyColor;
    vec4 tissueColor;
    vec4 fatColor;
    vec4 boneColor;
    vec4 cameraPosition;
} data;

layout (set = 0, binding = 1) uniform sampler3D volumeSampler;

// Push constants
layout (push_constant) uniform PushConstants {
    float resX;
    float resY;
    float elapsedTime;
    float maxSteps;
    float marchSize;
    float airFactor;
    float tissueFactor;
    float fatFactor;
    int nDotL;
} push;

// Transfer function with smooth transitions
vec4 transferFunction(float density) {
    vec4 color = vec4(0.0);

    if (density < push.airFactor) {
        color = vec4(0.0, 0.0, 0.0, 0.0);
    }
    else if (density < push.tissueFactor) {
        float t = (density - push.airFactor) / (push.tissueFactor - push.airFactor);
        color = mix(vec4(0.0, 0.0, 0.0, 0.0), data.tissueColor, t);
    }
    else if (density < push.fatFactor) {
        float t = (density - push.tissueFactor) / (push.fatFactor - push.tissueFactor);
        color = mix(data.tissueColor, data.fatColor, t);
    }
    else {
        float t = (density - push.fatFactor) / (1.0 - push.fatFactor);
        color = mix(data.fatColor, data.boneColor, t);
    }

    return color;
}

// Density function
float density(vec3 p) {
    if (any(lessThan(p, vec3(0.0))) || any(greaterThan(p, vec3(1.0)))) {
        return 0.0; // Outside the volume
    }
    return texture(volumeSampler, p).r;
}

// Raymarching function
vec4 raymarch(vec3 rayOrigin, vec3 rayDirection) {
    float depth = 0.0;
    vec3 p = rayOrigin;
    vec4 accumulatedColor = vec4(0.0);

    // Aspect ratio of the texture: 512x512x150
    const vec3 aspectRatio = vec3(1.0, 1.0, data.textureDim.b / data.textureDim.r);

    for (int i = 0; i < int(push.maxSteps); i++) {
        // Compute the current position in the volume
        p = rayOrigin + depth * rayDirection;

        // Convert world coordinates to texture coordinates
        vec3 textureCoords = (p * 0.5 + 0.5) / aspectRatio; // Normalize by aspect ratio

        // Sample density
        float d = density(textureCoords);

        // Apply transfer function and compositing
        vec4 color = transferFunction(d);
        accumulatedColor.rgb += (1.0 - accumulatedColor.a) * color.rgb * color.a;
        accumulatedColor.a += (1.0 - accumulatedColor.a) * color.a;

        if (accumulatedColor.a >= 1.0) break; // Early termination

        // Advance ray
        depth += push.marchSize;
    }

    return accumulatedColor;
}

void main() {
    vec2 ndc = gl_FragCoord.xy / vec2(push.resX, push.resY);
    vec2 uv = ndc - vec2(0.5, 0.5); // Center UV in NDC

    vec4 clipSpace = vec4(uv, -1.0, 1.0);
    vec4 cameraSpace = data.inverseProjection * clipSpace;
    cameraSpace.xyz /= cameraSpace.w;

    vec3 rayOrigin = (data.cameraPosition).xyz;
    vec3 rayDirection = normalize((data.view * vec4(cameraSpace.xyz, 0.0)).xyz);

    vec3 color = data.baseSkyColor.rgb;

    // Volume rendering with corrected aspect ratio
    if (push.nDotL == 1) {
        // Raymarch to find the first tissue voxel
        float depth = 0.0;
        vec3 hitPos = vec3(0.0);
        vec3 p = rayOrigin;

        // Aspect ratio of the texture: 512x512x150
        const vec3 aspectRatio = vec3(1.0, 1.0, data.textureDim.b / data.textureDim.r);

        bool foundTissue = false;

        for (int i = 0; i < int(push.maxSteps); i++) {
            // Compute the current position in the volume
            p = rayOrigin + depth * rayDirection;

            // Convert world coordinates to texture coordinates
            vec3 textureCoords = (p * 0.5 + 0.5) / aspectRatio; // Normalize by aspect ratio

            // Sample density
            float d = density(textureCoords);

            // Check if the density corresponds to tissue
            if (d >= push.airFactor && d <= push.tissueFactor) {
                foundTissue = true;
                hitPos = textureCoords; // Save the hit position
                break;
            }

            // Advance ray
            depth += push.marchSize;
        }

        if (foundTissue) {
            // Compute normals via central differences
            vec3 gradient = vec3(
            density(hitPos + vec3(push.marchSize, 0.0, 0.0)) - density(hitPos - vec3(push.marchSize, 0.0, 0.0)),
            density(hitPos + vec3(0.0, push.marchSize, 0.0)) - density(hitPos - vec3(0.0, push.marchSize, 0.0)),
            density(hitPos + vec3(0.0, 0.0, push.marchSize)) - density(hitPos - vec3(0.0, 0.0, push.marchSize))
            );

            vec3 normal = normalize(gradient); // Surface normal

            // Light direction (assumes a directional light)
            vec3 lightDir = normalize(data.lightPosition.xyz - p);

            // View direction
            vec3 viewDir = normalize(-rayDirection);

            // Ambient component
            vec3 ambient = 0.1 * data.tissueColor.rgb;

            // Diffuse component (Lambertian reflection)
            float diffuseFactor = max(dot(normal, lightDir), 0.0);
            vec3 diffuse = diffuseFactor * data.tissueColor.rgb;

            // Specular component (Blinn-Phong reflection)
            vec3 halfwayDir = normalize(lightDir + viewDir);
            float specularFactor = pow(max(dot(normal, halfwayDir), 0.0), 16.0); // Shininess factor
            vec3 specular = specularFactor * vec3(1.0); // White highlights

            // Combine all components
            color = ambient + diffuse + specular;
        } else {
            // No tissue was hit, render background
            color = data.baseSkyColor.rgb;
        }

        outColor = vec4(color, 1.0);
        return;
    }

    vec4 volumeColor = raymarch(rayOrigin, rayDirection);
    color = color * (1.0 - volumeColor.a) + volumeColor.rgb;
    outColor = vec4(color, 1.0);
}
