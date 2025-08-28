#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <stdexcept>
#include <vector>
#include <iostream>

#include "Utilities.h"
#include "VulkanValidation.h"

class VulkanRenderer
{
public:

	VulkanRenderer();

	int init(GLFWwindow* newWindow);
	void cleanup();


	~VulkanRenderer();

private:
	GLFWwindow* window;

	// Vulkan Components
	VkInstance instance;
	struct
	{
		VkPhysicalDevice physicalDevice;
		VkDevice logicalDevice;
	} mainDevice;

	VkQueue graphicsQueue;

	// Validation Layer Handler
	VulkanValidation vulkanValidation;


	// ----Vulkan Functions----

	// Create Functions
	void createInstance();
	void createLogicalDevice();

	// Get Functions
	void getPhysicalDevice();

	QueueFamilyIndices getQueueFamilies(VkPhysicalDevice device);

	// Support Functions
	bool checkInstanceExtensionSupport(std::vector<const char*>* checkExtensions);
	bool checkDeviceSuitable(VkPhysicalDevice device);

	// Checker Functions

};

