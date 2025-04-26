#version 450
#include "common/global_binding.glsl"
#include "common/projection_binding.glsl"
#include "common/mesh_binding.glsl"

// inputs
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 uv;
layout (location = 3) in vec4 tangent;

// outputs
layout (location = 0) out vec3 _normal;
layout (location = 1) out vec2 _uv;
layout (location = 2) out vec3 _position;
layout (location = 3) out vec4 _tangent;


void main()
{
	gl_Position = projection.projection * projection.view * push.model * vec4(position, 1.0);
	_uv = uv;

    // vertex pos in viewspace
	_position = vec3(projection.view * (push.model * vec4(position, 1.0)));

    // normal in view space
    mat3 normalMatrix = transpose(inverse(mat3(projection.view * push.model)));
	_normal = normalMatrix * normalize(normal);	
	_tangent = mat4(normalMatrix) * normalize(tangent);
}