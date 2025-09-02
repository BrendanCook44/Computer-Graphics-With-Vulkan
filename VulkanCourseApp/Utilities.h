#pragma once

const std::vector<const char*> deviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

// Indices (locations) of Queue Families (if they exist)
struct QueueFamilyIndices
{
	int graphicsFamily = -1;			// Location of Graphics Queue Family
	int presentationFamily = -1;		// Location of Presentation Queue Family


	// Check if queue families are valid
	bool isValid()
	{
		return graphicsFamily >= 0 && presentationFamily >= 0;
	}
};

struct SwapchainDetails {
	VkSurfaceCapabilitiesKHR surfaceCapabilities;			// Surface Properties
	std::vector<VkSurfaceFormatKHR> formats;				// Surface image formats
	std::vector<VkPresentModeKHR> presentationModes;		// How images should be presented to screen
};

struct SwapchainImage {
	VkImage image;
	VkImageView imageView;
};