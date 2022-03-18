#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec4 inCol;
layout(location = 2) in vec2 inTC;

layout(binding = 0) uniform UniformBufferObject {
    mat4 mVP;
} ubo;

layout(location = 0) out vec4 outCol;
layout(location = 1) out vec2 outTC;

void main() {
	gl_Position = ubo.mVP * vec4( inPosition, 1 );
	outCol = inCol;
	outTC = inTC;
}