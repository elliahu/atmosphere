#ifndef MESH_BINDING_GLSL
#define MESH_BINDING_GLSL

layout (push_constant) uniform PushConstants {
    mat4 model;
    int meshIndex;
} push;

#endif