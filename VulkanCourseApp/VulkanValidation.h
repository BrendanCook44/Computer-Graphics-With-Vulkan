#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vector>
#include <iostream>
#include <stdexcept>

class VulkanValidation
{
public:
    VulkanValidation();
    ~VulkanValidation();

    // Setup and cleanup functions
    void setupDebugMessenger(VkInstance instance);
    void cleanup(VkInstance instance);

    // Validation layer support functions
    bool checkValidationLayerSupport(const std::vector<const char*>& validationLayers);
    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
    
    // Get required validation layers
    std::vector<const char*> getValidationLayers();

    // Check if validation layers are enabled
    bool isValidationLayersEnabled() const;

private:
    VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;

    // Debug callback function
    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData);

    // Extension function wrappers
    VkResult createDebugUtilsMessengerEXT(VkInstance instance, 
        const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, 
        const VkAllocationCallbacks* pAllocator, 
        VkDebugUtilsMessengerEXT* pDebugMessenger);
        
    void destroyDebugUtilsMessengerEXT(VkInstance instance, 
        VkDebugUtilsMessengerEXT debugMessenger, 
        const VkAllocationCallbacks* pAllocator);
};