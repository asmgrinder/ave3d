/*
 * Physically Based Rendering
 * Forked from Michał Siejak PBR project
 */

#include <cstdio>
#include "mesh.hpp"
#include <assimp/postprocess.h>
#include <assimp/Importer.hpp>
#include <assimp/DefaultLogger.hpp>
#include <assimp/LogStream.hpp>


#include <iostream>

namespace
{
	const unsigned int ImportFlags = 
		aiProcess_CalcTangentSpace |
		aiProcess_Triangulate |
		aiProcess_SortByPType |
		aiProcess_PreTransformVertices |
		aiProcess_GenNormals |
		aiProcess_GenUVCoords |
		aiProcess_OptimizeMeshes |
		aiProcess_Debone |
		aiProcess_ValidateDataStructure;
}

struct LogStream : public Assimp::LogStream
{
	static void initialize()
	{
		if (Assimp::DefaultLogger::isNullLogger())
		{
			Assimp::DefaultLogger::create("", Assimp::Logger::VERBOSE);
			Assimp::DefaultLogger::get()->attachStream(new LogStream, Assimp::Logger::Err | Assimp::Logger::Warn);
		}
	}
	
	void write(const char* message) override
	{
		std::cerr << "Assimp: " << message << std::endl;
	}
};

std::string getFileNameFromPath(std::string PathStr)
{
	size_t pos = PathStr.find_last_of("/\\");
	return PathStr.substr((std::string::npos == pos) ? 0 : 1 + pos);
}

