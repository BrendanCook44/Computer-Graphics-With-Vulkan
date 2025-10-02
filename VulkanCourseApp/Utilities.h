#pragma once

#include <fstream>
#include <glm/glm.hpp>

const int MAX_FRAME_DRAWS = 3;
const int MAX_OBJECTS = 2;

const std::vector<const char*> deviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

struct Vertex
{
	glm::vec3 position;																												// Vertex Position (XYZ)
	glm::vec3 color;																												// Vertex Color (RGB)

};

// Indices (locations) of Queue Families (if they exist)
struct QueueFamilyIndices
{
	int graphicsFamily = -1;																										// Location of Graphics Queue Family
	int presentationFamily = -1;																									// Location of Presentation Queue Family


	// Check if queue families are valid
	bool isValid()
	{
		return graphicsFamily >= 0 && presentationFamily >= 0;
	}
};

struct SwapchainDetails 
{
	VkSurfaceCapabilitiesKHR surfaceCapabilities;																					// Surface Properties
	std::vector<VkSurfaceFormatKHR> formats;																						// Surface image formats
	std::vector<VkPresentModeKHR> presentationModes;																				// How images should be presented to screen
};

struct SwapchainImage 
{
	VkImage image;
	VkImageView imageView;
};

static std::vector<char> readFile(const std::string& filename)
{
	// Open stream from given file
	// Flag: std::ios::binary tells stream to read file as binary format
	// Flag: std::ios::ate tells stream to start reading from end of file
	std::ifstream file(filename, std::ios::binary | std::ios::ate);

	// Check if file stream successfully opened
	if (!file.is_open())
	{
		throw std::runtime_error("Failed to open file!");
	}

	// Get current read position and use to resize file buffer
	size_t fileSize = (size_t)file.tellg();
	std::vector<char> fileBuffer(fileSize);

	// Go to position 0 of the file
	file.seekg(0);

	// Read the file data into the buffer (stream "fileSize" in total)
	file.read(fileBuffer.data(), fileSize);

	// Close stream
	file.close();

	return fileBuffer;
}

static uint32_t findMemoryTypeIndex(VkPhysicalDevice physicalDevice, uint32_t allowedTypes, VkMemoryPropertyFlags properties)
{
	// Get properties of physical device memory
	VkPhysicalDeviceMemoryProperties memoryProperties;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);

	for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++)
	{
		if ((allowedTypes & (1 << i))																								// Index of memory type must match corresponding bit in allowedTypes
			&& (memoryProperties.memoryTypes[i].propertyFlags & properties) == properties)											// Desired property bit flags are part of memory type's property flags
		{
			// Return the index of the valid memory type
			return i;
		}
	}
}

static void createBuffer(VkPhysicalDevice physicalDevice, VkDevice logicalDevice, VkDeviceSize bufferSize, VkBufferUsageFlags bufferUsage, VkMemoryPropertyFlags bufferProperties, VkBuffer* buffer, VkDeviceMemory* bufferMemory)
{
	// Information to create a buffer
	VkBufferCreateInfo bufferCreateInfo = {};
	bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferCreateInfo.size = bufferSize;																								// Size of buffer (size of 1 vertex * number of vertices)
	bufferCreateInfo.usage = bufferUsage;																							// Buffer Usage Flags
	bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;																		// Choose sharing mode

	VkResult result = vkCreateBuffer(logicalDevice, &bufferCreateInfo, nullptr, buffer);

	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create Vertex Buffer!");
	}

	// Get buffer memory requirements
	VkMemoryRequirements memoryRequirements;
	vkGetBufferMemoryRequirements(logicalDevice, *buffer, &memoryRequirements);

	// Allocate memory to buffer
	VkMemoryAllocateInfo memoryAllocateInfo = {};
	memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memoryAllocateInfo.allocationSize = memoryRequirements.size;
	memoryAllocateInfo.memoryTypeIndex = findMemoryTypeIndex(physicalDevice, memoryRequirements.memoryTypeBits, bufferProperties);	// Index of memory type on Physical Device that has required bit flags);																		// VK_MEMORY_PROPERTY_HOST_COHERENT_BIT: Allows pacement of data straight into buffer after mapping

	// Allocate memory to VkDeviceMemory
	result = vkAllocateMemory(logicalDevice, &memoryAllocateInfo, nullptr, bufferMemory);

	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to allocate Vertex Buffer Memory!");
	}

	// Allocate memory to given vertex buffer
	vkBindBufferMemory(logicalDevice, *buffer, *bufferMemory, 0);
}

static void copyBuffer(VkDevice logicalDevice, VkQueue transferQueue, VkCommandPool transferCommandPool, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize bufferSize)
{
	// Command buffer to hold transfer commands
	VkCommandBuffer transferCommandBuffer;

	// Command buffer details
	VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
	commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	commandBufferAllocateInfo.commandPool = transferCommandPool;
	commandBufferAllocateInfo.commandBufferCount = 1;

	// Allocate command buffer from pool
	vkAllocateCommandBuffers(logicalDevice, &commandBufferAllocateInfo, &transferCommandBuffer);

	// Information to begin command buffer record
	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;																	// Only using the command buffer once

	// Begin recording transfer commands
	vkBeginCommandBuffer(transferCommandBuffer, &beginInfo);

	// Region of data for use during copy
	VkBufferCopy bufferCopyRegion = {};
	bufferCopyRegion.srcOffset = 0;
	bufferCopyRegion.dstOffset = 0;
	bufferCopyRegion.size = bufferSize;

	// Command to copy source buffer to destination buffer
	vkCmdCopyBuffer(transferCommandBuffer, srcBuffer, dstBuffer, 1, &bufferCopyRegion);

	// End commands
	vkEndCommandBuffer(transferCommandBuffer);

	// Queue submission information
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &transferCommandBuffer;

	// Submit transfer commands to transfer queue
	vkQueueSubmit(transferQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(transferQueue);

	// Free temporary command buffer back to pool
	vkFreeCommandBuffers(logicalDevice, transferCommandPool, 1, &transferCommandBuffer);
}