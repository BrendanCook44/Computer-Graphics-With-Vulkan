#include "VulkanRenderer.h"
#include "CommonValues.h"
#include <iostream>

VulkanRenderer::VulkanRenderer()
{
	// Initialize Vulkan handles to null
	instance = VK_NULL_HANDLE;
	mainDevice.physicalDevice = VK_NULL_HANDLE;
	mainDevice.logicalDevice = VK_NULL_HANDLE;
	surface = VK_NULL_HANDLE;
	graphicsQueue = VK_NULL_HANDLE;
	presentationQueue = VK_NULL_HANDLE;
}

int VulkanRenderer::init(GLFWwindow* newWindow)
{
	window = newWindow;

	try
	{
		createInstance();
		vulkanValidation.setupDebugMessenger(instance);
		createSurface();
		getPhysicalDevice();
		createLogicalDevice();
		createSwapchain();
	}
	catch (const std::runtime_error &e)
	{
		printf("ERROR: %s\n", e.what());
		return EXIT_FAILURE;
	}


	return 0;
}

void VulkanRenderer::createInstance()
{
	// Information about the application itself
	// Most data here doesn't affect the program and is for developer convenience
	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "Vulkan App";															// Custom Application Name
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);												// Custom Application Version
	appInfo.pEngineName = "N/A";																		// Custom Engine Name
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);													// Custom Engine Version
	appInfo.apiVersion = VK_API_VERSION_1_4;															// Vulkan API Version - Affects The Program


	// Creation information for a VKInstance (Vulkan Instance)
	VkInstanceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;

	// Create list to hold instance extensions
	std::vector<const char*> instanceExtensions = std::vector<const char*>();

	// Create list to hold instance validation layers
	std::vector<const char*> instanceValidationLayers = vulkanValidation.getValidationLayers();

	// Setup extensions the VKInstance will use
	uint32_t glfwExtensionCount = 0;																	// GLFW may require multiple extensions
	const char** glfwExtensions;																		// Extensions passed as array of cstrings, so need pointer (the array) to pointer (the cstring)

	// Get GLFW Extensions
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	// Add GLFW extensions to list of extensions
	for (size_t i = 0; i < glfwExtensionCount; i++)
	{
		instanceExtensions.push_back(glfwExtensions[i]);
	}

	// Add debug extension if validation layers are enabled
	if (vulkanValidation.isValidationLayersEnabled()) {
		instanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}

	// Check Instance Extensions Supported
	if (!checkInstanceExtensionSupport(&instanceExtensions))
	{
		throw std::runtime_error("VK Instance does not support required extensions!");
	}


	// Check Validation Layers Supported
	if (vulkanValidation.isValidationLayersEnabled() && !vulkanValidation.checkValidationLayerSupport(instanceValidationLayers))
	{
		throw std::runtime_error("Validation layers requested, but not available!");
	}

	createInfo.enabledExtensionCount = static_cast<uint32_t>(instanceExtensions.size());
	createInfo.ppEnabledExtensionNames = instanceExtensions.data();

	if (vulkanValidation.isValidationLayersEnabled())
	{
		createInfo.enabledLayerCount = static_cast<uint32_t>(instanceValidationLayers.size());
		createInfo.ppEnabledLayerNames = instanceValidationLayers.data();
	}
	else
	{
		createInfo.enabledLayerCount = 0;
		createInfo.ppEnabledLayerNames = nullptr;
	}

	// Create Instance
	VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);

	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create a Vulkan Instance");
	}

}

bool VulkanRenderer::checkInstanceExtensionSupport(std::vector<const char*>* checkExtensions)
{
	// Get the number of extensions
	uint32_t extensionCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

	// Create a list of VkExtensionProperties using extensionCount
	std::vector<VkExtensionProperties> extensions(extensionCount);
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

	// Check if given extensions are in list of available extensions
	for (const auto& checkExtension : *checkExtensions)
	{
		bool hasExtension = false;
		for (const auto& extension : extensions)
		{
			if (strcmp(checkExtension, extension.extensionName) == 0)
			{
				hasExtension = true;
				break;
			}
		}
		if (!hasExtension)
		{
			return false;
		}
	}

	return true;

}