Mesh::Mesh(const aiScene *ScenePtr, size_t MeshIndex)
{
	const aiMesh *meshPtr = ScenePtr->mMeshes[MeshIndex];
	assert(meshPtr->HasPositions());
	assert(meshPtr->HasNormals());

	m_vertices.reserve(meshPtr->mNumVertices);
	for (size_t i = 0; i < m_vertices.capacity(); i++)
	{
		Vertex vertex;
		vertex.position = { meshPtr->mVertices[i].x, meshPtr->mVertices[i].y, meshPtr->mVertices[i].z };
		vertex.normal = { meshPtr->mNormals[i].x, meshPtr->mNormals[i].y, meshPtr->mNormals[i].z };
		if (meshPtr->HasTangentsAndBitangents())
		{
			vertex.tangent = { meshPtr->mTangents[i].x, meshPtr->mTangents[i].y, meshPtr->mTangents[i].z };
			vertex.bitangent = { meshPtr->mBitangents[i].x, meshPtr->mBitangents[i].y, meshPtr->mBitangents[i].z };
		}
		if (meshPtr->HasTextureCoords(0))
		{
			vertex.texcoord = { meshPtr->mTextureCoords[0][i].x, meshPtr->mTextureCoords[0][i].y };
		}
		m_vertices.push_back(vertex);
	}

	m_faces.reserve(meshPtr->mNumFaces);
	for (size_t i = 0; i < m_faces.capacity(); ++i)
	{
		assert(meshPtr->mFaces[i].mNumIndices == 3);
		m_faces.push_back({ meshPtr->mFaces[i].mIndices[0], meshPtr->mFaces[i].mIndices[1], meshPtr->mFaces[i].mIndices[2] });
	}

	const aiMaterial *materialPtr = ScenePtr->mMaterials[meshPtr->mMaterialIndex];
	aiString textureStr;
	if (materialPtr->GetTextureCount(aiTextureType_DIFFUSE) > 0
		&& AI_SUCCESS == materialPtr->GetTexture(aiTextureType_DIFFUSE, 0, &textureStr))
	{
		m_textures[Mesh::TextureType::Albedo] = getFileNameFromPath(std::string(textureStr.C_Str()));
	}
	if (materialPtr->GetTextureCount(aiTextureType_NORMALS) > 0
		&& AI_SUCCESS == materialPtr->GetTexture(aiTextureType_NORMALS, 0, &textureStr))
	{
		m_textures[Mesh::TextureType::Normals] = getFileNameFromPath(std::string(textureStr.C_Str()));
	}
	if (materialPtr->GetTextureCount(aiTextureType_METALNESS) > 0
		&& AI_SUCCESS == materialPtr->GetTexture(aiTextureType_METALNESS, 0, &textureStr))
	{
		m_textures[Mesh::TextureType::Metalness] = getFileNameFromPath(std::string(textureStr.C_Str()));
	}
	else if (materialPtr->GetTextureCount(aiTextureType_SPECULAR) > 0
		&& AI_SUCCESS == materialPtr->GetTexture(aiTextureType_SPECULAR, 0, &textureStr))
	{
		m_textures[Mesh::TextureType::Metalness] = getFileNameFromPath(std::string(textureStr.C_Str()));
	}
	if (materialPtr->GetTextureCount(aiTextureType_SHININESS) > 0
		&& AI_SUCCESS == materialPtr->GetTexture(aiTextureType_SHININESS, 0, &textureStr))
	{
		m_textures[Mesh::TextureType::Roughness] = getFileNameFromPath(std::string(textureStr.C_Str()));
	}

// 	for (const auto &p : m_textures)
// 	{
// 		std::cout << p.second << std::endl;
// 	}

// 	std::vector<aiTextureType> types =
// 	{
// 		aiTextureType_DIFFUSE,// = 1,
// 		aiTextureType_SPECULAR,// = 2,
// 		aiTextureType_AMBIENT,// = 3,
// 		aiTextureType_EMISSIVE,// = 4,
// 		aiTextureType_HEIGHT,// = 5,
// 		aiTextureType_NORMALS,// = 6,
// 		aiTextureType_SHININESS,// = 7,
// 		aiTextureType_OPACITY,// = 8,
// 		aiTextureType_DISPLACEMENT,// = 9,
// 		aiTextureType_LIGHTMAP,// = 10,
// 		aiTextureType_REFLECTION,// = 11,
// 		aiTextureType_BASE_COLOR,// = 12,
// 		aiTextureType_NORMAL_CAMERA,// = 13,
// 		aiTextureType_EMISSION_COLOR,// = 14,
// 		aiTextureType_METALNESS,// = 15,
// 		aiTextureType_DIFFUSE_ROUGHNESS,// = 16,
// 		aiTextureType_AMBIENT_OCCLUSION	// = 17,
// 	};
//
// 	for (const aiTextureType type : types)
// 	{
// 		for (size_t i = 0; i < materialPtr->GetTextureCount(type); i++)
// 		{
// 			aiString textureStr;
// 			if (AI_SUCCESS == materialPtr->GetTexture(type, i, &textureStr))
// 			{
// 				std::cout << "type: " << type << ", Path name: " << " " << textureStr.C_Str() << std::endl;
// 			}
// 		}
// 	}
// 	aiColor3D colorData;
// 	if (AI_SUCCESS == materialPtr->Get(AI_MATKEY_COLOR_DIFFUSE, colorData))
// 	{
// 	}
}

std::shared_ptr<Mesh> Mesh::fromFile(const std::string& filename)
{
	LogStream::initialize();

	std::cout << "Loading mesh: " << filename << std::endl;

	std::shared_ptr<Mesh> meshPtr;
	Assimp::Importer importer;

	const aiScene* scenePtr = importer.ReadFile(filename, ImportFlags);
	if (scenePtr
		&& scenePtr->HasMeshes())
	{
		meshPtr = std::shared_ptr<Mesh>(new Mesh { scenePtr, 0 });
		return meshPtr;
	}
	throw std::runtime_error("Failed to load mesh file: " + filename);
}

std::shared_ptr<Mesh> Mesh::fromString(const std::string& data)
{
	LogStream::initialize();

	std::shared_ptr<Mesh> meshPtr;
	Assimp::Importer importer;

	const aiScene* scenePtr = importer.ReadFileFromMemory(data.c_str(), data.length(), ImportFlags, "nff");
	if (scenePtr && scenePtr->HasMeshes())
	{
		meshPtr = std::shared_ptr<Mesh>(new Mesh { scenePtr, 0 });
		return meshPtr;
	}
	throw std::runtime_error("Failed to create mesh from string: " + data);
}
