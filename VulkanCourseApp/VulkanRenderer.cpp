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

		// Create a mesh
		std::vector<Vertex> meshVertices = {
			{{0.0, -0.4, 0.0}},
			{{0.4, 0.4, 0.0}},
			{{-0.4, 0.4, 0.0}}
		};

		mesh1 = Mesh(mainDevice.physicalDevice, mainDevice.logicalDevice, &meshVertices);

		createSwapchain();
		createRenderPass();
		createGraphicsPipeline();
		createFramebuffers();
		createCommandPool();
		createCommandBuffers();
		recordCommands();
		createSynchronization();
	}
	catch (const std::runtime_error &e)
	{
		printf("ERROR: %s\n", e.what());
		return EXIT_FAILURE;
	}


	return 0;
}

void VulkanRenderer::draw()
{
	// 1.) Get next available image to draw to and set something to signal when finished with image (semaphore)
	// 2.) Submit command buffer to queue for execution. Ensure it waits for the image to be signaled as available before drawing and signals when it has finished rendering
	// 3.) Present image to screen when it has signalled as finished recording

	// Wait for given fence to signal (open) from last draw before continuing
	vkWaitForFences(mainDevice.logicalDevice, 1, &drawFences[currentFrame], VK_TRUE, std::numeric_limits<uint64_t>::max());

	// Reset fence (close) fences
	vkResetFences(mainDevice.logicalDevice, 1, &drawFences[currentFrame]);

	// Get index of next image & signal semaphore when ready to be drawn to
	uint32_t imageIndex;
	vkAcquireNextImageKHR(mainDevice.logicalDevice, swapchain, std::numeric_limits<uint64_t>::max(), imageAvailable[currentFrame], VK_NULL_HANDLE, &imageIndex);

	// Submit command buffer to render
	// Queue submission information
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.waitSemaphoreCount = 1;																	// Number of semaphores to wait on
	submitInfo.pWaitSemaphores = &imageAvailable[currentFrame];											// List of semaphores to wait on

	VkPipelineStageFlags waitStages[] = {
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
	};

	submitInfo.pWaitDstStageMask = waitStages;															// Stages to check semaphores at
	submitInfo.commandBufferCount = 1;																	// Number of command buffers to submit
	submitInfo.pCommandBuffers = &commandBuffers[imageIndex];											// Command buffer to submit
	submitInfo.signalSemaphoreCount = 1;																// Number of semaphores to signal
	submitInfo.pSignalSemaphores = &renderFinished[currentFrame];										// Semaphores to signal when command buffer finishes

	// Submit command buffer to queue
	VkResult result = vkQueueSubmit(graphicsQueue, 1, &submitInfo, drawFences[currentFrame]);

	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to submit Command Buffer to Queue!");
	}

	// Present rendered image to screen
	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;																	// Number of semaphores to wait on
	presentInfo.pWaitSemaphores = &renderFinished[currentFrame];										// Semaphores to wait on
	presentInfo.swapchainCount = 1;																		// Number of swapchains to present to
	presentInfo.pSwapchains = &swapchain;																// Swapchains to present images to
	presentInfo.pImageIndices = &imageIndex;															// Index of images in swapchains to present

	// Present image
	result = vkQueuePresentKHR(presentationQueue, &presentInfo);

	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to present Image!");
	}

	// Get next frame
	currentFrame = (currentFrame + 1) % MAX_FRAME_DRAWS;

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

	// Create swapchain
	VkResult result = vkCreateSwapchainKHR(mainDevice.logicalDevice, &swapchainCreateInfo, nullptr, &swapchain);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create a swapchain!");
	}

	swapchainImageFormat = surfaceFormat.format;
	swapchainExtent = extent;

	// Get swapchain images
	uint32_t swapchainImageCount;
	vkGetSwapchainImagesKHR(mainDevice.logicalDevice, swapchain, &swapchainImageCount, nullptr);
	std::vector<VkImage> images(swapchainImageCount);
	vkGetSwapchainImagesKHR(mainDevice.logicalDevice, swapchain, &swapchainImageCount, images.data());

	for (VkImage image : images)
	{
		// Store image handle
		SwapchainImage swapchainImage = {};
		swapchainImage.image = image;

		// Create image view and add to swapchain image list
		swapchainImage.imageView = createImageView(image, swapchainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);
		swapchainImages.push_back(swapchainImage);
	}
}

