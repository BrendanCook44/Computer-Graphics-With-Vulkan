#include "Mesh.h"

Mesh::Mesh()
{

}

Mesh::Mesh(VkPhysicalDevice newPhysicalDevice, VkDevice newLogicalDevice, VkQueue transferQueue, VkCommandPool transferCommandPool, std::vector<Vertex>* vertices, std::vector<uint32_t>* indices, int newTextureID)
{
	physicalDevice = newPhysicalDevice;
	logicalDevice = newLogicalDevice;
	vertexCount = vertices->size();
	indexCount = indices->size();
	model.model = glm::mat4(1.0f);
	textureID = newTextureID;

	createVertexBuffer(transferQueue, transferCommandPool, vertices);
	createIndexBuffer(transferQueue, transferCommandPool, indices);

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

void Mesh::createIndexBuffer(VkQueue transferQueue, VkCommandPool transferCommandPool, std::vector<uint32_t>* indices)
{
	VkDeviceSize bufferSize = sizeof(uint32_t) * indices->size();

	// Temporary buffer to stage index data before transferring to GPU
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;

	// Create and allocate memory to staging buffer
	createBuffer(physicalDevice, logicalDevice, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		&stagingBuffer, &stagingBufferMemory);

	// Map memory to index buffer
	void* data;																						// Create pointer to a point in normal memory
	vkMapMemory(logicalDevice, stagingBufferMemory, 0, bufferSize, 0, &data);						// Map the index buffer memory to the data pointer
	memcpy(data, indices->data(), (size_t)bufferSize);												// Copy memory from indices vector to the data pointer
	vkUnmapMemory(logicalDevice, stagingBufferMemory);												// Unmap the index buffer memory

	// Create buffer with TRANSFER_DST_BIT to mark as recipient of transfer data
	// Buffer memory is to be DEVICE_LOCAL_BIT designating use by the GPU
	createBuffer(physicalDevice, logicalDevice, bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				&indexBuffer, &indexBufferMemory);

	// Copy staging buffer to index buffer on GPU
	copyBuffer(logicalDevice, transferQueue, transferCommandPool, stagingBuffer, indexBuffer, bufferSize);

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

int Mesh::getIndexCount()
{
	return indexCount;
}

VkBuffer Mesh::getIndexBuffer()
{
	return indexBuffer;
}

void Mesh::destroyIndexBuffer()
{
	vkDestroyBuffer(logicalDevice, indexBuffer, nullptr);
	vkFreeMemory(logicalDevice, indexBufferMemory, nullptr);
}

void Mesh::setModel(glm::mat4 newModel)
{
	model.model = newModel;
}

ModelTransformationMatrix Mesh::getModel()
{
	return model;
}

void Mesh::setTextureID(int newTextureID)
{
	textureID = newTextureID;
}

int Mesh::getTextureID()
{
	return textureID;
}

Mesh::~Mesh() 
{

}