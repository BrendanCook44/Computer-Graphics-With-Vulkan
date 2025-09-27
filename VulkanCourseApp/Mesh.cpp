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
	// Information to create a buffer
	VkBufferCreateInfo bufferCreateInfo = {};
	bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferCreateInfo.size = sizeof(Vertex) * vertices->size();										// Size of buffer (size of 1 vertex * number of vertices)
	bufferCreateInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;										// Choose vertex buffer
	bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;										// Choose sharing mode

	VkResult result = vkCreateBuffer(logicalDevice, &bufferCreateInfo, nullptr, &vertexBuffer);

	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create Vertex Buffer!");
	}

	// Get buffer memory requirements
	VkMemoryRequirements memoryRequirements;
	vkGetBufferMemoryRequirements(logicalDevice, vertexBuffer, &memoryRequirements);

	// Allocate memory to buffer
	VkMemoryAllocateInfo memoryAllocateInfo = {};
	memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memoryAllocateInfo.allocationSize = memoryRequirements.size;
	memoryAllocateInfo.memoryTypeIndex = findMemoryTypeIndex(memoryRequirements.memoryTypeBits,		// Index of memory type on Physical Device that has required bit flags
															VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |	// VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT: CPU can interact with memory
															VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);	// VK_MEMORY_PROPERTY_HOST_COHERENT_BIT: Allows pacement of data straight into buffer after mapping

	// Allocate memory to VkDeviceMemory
	result = vkAllocateMemory(logicalDevice, &memoryAllocateInfo, nullptr, &vertexBufferMemory);

	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to allocate Vertex Buffer Memory!");
	}

	// Allocate memory to given vertex buffer
	vkBindBufferMemory(logicalDevice, vertexBuffer, vertexBufferMemory, 0);

	// Map memory to vertex buffer
	void* data;																						// Create pointer to a point in normal memory
	vkMapMemory(logicalDevice, vertexBufferMemory, 0, bufferCreateInfo.size, 0, &data);				// Map the vertex buffer memory to the data pointer
	memcpy(data, vertices->data(), (size_t)bufferCreateInfo.size);									// Copy memory from vertices vector to the data pointer
	vkUnmapMemory(logicalDevice, vertexBufferMemory);												// Unmap the vertex buffer memory

}

uint32_t Mesh::findMemoryTypeIndex(uint32_t allowedTypes, VkMemoryPropertyFlags properties)
{
	// Get properties of physical device memory
	VkPhysicalDeviceMemoryProperties memoryProperties;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);

	for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++)
	{
		if ((allowedTypes & (1 << i))																// Index of memory type must match corresponding bit in allowedTypes
			&& (memoryProperties.memoryTypes[i].propertyFlags & properties) == properties)			// Desired property bit flags are part of memory type's property flags
		{
			// Return the index of the valid memory type
			return i;
		}
	}
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