void VulkanRenderer::createRenderPass()
{
	// Color attachment of render pass
	VkAttachmentDescription colorAttachment = {};
	colorAttachment.format = swapchainImageFormat;														// Format to use for attachment
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;													// Number of samples to write for multisampling
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;												// Describes what to do with attachment before rendering
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;												// Describes what to do with attachment after rendering
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;									// Describes what to do with stencil before rendering
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;									// Describes what to do with stencil after rendering

	// Framebuffer data will be stored as an image, but images can be given different data layouts to give optimal use for certain operations
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;											// Image data layout before render pass starts
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;										// Image data layout after render pass (to change to)

	// Attachment reference uses an attachment index that refers to index in the attachment list passed to renderPassCreateInfo
	VkAttachmentReference colorAttachmentReference = {};
	colorAttachmentReference.attachment = 0;
	colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	// Information about a particular subpass the render pass is using
	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;										// Pipeline type subpass is to be bound to
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentReference;

	// Need to determine when layout transitions occur using subpass dependencies
	std::array<VkSubpassDependency, 2> subpassDependencies;

	// Conversion from VK_IMAGE_LAYOUT_UNDEFINED to VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	// Transition must happen after...
	subpassDependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;											// Subpass index (VK_SUBPASS_EXTERNAL = Special value meaning outside of renderpass)
	subpassDependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;							// Pipeline stage
	subpassDependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;									// Stage access mask (memory access)

	// But must happen before...
	subpassDependencies[0].dstSubpass = 0;
	subpassDependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpassDependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	subpassDependencies[0].dependencyFlags = 0;

	// Conversion from VK_IMAGE_LAYOUT_UNDEFINED to VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
	// Transition must happen after...
	subpassDependencies[1].srcSubpass = 0;																
	subpassDependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;				 
	subpassDependencies[1].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;

	// But must happen before...
	subpassDependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
	subpassDependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	subpassDependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	subpassDependencies[1].dependencyFlags = 0;

	// Create info for render pass
	VkRenderPassCreateInfo renderPassCreateInfo = {};
	renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassCreateInfo.attachmentCount = 1;
	renderPassCreateInfo.pAttachments = &colorAttachment;
	renderPassCreateInfo.subpassCount = 1;
	renderPassCreateInfo.pSubpasses = &subpass;
	renderPassCreateInfo.dependencyCount = static_cast<uint32_t>(subpassDependencies.size());
	renderPassCreateInfo.pDependencies = subpassDependencies.data();

	VkResult result = vkCreateRenderPass(mainDevice.logicalDevice, &renderPassCreateInfo, nullptr, &renderPass);

	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create a Render Pass!");
	}

}

