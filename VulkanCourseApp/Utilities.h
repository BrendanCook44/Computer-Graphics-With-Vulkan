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

static VkCommandBuffer beginCommandBuffer(VkDevice logicalDevice, VkCommandPool commandPool)
{
	// Command buffer to hold transfer commands
	VkCommandBuffer commandBuffer;

	// Command buffer details
	VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
	commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	commandBufferAllocateInfo.commandPool = commandPool;
	commandBufferAllocateInfo.commandBufferCount = 1;

	// Allocate command buffer from pool
	vkAllocateCommandBuffers(logicalDevice, &commandBufferAllocateInfo, &commandBuffer);

	// Information to begin command buffer record
	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;																	// Only using the command buffer once

	// Begin recording transfer commands
	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	return commandBuffer;
}

static void endCommandBuffer(VkDevice logicalDevice, VkCommandPool commandPool, VkQueue queue, VkCommandBuffer commandBuffer)
{
	// End commands
	vkEndCommandBuffer(commandBuffer);

	// Queue submission information
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	// Submit transfer commands to transfer queue
	vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(queue);

	// Free temporary command buffer back to pool
	vkFreeCommandBuffers(logicalDevice, commandPool, 1, &commandBuffer);
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
	// Create buffer
	VkCommandBuffer transferCommandBuffer = beginCommandBuffer(logicalDevice, transferCommandPool);

	// Region of data for use during copy
	VkBufferCopy bufferCopyRegion = {};
	bufferCopyRegion.srcOffset = 0;
	bufferCopyRegion.dstOffset = 0;
	bufferCopyRegion.size = bufferSize;

	// Command to copy source buffer to destination buffer
	vkCmdCopyBuffer(transferCommandBuffer, srcBuffer, dstBuffer, 1, &bufferCopyRegion);

	// End and submit the command buffer
	endCommandBuffer(logicalDevice, transferCommandPool, transferQueue, transferCommandBuffer);

	
}

static void copyImageBuffer(VkDevice logicalDevice, VkQueue transferQueue, VkCommandPool transferCommandPool, VkBuffer srcBuffer, VkImage image, uint32_t width, uint32_t height)
{
	// Create buffer
	VkCommandBuffer transferCommandBuffer = beginCommandBuffer(logicalDevice, transferCommandPool);

	VkBufferImageCopy imageRegion = {};
	imageRegion.bufferOffset = 0;																									// Offset into data
	imageRegion.bufferRowLength = 0;																								// Row length of data to calculate data spacing
	imageRegion.bufferImageHeight = 0;																								// Image height to calculate data spacing
	imageRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;															// Which aspect of image to copy
	imageRegion.imageSubresource.mipLevel = 0;																						// Which mipmap texture levels to copy across
	imageRegion.imageSubresource.baseArrayLayer = 0;																				// Starting array layer (if array)
	imageRegion.imageSubresource.layerCount = 1;																					// Number of layers to copy starting at baseArrayLayer
	imageRegion.imageOffset = { 0, 0, 0 };																							// Offset into image (as opposed to raw data in bufferOffset)
	imageRegion.imageExtent = { width, height, 1 };																					// Size of region to copy as (x,y,z) values

	// Copy buffer to given image
	vkCmdCopyBufferToImage(transferCommandBuffer, srcBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageRegion);

	// End and submit the command buffer
	endCommandBuffer(logicalDevice, transferCommandPool, transferQueue, transferCommandBuffer);

}

static void transitionImageLayout(VkDevice logicalDevice, VkQueue queue, VkCommandPool commandPool, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout)
{
	// Create buffer
	VkCommandBuffer commandBuffer = beginCommandBuffer(logicalDevice, commandPool);

	// Create image memory pipeline barrier
	VkImageMemoryBarrier imageMemoryBarrier = {};
	imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	imageMemoryBarrier.image = image;																								// Image being accessed and modified
	imageMemoryBarrier.oldLayout = oldLayout;																						// Layout to transition from
	imageMemoryBarrier.newLayout = newLayout;																						// Layout to transition to
	imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;																// Queue family to transition from
	imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;																// Queue family to transition to
	imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;														// Aspect of image being altered
	imageMemoryBarrier.subresourceRange.baseMipLevel = 0;																			// First mip level to start alterations on
	imageMemoryBarrier.subresourceRange.levelCount = 1;																				// Number of mipmap levels to alter starting from base mip level
	imageMemoryBarrier.subresourceRange.baseArrayLayer = 0;																			// First layer to start alterations on
	imageMemoryBarrier.subresourceRange.layerCount = 1;																				// Number of layers to alter starting from base array layer
	
	VkPipelineStageFlags srcStage;
	VkPipelineStageFlags dstStage;

	// If transitioning from new image to image ready to receive data:
	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
	{
		srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;

		imageMemoryBarrier.srcAccessMask = 0;																						// Memory access stage transition must happen after:
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;															// Memory access stage transition must happen before:
	}

	// If transitioning from transfer destination to shader readable:
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
		srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

		imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	}

	vkCmdPipelineBarrier(commandBuffer,
						 srcStage, dstStage,																						// Pipeline stages (match to src and dst AccessMasks)
						 0,																											// Dependency flags
						 0, nullptr,																								// Memory barrier count + data
						 0, nullptr,																								// Buffer memory barrier count + data
						 1, &imageMemoryBarrier																						// Image memory barrier count + data
	);

	// End and submit the command buffer
	endCommandBuffer(logicalDevice, commandPool, queue, commandBuffer);
}