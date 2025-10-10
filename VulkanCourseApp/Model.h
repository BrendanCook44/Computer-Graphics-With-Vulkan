#pragma once
#include <vector>

#include <glm/glm.hpp>
#include <assimp/scene.h>

#include "Mesh.h"

class Model
{
public:

	Model();
	Model(std::vector<Mesh> newMeshList);

	size_t getMeshCount();
	Mesh* getMesh(size_t index);

	void setModel(glm::mat4 newModel);
	glm::mat4 getModel();

	static std::vector<std::string> LoadMaterials(const aiScene* scene);
	static std::vector<Mesh> LoadNode(VkPhysicalDevice, VkDevice newDevice, VkQueue transferQueue, VkCommandPool transferCommandPool, aiNode* node, const aiScene* scene, std::vector<int> materialToTexture);
	static Mesh LoadMesh(VkPhysicalDevice, VkDevice newDevice, VkQueue transferQueue, VkCommandPool transferCommandPool, aiMesh* mesh, const aiScene* scene, std::vector<int> materialToTexture);

	void destroyMeshModel();
	~Model();


private:
	std::vector<Mesh> meshList;
	glm::mat4 model;

};