void VulkanRenderer::createGraphicsPipeline()
{
	// Read in SPIR-V code of shaders
	auto vertexShaderCode = readFile("Shaders/vert.spv");
	auto fragmentShaderCode = readFile("Shaders/frag.spv");

	// Create Shader Modules to link to Graphics Pipeline
	VkShaderModule vertexShaderModule = createShaderModule(vertexShaderCode);
	VkShaderModule fragmentShaderModule = createShaderModule(fragmentShaderCode);

	// -- Shader Stage Creation Information --
	// Vertex
	VkPipelineShaderStageCreateInfo vertexShaderCreateInfo = {};
	vertexShaderCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertexShaderCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;											// Shader stage name
	vertexShaderCreateInfo.module = vertexShaderModule;													// Shader module to be used by stage
	vertexShaderCreateInfo.pName = "main";																// Entry point into shader

	// Fragment
	VkPipelineShaderStageCreateInfo fragmentShaderCreateInfo = {};
	fragmentShaderCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragmentShaderCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;										// Shader stage name
	fragmentShaderCreateInfo.module = fragmentShaderModule;												// Shader module to be used by stage
	fragmentShaderCreateInfo.pName = "main";															// Entry point into shader

	// Add vertex and fragment shader create structs into array for Graphics Pipeline to use
	VkPipelineShaderStageCreateInfo shaderStages[] = { vertexShaderCreateInfo, fragmentShaderCreateInfo };

	// How the data for a single vertex (position, color, texture coordinates, normals) is as a whole
	VkVertexInputBindingDescription bindingDescription = {};
	bindingDescription.binding = 0;																		// Can bind multiple streams of data
	bindingDescription.stride = sizeof(Vertex);															// Size of a single vertex object
	bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;											// How to move between data after each vertex
																										// VK_VERTEX_INPUT_RATE_VERTEX: Move on to the next vertex
																										// VK_VERTEX_INPUT_RATE_INSTANCE: Move to a vertex for the next instance

	// How the data for an attribute is defined within a vertex
	std::array<VkVertexInputAttributeDescription, 1> attributeDescriptions;

	// Position Attribute
	attributeDescriptions[0].binding = 0;																// Which binding the data is at (should be same as above)
	attributeDescriptions[0].location = 0;																// Which location the data is at (should be same as above)
	attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;										// Format the data will take (also helps define size of data)
	attributeDescriptions[0].offset = offsetof(Vertex, position);										// Where the attribute is defined in the data for a single vertex

	// Color Attribute

	// -- Pipeline Creation Information --
	// Vertex Input
	VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo = {};
	vertexInputCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputCreateInfo.vertexBindingDescriptionCount = 1;
	vertexInputCreateInfo.pVertexBindingDescriptions = &bindingDescription;								// List of Vertex Binding Descriptions (data spacing/stride information)
	vertexInputCreateInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
	vertexInputCreateInfo.pVertexAttributeDescriptions = attributeDescriptions.data();					// List of Vertex Attribute Descriptions (data format and binding parameters)

	// Input Assembly
	VkPipelineInputAssemblyStateCreateInfo inputAssemblyCreateInfo = {};
	inputAssemblyCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssemblyCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;								// Primitive type to assmeble vertices as
	inputAssemblyCreateInfo.primitiveRestartEnable = VK_FALSE;											// Allow overriding of "strip" topology to start new primitives

	// Viewport & Scissor
	VkViewport viewport = {};
	viewport.x = 0.0f;																					// X start coordinate
	viewport.y = 0.0f;																					// Y start coordinate
	viewport.width = (float)swapchainExtent.width;														// Width of viewport
	viewport.height = (float)swapchainExtent.height;													// Height of viewport
	viewport.minDepth = 0.0f;																			// Minimum framebuffer depth
	viewport.maxDepth = 1.0f;																			// Maximum framebuffer depth

	VkRect2D scissor = {};
	scissor.offset = { 0, 0 };																			// Offset to use region from
	scissor.extent = swapchainExtent;																	// Extent to describe region to use, starting at offset

	VkPipelineViewportStateCreateInfo viewportStateCreateInfo = {};
	viewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportStateCreateInfo.viewportCount = 1;
	viewportStateCreateInfo.pViewports = &viewport;
	viewportStateCreateInfo.scissorCount = 1;
	viewportStateCreateInfo.pScissors = &scissor;

	// -- Dynamic States --
	// std::vector<VkDynamicState> dynamicStateEnables;
	// dynamicStateEnables.push_back(VK_DYNAMIC_STATE_VIEWPORT);										// Dynamic Viewport: Can resize in command buffer with vkCmdSetViewport(commandBuffer, 0, 1, &viewport)
	// dynamicStateEnables.push_back(VK_DYNAMIC_STATE_SCISSOR);											// Dynamic Scissor: Can resize in command buffer with vkCmdSetScissor(commandBuffer, 0, 1, &scissor)

	// VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo = {};
	// dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	// dynamicStateCreateInfo.dynamicStateCount = static_cast<uint32_t>(dynamicStateEnables.size());
	// dynamicStateCreateInfo.pDynamicStates = dynamicStateEnables.data();

	// Rasterizer
	VkPipelineRasterizationStateCreateInfo rasterizerCreateInfo = {};
	rasterizerCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizerCreateInfo.depthClampEnable = VK_FALSE;													// Change if fragments beyond near/far planes are clipped (default) or clamped to plane
	rasterizerCreateInfo.rasterizerDiscardEnable = VK_FALSE;											// Whether to discard data and skip rasterizer. Never creates fragments, only suitable for pipeline without framebuffer output
	rasterizerCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;											// How to handle filling points between vertices
	rasterizerCreateInfo.lineWidth = 1.0f;																// How thick lines should be when drawn
	rasterizerCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;												// Which face of a triangle to cull
	rasterizerCreateInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;											// Winding to determing which side is front
	rasterizerCreateInfo.depthBiasEnable = VK_FALSE;													// Whether to add depth bias to fragments (good for stopping "shadow acne" in shadow mapping)

	// Multisampling
	VkPipelineMultisampleStateCreateInfo multisamplingCreateInfo = {};
	multisamplingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisamplingCreateInfo.sampleShadingEnable = VK_FALSE;												// Enable multisample shading or not
	multisamplingCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;								// Number of samples to use per fragment

	// Blending
	VkPipelineColorBlendAttachmentState colorStateAttachment = {};
	colorStateAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |			// Colors to apply blending
								VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorStateAttachment.blendEnable = VK_TRUE;															// Enable blending

	// Blending Equation: (srcColorBlendFactor * new color) colorBlendOp (dstColorBlendFactor * old color)
	colorStateAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	colorStateAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	colorStateAttachment.colorBlendOp = VK_BLEND_OP_ADD;

	colorStateAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colorStateAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorStateAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

	VkPipelineColorBlendStateCreateInfo colorBlendingCreateInfo = {};
	colorBlendingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlendingCreateInfo.logicOpEnable = VK_FALSE;													// Alternative to calculations is to use logical operations
	colorBlendingCreateInfo.logicOp = VK_LOGIC_OP_COPY;
	colorBlendingCreateInfo.attachmentCount = 1;
	colorBlendingCreateInfo.pAttachments = &colorStateAttachment;

	// Pipeline Layout
	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
	pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutCreateInfo.setLayoutCount = 0;
	pipelineLayoutCreateInfo.pSetLayouts = nullptr;
	pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
	pipelineLayoutCreateInfo.pPushConstantRanges = nullptr;

	// Create Pipeline Layout
	VkResult result = vkCreatePipelineLayout(mainDevice.logicalDevice, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout);

	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create Pipeline Layout!");
	}

	// Depth Stencil Testing (To Do)

	// Graphics Pipeline Creation
	VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo = {};
	graphicsPipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	graphicsPipelineCreateInfo.stageCount = 2;															// Number of shader stages
	graphicsPipelineCreateInfo.pStages = shaderStages;													// List of shader stages
	graphicsPipelineCreateInfo.pVertexInputState = &vertexInputCreateInfo;								// All the fixed function pipeline states
	graphicsPipelineCreateInfo.pInputAssemblyState = &inputAssemblyCreateInfo;
	graphicsPipelineCreateInfo.pViewportState = &viewportStateCreateInfo;
	graphicsPipelineCreateInfo.pDynamicState = nullptr;
	graphicsPipelineCreateInfo.pRasterizationState = &rasterizerCreateInfo;
	graphicsPipelineCreateInfo.pMultisampleState = &multisamplingCreateInfo;
	graphicsPipelineCreateInfo.pColorBlendState = &colorBlendingCreateInfo;
	graphicsPipelineCreateInfo.pDepthStencilState = nullptr;
	graphicsPipelineCreateInfo.layout = pipelineLayout;													// Pipeline Layout that the pipeline should use
	graphicsPipelineCreateInfo.renderPass = renderPass;													// Render pass description that the pipeline is compatible with
	graphicsPipelineCreateInfo.subpass = 0;																// Subpass of render pass to use with pipeline

	// Pipeline Derivates - Can create multiple pipelines that derive from one another for optimization
	graphicsPipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;										// Existing pipeline to create derivative from
	graphicsPipelineCreateInfo.basePipelineIndex = -1;													// Index of pipeline being created to derive from (if creating multiple pipelines at once)

	// Create Graphics Pipeline
	result = vkCreateGraphicsPipelines(mainDevice.logicalDevice, VK_NULL_HANDLE, 1, &graphicsPipelineCreateInfo, nullptr, &graphicsPipeline);

	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create Graphics Pipeline!");
	}

	// Destroy Shader Modules
	// No longer needed after Graphics Pipeline has been created
	vkDestroyShaderModule(mainDevice.logicalDevice, fragmentShaderModule, nullptr);
	vkDestroyShaderModule(mainDevice.logicalDevice, vertexShaderModule, nullptr);
}

