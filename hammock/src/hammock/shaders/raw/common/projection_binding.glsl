#ifndef PROJECTION_BINDING_GLSL
#define PROJECTION_BINDING_GLSL


layout (set = 1, binding = 0) uniform UBO{
    mat4 projection;
    mat4 view;
    mat4 inverseView;
    vec4 exposureGammaWhitePoint;
} projection;

#endif