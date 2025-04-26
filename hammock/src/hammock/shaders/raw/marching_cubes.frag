#version 450

layout (location = 0) in vec3 normal;
layout (location = 1) in vec2 uv;
layout (location = 2) in vec3 position;
layout (location = 3) in vec4 tangent;

layout (location = 0) out vec4 color;

layout (set = 0, binding = 0) uniform UBO{
    mat4 mvp;
    vec4 lightPos;
} projection;


void main() {
    // Normalize the normal vector
    vec3 norm = normalize(normal);

    // Compute the light direction
    vec3 lightDir = normalize(vec3(projection.lightPos) - position);

    // Ambient lighting (basic white light)
    vec3 ambient = vec3(0.1);

    // Diffuse lighting (Lambert's cosine law)
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * vec3(1.0); // white diffuse color

    // Specular lighting (Phong model)
    vec3 viewDir = normalize(-position); // Simple view direction assumption
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0); // shininess exponent of 32
    vec3 specular = spec * vec3(1.0); // white specular highlight

    // Combine the ambient, diffuse, and specular components
    vec3 finalColor = ambient + diffuse + specular;

    // Output the final color
    color = vec4(finalColor, 1.0);
}