void VulkanRenderer::createFramebuffers()
{
	// Resize framebuffer count to equal swapchain image count
	swapchainFramebuffers.resize(swapchainImages.size());

	// Create a framebuffer for each swapchain image
	for (size_t i = 0; i < swapchainFramebuffers.size(); i++)
	{

		std::array<VkImageView, 1> attachments = {
			swapchainImages[i].imageView
		};

		VkFramebufferCreateInfo framebufferCreateInfo = {};
		framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferCreateInfo.renderPass = renderPass;													// Render Pass layout the framebuffer will be used with
		framebufferCreateInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		framebufferCreateInfo.pAttachments = attachments.data();										// List of attachments (1:1 with Render Pass)
		framebufferCreateInfo.width = swapchainExtent.width;											// Framebuffer width
		framebufferCreateInfo.height = swapchainExtent.height;											// Framebuffer height
		framebufferCreateInfo.layers = 1;																// Framebuffer layers

		VkResult result = vkCreateFramebuffer(mainDevice.logicalDevice, &framebufferCreateInfo, nullptr, &swapchainFramebuffers[i]);

		if (result != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create framebuffer!");
		}
	}
}

void VulkanRenderer::createCommandPool()
{
	// Get indices of queue families from device
	QueueFamilyIndices queueFamilyIndices = getQueueFamilies(mainDevice.physicalDevice);

	VkCommandPoolCreateInfo poolCreateInfo = {};
	poolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolCreateInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily;								// Queue family type that the buffers from this command pool will use

	// Create a Graphics Queue Family Command Pool
	VkResult result = vkCreateCommandPool(mainDevice.logicalDevice, &poolCreateInfo, nullptr, &graphicsCommandPool);

	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create Command Pool!");
	}
}

