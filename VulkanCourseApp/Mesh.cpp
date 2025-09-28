#include "Mesh.h"

Mesh::Mesh()
{

}

Mesh::Mesh(VkPhysicalDevice newPhysicalDevice, VkDevice newLogicalDevice, VkQueue transferQueue, VkCommandPool transferCommandPool, std::vector<Vertex>* vertices)
{
	physicalDevice = newPhysicalDevice;
	logicalDevice = newLogicalDevice;
	vertexCount = vertices->size();
	createVertexBuffer(transferQueue, transferCommandPool, vertices);
}

void Mesh::createVertexBuffer(VkQueue transferQueue, VkCommandPool transferCommandPool, std::vector<Vertex>* vertices)
{
	// Get size of buffer needed for vertices
	VkDeviceSize bufferSize = sizeof(Vertex) * vertices->size();

	// Temporary buffer to stage vertex data before transferring to GPU
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;

	// Create and allocate memory to staging buffer
	createBuffer(physicalDevice, logicalDevice, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				&stagingBuffer, &stagingBufferMemory);

	// Map memory to vertex buffer
	void* data;																						// Create pointer to a point in normal memory
	vkMapMemory(logicalDevice, stagingBufferMemory, 0, bufferSize, 0, &data);						// Map the vertex buffer memory to the data pointer
	memcpy(data, vertices->data(), (size_t)bufferSize);												// Copy memory from vertices vector to the data pointer
	vkUnmapMemory(logicalDevice, stagingBufferMemory);												// Unmap the vertex buffer memory

	// Create buffer with TRANSFER_DST_BIT to mark as recipient of transfer data
	// Buffer memory is to be DEVICE_LOCAL_BIT designating use by the GPU
	createBuffer(physicalDevice, logicalDevice, bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				&vertexBuffer, &vertexBufferMemory);

	// Copy staging buffer to vertex buffer on GPU
	copyBuffer(logicalDevice, transferQueue, transferCommandPool, stagingBuffer, vertexBuffer, bufferSize);

	// Cleanup staging buffer
	vkDestroyBuffer(logicalDevice, stagingBuffer, nullptr);
	vkFreeMemory(logicalDevice, stagingBufferMemory, nullptr);


}

int Mesh::getVertexCount()
{
	return vertexCount;
}


VkBuffer Mesh::getVertexBuffer()
{
	return vertexBuffer;
}

void Mesh::destroyVertexBuffer()
{
	vkDestroyBuffer(logicalDevice, vertexBuffer, nullptr);
	vkFreeMemory(logicalDevice, vertexBufferMemory, nullptr);
}

Mesh::~Mesh() 
{

}