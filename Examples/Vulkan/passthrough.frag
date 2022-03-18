#version 450

layout(location = 0) in vec4 inCol;
layout(location = 1) in vec2 inTC;

layout(location = 0) out vec4 outColor;

void main() {
	outColor = inCol;
}