bool VulkanRenderer::checkDeviceExtensionSupport(VkPhysicalDevice device)
{
	// Get the number of extensions
	uint32_t extensionCount = 0;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

	// Return false if device does not support extensions
	if (extensionCount == 0)
	{
		return false;
	}

	// Create a list of VkExtensionProperties using extensionCount
	std::vector<VkExtensionProperties> extensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, extensions.data());

	// Check if given extensions are in list of available extensions
	for (const auto& deviceExtension : deviceExtensions)
	{
		bool hasExtension = false;
		for (const auto& extension : extensions)
		{
			if (strcmp(deviceExtension, extension.extensionName) == 0)
			{
				hasExtension = true;
				break;
			}
		}
		if (!hasExtension)
		{
			return false;
		}
	}

	return true;
}

bool VulkanRenderer::checkDeviceSuitable(VkPhysicalDevice device)
{
	//// Information about the device itself (ID, name, type, vendor, etc)
	//VkPhysicalDeviceProperties deviceProperties;
	//vkGetPhysicalDeviceProperties(device, &deviceProperties);

	//// Information about what the device can do (geo shader, tess shader, wide lines, etc)
	//VkPhysicalDeviceFeatures deviceFeatures;
	//vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

	QueueFamilyIndices indices = getQueueFamilies(device);

	bool extensionsSupported = checkDeviceExtensionSupport(device);

	bool swapchainValid = false;

	if (extensionsSupported)
	{
		SwapchainDetails SwapchainDetails = getSwapchainDetails(device);

		swapchainValid = !SwapchainDetails.presentationModes.empty() && !SwapchainDetails.formats.empty();
	}

	return indices.isValid() && extensionsSupported && swapchainValid;
}

void VulkanRenderer::createSurface()
{
	// Create Surface (Platform Independent)
	VkResult result = glfwCreateWindowSurface(instance, window, nullptr, &surface);

	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create a surface!");
	}
}

void VulkanRenderer::createSwapchain()
{
	// Get swapchain details
	SwapchainDetails SwapchainDetails = getSwapchainDetails(mainDevice.physicalDevice);

	// Find optimal surface values for swap chain
	VkSurfaceFormatKHR surfaceFormat = chooseBestSurfaceFormat(SwapchainDetails.formats);
	VkPresentModeKHR presentationMode = chooseBestPresentationMode(SwapchainDetails.presentationModes);
	VkExtent2D extent = chooseSwapExtent(SwapchainDetails.surfaceCapabilities);

	// Calculate images in the swap chain
	// Add an extra image to allow for triple buffering
	uint32_t imageCount = SwapchainDetails.surfaceCapabilities.minImageCount + 1;

	// If ImageCount is higher than max, clamp down to max
	if (SwapchainDetails.surfaceCapabilities.maxImageCount > 0
		&& imageCount > SwapchainDetails.surfaceCapabilities.maxImageCount)
	{
		imageCount = SwapchainDetails.surfaceCapabilities.maxImageCount;
	}

	// Creation information for swap chain
	VkSwapchainCreateInfoKHR swapchainCreateInfo = {};
	swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchainCreateInfo.imageFormat = surfaceFormat.format;												// Swapchain format
	swapchainCreateInfo.imageColorSpace = surfaceFormat.colorSpace;										// Swapchain color space
	swapchainCreateInfo.presentMode = presentationMode;													// Swapchain presentation mode
	swapchainCreateInfo.imageExtent = extent;															// Swapchain image extents
	swapchainCreateInfo.surface = surface;																// Surface to create swapchain for
	swapchainCreateInfo.minImageCount = imageCount;														// Minimum images in swapchain
	swapchainCreateInfo.imageArrayLayers = 1;															// Number of layers for each image in chain
	swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;								// What attachment images will be used as
	swapchainCreateInfo.preTransform = SwapchainDetails.surfaceCapabilities.currentTransform;			// Transform to perform on swap chain images
	swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;								// How to handle blending images with external graphics --- Set to no additional blending
	swapchainCreateInfo.clipped = VK_TRUE;																// Clip parts of the images not in view (behind a window, offscreen, etc)

	// Get Queue Family Indices
	QueueFamilyIndices indices = getQueueFamilies(mainDevice.physicalDevice);

	// If Graphics and Presentation families are different, then swapchain must let images be shared between familiies
	if (indices.graphicsFamily != indices.presentationFamily)
	{
		uint32_t queueFamilyIndices[] = {
			(uint32_t)indices.graphicsFamily,
			(uint32_t)indices.presentationFamily
		};

		swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;								// Image share handling
		swapchainCreateInfo.queueFamilyIndexCount = 2;													// Number of queues to share images between
		swapchainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;									// Array of queues to share between
	}

	else
	{
		swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		swapchainCreateInfo.queueFamilyIndexCount = 0;
		swapchainCreateInfo.pQueueFamilyIndices = nullptr;
	}

	// If old swapchain has been destroyed and this one replaces it, then link old one to quickly hand over responsibilities
	swapchainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

	VkResult result = vkCreateSwapchainKHR(mainDevice.logicalDevice, &swapchainCreateInfo, nullptr, &swapchain);

	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create a swapchain!");
	}
}

