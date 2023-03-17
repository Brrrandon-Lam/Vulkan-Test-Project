// Created by Brandon Lam, base of project inspired by Vulkan tutorial found here:
// https://vulkan-tutorial.com/

// Have GLFW include the Vulkan header and include the GLFW header
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <optional>
#include <cstdlib>
#include <vector>
#include <cstring>

// GLFW Window Height and Width
const uint32_t WINDOW_WIDTH = 1920;
const uint32_t WINDOW_HEIGHT = 1080;

// Validation Layer
const std::vector<const char*> validationLayers = { "VK_LAYER_KHRONOS_validation" };
#ifdef NDEBUG
	const bool enableValidationLayers = false;
#else
	const bool enableValidationLayers = true;
#endif

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr) {
		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	}
	else {
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr) {
		func(instance, debugMessenger, pAllocator);
	}
}

struct QueueFamilyIndices {
	// No value unless one is assigned
	std::optional<uint32_t> graphicsFamily;
	// True if the data member has a value
	bool isComplete() { return graphicsFamily.has_value(); }
};

class Application {
public:
	void run();
private:
	// Functions 
	void initWindow();
	void initVulkan();
	void mainLoop();
	void cleanup();
	void createInstance();
	bool checkValidationLayerSupport();
	bool isDeviceSuitable(VkPhysicalDevice device);
	void selectPhysicalDevice();
	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);

	// Debug Messenger
	void setupDebugMessenger();
	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);
	std::vector<const char*> getRequiredExtensions();
	void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);

	// Variables
	GLFWwindow* window;
	VkInstance instance;
	VkDebugUtilsMessengerEXT debugMessenger;
};


int main() {
	Application app;
	try {
		app.run();
	}
	catch(const std::exception& e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}

// Run the program
void Application::run() {
	initWindow();
	initVulkan();
	mainLoop();
	cleanup();
}

// Initialize our Window
void Application::initWindow() {
	// Initialize GLFW
	glfwInit();
	// Don't create an OpenGL context object & no resize
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	// Create the window
	window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Vulkan Environment", nullptr, nullptr);
}

// Initialize Vulkan
void Application::initVulkan() {
	createInstance();
	setupDebugMessenger();
}

// Creates our Vulkan instance
void Application::createInstance() {
	// Verify validation layer usage
	if (enableValidationLayers && !checkValidationLayerSupport()) {
		throw std::runtime_error("Validation Layers were requested, but not available!");
	}
	// Application Info
	VkApplicationInfo appInfo{};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "Vulkan Renderer";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 3, 249);
	appInfo.pEngineName = "No Engine";
	appInfo.engineVersion = VK_MAKE_API_VERSION(0, 1, 0, 0);
	// 1.3.239.0 formatted Major, Minor, Patch, Variant
	appInfo.apiVersion = VK_MAKE_API_VERSION(0, 1, 3, 249);

	// Create Info
	VkInstanceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;

	// Set the number of extensions and the names of the extensions
	auto extensions = getRequiredExtensions();
	createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	createInfo.ppEnabledExtensionNames = extensions.data();

	// Debug Messenger for Create and Destroy Instance
	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
	// If validation layers are enabled, initialize them 
	if (enableValidationLayers) {
		// Initialize validation layer amount and names
		createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();

		// Populate the debug messenger's createInfo and set createInfo's pNext to debugCreateInfo, extending this structure.
		populateDebugMessengerCreateInfo(debugCreateInfo);
		createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
	}
	else {
		createInfo.enabledLayerCount = 0;
		createInfo.pNext = nullptr;
	}
	if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create instance!");
	}

}

// Main Loop
void Application::mainLoop() {
	// While the window is open
	while (!glfwWindowShouldClose(window)) {
		// Poll events
		glfwPollEvents();
	}
}

// Cleanup (Not RAII)
void Application::cleanup() {

	// Destroy our debug messenger
	if (enableValidationLayers) {
		//DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
	}

	// Destroy the VkInstance
	vkDestroyInstance(instance, nullptr);
	// Destroy the window and terminate GLFW
	glfwDestroyWindow(window);
	glfwTerminate();
}

// Check that validation layers exist
bool Application::checkValidationLayerSupport() {
	// Get the number of layers
	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
	std::vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());
	
	// Check that all of the validation layers exist in the list of available layers.
	for (const char* layerName : validationLayers) {
		bool layerFound = false;
		for (const auto& layerProperties : availableLayers) {
			if (strcmp(layerName, layerProperties.layerName) == 0) {
				layerFound = true;
				break;
			}
		}
		if (!layerFound) {
			return false;
		}
	}
	return true;
}

bool Application::isDeviceSuitable(VkPhysicalDevice device)
{
	// Find the queue families for a particular device. If they have values then return true
	QueueFamilyIndices indices = findQueueFamilies(device);
	return indices.graphicsFamily.has_value();
}

void Application::selectPhysicalDevice() {
	// Find devices if possible.
	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
	// No devices found
	if (deviceCount == 0) {
		throw std::runtime_error("Failed to find GPU with Vulkan support!");
	}
	// If devices exist, then allocate a vector of size deviceCount to hold their handles.
	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

	// Iterate through our devices and grab the first suitable device.
	for (const VkPhysicalDevice& device : devices) {
		if (isDeviceSuitable(device)) {
			physicalDevice = device;
			break;
		}
	}
	// No suitable devices found
	if (physicalDevice == VK_NULL_HANDLE) {
		throw::std::runtime_error("Failed to find a suitable GPU!");
	}


}

QueueFamilyIndices Application::findQueueFamilies(VkPhysicalDevice device)
{
	// Get available queue families supported from our physical device
	QueueFamilyIndices indices;
	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

	// Assign an index to our queue families in the vector 
	// We only want to assign an index if a queue family has VK_QUEUE_GRAPHICS_BIT set.
	int i = 0;
	for (const auto& queueFamily : queueFamilies) {
		if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			indices.graphicsFamily = i;
		}
		if (indices.isComplete()) {
			break;
		}
		i++;
	}
	return indices;
}

// Returns required extensions for validation layers if enabled to handle message callbacks
std::vector<const char*> Application::getRequiredExtensions() {
	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
	std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
	if (enableValidationLayers) {
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}
	return extensions;
}

VKAPI_ATTR VkBool32 VKAPI_CALL Application::debugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData) {

	std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

	return VK_FALSE;
}

void Application::setupDebugMessenger() {
	if(!enableValidationLayers) {
		return;
	}
	VkDebugUtilsMessengerCreateInfoEXT createInfo{};
	populateDebugMessengerCreateInfo(createInfo);

	if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
		throw std::runtime_error("Failed to set up debug messenger!");
	}
}

void Application::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
	createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	createInfo.pfnUserCallback = debugCallback;
}