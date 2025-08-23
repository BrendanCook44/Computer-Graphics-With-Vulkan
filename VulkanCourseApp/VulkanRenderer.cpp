#include "VulkanRenderer.h"

VulkanRenderer::VulkanRenderer()
{

}

int VulkanRenderer::init(GLFWwindow* newWindow)
{
	window = newWindow;

	try
	{
		createInstance();
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
	appInfo.pApplicationName = "Vulkan App";					// Custom Application Name
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);		// Custom Application Version
	appInfo.pEngineName = "N/A";								// Custom Engine Name
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);			// Custom Engine Version
	appInfo.apiVersion = VK_API_VERSION_1_4;					// Vulkan API Version - Affects The Program


	// Creation information for a VKInstance (Vulkan Instance)
	VkInstanceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;

	// Create list to hold instance extensions
	std::vector<const char*> instanceExtensions = std::vector<const char*>();

	// Setup extensions the VKInstance will use
	uint32_t glfwExtensionCount = 0;							// GLFW may require multiple extensions
	const char** glfwExtensions;								// Extensions passed as array of cstrings, so need pointer (the array) to pointer (the cstring)

	// Get GLFW Extensions
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	// Add GLFW extensions to list of extensions
	for (size_t i = 0; i < glfwExtensionCount; i++)
	{
		instanceExtensions.push_back(glfwExtensions[i]);
	}

	createInfo.enabledExtensionCount = static_cast<uint32_t>(instanceExtensions.size());
	createInfo.ppEnabledExtensionNames = instanceExtensions.data();

	// TODO: Setup Validation Layers that Instance will use
	createInfo.enabledLayerCount = 0;
	createInfo.ppEnabledLayerNames = nullptr;

	// Create Instance
	VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);

	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create a Vulkan Instance");
	}

}

VulkanRenderer::~VulkanRenderer()
{

}