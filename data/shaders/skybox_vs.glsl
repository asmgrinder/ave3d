#version 450 core
// Physically Based Rendering
// * Forked from Michał Siejak PBR project

// Environment skybox: Vertex program.
layout(std140, binding=0) uniform SkyboxUniforms
{
	mat4 skyViewProjectionMatrix;
};

layout(location=0) in vec3 position;
layout(location=0) out vec3 localPosition;

void main()
{
	localPosition = position.xyz;
	gl_Position   = skyViewProjectionMatrix * vec4(position, 1.0);
}