void VulkanRenderer::getPhysicalDevice()
{
	// Enumerate Physical devices the vkInstance can access
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

	// If no devices available, then none support Vulkan
	if (deviceCount == 0)
	{
		throw std::runtime_error("No GPUs found that support Vulkan Instance!");
	}

	// Get list of Physical Devices
	std::vector<VkPhysicalDevice> deviceList(deviceCount);
	vkEnumeratePhysicalDevices(instance, &deviceCount, deviceList.data());

	// Check for device containing graphics queue family
	for (const auto& device : deviceList)
	{
		if (checkDeviceSuitable(device))
		{
			mainDevice.physicalDevice = device;
			break;
		}
	}

	// Check if we found a suitable physical device
	if (mainDevice.physicalDevice == VK_NULL_HANDLE)
	{
		throw std::runtime_error("Failed to find a suitable GPU!");
	}
}

void VulkanRenderer::createLogicalDevice()
{
	// Get the queue family inddices for the chosen Physical Device
	QueueFamilyIndices indices = getQueueFamilies(mainDevice.physicalDevice);

	// Vector for queue creation information
	// Set for family indices
	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<int> queueFamilyIndices = { indices.graphicsFamily, indices.presentationFamily };

	// Queues that the logical device needs to create
	for (int queueFamilyIndex : queueFamilyIndices)
	{
		VkDeviceQueueCreateInfo queueCreateInfo = {};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamilyIndex;											// The index of the family to create a queue from
		queueCreateInfo.queueCount = 1;																	// Number of queues to create
		float priority = 1.0f;
		queueCreateInfo.pQueuePriorities = &priority;													// Handle multiple queue priority (1 = highest)

		queueCreateInfos.push_back(queueCreateInfo);
	}

	// Information to create logical device (also just referred to as "device")
	VkDeviceCreateInfo deviceCreateInfo = {};
	deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());				// Number of Queue Create Infos
	deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();										// List of queue create infos to create required queues
	deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());			// Number of enabled logical device extensions
	deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();									// List of enabled logical device extensions

	// Physical Device Features the Logical Device will be using
	VkPhysicalDeviceFeatures deviceFeatures = {};

	deviceCreateInfo.pEnabledFeatures = &deviceFeatures;												// Physical Device features that the Logical Device will use

	// Create the logical device for the given physical device
	VkResult result = vkCreateDevice(mainDevice.physicalDevice, &deviceCreateInfo, nullptr, &mainDevice.logicalDevice);

	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create a logical device!");
	}

	// Queues are created at the same time as the device
	// From given logical device, of given Queue Family, of given Queue Index, place the reference to the given VkQueue in a variable
	vkGetDeviceQueue(mainDevice.logicalDevice, indices.graphicsFamily, 0, &graphicsQueue);
	vkGetDeviceQueue(mainDevice.logicalDevice, indices.presentationFamily, 0, &presentationQueue);

}

