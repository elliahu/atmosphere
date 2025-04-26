#version 450

layout (binding = 0) uniform sampler3D samplerColor;

layout (location = 0) in vec2 inUV;

layout (location = 0) out vec4 outFragColor;

layout (push_constant) uniform PushConstants {
    int channel;
    float slice;
    float scale;
} push;


void main()
{
    vec4 color = texture(samplerColor, vec3(inUV.s, 1.0 - inUV.t, push.slice) * push.scale);

    if(push.channel == 0){
        outFragColor = vec4(vec3(color.r), 1.0);
        return;
    }
    if(push.channel == 1){
        outFragColor = vec4(vec3(color.g), 1.0);
        return;
    }
    if(push.channel == 2){
        outFragColor = vec4(vec3(color.b), 1.0);
        return;
    }
    if(push.channel == 3){
        outFragColor = vec4(vec3(color.a), 1.0);
        return;
    }
}