#pragma once

#include <fstream>
#include <glm/glm.hpp>

const int MAX_FRAME_DRAWS = 3;

const std::vector<const char*> deviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

struct Vertex
{
	glm::vec3 position;										// Vertex Position (x, y, z)

};

// Indices (locations) of Queue Families (if they exist)
struct QueueFamilyIndices
{
	int graphicsFamily = -1;								// Location of Graphics Queue Family
	int presentationFamily = -1;							// Location of Presentation Queue Family


	// Check if queue families are valid
	bool isValid()
	{
		return graphicsFamily >= 0 && presentationFamily >= 0;
	}
};

struct SwapchainDetails 
{
	VkSurfaceCapabilitiesKHR surfaceCapabilities;			// Surface Properties
	std::vector<VkSurfaceFormatKHR> formats;				// Surface image formats
	std::vector<VkPresentModeKHR> presentationModes;		// How images should be presented to screen
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