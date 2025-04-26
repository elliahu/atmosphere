#version 450

// inputs ignored but bound
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 uv;
layout (location = 3) in vec4 tangent;

layout (location = 0) out vec3 _position;
layout (location = 1) out vec3 _normal;
layout (location = 2) out vec2 _uv;

layout (set = 0, binding = 0) uniform SceneUbo {
    mat4 mvp;
} ubo;

void main(){
    _position = position;
    _normal = normal;
    _uv = uv;

    gl_Position = ubo.mvp * vec4(position, 1.0);
}