void VulkanRenderer::createCommandBuffers()
{
	// Resize command buffer count to have one for each framebuffer
	commandBuffers.resize(swapchainFramebuffers.size());

	VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
	commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	commandBufferAllocateInfo.commandPool = graphicsCommandPool;
	commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;									// VK_COMMAND_BUFFER_LEVEL_PRIMARY: Buffer submitted directly to queue		
																										// VK_COMMAND_BUFFER_LEVEL_SECONDARY: Buffer can't be called directly. Can be called from primary buffers.
	commandBufferAllocateInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

	VkResult result = vkAllocateCommandBuffers(mainDevice.logicalDevice, &commandBufferAllocateInfo, commandBuffers.data());

	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to allocate Command Buffers!");
	}

}

void VulkanRenderer::createSynchronization()
{

	imageAvailable.resize(MAX_FRAME_DRAWS);
	renderFinished.resize(MAX_FRAME_DRAWS);
	drawFences.resize(MAX_FRAME_DRAWS);

	// Semaphore creation information
	VkSemaphoreCreateInfo semaphoreCreateInfo = {};
	semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	// Fence creation information
	VkFenceCreateInfo fenceCreateInfo = {};
	fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (size_t i = 0; i < MAX_FRAME_DRAWS; i++)
	{
		if (vkCreateSemaphore(mainDevice.logicalDevice, &semaphoreCreateInfo, nullptr, &imageAvailable[i]) != VK_SUCCESS ||
			vkCreateSemaphore(mainDevice.logicalDevice, &semaphoreCreateInfo, nullptr, &renderFinished[i]) != VK_SUCCESS ||
			vkCreateFence(mainDevice.logicalDevice, &fenceCreateInfo, nullptr, &drawFences[i]) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create semaphores and fences!");
		}
	}

}

VkImageView VulkanRenderer::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags)
{
	VkImageViewCreateInfo viewCreateInfo = {};
	viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewCreateInfo.image = image;																		// Image to create view for
	viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;													// Type of image
	viewCreateInfo.format = format;																		// Format of image data
	viewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;										// Allows remapping of rgba components to other rgba components
	viewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	viewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	viewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

	// Subresources allow the view to view only a part of an image
	viewCreateInfo.subresourceRange.aspectMask = aspectFlags;											// Which aspect of the image to view
	viewCreateInfo.subresourceRange.baseMipLevel = 0;													// Start mipmap level to view from
	viewCreateInfo.subresourceRange.levelCount = 1;														// Number of mipmap levels to view
	viewCreateInfo.subresourceRange.baseArrayLayer = 0;													// Start array level to view from
	viewCreateInfo.subresourceRange.layerCount = 1;														// Number of array levels to view

	// Create image view and return it
	VkImageView imageView;
	VkResult result = vkCreateImageView(mainDevice.logicalDevice, &viewCreateInfo, nullptr, &imageView);

	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create an Image View!");
	}

	return imageView;
}

