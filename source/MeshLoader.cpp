#include "MeshLoader.h"

#include <fstream>
#include <string>
#include <iostream>
#include <string.h>

#include "assimp/PostProcess.h"

namespace {
	const std::string c_sceneFile = "data/scene.obj";
}

MeshLoader::MeshLoader()
	: m_importer()
{
}

void MeshLoader::getLineVertices(std::vector<glm::vec3> & verticesContainer, std::vector<unsigned int> & lineIndicesContainer)
{
	verticesContainer.emplace_back(-16.8429f, -0.7704f, -33.3231f);
	verticesContainer.emplace_back(-16.8429f, -0.7704f, -15.7980f);
	verticesContainer.emplace_back(-5.5530f, -0.7704f, -15.7980f);
	verticesContainer.emplace_back(-5.4952f, -0.7704f, -0.4148f);
	verticesContainer.emplace_back(-11.6776f, -0.7704f, -0.3741f);
	verticesContainer.emplace_back(-11.6384f, -0.7704f, 9.2194f);
	verticesContainer.emplace_back(8.1854f, -0.7704f, 9.1957f);
	verticesContainer.emplace_back(8.2115f, -0.7704f, 15.0166f);
	verticesContainer.emplace_back(26.8190f, -0.7704f, 14.9882f);
	verticesContainer.emplace_back(26.8091f, -0.7704f, -0.3075f);
	verticesContainer.emplace_back(23.3483f, -0.7704f, -0.3034f);
	verticesContainer.emplace_back(23.3394f, -0.7704f, -9.2318f);
	verticesContainer.emplace_back(30.7871f, -0.7704f, -9.2393f);

	for (unsigned int i = 0; i < verticesContainer.size(); i++)
	{
		lineIndicesContainer.push_back(i);
	}
}

bool MeshLoader::getVertices(std::vector<glm::vec3> & verticesContainer, std::vector<glm::vec3> & lineVerticesContainer, std::vector<glm::vec3> & lineVerticesSecondContainer, std::vector<glm::vec3> & flatLineVerticesContainer, std::vector<glm::vec3> & flatSecondLineVerticesContainer, std::vector<glm::vec3> & planeVerticesContainer, std::vector<glm::vec3> & streetsVerticesContainer)
{
	
	if (!m_importer.GetScene())
		return false;

	const aiScene & scene = *m_importer.GetScene();

	// For each mesh
	for (unsigned int n = 0; n < scene.mNumMeshes; ++n)
	{
		const aiMesh * mesh = scene.mMeshes[n];
		if (!mesh)
			return false;

		if (strcmp(mesh->mName.C_Str(), "g lineVertices") == 0)
		{
			continue;
		}
		else if (strcmp(mesh->mName.C_Str(), "g Line") == 0)
		{
			pushVertices(mesh, lineVerticesContainer);
		}
		else if (strcmp(mesh->mName.C_Str(), "g Line2") == 0)
		{
			pushVertices(mesh, lineVerticesSecondContainer);
		}
		if (strcmp(mesh->mName.C_Str(), "g Lineflat") == 0)
		{
			pushVertices(mesh, flatLineVerticesContainer);
		}
		else if (strcmp(mesh->mName.C_Str(), "g Line2flat") == 0)
		{
			pushVertices(mesh, flatSecondLineVerticesContainer);
		}
		else if (strcmp(mesh->mName.C_Str(), "g GroundPlane") == 0)
		{
			pushVertices(mesh, planeVerticesContainer);
		}
		else if (strcmp(mesh->mName.C_Str(), "g Streets") == 0)
		{
			pushVertices(mesh, streetsVerticesContainer);
		}
		else
		{
			pushVertices(mesh, verticesContainer);
		}
	}

	return (verticesContainer.size() <= 0) ? false : true;
}

