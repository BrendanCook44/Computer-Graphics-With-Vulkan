#include "Model.h"

Model::Model()
{
	model = glm::mat4(1.0f);
}

Model::Model(std::vector<Mesh> newMeshList)
{
	meshList = newMeshList;
}

size_t Model::getMeshCount()
{
	return meshList.size();
}
Mesh* Model::getMesh(size_t index)
{
	if (index >= meshList.size())
	{
		throw std::runtime_error("Failed to access invalid mesh index!");
	}

	else
	{
		return &meshList[index];
	}
}

void Model::setModel(glm::mat4 newModel)
{
	model = newModel;
}

glm::mat4 Model::getModel()
{
	return model;
}

std::vector<std::string> Model::LoadMaterials(const aiScene* scene)
{
	// Create 1:1 sized list of textures
	std::vector<std::string> textureList(scene->mNumMaterials);

	// Go through each material and copy its texture file name (if exists)
	for (size_t i = 0; i < scene->mNumMaterials; i++)
	{
		// Get the material
		aiMaterial* material = scene->mMaterials[i];

		// Initialize texture to empty string, and will be replaced if texture exists)
		textureList[i] = "";

		// Check for a Diffuse Texture (standard detail texture)
		if (material->GetTextureCount(aiTextureType_DIFFUSE))
		{
			aiString path;
			if (material->GetTexture(aiTextureType_DIFFUSE, 0, &path) == AI_SUCCESS)
			{
				// Cut off any directory information coming from the model import to only include the file name at the end
				int index = std::string(path.data).rfind("\\");
				std::string fileName = std::string(path.data).substr(index + 1);

				textureList[i] = fileName;
			}
		}
	}

	return textureList;
}

std::vector<Mesh> Model::LoadNode(VkPhysicalDevice physicalDevice, VkDevice logicalDevice, VkQueue transferQueue, VkCommandPool transferCommandPool, aiNode* node, const aiScene* scene, std::vector<int> materialToTexture)
{
	std::vector<Mesh> meshList;

	for (size_t i = 0; i < node->mNumMeshes; i++)
	{
		meshList.push_back(Model::LoadMesh(physicalDevice, logicalDevice, transferQueue, transferCommandPool, scene->mMeshes[node->mMeshes[i]], scene, materialToTexture));
	}

	// Go through each node attached to this node and load it, then append their meshes to this node's mesh list
	for (size_t i = 0; i < node->mNumChildren; i++)
	{
		std::vector<Mesh> newList = LoadNode(physicalDevice, logicalDevice, transferQueue, transferCommandPool, node->mChildren[i], scene, materialToTexture);
		meshList.insert(meshList.end(), newList.begin(), newList.end());
	}

	return meshList;
}

Mesh Model::LoadMesh(VkPhysicalDevice physicalDevice, VkDevice logicalDevice, VkQueue transferQueue, VkCommandPool transferCommandPool, aiMesh* mesh, const aiScene* scene, std::vector<int> materialToTexture)
{
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;

	// Resize vertex list to hold all vertices for mesh
	vertices.resize(mesh->mNumVertices);

	// Go through each vertex and copy it across to vertices
	for (size_t i = 0; i < mesh->mNumVertices; i++)
	{
		// Set position
		vertices[i].position = { mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z };

		// Set texture coordinates if available
		if (mesh->mTextureCoords[0])
		{
			vertices[i].texture = { mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y };
		}

		else
		{
			vertices[i].texture = { 0.0f, 0.0f };
		}

		// Set color
		vertices[i].color = { 1.0f, 1.0f, 1.0f };
	}

	// Iterate over indices in triangle faces and copy across to indices
	for (size_t i = 0; i < mesh->mNumFaces; i++)
	{

		aiFace face = mesh->mFaces[i];
		for (size_t j = 0; j < face.mNumIndices; j++)
		{
			indices.push_back(face.mIndices[j]);
		}
	}

	// Crete new mesh with details and return
	Mesh newMesh = Mesh(physicalDevice, logicalDevice, transferQueue, transferCommandPool, &vertices, &indices, materialToTexture[mesh->mMaterialIndex]);

	return newMesh;
	
}

void Model::destroyMeshModel()
{
	for (auto& mesh : meshList)
	{
		mesh.destroyVertexBuffer();
		mesh.destroyIndexBuffer();
	}
}

Model::~Model()
{

}