VkShaderModule VulkanRenderer::createShaderModule(const std::vector<char>& code)
{
	// Shader Module struct info
	VkShaderModuleCreateInfo shaderModuleCreateInfo = {};
	shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	shaderModuleCreateInfo.codeSize = code.size();														// Size of code
	shaderModuleCreateInfo.pCode = reinterpret_cast<const uint32_t *>(code.data());						// Pointer to code (of uint32_t pointer type)

	VkShaderModule shaderModule;
	VkResult result = vkCreateShaderModule(mainDevice.logicalDevice, &shaderModuleCreateInfo, nullptr, &shaderModule);

	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create Shader Module!");
	}

	return shaderModule;
}

void VulkanRenderer::recordCommands()
{
	// Information about how to begin each command buffer
	VkCommandBufferBeginInfo commandBufferBeginInfo = {};
	commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	// Information about how to begin a render pass (only needed for graphical applications)
	VkRenderPassBeginInfo renderPassBeginInfo = {};
	renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassBeginInfo.renderPass = renderPass;														// Render pass to begin
	renderPassBeginInfo.renderArea.offset = { 0, 0 };													// Start point of render pass in pixels
	renderPassBeginInfo.renderArea.extent = swapchainExtent;											// Size of region to run render pass on (starting at offset)

	VkClearValue clearValues[] = {
		{0.6f, 0.65f, 0.4, 1.0f }
	};

	renderPassBeginInfo.pClearValues = clearValues;														// List of clear values (TODO: Depth Attachment Clear Value)
	renderPassBeginInfo.clearValueCount = 1;															// Clear value count

	for (size_t i = 0; i < commandBuffers.size(); i++)
	{

		renderPassBeginInfo.framebuffer = swapchainFramebuffers[i];

		// Start recording commands to the command buffer
		VkResult result = vkBeginCommandBuffer(commandBuffers[i], &commandBufferBeginInfo);

		if (result != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to start recording a Command Buffer!");
		}

		// Begin Render Pass
		vkCmdBeginRenderPass(commandBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

		// Bind Pipeline to be used in render pass
		vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

		VkBuffer vertexBuffers[] = { mesh1.getVertexBuffer() };											// Buffers to bind
		VkDeviceSize offsets[] = { 0 };																	// Offsets into buffers being bound
		vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, vertexBuffers, offsets);						// Command to bind vertex buffer before drawing with them

		// Execute pipeline
		vkCmdDraw(commandBuffers[i], static_cast<uint32_t>(mesh1.getVertexCount()), 1, 0, 0);

		// End Render Pass
		vkCmdEndRenderPass(commandBuffers[i]);

		// Stop recording to command buffer
		result = vkEndCommandBuffer(commandBuffers[i]);

		if (result != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to stop recording a Command Buffer!");
		}
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

	// Wait until no actions being run on device before destroying
	vkDeviceWaitIdle(mainDevice.logicalDevice);

	mesh1.destroyVertexBuffer();

	for (size_t i = 0; i < MAX_FRAME_DRAWS; i++)
	{

		vkDestroyFence(mainDevice.logicalDevice, drawFences[i], nullptr);

		vkDestroySemaphore(mainDevice.logicalDevice, renderFinished[i], nullptr);

		vkDestroySemaphore(mainDevice.logicalDevice, imageAvailable[i], nullptr);
	}

	vkDestroyCommandPool(mainDevice.logicalDevice, graphicsCommandPool, nullptr);

	for (auto framebuffer : swapchainFramebuffers)
	{
		vkDestroyFramebuffer(mainDevice.logicalDevice, framebuffer, nullptr);
	}

	vkDestroyPipeline(mainDevice.logicalDevice, graphicsPipeline, nullptr);

	vkDestroyPipelineLayout(mainDevice.logicalDevice, pipelineLayout, nullptr);

	vkDestroyRenderPass(mainDevice.logicalDevice, renderPass, nullptr);

	for (auto image : swapchainImages)
	{
		vkDestroyImageView(mainDevice.logicalDevice, image.imageView, nullptr);
	}

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