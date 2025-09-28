#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vector>

#include "Utilities.h"

class Mesh
{
public:
	Mesh();
	Mesh(VkPhysicalDevice newPhysicalDevice, VkDevice newLogicalDevice, VkQueue transferQueue, VkCommandPool transferCommandPool, std::vector<Vertex>* vertices, std::vector<uint32_t>* indices);

	int getVertexCount();
	VkBuffer getVertexBuffer();
	void destroyVertexBuffer();

	int getIndexCount();
	VkBuffer getIndexBuffer();
	void destroyIndexBuffer();

	~Mesh();

private:

	VkPhysicalDevice physicalDevice;
	VkDevice logicalDevice;

	int vertexCount;
	VkBuffer vertexBuffer;
	VkDeviceMemory vertexBufferMemory;
	void createVertexBuffer(VkQueue transferQueue, VkCommandPool transferCommandPool, std::vector<Vertex>* vertices);

	int indexCount;
	VkBuffer indexBuffer;
	VkDeviceMemory indexBufferMemory;
	void createIndexBuffer(VkQueue transferQueue, VkCommandPool transferCommandPool, std::vector<uint32_t>* indices);

};

