#define GLFW_INCLUDE_VULKAN
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define STB_IMAGE_IMPLEMENTATION

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

	float angle = 0.0f;
	float deltaTime = 0.0f;
	float lastTime = 0.0f;

	int helicopter = vulkanRenderer.createModel("Models/uh60.obj");

	// Loop until closed
	while (!glfwWindowShouldClose(mainWindow))
	{
		glfwPollEvents();

		float now = glfwGetTime();
		deltaTime = now - lastTime;
		lastTime = now;

		angle += 10.0f * deltaTime;
		if (angle > 360.0f) 
		{
			angle -= 360.0f;
		}

		glm::mat4 testMatrix = glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0.0f, 1.0f, 0.0f));
		testMatrix = glm::rotate(testMatrix, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		vulkanRenderer.updateModel(helicopter, testMatrix);

		vulkanRenderer.draw();
	}

	vulkanRenderer.cleanup();

	// Destroy GLFW Window and stop GLFW
	glfwDestroyWindow(mainWindow);
	glfwTerminate();

	return 0;
}