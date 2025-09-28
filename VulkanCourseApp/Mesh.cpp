#include "Mesh.h"

Mesh::Mesh()
{

}

Mesh::Mesh(VkPhysicalDevice newPhysicalDevice, VkDevice newLogicalDevice, std::vector<Vertex>* vertices)
{
	physicalDevice = newPhysicalDevice;
	logicalDevice = newLogicalDevice;
	vertexCount = vertices->size();
	createVertexBuffer(vertices);
}

void Mesh::createVertexBuffer(std::vector<Vertex>* vertices)
{
	// Get size of buffer needed for vertices
	VkDeviceSize bufferSize = sizeof(Vertex) * vertices->size();

	// Create and allocate memory to buffer
	createBuffer(physicalDevice, logicalDevice, bufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				&vertexBuffer, &vertexBufferMemory);

	// Map memory to vertex buffer
	void* data;																						// Create pointer to a point in normal memory
	vkMapMemory(logicalDevice, vertexBufferMemory, 0, bufferSize, 0, &data);						// Map the vertex buffer memory to the data pointer
	memcpy(data, vertices->data(), (size_t)bufferSize);												// Copy memory from vertices vector to the data pointer
	vkUnmapMemory(logicalDevice, vertexBufferMemory);												// Unmap the vertex buffer memory

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