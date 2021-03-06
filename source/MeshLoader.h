#pragma once

#include <vector>
#include <memory>

#include <glm\vec3.hpp>

#include <glbinding\gl\gl.h>

#include "assimp/Importer.hpp"
#include "assimp/Scene.h"

class MeshLoader
{
public:
	MeshLoader();

	//ownership shifts to painter
	void getLineVertices(std::vector<glm::vec3> & verticesContainer, std::vector<unsigned int> & lineIndicesContainer, bool firstLine);
	bool getVertices(std::vector<glm::vec3> & verticesContainer, std::vector<glm::vec3> & lineVerticesContainer, std::vector<glm::vec3> & lineVerticesSecondContainer, std::vector<glm::vec3> & flatLineVerticesContainer, std::vector<glm::vec3> & flatSecondLineVerticesContainer, std::vector<glm::vec3> & planeVerticesContainer, std::vector<glm::vec3> & streetsVerticesContainer);
	bool getIndices(std::vector<unsigned int> & indicesContainer, std::vector<unsigned int> & lineIndicesContainer, std::vector<unsigned int> & lineIndicesSecondContainer, std::vector<unsigned int> & flatLineIndicesContainer, std::vector<unsigned int> & flatSecondLineIndicesContainer, std::vector<unsigned int> & planeIndicesContainer, std::vector<unsigned int> & streetsIndicesContainer);
	bool getNormals(std::vector<glm::vec3> & normalsContainer);

	void loadFileData();

protected:
	bool import3DFromFile(const std::string & file);
	bool pushVertices(const aiMesh * mesh, std::vector<glm::vec3>& verticesContainer);

protected:
	Assimp::Importer m_importer;
};