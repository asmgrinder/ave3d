#version 450 core
// Physically Based Rendering
// * Forked from Michał Siejak PBR project

// Physically Based shading model: Vertex program.

layout(location=0) in vec3 position;
layout(location=1) in vec3 normal;
layout(location=2) in vec3 tangent;
layout(location=3) in vec3 bitangent;
layout(location=4) in vec2 texcoord;

layout(std140, binding=0) uniform TransformUniforms
{
// 	mat4 skyViewProjectionMatrix;
	mat4 viewProjectionMatrix;
	mat4 modelMatrix;
};

layout(location=0) out Vertex
{
	vec3 position;
	vec2 texcoord;
	mat3 tangentBasis;
} vout;

void main()
{
	vout.position = vec3(modelMatrix * vec4(position, 1.0));
	vout.texcoord = vec2(texcoord.x, 1.0 - texcoord.y);

	// Pass tangent space basis vectors (for normal mapping).
	vout.tangentBasis = mat3(modelMatrix) * mat3(tangent, bitangent, normal);

	gl_Position = viewProjectionMatrix * modelMatrix * vec4(position, 1.0);
}
