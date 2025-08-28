#define GLFW_INCLUDE_VULKAN
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/mat4x4.hpp>

#include <iostream>

#include "VulkanRenderer.h"

#include "CommonValues.h"

GLFWwindow* mainWindow;
VulkanRenderer vulkanRenderer;

void initWindow(std::string windowName = "Test Window")
{
	// Initialize GLFW
	glfwInit();

	// Set GLFW to NOT work with OpenGL
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	mainWindow = glfwCreateWindow(WIDTH, HEIGHT, windowName.c_str(), nullptr, nullptr);
}

int main()
{
	// Create Window
	initWindow("Test Window");

	// Create Vulkan Renderer Instance
	if (vulkanRenderer.init(mainWindow) == EXIT_FAILURE)
	{
		return EXIT_FAILURE;
	}

	// Loop until closed
	while (!glfwWindowShouldClose(mainWindow))
	{
		glfwPollEvents();
	}

	vulkanRenderer.cleanup();

	// Destroy GLFW Window and stop GLFW
	glfwDestroyWindow(mainWindow);
	glfwTerminate();

	return 0;
}