QueueFamilyIndices VulkanRenderer::getQueueFamilies(VkPhysicalDevice device)
{
	QueueFamilyIndices indices;

	// Get Queue Family Property Info for Device
	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilyList(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilyList.data());

	// Go through each queue family to check if it has 1 of the required queue types
	int i = 0;
	for (const auto &queueFamily : queueFamilyList)
	{
		// First check if queue family has at least 1 queue in that family (could have no queues)
		// Queue can be multiple types defined through bitfield. Bitwise AND with VK_QUEUE_*_BIT to check if contains required queue family.
		if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
		{
			indices.graphicsFamily = i;				// If queue family is valid, then get index
		}

		// Check if Queue Family supports presentation
		VkBool32 presentationSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentationSupport);

		// Check if queue is presentation type (can be both graphics and presentation)
		if (queueFamily.queueCount > 0 && presentationSupport)
		{
			indices.presentationFamily = i;
		}

		// Stop searching for validity after finding required queue family.
		if (indices.isValid())
		{
			break;
		}

		i++;
	}
	return indices;
}

SwapchainDetails VulkanRenderer::getSwapchainDetails(VkPhysicalDevice device)
{
	SwapchainDetails SwapchainDetails;

	// Get List of Surface Capabilities
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &SwapchainDetails.surfaceCapabilities);

	// Get List of Formats
	uint32_t formatCount = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

	if (formatCount != 0)
	{
		SwapchainDetails.formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, SwapchainDetails.formats.data());
	}

	// Get List of Presentation Modes
	uint32_t presentationCount = 0;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentationCount, nullptr);

	if (presentationCount != 0)
	{
		SwapchainDetails.presentationModes.resize(presentationCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentationCount, SwapchainDetails.presentationModes.data());
	}

	return SwapchainDetails;
}

VkSurfaceFormatKHR VulkanRenderer::chooseBestSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats)
{
	// Format		:	VK_FORMAT_R8G8B8A8_UNORM
	// Color Space	:	VK_COLOR_SPACE_SRGB_NONLINEAR_KHR

	// All Formats Available
	if (formats.size() == 1 && formats[0].format == VK_FORMAT_UNDEFINED)
	{
		return { VK_FORMAT_R8G8B8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
	}

	for (const auto& format : formats)
	{
		if ((format.format == VK_FORMAT_R8G8B8A8_UNORM || format.format == VK_FORMAT_B8G8R8A8_UNORM) && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
		{
			return format;
		}
	}

	return formats[0];
}

VkPresentModeKHR VulkanRenderer::chooseBestPresentationMode(const std::vector<VkPresentModeKHR> presentationModes)
{
	for (const auto& presentationMode : presentationModes)
	{
		if (presentationMode == VK_PRESENT_MODE_MAILBOX_KHR)
		{
			return presentationMode;
		}
	}

	// Use Default Vulkan Presentation Mode
	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D VulkanRenderer::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& surfaceCapabilities)
{
	// If current extent is at numeric limits, then extent can vary. Otherwise, it is the size of the window
	if (surfaceCapabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
	{
		return surfaceCapabilities.currentExtent;
	}

	else
	{
		// If value can vary, need to set manually

		int width;
		int height;
		glfwGetFramebufferSize(window, &width, &height);

		// Create new extent using window size
		VkExtent2D newExtent = {};
		newExtent.width = static_cast<uint32_t>(width);
		newExtent.height = static_cast<uint32_t>(height);

		// Surface also defines max and min, make sure within boundaries by clamping value
		newExtent.width = std::max(surfaceCapabilities.minImageExtent.width, std::min(surfaceCapabilities.maxImageExtent.width, newExtent.width));
		newExtent.height = std::max(surfaceCapabilities.minImageExtent.height, std::min(surfaceCapabilities.maxImageExtent.height, newExtent.height));

		return newExtent;
	}
}

void VulkanRenderer::cleanup()
{
	vkDestroySwapchainKHR(mainDevice.logicalDevice, swapchain, nullptr);

	if (surface != VK_NULL_HANDLE)
	{
		vkDestroySurfaceKHR(instance, surface, nullptr);
	}
	
	if (mainDevice.logicalDevice != VK_NULL_HANDLE)
	{
		vkDestroyDevice(mainDevice.logicalDevice, nullptr);
	}
	
	vulkanValidation.cleanup(instance);
	
	if (instance != VK_NULL_HANDLE)
	{
		vkDestroyInstance(instance, nullptr);
	}
}

VulkanRenderer::~VulkanRenderer()
{

}