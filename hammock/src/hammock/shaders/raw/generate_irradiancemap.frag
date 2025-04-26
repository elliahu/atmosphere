#version 450
#define PI 3.1415926535897932384626433832795

layout (set = 0, binding = 0) uniform sampler2D envMap;

layout (location = 0) in vec2 texCoord;
layout (location = 0) out vec4 fragColor;


layout (push_constant) uniform PushConsts {
    float deltaPhi;
    float deltaTheta;
} consts;


// Function to convert UV coordinates to spherical direction
vec3 uvToSphericalDirection(vec2 uv)
{
    float phi = uv.x * 2.0 * PI;
    float theta = (1.0 - uv.y) * PI;
    float sinTheta = sin(theta);
    return vec3(sinTheta * cos(phi), cos(theta), sinTheta * sin(phi));
}


vec2 directionToUV(vec3 direction)
{
    float phi = atan(direction.z, direction.x);
    float theta = acos(direction.y);
    return vec2(phi / (2.0 * PI) + 0.5, theta / PI);
}

void main()
{
    // uv to direction
    vec3 N = vec3(
    -sin(texCoord.y * PI) * cos(texCoord.x * 2.0 * PI),
    cos(texCoord.y * PI),
    -sin(texCoord.y * PI) * sin(texCoord.x * 2.0 * PI)
    );
    vec3 up = vec3(0.0, 1.0, 0.0);
    vec3 right = normalize(cross(up, N));
    up = cross(N, right);

    const float TWO_PI = PI * 2.0;
    const float HALF_PI = PI * 0.5;

    vec3 color = vec3(0.0);
    uint sampleCount = 0u;
    for (float phi = 0.0; phi < TWO_PI; phi += consts.deltaPhi) {
        for (float theta = 0.0; theta < HALF_PI; theta += consts.deltaTheta) {
            vec3 tempVec = cos(phi) * right + sin(phi) * up;
            vec3 sampleVector = cos(theta) * N + sin(theta) * tempVec;
            color += texture(envMap, directionToUV(sampleVector)).rgb * cos(theta) * sin(theta);
            sampleCount++;
        }
    }

    fragColor = vec4(PI * color / float(sampleCount), 1.0);
}