bool MeshLoader::getIndices(std::vector<unsigned int> & indicesContainer, std::vector<unsigned int> & lineIndicesContainer, std::vector<unsigned int> & lineIndicesSecondContainer, std::vector<unsigned int> & flatLineIndicesContainer, std::vector<unsigned int> & flatSecondLineIndicesContainer, std::vector<unsigned int> & planeIndicesContainer, std::vector<unsigned int> & streetsIndicesContainer)
{
	if (!m_importer.GetScene())
		return false;

	const aiScene & scene = *m_importer.GetScene();
	unsigned int vertexCount = 0;

	// For each mesh
	for (unsigned int n = 0; n < scene.mNumMeshes; ++n)
	{
		const aiMesh * mesh = scene.mMeshes[n];

		if (strcmp(mesh->mName.C_Str(), "g Line") == 0)
		{
			for (unsigned int t = 0; t < mesh->mNumFaces; ++t)
			{
				const auto & face = mesh->mFaces[t];
				lineIndicesContainer.push_back(face.mIndices[0]);
				lineIndicesContainer.push_back(face.mIndices[1]);
				lineIndicesContainer.push_back(face.mIndices[2]);
			}
		}
		else if (strcmp(mesh->mName.C_Str(), "g Line2") == 0)
		{
			for (unsigned int t = 0; t < mesh->mNumFaces; ++t)
			{
				const auto & face = mesh->mFaces[t];
				lineIndicesSecondContainer.push_back(face.mIndices[0]);
				lineIndicesSecondContainer.push_back(face.mIndices[1]);
				lineIndicesSecondContainer.push_back(face.mIndices[2]);
			}
		}
		else if (strcmp(mesh->mName.C_Str(), "g Lineflat") == 0)
		{
			for (unsigned int t = 0; t < mesh->mNumFaces; ++t)
			{
				const auto & face = mesh->mFaces[t];
				flatLineIndicesContainer.push_back(face.mIndices[0]);
				flatLineIndicesContainer.push_back(face.mIndices[1]);
				flatLineIndicesContainer.push_back(face.mIndices[2]);
			}
		}
		else if (strcmp(mesh->mName.C_Str(), "g Line2flat") == 0)
		{
			for (unsigned int t = 0; t < mesh->mNumFaces; ++t)
			{
				const auto & face = mesh->mFaces[t];
				flatSecondLineIndicesContainer.push_back(face.mIndices[0]);
				flatSecondLineIndicesContainer.push_back(face.mIndices[1]);
				flatSecondLineIndicesContainer.push_back(face.mIndices[2]);
			}
		}
		else if (strcmp(mesh->mName.C_Str(), "g GroundPlane") == 0)
		{
			for (unsigned int t = 0; t < mesh->mNumFaces; ++t)
			{
				const auto & face = mesh->mFaces[t];
				planeIndicesContainer.push_back(face.mIndices[0]);
				planeIndicesContainer.push_back(face.mIndices[1]);
				planeIndicesContainer.push_back(face.mIndices[2]);
			}
		}
		else if (strcmp(mesh->mName.C_Str(), "g Streets") == 0)
		{
			for (unsigned int t = 0; t < mesh->mNumFaces; ++t)
			{
				const auto & face = mesh->mFaces[t];
				streetsIndicesContainer.push_back(face.mIndices[0]);
				streetsIndicesContainer.push_back(face.mIndices[1]);
				streetsIndicesContainer.push_back(face.mIndices[2]);
			}
		}
		else
		{
			for (unsigned int t = 0; t < mesh->mNumFaces; ++t)
			{
				const auto & face = mesh->mFaces[t];
				indicesContainer.push_back(face.mIndices[0] + vertexCount);
				indicesContainer.push_back(face.mIndices[1] + vertexCount);
				indicesContainer.push_back(face.mIndices[2] + vertexCount);
			}
			vertexCount = vertexCount + mesh->mNumVertices;
		}
	}

	return (indicesContainer.size() <= 0) ? false : true;
}

bool MeshLoader::getNormals(std::vector<glm::vec3> & normalsContainer)
{
	if (!m_importer.GetScene())
		return false;

	const aiScene & scene = *m_importer.GetScene();

	// For each mesh
	for (unsigned int n = 0; n < scene.mNumMeshes; ++n)
	{
		const aiMesh * mesh = scene.mMeshes[n];
		if (strcmp(mesh->mName.C_Str(), "g GroundPlane") == 0)
			continue;
		if (strcmp(mesh->mName.C_Str(), "g Streets") == 0)
			continue;

		if (mesh->HasNormals())
		{
			for (unsigned int t = 0; t < mesh->mNumVertices; ++t)
			{
				const auto & normal = mesh->mNormals[t];
				normalsContainer.push_back(glm::vec3(normal.x, normal.y, normal.z));
			}
		}
	}

	return (normalsContainer.size() <= 0) ? false : true;
}

void MeshLoader::loadFileData()
{
	import3DFromFile(c_sceneFile);
}

bool MeshLoader::import3DFromFile(const std::string & file)
{

	//check if file exists
	std::ifstream fin(file.c_str());
	if (!fin.fail()) {
		fin.close();
	}
	else {
		std::cout << "Couldn't open file: " << file.c_str() << std::endl;
		std::cout << m_importer.GetErrorString() << std::endl;
		return false;
	}
	
	m_importer.ReadFile(file, aiProcessPreset_TargetRealtime_Quality);

	// If the import failed, report it
	if (!m_importer.GetScene())
	{
		printf("%s\n", m_importer.GetErrorString());
		return false;
	}

	// Now we can access the file's contents.
	printf("Import of m_scene %s succeeded.", file.c_str());

	return true;
}

bool MeshLoader::pushVertices(const aiMesh * mesh, std::vector<glm::vec3>& verticesContainer)
{
	if (!mesh)
		return false;

	glm::vec3 vertex;

	for (unsigned int t = 0; t < mesh->mNumVertices; ++t)
	{
		aiVector3D aiVertex = mesh->mVertices[t];
		vertex.x = aiVertex.x;
		vertex.y = aiVertex.y;
		vertex.z = aiVertex.z;

		verticesContainer.push_back(vertex);
	}

	return true;
}
