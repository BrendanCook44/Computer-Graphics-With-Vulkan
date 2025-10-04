#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <stdexcept>
#include <vector>
#include <set>
#include <algorithm>
#include <array>

#include "VulkanValidation.h"
#include "Mesh.h"
#include "Utilities.h"

class VulkanRenderer
{
public:

	VulkanRenderer();

	int init(GLFWwindow* newWindow);
	void updateModel(int modelId, glm::mat4 newModel);
	void draw();
	void cleanup();


	~VulkanRenderer();

private:
	GLFWwindow* window;
	int currentFrame = 0;

	// Scene Objects
	std::vector<Mesh> meshList;

	// Scene Settings
	struct ViewProjection
	{
		glm::mat4 projection;
		glm::mat4 view;
	} viewProjection;

	// Vulkan Components
	VkInstance instance;
	struct
	{
		VkPhysicalDevice physicalDevice;
		VkDevice logicalDevice;
	} mainDevice;

	VkQueue graphicsQueue;
	VkQueue presentationQueue;
	VkSurfaceKHR surface;
	VkSwapchainKHR swapchain;

	std::vector<SwapchainImage> swapchainImages;
	std::vector<VkFramebuffer> swapchainFramebuffers;
	std::vector<VkCommandBuffer> commandBuffers;

	// Depth Buffer
	VkImage depthBufferImage;
	VkFormat depthBufferFormat;
	VkDeviceMemory depthBufferImageMemory;
	VkImageView depthBufferImageView;

	// Descriptors
	VkDescriptorSetLayout descriptorSetLayout;
	VkPushConstantRange pushConstantRange;

	VkDescriptorPool descriptorPool;
	std::vector<VkDescriptorSet> descriptorSets;

	std::vector<VkBuffer> viewProjectionUniformBuffer;
	std::vector<VkDeviceMemory> viewProjectionUniformBufferMemory;

	// Pipeline
	VkPipeline graphicsPipeline;
	VkPipelineLayout pipelineLayout;
	VkRenderPass renderPass;

	// Pools
	VkCommandPool graphicsCommandPool;

	// Utility
	VkFormat swapchainImageFormat;
	VkExtent2D swapchainExtent;

	// Synchronization
	std::vector<VkSemaphore> imageAvailable;
	std::vector<VkSemaphore> renderFinished;
	std::vector<VkFence> drawFences;

	// Validation Layer Handler
	VulkanValidation vulkanValidation;

	// Create Functions
	void createInstance();
	void createLogicalDevice();
	void createSurface();
	void createSwapchain();
	void createRenderPass();
	void createDescriptorSetLayout();
	void createPushConstantRange();
	void createGraphicsPipeline();
	void createDepthBufferImage();
	void createFramebuffers();
	void createCommandPool();
	void createCommandBuffers();
	void createSynchronization();

	void createUniformBuffers();
	void createDescriptorPool();
	void createDescriptorSets();

	// Create Functions
	VkImage createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags useFlags, VkMemoryPropertyFlags propertyFlags, VkDeviceMemory* imageMemory);
	VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);
	VkShaderModule createShaderModule(const std::vector<char>& code);

	// Update Functions
	void updateUniformBuffers(uint32_t imageIndex);

	// Record Functions
	void recordCommands(uint32_t currentImage);

	// Get Functions
	void getPhysicalDevice();
	QueueFamilyIndices getQueueFamilies(VkPhysicalDevice device);
	SwapchainDetails getSwapchainDetails(VkPhysicalDevice device);

	// Allocate Functions

	// Checker Functions
	bool checkInstanceExtensionSupport(std::vector<const char*>* checkExtensions);
	bool checkDeviceExtensionSupport(VkPhysicalDevice device);
	bool checkDeviceSuitable(VkPhysicalDevice device);

	// Choose Functions
	VkSurfaceFormatKHR chooseBestSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &formats);
	VkPresentModeKHR chooseBestPresentationMode(const std::vector<VkPresentModeKHR> presentationModes);
	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &surfaceCapabilities);
	VkFormat chooseSupportedFormat(const std::vector<VkFormat>& formats, VkImageTiling tiling, VkFormatFeatureFlags featureFlags);




};

