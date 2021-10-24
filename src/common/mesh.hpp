/*
 * Physically Based Rendering
 * Forked from Michał Siejak PBR project
 */

#pragma once

#include <assimp/scene.h>

#include <cstdint>
#include <string>
#include <memory>
#include <vector>
#include <glm/glm.hpp>
#include <unordered_map>


class Mesh
{
public:
	enum TextureType : char { Albedo = 0, Normals, Metalness, Roughness, Count };

	struct Vertex
	{
		glm::vec3 position;
		glm::vec3 normal;
		glm::vec3 tangent;
		glm::vec3 bitangent;
		glm::vec2 texcoord;
	};
	static_assert(sizeof(Vertex) == 14 * sizeof(float), "Vertex structure size is incorrect.");
	static const int NumAttributes = 5;

	struct Face
	{
		uint32_t v1, v2, v3;
	};
	static_assert(sizeof(Face) == 3 * sizeof(uint32_t), "Face structure size is incorrect.");

	static std::shared_ptr<Mesh> fromFile(const std::string& filename);
	static std::shared_ptr<Mesh> fromString(const std::string& data);

	const std::vector<Vertex>& vertices() const { return m_vertices; }
	const std::vector<Face>& faces() const { return m_faces; }
	std::string textureName(TextureType TexType) { return (m_textures.count(TexType) > 0) ? m_textures[TexType] : std::string(); }

private:
	Mesh(const aiScene *ScenePtr, size_t MeshIndex = 0);

	std::vector<Vertex> m_vertices;
	std::vector<Face> m_faces;

	std::unordered_map<TextureType, std::string> m_textures;
};
