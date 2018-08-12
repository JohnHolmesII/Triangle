#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <algorithm>
#include <set>

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

typedef uint32_t u32;

const int WIDTH = 800;
const int HEIGHT = 600;
const std::vector<const char*> validationLayers = {"VK_LAYER_LUNARG_standard_validation"};
const std::vector<const char*> deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pCallback)
{
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");

	if (func != nullptr)
	{
		return func(instance, pCreateInfo, pAllocator, pCallback);
	}
	else
	{
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT callback, const VkAllocationCallbacks* pAllocator)
{
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");

	if (func != nullptr)
	{
		func(instance, callback, pAllocator);
	}
}

class HelloTriangleApplication
{
	public:

	void run()
	{
		initWindow();
		initVulkan();
		mainLoop();
		cleanup();
	}

	private:

	GLFWwindow*              window;
	VkInstance               instance;
	VkDebugUtilsMessengerEXT callback;
	VkPhysicalDevice         physicalDevice = VK_NULL_HANDLE;
	VkDevice                 logicalDevice;
	VkQueue                  graphicsQueue;
	VkQueue                  presentQueue;
	VkSurfaceKHR             surface;
	VkSwapchainKHR           swapChain;
	std::vector<VkImage>     swapChainImages;
	VkFormat                 swapChainImageFormat;
	VkExtent2D               swapChainExtent;

	struct QueueFamilyIndices
	{
		int graphicsFamily = -1;
		int presentFamily  = -1;

		bool isComplete()
		{
			return graphicsFamily >= 0 &&
				   presentFamily  >= 0;
		}
	};

	struct SwapChainSupportDetails
	{
		VkSurfaceCapabilitiesKHR        capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR>   presentModes;
	};

	void initWindow()
	{
		glfwInit();

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

		window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
	}

	void initVulkan()
	{
		createInstance();
		setupDebugCallback();
		createSurface();
		pickPhysicalDevice();
		createLogicalDevice();
		createSwapChain(logicalDevice);
	}

	void mainLoop()
	{
		while (!glfwWindowShouldClose(window))
		{
			glfwPollEvents();
		}
	}

	void cleanup()
	{
		if (enableValidationLayers)
		{
			DestroyDebugUtilsMessengerEXT(instance, callback, nullptr);
		}

		vkDestroySwapchainKHR(logicalDevice, swapChain, nullptr);
		vkDestroyDevice(logicalDevice, nullptr);
		vkDestroySurfaceKHR(instance, surface, nullptr);
		vkDestroyInstance(instance, nullptr);
		glfwDestroyWindow(window);
		glfwTerminate();
	}

	void createInstance()
	{
		if (enableValidationLayers && !checkValidationLayerSupport())
		{
			throw std::runtime_error("validation layers requested, but not available!");
		}

		VkApplicationInfo    appInfo    = {};
		VkInstanceCreateInfo createInfo = {};
		auto                 extensions = getRequiredExtensions();

		appInfo.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName   = "Hello Triangle";
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName        = "No Engine";
		appInfo.engineVersion      = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion         = VK_API_VERSION_1_0;

		createInfo.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo        = &appInfo;
		createInfo.enabledExtensionCount   = static_cast<u32>(extensions.size());
		createInfo.ppEnabledExtensionNames = extensions.data();

		if (enableValidationLayers)
		{
			createInfo.enabledLayerCount   = static_cast<u32>(validationLayers.size());
			createInfo.ppEnabledLayerNames = validationLayers.data();
		}
		else
		{
			createInfo.enabledLayerCount = 0;
		}

		if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create instance!");
		}
	}

	void createLogicalDevice()
	{
		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
		QueueFamilyIndices                   indices             = findQueueFamilies(physicalDevice);
		std::set<int>                        uniqueQueueFamilies = {indices.graphicsFamily, indices.presentFamily};
		VkPhysicalDeviceFeatures             deviceFeatures      = {};
		VkDeviceCreateInfo                   createInfo          = {};
		float                                queuePriority       = 1.0f;


		for (int queueFamily : uniqueQueueFamilies)
		{
			VkDeviceQueueCreateInfo queueCreateInfo = {};

			queueCreateInfo.sType                   = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfo.queueFamilyIndex        = queueFamily;
			queueCreateInfo.queueCount              = 1;
			queueCreateInfo.pQueuePriorities        = &queuePriority;

			queueCreateInfos.push_back(queueCreateInfo);
		}

		createInfo.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		createInfo.queueCreateInfoCount    = static_cast<u32>(queueCreateInfos.size());
		createInfo.pQueueCreateInfos       = queueCreateInfos.data();
		createInfo.pEnabledFeatures        = &deviceFeatures;
		createInfo.enabledExtensionCount   = static_cast<u32>(deviceExtensions.size());
		createInfo.ppEnabledExtensionNames = deviceExtensions.data();

		if (enableValidationLayers)
		{
			createInfo.enabledLayerCount   = static_cast<u32>(validationLayers.size());
			createInfo.ppEnabledLayerNames = validationLayers.data();
		}
		else
		{
			createInfo.enabledLayerCount = 0;
		}

		if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &logicalDevice) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create logical device!");
		}

		vkGetDeviceQueue(logicalDevice, indices.graphicsFamily, 0, &graphicsQueue);
		vkGetDeviceQueue(logicalDevice, indices.presentFamily, 0, &presentQueue);
	}

	void createSurface()
	{
		if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create window surface!");
		}
	}

	void createSwapChain(VkDevice device)
	{
		SwapChainSupportDetails  swapChainSupport     = querySwapChainSupport(physicalDevice);
		VkSurfaceFormatKHR       surfaceFormat        = chooseSwapSurfaceFormat(swapChainSupport.formats);
		VkPresentModeKHR         presentMode          = chooseSwapPresentMode(swapChainSupport.presentModes);
		VkExtent2D               extent               = chooseSwapExtent(swapChainSupport.capabilities);
		u32                      imageCount           = swapChainSupport.capabilities.minImageCount + 1;
		VkSwapchainCreateInfoKHR createInfo           = {};
		QueueFamilyIndices       indices              = findQueueFamilies(physicalDevice);
		u32                      queueFamilyIndices[] = {(u32) indices.graphicsFamily, (u32) indices.presentFamily};

		swapChainImageFormat = surfaceFormat.format;
		swapChainExtent      = extent;

		if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount)
		{
			imageCount = swapChainSupport.capabilities.maxImageCount;
		}

		createInfo.sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.surface          = surface;
		createInfo.minImageCount    = imageCount;
		createInfo.imageFormat      = surfaceFormat.format;
		createInfo.imageColorSpace  = surfaceFormat.colorSpace;
		createInfo.imageExtent      = extent;
		createInfo.imageArrayLayers = 1;
		createInfo.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		createInfo.preTransform     = swapChainSupport.capabilities.currentTransform;
		createInfo.compositeAlpha   = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		createInfo.presentMode      = presentMode;
		createInfo.clipped          = VK_TRUE;
		createInfo.oldSwapchain     = VK_NULL_HANDLE;

		if (indices.graphicsFamily != indices.presentFamily)
		{
			createInfo.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
			createInfo.queueFamilyIndexCount = 2;
			createInfo.pQueueFamilyIndices   = queueFamilyIndices;
		}
		else
		{
			createInfo.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE;
			createInfo.queueFamilyIndexCount = 0;
			createInfo.pQueueFamilyIndices   = nullptr;
		}

		if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create swap chain!");
		}

		vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
		swapChainImages.resize(imageCount);
		vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());
	}

	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
	{
		if (availableFormats.size() == 1 && availableFormats[0].format == VK_FORMAT_UNDEFINED)
		{
			return {VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
		}

		for (const auto& availableFormat : availableFormats)
		{
			if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			{
				return availableFormat;
			}
		}

		return availableFormats[0];
	}

	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> availablePresentModes)
	{
		for (const auto& availablePresentMode : availablePresentModes)
		{
			if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
			{
				return availablePresentMode;
			}
			else if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR)
			{
				return availablePresentMode;
			}
		}

		return VK_PRESENT_MODE_FIFO_KHR;
	}

	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
	{
		if (capabilities.currentExtent.width != std::numeric_limits<u32>::max())
		{
			return capabilities.currentExtent;
		}
		else
		{
			VkExtent2D actualExtent = {WIDTH, HEIGHT};

			actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
			actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));

			return actualExtent;
		}
	}

	SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device)
	{
		SwapChainSupportDetails details;
		u32                     formatCount;
		u32                     presentModeCount;

		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

		if (formatCount != 0)
		{
			details.formats.resize(formatCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
		}

		if (presentModeCount != 0)
		{
			details.presentModes.resize(presentModeCount);
			vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
		}

		return details;
	}

	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device)
	{
		QueueFamilyIndices indices;
		u32                queueFamilyCount = 0;
		VkBool32           presentSupport   = false;

		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

		int i = 0;
		for (const auto& queueFamily : queueFamilies)
		{
			vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

			if (queueFamily.queueCount > 0)
			{
				if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
				{
					indices.graphicsFamily = i;
				}
				else if (presentSupport)
				{
					indices.presentFamily = i;
				}
			}

			if (indices.isComplete())
			{
				break;
			}

			i++;
		}

		return indices;
	}

	void pickPhysicalDevice()
	{
		u32 deviceCount = 0;
		vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
		std::vector<VkPhysicalDevice> devices(deviceCount);

		vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

		for (const auto& device : devices)
		{
			if (isDeviceSuitable(device))
			{
				physicalDevice = device;
				break;
			}
		}

		if (deviceCount == 0 || physicalDevice == VK_NULL_HANDLE)
		{
			throw std::runtime_error("failed to find a suitable GPU!");
		}
	}

	bool isDeviceSuitable(VkPhysicalDevice device)
	{
		SwapChainSupportDetails swapChainSupport;
		QueueFamilyIndices      indices           = findQueueFamilies(device);
		bool                    extsSupported     = checkDeviceExtensionSupport(device);
		bool                    swapChainAdequate = false;

		if (extsSupported)
		{
			swapChainSupport  = querySwapChainSupport(device);
			swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
		}

		return indices.isComplete() && extsSupported && swapChainAdequate;
	}

	void setupDebugCallback()
	{
		if (enableValidationLayers)
		{
			VkDebugUtilsMessengerCreateInfoEXT createInfo = {};

			createInfo.sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
			createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
			createInfo.messageType     = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
			createInfo.pfnUserCallback = debugCallback;

			if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &callback) != VK_SUCCESS)
			{
				throw std::runtime_error("failed to set up debug callback!");
			}
		}
	}

	std::vector<const char*> getRequiredExtensions()
	{
		u32          glfwExtensionCount = 0;
		const char** glfwExtensions     = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

		std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

		if (enableValidationLayers)
		{
			extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}

		return extensions;
	}

	bool checkValidationLayerSupport()
	{
		u32 layerCount;
		vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

		std::vector<VkLayerProperties> availableLayers(layerCount);
		vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

		for (const char* layerName : validationLayers)
		{
			bool layerFound = false;

			for (const auto& layerProperties : availableLayers)
			{
				if (strcmp(layerName, layerProperties.layerName) == 0)
				{
					layerFound = true;
					break;
				}
			}

			if (!layerFound)
			{
				return false;
			}
		}

		return true;
	}

	bool checkDeviceExtensionSupport(VkPhysicalDevice device)
	{
		u32 extensionCount;
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

		std::vector<VkExtensionProperties> availableExtensions(extensionCount);
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

		std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

		for (const auto& extension : availableExtensions)
		{
			requiredExtensions.erase(extension.extensionName);
		}

		return requiredExtensions.empty();
	}

	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT      messageSeverity,
														VkDebugUtilsMessageTypeFlagsEXT             messageType,
														const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
														void*                                       pUserData)
	{
		std::cerr << "validation layer: " << pCallbackData->pMessage << '\n';

		return VK_FALSE;
	}
};

int main()
{
	HelloTriangleApplication app;

	try
	{
		app.run();
	}
	catch (const std::exception& e)
	{
		std::cerr << e.what() << '\n';
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

