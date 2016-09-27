#define GLFW_INCLUDE_VULKAN // replaces //#include <vulkan/vulkan.h>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE // GLM will use the OpenGL depth range of -1.0 to 1.0 by default. Use the Vulkan range of 0.0 to 1.0 instead
#include <glm/gtc/matrix_transform.hpp>
#include <GLFW/glfw3.h> 
#include <iostream>
#include <stdexcept>
#include <fstream>
#include <algorithm>
#include <vector>
#include <cstring>
#include <chrono>
#include <set>

#include "vdeleter.h"
#include "vertexBuffer.h"
#include "uniformBuffer.h"
#include "lodepng.h"

const int WIDTH = 800;
const int HEIGHT = 600;

const std::vector<const char*> validationLayers = {
	"VK_LAYER_LUNARG_standard_validation"
};

const std::vector<const char*> deviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

// This proxy function is necessary because vkCreateDebugReportCallbackEXT is and extension function and hence not loaded automatically.
// Needs to be loaded manually via vkGetInstanceProcAddr
VkResult CreateDebugReportCallbackEXT(VkInstance instance, const VkDebugReportCallbackCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugReportCallbackEXT* pCallback) {
	auto func = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT");
	if (func != nullptr) {
		return func(instance, pCreateInfo, pAllocator, pCallback);
	}
	else {
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

// This proxy function is necessary because vkDestroyDebugReportCallbackEXT is and extension function and hence not loaded automatically.
// Needs to be loaded manually via vkGetInstanceProcAddr
void DestroyDebugReportCallbackEXT(VkInstance instance, VkDebugReportCallbackEXT callback, const VkAllocationCallbacks* pAllocator) {
	auto func = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT");
	if (func != nullptr) {
		func(instance, callback, pAllocator);
	}
}

struct QueueFamilyIndices {
	int graphicsFamily = -1;
	// It's actually possible that the queue families supporting drawing commands and the ones supporting presentation do not overlap. 
	// Therefore, we have to take into account that there could be a distinct presentation queue
	int presentFamily = -1;

	bool isComplete() {
		return graphicsFamily >= 0 && presentFamily >= 0;
	}
};

struct SwapChainSupportDetails {
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
};

class HelloTriangleApplication {
public:
	void run() {
		initWindow();
		initVulkan();
		mainLoop();
	}

private:
	GLFWwindow* window;

	VDeleter<VkInstance> instance{ vkDestroyInstance };
	// Even the debug callback is managed with a handle that needs to be explicitly created
	// and destroyed
	VDeleter<VkDebugReportCallbackEXT> callback{ instance, DestroyDebugReportCallbackEXT };
	VDeleter<VkSurfaceKHR> surface{ instance, vkDestroySurfaceKHR };

	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE; // this object will be implicityly destroyed when VKInstance is destroyed
	VDeleter<VkDevice> device{ vkDestroyDevice };

	VkQueue graphicsQueue;
	VkQueue presentQueue;

	// swapChain is added after device so it gets deleted before device is
	VDeleter<VkSwapchainKHR> swapChain{ device, vkDestroySwapchainKHR };
	// These below are automatically created by the implementation for the 
	// swap chain and they are automatically cleaned up when the swap chain 
	// is destroyed
	std::vector<VkImage> swapChainImages;
	VkFormat swapChainImageFormat;
	VkExtent2D swapChainExtent;
	// An Image view is the only way to access an image as it specifies
	// what to access and how to access it (e.g., as 2d texture map with no mimmapping)
	std::vector<VDeleter<VkImageView>> swapChainImageViews;
	std::vector<VDeleter<VkFramebuffer>> swapChainFramebuffers;

	VDeleter<VkRenderPass> renderPass{ device, vkDestroyRenderPass };
	VDeleter<VkDescriptorSetLayout> descriptorSetLayout{ device, vkDestroyDescriptorSetLayout }; // All of the descriptor bindings used in shaders are combined into a single set
	VDeleter<VkPipelineLayout> pipelineLayout{ device, vkDestroyPipelineLayout };
	VDeleter<VkPipeline> graphicsPipeline{ device, vkDestroyPipeline };

	// A depth attachment is based on an image, just like the color attachment.
	// The difference is that the swap chain will not automatically create depth images for us.
	// We only need a single depth image, because only one draw operation is running at once.
	// The depth image will again require the trifecta of resources : image, memory and image view.
	VDeleter<VkImage> depthImage{ device, vkDestroyImage };
	VDeleter<VkDeviceMemory> depthImageMemory{ device, vkFreeMemory };
	VDeleter<VkImageView> depthImageView{ device, vkDestroyImageView };

	// Handle to the texture image and its memory
	VDeleter<VkImage> textureImage{ device, vkDestroyImage };
	VDeleter<VkDeviceMemory> textureImageMemory{ device, vkFreeMemory };
	VDeleter<VkImageView> textureImageView{ device, vkDestroyImageView };
	VDeleter<VkSampler> textureSampler{ device, vkDestroySampler };

	// Vertex Buffer acts more like a buffer descriptor, as the memory is hold separately
	VDeleter<VkBuffer> vertexBuffer{ device, vkDestroyBuffer };
	VDeleter<VkDeviceMemory> vertexBufferMemory{ device, vkFreeMemory };
	// Note that specifying the vertexBuffer and vertexBufferMemory members 
	// in this order will cause the memory to be freed before the buffer is destroyed, 
	// but that's allowed as long as the buffer is no longer used.
	VDeleter<VkBuffer> indexBuffer{ device, vkDestroyBuffer };
	VDeleter<VkDeviceMemory> indexBufferMemory{ device, vkFreeMemory };

	VDeleter<VkBuffer> uniformStagingBuffer{ device, vkDestroyBuffer };
	VDeleter<VkDeviceMemory> uniformStagingBufferMemory{ device, vkFreeMemory };
	VDeleter<VkBuffer> uniformBuffer{ device, vkDestroyBuffer };
	VDeleter<VkDeviceMemory> uniformBufferMemory{ device, vkFreeMemory };

	VDeleter<VkDescriptorPool> descriptorPool{ device, vkDestroyDescriptorPool };
	// Descriptor set will automatically be freed when the descriptor pool is destroyed.
	VkDescriptorSet descriptorSet;

	VDeleter<VkCommandPool> commandPool{ device, vkDestroyCommandPool };
	// command buffers will automatically be destroyed when their command pool is
	// destroyed
	std::vector<VkCommandBuffer> commandBuffers;

	// We'll need one semaphore to signal that an image has been acquired and is 
	// ready for rendering, and another one to signal that rendering has finished 
	// and presentation can happen
	VDeleter<VkSemaphore> imageAvailableSemaphore{ device, vkDestroySemaphore };
	VDeleter<VkSemaphore> renderFinishedSemaphore{ device, vkDestroySemaphore };

	void initWindow() {
		glfwInit();
		// tell glfw we don't need an openGL context
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

		window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);

		glfwSetWindowUserPointer(window, this); // pass a pointer of this class to the callback function below
		glfwSetWindowSizeCallback(window, HelloTriangleApplication::onWindowResized);
	}

	void initVulkan() {
		// statically linking with the Vulkan loader from the SDK
		createInstance();
		setupDebugCallback();
		createSurface();
		pickPhysicalDevice();
		createLogicalDevice();
		createSwapChain();
		createImageViews();
		createRenderPass();
		createDescriptorSetLayout();
		createGraphicsPipeline();
		createCommandPool();
		createDepthResources();
		createFramebuffers();
		createTextureImage();
		createTextureImageView();
		createTextureSampler();
		createVertexBuffer();
		createIndexBuffer();
		createUniformBuffer();
		createDescriptorPool();
		createDescriptorSet();
		createCommandBuffers();
		createSemaphores();
	}

	void mainLoop() {
		while (!glfwWindowShouldClose(window)) {
			glfwPollEvents();
			updateUniformBuffer();
			drawFrame();
		}
		// Remember that all of the operations in drawFrame are asynchronous.
		// That means that when we exit the loop in mainLoop, drawing and presentation operations 
		// may still be going on. Cleaning up resources while that is happening is a bad idea.
		// To fix that problem, we should wait for the logical device to finish operations before 
		// exiting mainLoop :
		vkDeviceWaitIdle(device);
	}

	static void onWindowResized(GLFWwindow* window, int width, int height) {
		if (width == 0 || height == 0) return;

		HelloTriangleApplication* app = reinterpret_cast<HelloTriangleApplication*>(glfwGetWindowUserPointer(window));
		app->recreateSwapChain();
	}

	void recreateSwapChain() {

		// On window resizing, the window surface change makes the swap chain to be no longer compatible with it
		vkDeviceWaitIdle(device); // We sholdn't use resources that are still in use

		createSwapChain();
		createImageViews();
		createRenderPass();
		createGraphicsPipeline();
		createDepthResources();
		createFramebuffers();
		createCommandBuffers();
	}

	void createInstance() {

		if (enableValidationLayers && !checkValidationLayerSupport()) {
			throw std::runtime_error("validation layers requested, but not available!");
		}
		// Vulkan instance is analogous to an OpenGL rendering context: it stores application
		// state like enabled instance-level layers and extensions
		VkApplicationInfo appInfo = {};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "Hello Triangle";
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = "No Engine";
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_0;

		VkInstanceCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;

		auto extensions = getRequiredExtensions();
		createInfo.enabledExtensionCount = extensions.size();
		createInfo.ppEnabledExtensionNames = extensions.data();

		if (enableValidationLayers) {
			createInfo.enabledLayerCount = validationLayers.size();
			createInfo.ppEnabledLayerNames = validationLayers.data();
		}
		else {
			createInfo.enabledLayerCount = 0;
		}

		if (vkCreateInstance(&createInfo, nullptr, instance.replace()) != VK_SUCCESS) {
			throw std::runtime_error("failed to create instance!");
		}
	}

	void setupDebugCallback() {
		if (!enableValidationLayers) return;

		VkDebugReportCallbackCreateInfoEXT createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
		createInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
		createInfo.pfnCallback = debugCallback;

		if (CreateDebugReportCallbackEXT(instance, &createInfo, nullptr, callback.replace()) != VK_SUCCESS) {
			throw std::runtime_error("failed to set up debug callback!");
		}
	}

	void createSurface() {

		// a Vulkan surface is an interface between the windowing system and the present engine
		// for the swap chain. More specifically, it is one of the WSI (Window System Integration
		// Extensions), the VK_KHR_surface extension. It exposes a VKSurfaceHKR object that represents
		// an abstract type of surface to be rendered images to. This extension has already been enabled
		// by the call to glfwGetRequiredInstanceExtensions among other WSI extensions

		/* This is what glfwWindowSurface is going for us behind scenes: 

			VkWin32SurfaceCreateInfoKHR createInfo;
			createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
			createInfo.hwnd = glfwGetWin32Window(window); // get the raw HWND from the GLFW window object
			createInfo.hinstance = GetModuleHandle(nullptr); returns the HINSTANCE handle of the current process

			// CreateWin32SurfaceKHR needs to be explicitly loaded
			auto CreateWin32SurfaceKHR = (PFN_vkCreateWin32SurfaceKHR) vkGetInstanceProcAddr(instance, "vkCreateWin32SurfaceKHR");

			if (!CreateWin32SurfaceKHR || CreateWin32SurfaceKHR(instance, &createInfo,
			nullptr, &surface) != VK_SUCCESS) {
			throw std::runtime_error("failed to create window surface!");
			}
		*/

		if (glfwCreateWindowSurface(instance, window, nullptr, surface.replace()) != VK_SUCCESS) {
			throw std::runtime_error("failed to create window surface!");
		}
	}

	void pickPhysicalDevice() {
		uint32_t deviceCount = 0;
		vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

		if (deviceCount == 0) {
			throw std::runtime_error("failed to find GPUs with Vulkan support!");
		}

		std::vector<VkPhysicalDevice> devices(deviceCount);
		vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

		for (const auto& device : devices) {
			if (isDeviceSuitable(device)) {
				physicalDevice = device;
				break;
			}
		}

		if (physicalDevice == VK_NULL_HANDLE) {
			throw std::runtime_error("failed to find a suitable GPU!");
		}
	}

	void createLogicalDevice() {
		QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

		/* we need to have multiple VkDeviceQueueCreateInfo structs to create a queue 
		   from both families. An elegant way to do that is to create a set of all unique queue 
		   families that are necessary for the required queues
		*/
		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
		// remember that these below could overlap and hence set solution
		std::set<int> uniqueQueueFamilies = { indices.graphicsFamily, indices.presentFamily };

		float queuePriority = 1.0f;
		for (int queueFamily : uniqueQueueFamilies) {
			VkDeviceQueueCreateInfo queueCreateInfo = {};
			queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfo.queueFamilyIndex = queueFamily;
			queueCreateInfo.queueCount = 1;
			queueCreateInfo.pQueuePriorities = &queuePriority;
			queueCreateInfos.push_back(queueCreateInfo);
		}

		VkPhysicalDeviceFeatures deviceFeatures = {};

		VkDeviceCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

		createInfo.pQueueCreateInfos = queueCreateInfos.data();
		createInfo.queueCreateInfoCount = (uint32_t)queueCreateInfos.size();

		createInfo.pEnabledFeatures = &deviceFeatures;

		createInfo.enabledExtensionCount = deviceExtensions.size();
		createInfo.ppEnabledExtensionNames = deviceExtensions.data();

		// enable the same validation layers for devices as it was done
		// for the instance
		if (enableValidationLayers) {
			createInfo.enabledLayerCount = validationLayers.size();
			createInfo.ppEnabledLayerNames = validationLayers.data();
		}
		else {
			createInfo.enabledLayerCount = 0;
		}

		// the queues are automatically created along with the device
		if (vkCreateDevice(physicalDevice, &createInfo, nullptr, device.replace()) != VK_SUCCESS) {
			throw std::runtime_error("failed to create logical device!");
		}

		// retrieve the queue handles for each family. use zero because we are only
		// creating one queue for each family
		vkGetDeviceQueue(device, indices.graphicsFamily, 0, &graphicsQueue);
		vkGetDeviceQueue(device, indices.presentFamily, 0, &presentQueue);
	}

	void createSwapChain() {
		/* In Vulkan the swap chain needs to be created explicitly. Essentially a swap chain
		   is a queue of images that are waiting to be presented to the screen. The application
		   will aquire such an image to draw to it and then return to the queue. The details of
		   how this works and the conditions for presenting an image depend on the swap chain 
		   set up but the general purpose of the swap chain is to synchronize the presentation
		   of images with the refresh rate of the screen
		*/
		SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);

		/* Find the right settings for:
			- Surface format(color depth)
			- Presentation mode(conditions for "swapping" images to the screen)
			- Swap extent(resolution of images in swap chain)
		*/
		VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
		VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
		VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

		uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1; // + 1 for triple buffering
		// a value if maxImageCount of zero means there is no limit besides memory requirements
		if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
			imageCount = swapChainSupport.capabilities.maxImageCount;
		}

		VkSwapchainCreateInfoKHR createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.surface = surface;

		createInfo.minImageCount = imageCount;
		createInfo.imageFormat = surfaceFormat.format;
		createInfo.imageColorSpace = surfaceFormat.colorSpace;
		createInfo.imageExtent = extent;
		createInfo.imageArrayLayers = 1; // always 1 unless its a stereoscopic 3D application
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
		uint32_t queueFamilyIndices[] = { (uint32_t)indices.graphicsFamily, (uint32_t)indices.presentFamily };

		// There are two ways to handle swap images that will be used across multiple queue families:
		// In the first one an image is only by only one queue at a time and in the second one 
		// the image can be used across without ownership transfers
		if (indices.graphicsFamily != indices.presentFamily) {
			createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			createInfo.queueFamilyIndexCount = 2;
			createInfo.pQueueFamilyIndices = queueFamilyIndices;
		}
		else {
			createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		}
		// line below specifies that there is no need for an image transformation
		createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
		// line below says that we don't care about alpha (no need to blend with other windows)
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		createInfo.presentMode = presentMode;
		// don't care about color of pixels obscured by other windows
		createInfo.clipped = VK_TRUE;

		VkSwapchainKHR oldSwapChain = swapChain;
		// We need to pass the previous swap chain object in the oldSwapchain parameter
		// to indicate that we intend to replace it if this is a recreate swap chain scenario
		createInfo.oldSwapchain = oldSwapChain;

		VkSwapchainKHR newSwapChain;
		if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &newSwapChain) != VK_SUCCESS) {
			throw std::runtime_error("failed to create swap chain!");
		}
		// destroy the old swap chain and replace the handle with the handle of the new swap chain.
		swapChain = newSwapChain;

		vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
		// Retrieve the swap chain images and remember their format and extent
		swapChainImages.resize(imageCount);
		vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());

		swapChainImageFormat = surfaceFormat.format;
		swapChainExtent = extent;
	}

	void createImageViews() {
		// the resize function initializes all of the list items with the right deleter
		swapChainImageViews.resize(swapChainImages.size(), VDeleter<VkImageView>{device, vkDestroyImageView});

		for (uint32_t i = 0; i < swapChainImages.size(); i++) {
			createImageView(swapChainImages[i], swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, swapChainImageViews[i]);
		}
	}

	void createRenderPass() {

		/* 
		In Vulkan, a render pass represents(or describes) a set of framebuffer attachments(images) required for 
		drawing operations and a collection of subpasses that drawing operations will be ordered into. It is a 
		construct that collects all color, depth and stencil attachments and operations modifying them in such 
		a way that driver does not have to deduce this information by itself what may give substantial optimization 
		opportunities on some GPUs. A subpass consists of drawing operations that use(more or less) the same attachments.
		Each of these drawing operations may read from some input attachments and render data into some other
		(color, depth, stencil) attachments. A render pass also describes the dependencies between these attachments: 
		in one subpass we perform rendering into the texture, but in another this texture will be used as a source of 
		data(that is, it will be sampled from).All this data help the graphics hardware optimize drawing operations.
		*/
		VkAttachmentDescription colorAttachment = {}; // We may have multiple attachments per render pass
		colorAttachment.format = swapChainImageFormat;
		colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		// We have the following choices for loadOp:
		// VK_ATTACHMENT_LOAD_OP_LOAD: Preserve the existing contents of the attachment
		// VK_ATTACHMENT_LOAD_OP_CLEAR : Clear the values to a constant at the start
		// VK_ATTACHMENT_LOAD_OP_DONT_CARE : Existing contents are undefined; we don't care about them
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		// There are only two possibilities for the storeOp :
		// VK_ATTACHMENT_STORE_OP_STORE: Rendered contents will be stored in memory and can be read later
		// VK_ATTACHMENT_STORE_OP_DONT_CARE : Contents of the framebuffer will be undefined after the rendering operation
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		// Textures and framebuffers in Vulkan are represented by VkImage objects with a certain pixel format, 
		// however the layout of the pixels in memory can change based on what you're trying to do with an image.
		// Some of the most common layouts are :
		// VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL: Images used as color attachment
		// VK_IMAGE_LAYOUT_PRESENT_SRC_KHR : Images to be presented in the swap chain
		// VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL : Images to be used as destination for a memory copy operation
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		// The format should be the same as the depth image itself. This time we don't care about storing 
		// the depth data (storeOp), because it will not be used after drawing has finished. This may allow 
		// the hardware to perform additional optimizations. The layout of the image will not change during 
		// rendering, so the initialLayout and finalLayout are the same.
		VkAttachmentDescription depthAttachment = {};
		depthAttachment.format = findDepthFormat();
		depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		// Every subpass references one or more of the attachments that we've described.
		// These references are themselves VkAttachmentReference
		VkAttachmentReference colorAttachmentRef = {};
		colorAttachmentRef.attachment = 0;
		// The layout specifies which layout we would like the attachment to have during a subpass that uses this reference.
		// Vulkan will automatically transition the attachment to this layout when the subpass is started
		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		// Add a reference to the attachment for the first (and only) subpass:
		VkAttachmentReference depthAttachmentRef = {};
		depthAttachmentRef.attachment = 1;
		depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		// A single render pass can consist of multiple subpasses. Subpasses are subsequent rendering operations 
		// that depend on the contents of framebuffers in previous passes, for example a sequence of post - processing 
		// effects that are applied one after another. If you group these rendering operations into one render pass, 
		// then Vulkan is able to reorder the operations and conserve memory bandwidth for possibly better performance.
		// For our very first triangle, however, we'll stick to a single subpass.
		VkSubpassDescription subPass = {};
		subPass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS; // Vulkan will support compute here so need to be explicit
		subPass.colorAttachmentCount = 1;
		subPass.pColorAttachments = &colorAttachmentRef;
		subPass.pDepthStencilAttachment = &depthAttachmentRef;

		// The index of the attachment in this array is directly referenced from the fragment shader with the 
		// layout(location = 0) out vec4 outColor directive!
		// The following other types of attachments can be referenced by a subpass :
		// pInputAttachments: Attachments that are read from a shader
		// pResolveAttachments : Attachments used for multisampling color attachments
		// pDepthStencilAttachment : Attachments for depth and stencil data
		// pPreserveAttachments : Attachments that are not used by this subpass, but for which the data must be preserved

		// Remember that the subpasses in a render pass automatically take care of image layout transitions.
		// These transitions are controlled by subpass dependencies, which specify memory and execution dependencies between 
		// subpasses.

		// There are two built - in dependencies that take care of the transition at the start of the render pass and at the end 
		// of the render pass, but the former does not occur at the right time. It assumes that the transition occurs at the start 
		// of the pipeline, but we haven't acquired the image yet at that point! The image is not ready until the 
		// VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT stage. Therefore we need to override this dependency with our own dependency.
		VkSubpassDependency dependency = {};
		// The first two fields specify the indices of the dependency and the dependent subpass
		// The special value VK_SUBPASS_EXTERNAL refers to the implicit subpass before or after the render pass depending on 
		// whether it is specified in srcSubpass or dstSubpass. The index 0 refers to our subpass, which is the first and only 
		// one. The dstSubpass must always be higher than srcSubpass to prevent cycles in the dependency graph.
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		// The next two fields specify the operations to wait on and the stages in which these operations occur. We need to wait 
		// for the swap chain to finish reading from the image before we can access it.This reading happens in the last pipeline stage.
		dependency.srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		dependency.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		// The operations that should wait on this are in the color attachment stage and involve the reading and writing of the 
		// color attachment. These settings will prevent the transition from happening until it's actually necessary (and allowed): 
		// when we want to start writing colors to it.
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		std::array<VkAttachmentDescription, 2> attachments = { colorAttachment, depthAttachment };
		VkRenderPassCreateInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = attachments.size();
		renderPassInfo.pAttachments = attachments.data();
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subPass;
		renderPassInfo.dependencyCount = 1;
		renderPassInfo.pDependencies = &dependency;

		if (vkCreateRenderPass(device, &renderPassInfo, nullptr, renderPass.replace()) != VK_SUCCESS) {
			throw std::runtime_error("failed to create render pass!");
		}
	}

	void createGraphicsPipeline() {
		auto vertShaderCode = readFile("shaders/vert.spv");
		auto fragShaderCode = readFile("shaders/frag.spv");

		// Shader module objects are only required during the pipeline creation process
		VDeleter<VkShaderModule> vertShaderModule{ device, vkDestroyShaderModule };
		VDeleter<VkShaderModule> fragShaderModule{ device, vkDestroyShaderModule };
		createShaderModule(vertShaderCode, vertShaderModule);
		createShaderModule(fragShaderCode, fragShaderModule);

		VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
		vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertShaderStageInfo.module = vertShaderModule;
		vertShaderStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
		fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragShaderStageInfo.module = fragShaderModule;
		fragShaderStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };
		// End of programmable pipeline stages configuration
		// Beginning of fixed function pipeline stages

		// describes the format of the vertex data that will be passed to the vertex shader
		VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		auto bindingDescription = VertexBuffer::Vertex::getBindingDescription();
		auto attributeDescriptions = VertexBuffer::Vertex::getAttributeDescriptions();
		// spacing between data and whether data is per-vertex or per-instance
		vertexInputInfo.vertexBindingDescriptionCount = 1;
		// type of the attributes passed to the vertex shader, which binding to load them from and at which offset
		vertexInputInfo.vertexAttributeDescriptionCount = attributeDescriptions.size();
		vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
		vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

		// input assembly describes two things: what kind of geometry will be drawn from the vertices and 
		// if primitive restart should be enabled
		VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
		inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		inputAssembly.primitiveRestartEnable = VK_FALSE;

		VkViewport viewport = {};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = (float)swapChainExtent.width;
		viewport.height = (float)swapChainExtent.height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		VkRect2D scissor = {};
		scissor.offset = { 0, 0 };
		scissor.extent = swapChainExtent;

		VkPipelineViewportStateCreateInfo viewportState = {};
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = 1;
		viewportState.pViewports = &viewport;
		viewportState.scissorCount = 1;
		viewportState.pScissors = &scissor;

		// Rasterizer
		VkPipelineRasterizationStateCreateInfo rasterizer = {};
		rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizer.depthClampEnable = VK_FALSE; // set to VK_TRUE, then fragments that are beyond the near and far planes are clamped to them as opposed to discarding them
		rasterizer.rasterizerDiscardEnable = VK_FALSE; // this basically discards any output to the framebuffer
		rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizer.lineWidth = 1.0f;
		rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
		//rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
		// Because of the Y - flip we did in the projection matrix, the vertices are now being drawn in clockwise order instead of counter - clockwise 
		// order. This causes backface culling to kick in and prevents any geometry from being drawn.
		rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		rasterizer.depthBiasEnable = VK_FALSE;

		// Multisampling (anti-aliasing) is turned off in this sample
		VkPipelineMultisampleStateCreateInfo multisampling = {};
		multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampling.sampleShadingEnable = VK_FALSE;
		multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		
		VkPipelineDepthStencilStateCreateInfo depthStencil = {};
		depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencil.depthTestEnable = VK_TRUE;
		// depthWriteEnable field specifies if the new depth of fragments that pass the depth test should actually 
		// be written to the depth buffer.This is useful for drawing transparent objects. They should be compared 
		// to the previously rendered opaque objects, but not cause further away transparent objects to not be drawn.
		depthStencil.depthWriteEnable = VK_TRUE;
		depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
		depthStencil.depthBoundsTestEnable = VK_FALSE;
		//depthStencil.minDepthBounds = 0.0f; // Optional
		//depthStencil.maxDepthBounds = 1.0f; // Optional
		depthStencil.stencilTestEnable = VK_FALSE;

		/* Color blending pseudo-code
		if (blendEnable) {
			finalColor.rgb = (srcColorBlendFactor * newColor.rgb) <colorBlendOp> (dstColorBlendFactor * oldColor.rgb);
			finalColor.a = (srcAlphaBlendFactor * newColor.a) <alphaBlendOp> (dstAlphaBlendFactor * oldColor.a);
		}
		else {
			finalColor = newColor;
		}
		finalColor = finalColor & colorWriteMask;
		*/
		VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
		colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		colorBlendAttachment.blendEnable = VK_FALSE;
		// To implement alpha blending like this
		// finalColor.rgb = newAlpha * newColor + (1 - newAlpha) * oldColor;
		// finalColor.a = newAlpha.a;
		// do following:
		//colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
		//colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
		//colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
		//colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
		//colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
		//colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

		VkPipelineColorBlendStateCreateInfo colorBlending = {};
		colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlending.logicOpEnable = VK_FALSE; // this will (override) disable the above method of color blending
		colorBlending.logicOp = VK_LOGIC_OP_COPY;
		colorBlending.attachmentCount = 1;
		colorBlending.pAttachments = &colorBlendAttachment;
		colorBlending.blendConstants[0] = 0.0f;
		colorBlending.blendConstants[1] = 0.0f;
		colorBlending.blendConstants[2] = 0.0f;
		colorBlending.blendConstants[3] = 0.0f;

		// This structure is used to specify the layout of shader uniforms during pipeline creation
		VkDescriptorSetLayout setLayouts[] = { descriptorSetLayout };
		// It is actually possible to bind multiple descriptor sets. You need to specify a descriptor 
		// layout for each descriptor set when creating the pipeline layout. Shaders can then reference 
		// specific descriptor sets like this:
		// layout(set = 0, binding = 0) uniform UniformBufferObject { ... }
		// You can use this feature to put descriptors that vary per - object and descriptors that are 
		// shared into separate descriptor sets.In that case you avoid rebinding most of the descriptors 
		// across draw calls which is potentially more efficient.
		VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 1;
		pipelineLayoutInfo.pSetLayouts = setLayouts;

		if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, pipelineLayout.replace()) != VK_SUCCESS) {
			throw std::runtime_error("failed to create pipeline layout!");
		}

		// Note that a render pass has been created at this point prior to finally
		// creating the pipeline object
		VkGraphicsPipelineCreateInfo pipelineInfo = {};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.stageCount = 2;
		pipelineInfo.pStages = shaderStages;
		pipelineInfo.pVertexInputState = &vertexInputInfo;
		pipelineInfo.pInputAssemblyState = &inputAssembly;
		pipelineInfo.pViewportState = &viewportState;
		pipelineInfo.pRasterizationState = &rasterizer;
		pipelineInfo.pMultisampleState = &multisampling;
		pipelineInfo.pDepthStencilState = &depthStencil;
		pipelineInfo.pColorBlendState = &colorBlending;
		pipelineInfo.layout = pipelineLayout;
		pipelineInfo.renderPass = renderPass;
		pipelineInfo.subpass = 0;
		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

		/*
		  In conclusion the graphics pipeline consists of:
		   - Shader stages : the shader modules that define the functionality of the programmable stages of the graphics pipeline
		   - Fixed - function state : all of the structures that define the fixed - function stages of the pipeline, like input 
		     assembly, rasterizer, viewport and color blending
		   - Pipeline layout : the uniform and push values referenced by the shader that can be updated at draw time
		   - Render pass : the attachments referenced by the pipeline stages and their usage
		 */
		if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, graphicsPipeline.replace()) != VK_SUCCESS) {
			throw std::runtime_error("failed to create graphics pipeline!");
		}
	}

	void createFramebuffers() {
		/* 
		   We have created a render pass. It describes all attachments and all subpasses used during the render pass.
		   But this description is quite abstract. We have specified formats of all attachments and described how attachments 
		   will be used by each subpass. But we didn’t specify WHAT attachments we will be using or , in other words, what images
		   will be (bound) used as these attachments. This is done through a framebuffer.

		   A framebuffer describes specific images that the render pass operates on. This separation of render pass and framebuffer 
		   gives us some additional flexibility. We can use the given render pass with different framebuffers and a given framebuffer 
		   with different render passes, if they are compatible, meaning that they operate in a similar fashion on images of similar 
		   types and usages.

		   Before we can create a framebuffer, we must create image views for each image used as a framebuffer and render pass attachment. 
		   In Vulkan, not only in the case of framebuffers, but in general, we don’t operate on images themselves. Images are not accessed 
		   directly.

		   The image that we have to use as attachment depends on which image the swap chain returns when we retrieve one for presentation. 
		   That means that we have to create a framebuffer for all of the images in the swap chain and use the one that corresponds to the 
		   retrieved image at drawing time.
		*/
		swapChainFramebuffers.resize(swapChainImageViews.size(), VDeleter<VkFramebuffer>{device, vkDestroyFramebuffer});

		// We'll then iterate through the image views and create framebuffers from them
		for (size_t i = 0; i < swapChainImageViews.size(); i++) {
			std::array<VkImageView, 2> attachments = {
				// The color attachment differs for every swap chain image, but the same depth image can be used by all 
				// of them because only a single subpass is running at the same time due to our semaphores.
				swapChainImageViews[i],
				depthImageView
			};

			VkFramebufferCreateInfo framebufferInfo = {};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass = renderPass;
			framebufferInfo.attachmentCount = attachments.size();
			framebufferInfo.pAttachments = attachments.data();
			framebufferInfo.width = swapChainExtent.width;
			framebufferInfo.height = swapChainExtent.height;
			framebufferInfo.layers = 1;

			if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, swapChainFramebuffers[i].replace()) != VK_SUCCESS) {
				throw std::runtime_error("failed to create framebuffer!");
			}
		}
	}

	void createDepthResources() {
		VkFormat depthFormat = findDepthFormat();

		createImage(swapChainExtent.width, swapChainExtent.height, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depthImage, depthImageMemory);
		createImageView(depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, depthImageView);

		// It still needs to be transitioned to a layout that is suitable for depth attachment usage.
		// We could do this in the render pass like the color attachment, but here I've chosen to use 
		// a pipeline barrier because the transition only needs to happen once:
		transitionImageLayout(depthImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
		// The undefined layout can be used as initial layout, because there are no existing depth image contents that matter.
	}

	VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) {
		for (VkFormat format : candidates) {
			VkFormatProperties props;
			vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);

			// The VkFormatProperties struct contains three fields :
		    // linearTilingFeatures: Use cases that are supported with linear tiling
			// optimalTilingFeatures : Use cases that are supported with optimal tiling
			// bufferFeatures : Use cases that are supported for buffers
			// Only the first two are relevant here, and the one we check depends on the tiling parameter of the function
			if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
				return format;
			}
			else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
				return format;
			}
		}

		throw std::runtime_error("failed to find supported format!");
	}

	VkFormat findDepthFormat() {
		// Unlike the texture image, we don't necessarily need a specific format, because we won't be 
		// directly accessing the texels from the program. It just needs to have a reasonable accuracy, 
		// at least 24 bits is common in real - world applications. There are several formats that fit 
		//this requirement:
		// VK_FORMAT_D32_SFLOAT: 32 - bit float for depth
		// VK_FORMAT_D32_SFLOAT_S8_UINT : 32 - bit signed float for depth and 8 bit stencil component
		// VK_FORMAT_D24_UNORM_S8_UINT : 24 - bit float for depth and 8 bit stencil component
		// Also an image usage appropriate for a depth attachment, optimal tiling and device local memory.

		// Therefore provide a list of candidate formats in order from most desirable to least desirable, 
		// and check which is the first one that is supported :
		return findSupportedFormat(
		{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
			VK_IMAGE_TILING_OPTIMAL,
			VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
		);
	}

	void createTextureImage() {

		//decode
		unsigned int texWidth, texHeight, texChannels;
		std::vector<unsigned char> pixels;
		std::string filename = "textures/texture.png";
		unsigned error = lodepng::decode(pixels, texWidth, texHeight, filename);

		//if there's an error, display it
		if (error) {
			std::cout << "decoder error " << error << ": " << lodepng_error_text(error) << std::endl;
			throw std::runtime_error("failed to load texture image!");
		}

		//the pixels are now in the vector "image", 4 bytes per pixel, ordered RGBARGBA..., use it as texture, draw it, ...
		VkDeviceSize imageSize = texWidth * texHeight * 4;

		// Image objects before were automatically created by the swap chain extension. This time we'll have to 
		// create one by ourselves. Creating an image and filling it with data is very similar to vertex buffer creation. 
		// You create a VkImage, query its memory requirements, allocate device memory, bind the memory to the image, and 
		// finally map the memory to upload the pixel data. We'll use a staging and final image again, to make sure that 
		// the texture image itself ends up in fast device local memory. There is a command to copy the contents of images 
		// similar to vkCmdCopyBuffer.

		VDeleter<VkImage> stagingImage{ device, vkDestroyImage };
		VDeleter<VkDeviceMemory> stagingImageMemory{ device, vkFreeMemory };
		// Remember that we need the memory to be host visible to be able to use vkMapMemory, so you should specify 
		// that property when looking for the right memory type.
		createImage(texWidth, texHeight, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_LINEAR, VK_IMAGE_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingImage, stagingImageMemory);

		void* data;
		vkMapMemory(device, stagingImageMemory, 0, imageSize, 0, &data);
		memcpy(data, pixels.data(), (size_t)imageSize);
		vkUnmapMemory(device, stagingImageMemory);

		// The dimensions of the image should be the same as the staging image. The formats should also be compatible, 
		// because the command simply copies the raw image data. Two color formats are compatible if they have the same 
		// number of bytes per pixel. Depth / stencil formats, which we'll see in one of the next chapters, need to be 
		// exactly equal. The tiling mode on the other hand does not need to be the same. The texture image will be used 
		// as the destination in the transfer, and we want to be able to sample texels from it in the shader. The 
		// VK_IMAGE_USAGE_SAMPLED_BIT flag is necessary to allow that. The memory of the image should be device local for 
		// best performance, just like the vertex buffer.
		createImage(texWidth, texHeight, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textureImage, textureImageMemory);

		// All of the helper functions that submit commands so far have been set up to execute synchronously by waiting 
		// for the queue to become idle. For practical applications it is recommended to combine these operations in a 
		// single command buffer and execute them asynchronously for higher throughput, especially the transitions and 
		// copy in the createTextureImage function
		transitionImageLayout(stagingImage, VK_IMAGE_LAYOUT_PREINITIALIZED, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
		transitionImageLayout(textureImage, VK_IMAGE_LAYOUT_PREINITIALIZED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		copyImage(stagingImage, textureImage, texWidth, texHeight);

		transitionImageLayout(textureImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		// In this tutorial we used another image as staging resource for the texture, but it's also possible to use 
		// a buffer and copy pixels from it using vkCmdCopyBufferToImage. It is recommended to use this approach for 
		// improved performance on some hardware if you need to update the data in an image often.
	}

	void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VDeleter<VkImage>& image, VDeleter<VkDeviceMemory>& imageMemory) {
		VkImageCreateInfo imageInfo = {};
		// The image type, specified in the imageType field, tells Vulkan with that kind of coordinate 
		// system the texels in the image are going to be addressed. It is possible to create 1D, 2D and 3D images.
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.imageType = VK_IMAGE_TYPE_2D;
		// The extent field specifies the dimensions of the image, basically how many texels there are on each axis.
		// That's why depth must be 1 instead of 0. Our texture will not be an array and we won't be using mipmapping 
		// for now.
		imageInfo.extent.width = width;
		imageInfo.extent.height = height;
		imageInfo.extent.depth = 1;
		imageInfo.mipLevels = 1;
		imageInfo.arrayLayers = 1;
		imageInfo.format = format;
		// The tiling field can have one of two values :
		// VK_IMAGE_TILING_LINEAR: Texels are laid out in row - major order like our pixels array
		// VK_IMAGE_TILING_OPTIMAL : Texels are laid out in an implementation defined order for optimal access
		// If you want to be able to directly access texels in the memory of the image, then you must use 
		// VK_IMAGE_TILING_LINEAR. Also, keep in mind that the tiling cannot be changed at a later time.
		imageInfo.tiling = tiling;
	    // VK_IMAGE_LAYOUT_UNDEFINED: Not usable by the GPU and the very first transition will discard the texels.
		// VK_IMAGE_LAYOUT_PREINITIALIZED : Not usable by the GPU, but the first transition will preserve the texels.
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
		// The usage field has the same semantics as the one during buffer creation
		imageInfo.usage = usage;
		imageInfo.samples = VK_SAMPLE_COUNT_1_BIT; // Relevant only for multisampling.
		// The staging image will only be used by one queue family : the one that supports transfer operations.
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		if (vkCreateImage(device, &imageInfo, nullptr, image.replace()) != VK_SUCCESS) {
			throw std::runtime_error("failed to create image!");
		}

		// Allocating memory for an image works in exactly the same way as allocating memory for a buffer
		VkMemoryRequirements memRequirements;
		vkGetImageMemoryRequirements(device, image, &memRequirements);

		VkMemoryAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

		if (vkAllocateMemory(device, &allocInfo, nullptr, imageMemory.replace()) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate image memory!");
		}

		vkBindImageMemory(device, image, imageMemory, 0);
	}

	void transitionImageLayout(VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout) {

		// vkCmdCopyImage requires the images to be in the right layout first.
		VkCommandBuffer commandBuffer = beginSingleTimeCommands();

		// One of the most common ways to perform layout transitions is using an 
		// image memory barrier. A pipeline barrier like that is generally used to 
		// synchronize access to resources, like ensuring that a write to a buffer 
		// completes before reading from it, but it can also be used to transition 
		// image layouts and transfer queue family ownership when VK_SHARING_MODE_EXCLUSIVE 
		// is used. There is an equivalent buffer memory barrier to do this for buffers.
		VkImageMemoryBarrier barrier = {};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.oldLayout = oldLayout;
		barrier.newLayout = newLayout;
		// If you are using the barrier to transfer queue family ownership, then these two 
		// fields should be the indices of the queue families
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		// The image and subresourceRange specify the image that is affected and the specific 
		// part of the image. Our image is not an array and does not mipmapping levels, so only 
		// one level and layer are specified.
		barrier.image = image;

		if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		}
		else {
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		}

		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = 1;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;

		// You must specify which types of operations that involve the resource must happen before the barrier, 
		// and which operations that involve the resource must wait on the barrier. We need to do that despite 
		// already using vkQueueWaitIdle to manually synchronize.
		// You do that through transition barrier masks
		// There are three transitions we need to handle :

		// Preinitialized -> transfer source : transfer reads should wait on host writes
		// Preinitialized -> transfer destination : transfer writes should wait on host writes
		// Transfer destination -> shader reading : shader reads should wait on transfer writes

		if (oldLayout == VK_IMAGE_LAYOUT_PREINITIALIZED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL) {
			barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_PREINITIALIZED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
			barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		}
		else {
			throw std::invalid_argument("unsupported layout transition!");
		}

		// The first parameter specifies in which pipeline stage the operations occur that should happen 
		// before the barrier. The second parameter specifies the pipeline stage in which operations will 
		// wait on the barrier. We want it to happen immediately, so we're going with the top of the pipeline.
		// The third parameter is either 0 or VK_DEPENDENCY_BY_REGION_BIT. The latter turns the barrier into 
		// a per - region condition. That means that the implementation is allowed to already begin reading 
		// from the parts of a resource that were written so far, for example.
		vkCmdPipelineBarrier(
			commandBuffer,
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
			0,
			0, nullptr, // memory barriers
			0, nullptr, // buffer memory barriers
			1, &barrier // image memory barriers
		);

		endSingleTimeCommands(commandBuffer);
	}

	void copyImage(VkImage srcImage, VkImage dstImage, uint32_t width, uint32_t height) {

		VkCommandBuffer commandBuffer = beginSingleTimeCommands();

		VkImageSubresourceLayers subResource = {};
		subResource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		subResource.baseArrayLayer = 0;
		subResource.mipLevel = 0;
		subResource.layerCount = 1;

		// Just like with buffers, you need to specify which part of the image needs to be copied 
		// to which part of the other image.This happens through VkImageCopy structs
		VkImageCopy region = {};
		region.srcSubresource = subResource;
		region.dstSubresource = subResource;
		region.srcOffset = { 0, 0, 0 };
		region.dstOffset = { 0, 0, 0 };
		region.extent.width = width;
		region.extent.height = height;
		region.extent.depth = 1;

		// Image copy operations are enqueued using the vkCmdCopyImage
		vkCmdCopyImage(
			commandBuffer,
			// These two pairs of parameters specify the source image / layout and destination image / layout.
			// Assuming here that they've been previously transitioned to the optimal transfer layouts
			srcImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1, &region
		);

		endSingleTimeCommands(commandBuffer);
	}

	void createTextureImageView() {
		createImageView(textureImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT, textureImageView);
	}

	void createTextureSampler() {
		// Textures are usually accessed through samplers, which will apply 
		// filtering and transformations to compute the final color that is retrieved.
		VkSamplerCreateInfo samplerInfo = {};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		// modes available: VK_FILTER_NEAREST and VK_FILTER_LINEAR
		samplerInfo.magFilter = VK_FILTER_LINEAR;
		samplerInfo.minFilter = VK_FILTER_LINEAR;
		// The addressing mode can be specified per axis using the addressMode fields.
		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		// There is no reason not to use this unless performance is a concern
		samplerInfo.anisotropyEnable = VK_TRUE;
		samplerInfo.maxAnisotropy = 16;
		// The borderColor field specifies which color is returned when sampling beyond 
		// the image with clamp to border addressing mode.
		samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		// If this field is VK_TRUE, then you can simply use coordinates within the[0, texWidth) 
		// and [0, texHeight) range. If it is VK_FALSE, then the texels are addressed using the
		// [0, 1) range on all axes.
		samplerInfo.unnormalizedCoordinates = VK_FALSE;
		// If a comparison function is enabled, then texels will first be compared to a value, 
		// and the result of that comparison is used in filtering operations. This is mainly used 
		// for percentage - closer filtering on shadow maps.
		samplerInfo.compareEnable = VK_FALSE;
		samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

		// Note the sampler does not reference a VkImage anywhere. The sampler is a distinct object 
		// that provides an interface to extract colors from a texture. It can be applied to any image 
		// you want, whether it is 1D, 2D or 3D. This is different from many older APIs, which combined 
		// texture images and filtering into a single state.
		if (vkCreateSampler(device, &samplerInfo, nullptr, textureSampler.replace()) != VK_SUCCESS) {
			throw std::runtime_error("failed to create texture sampler!");
		}
	}

	void createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, VDeleter<VkImageView>& imageView) {

		// We've seen before, with the swap chain images and the framebuffer, 
		// that images are accessed through image views rather than directly. 
		// We will also need to create such an image view for the texture image.

		VkImageViewCreateInfo viewInfo = {};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image = image;
		// 2D texture
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format = format;
		// stick to default mapping  (you could for example map all of the channels to red in a monochrome texture)
		viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		// the subresourceRange field describes the image's purpose and how to access
		// For the render target case for example, this will be used as a color target without any mipmapping or layers
		viewInfo.subresourceRange.aspectMask = aspectFlags;
		viewInfo.subresourceRange.baseMipLevel = 0;
		viewInfo.subresourceRange.levelCount = 1;
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.layerCount = 1;

		if (vkCreateImageView(device, &viewInfo, nullptr, imageView.replace()) != VK_SUCCESS) {
			throw std::runtime_error("failed to create texture image view!");
		}
		// An image view is sufficient to start using an image as a texture but not enough for using it
		// as a render target. That requires one more step of indirection known as a framebuffer
	}

	void createVertexBuffer() {

		// The most optimal memory has the VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT flag and is usually 
		// not accessible by the CPU on dedicated graphics cards. 
		VkDeviceSize bufferSize = sizeof(VertexBuffer::vertices[0]) * VertexBuffer::vertices.size();

		// Create two vertex buffers. One staging buffer in CPU accessible memory to upload the data 
		// from the vertex array to, and the final vertex buffer in device local memory. 
		// We'll then use a buffer copy command to move the data from the staging buffer to the actual 
		// vertex buffer.
		VDeleter<VkBuffer> stagingBuffer{ device, vkDestroyBuffer };
		VDeleter<VkDeviceMemory> stagingBufferMemory{ device, vkFreeMemory };
		createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

		void* data;
		// This function allows us to access a region of the specified memory resource defined by an offset and size
		vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, VertexBuffer::vertices.data(), (size_t)bufferSize);
		vkUnmapMemory(device, stagingBufferMemory);
		// Unfortunately the driver may not immediately copy the data into the buffer memory, for example because of caching.
		// It is also possible that writes to the buffer are not visible in the mapped memory yet.
		// There are two ways to deal with that problem :
		// 	Use a memory heap that is host coherent, indicated with VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
		// Call vkFlushMappedMemoryRanges to after writing to the mapped memory, and call vkInvalidateMappedMemoryRanges 
		// before reading from the mapped memory

		createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexBufferMemory);
	    // VK_BUFFER_USAGE_TRANSFER_SRC_BIT: Buffer can be used as source in a memory transfer operation.
		// VK_BUFFER_USAGE_TRANSFER_DST_BIT : Buffer can be used as destination in a memory transfer operation.

		copyBuffer(stagingBuffer, vertexBuffer, bufferSize);
	}

	void createIndexBuffer() {

		// Very simmilar to createVertexBuffer above
		VkDeviceSize bufferSize = sizeof(VertexBuffer::indices[0]) * VertexBuffer::indices.size();

		VDeleter<VkBuffer> stagingBuffer{ device, vkDestroyBuffer };
		VDeleter<VkDeviceMemory> stagingBufferMemory{ device, vkFreeMemory };
		createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

		void* data;
		vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, VertexBuffer::indices.data(), (size_t)bufferSize);
		vkUnmapMemory(device, stagingBufferMemory);

		createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer, indexBufferMemory);
		copyBuffer(stagingBuffer, indexBuffer, bufferSize);
		// Driver developers recommend that you also store multiple buffers, like the vertex and index buffer, 
		// into a single VkBuffer and use offsets in commands like vkCmdBindVertexBuffers.The advantage is that 
		// your data is more cache friendly in that case, because it's closer together. It is even possible to 
		// reuse the same chunk of memory for multiple resources if they are not used during the same render 
		// operations, provided that their data is refreshed, of course. This is known as aliasing and some 
		// Vulkan functions have explicit flags to specify that you want to do this.
	}

	void createUniformBuffer() {
		// Allocate the buffers (descriptors).
		// We're going to write a separate function that updates the uniform buffer with a new transformation every frame
		// , so there will be no vkMapMemory and copyBuffer operations here and handles to the staging buffers need to be
		// kept.
		VkDeviceSize bufferSize = sizeof(UniformBuffer::UniformBufferObject);

		createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniformStagingBuffer, uniformStagingBufferMemory);
		createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, uniformBuffer, uniformBufferMemory);
	}

	void createDescriptorSetLayout() {
		// We need to provide details about every descriptor binding used in the shaders for pipeline 
		// creation, just like we had to do for every vertex attribute and its location index.
		VkDescriptorSetLayoutBinding uboLayoutBinding = {};
		// Every binding needs to be described through a VkDescriptorSetLayoutBinding struct.
		uboLayoutBinding.binding = 0;
		uboLayoutBinding.descriptorCount = 1;
		uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uboLayoutBinding.pImmutableSamplers = nullptr;
		// The first two fields specify the binding used in the shader and the type of descriptor, 
		// which is a uniform buffer. It is possible for a uniform buffer descriptor to be an array 
		// of data, and descriptorCount specifies the number of values in the array. This could be 
		// used to specify a transformation for each of the bones in a skeleton for skeletal animation, 
		// for example. Our MVP transformation is a single object, so we're using a descriptorCount of 1
		uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		// We also need to specify in which shader stages the descriptor is going to be referenced.
		
		// Create the binding layout for the combined image sampler.
		VkDescriptorSetLayoutBinding samplerLayoutBinding = {};
		samplerLayoutBinding.binding = 1;
		samplerLayoutBinding.descriptorCount = 1;
		samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		samplerLayoutBinding.pImmutableSamplers = nullptr;
		// Make sure to set the stageFlags to indicate that we intend to use the combined image sampler 
		// descriptor in the fragment shader.That's where the color of the fragment is going to be determined. 
		// It is possible to use texture sampling in the vertex shader, for example to dynamically deform a 
		// grid of vertices by a heightmap.
		samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		std::array<VkDescriptorSetLayoutBinding, 2> bindings = { uboLayoutBinding, samplerLayoutBinding };

		VkDescriptorSetLayoutCreateInfo layoutInfo = {};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = bindings.size();
		layoutInfo.pBindings = bindings.data();

		// This function accepts a simple VkDescriptorSetLayoutCreateInfo with the array of bindings
		if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, descriptorSetLayout.replace()) != VK_SUCCESS) {
			throw std::runtime_error("failed to create descriptor set layout!");
		}
		// A descriptor layout describes the type of descriptors that can be bound but a descriptor set will 
		// actually specify a VkBuffer resource to bind to the uniform buffer descriptor.
	}

	void createDescriptorPool() {
		// Descriptor sets can't be created directly, they must be allocated from a pool like command buffers
		std::array<VkDescriptorPoolSize, 2> poolSizes = {};
		poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSizes[0].descriptorCount = 1;
		poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		poolSizes[1].descriptorCount = 1;

		VkDescriptorPoolCreateInfo poolInfo = {};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = poolSizes.size();
		poolInfo.pPoolSizes = poolSizes.data();
		poolInfo.maxSets = 1;

		if (vkCreateDescriptorPool(device, &poolInfo, nullptr, descriptorPool.replace()) != VK_SUCCESS) {
			throw std::runtime_error("failed to create descriptor pool!");
		}
	}

	void createDescriptorSet() {
		// Descriptor sets, which will actually bind the VkBuffer to the uniform buffer descriptor 
		// so that the shader can access this transformation data.

		VkDescriptorSetLayout layouts[] = { descriptorSetLayout };
		VkDescriptorSetAllocateInfo allocInfo = {};
		// You need to specify the descriptor pool to allocate from, the number of descriptor sets to allocate, 
		// and the descriptor layout to base them on :
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = descriptorPool;
		allocInfo.descriptorSetCount = 1;
		allocInfo.pSetLayouts = layouts;

		// The call to vkAllocateDescriptorSets will allocate one descriptor set with one uniform buffer 
		// descriptor and one combined image sample descriptor.
		if (vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate descriptor set!");
		}

		// The descriptor set has been allocated now, but the descriptors within still need to be configured.
		// Descriptors that refer to buffers, like our uniform buffer descriptor, are configured with a VkDescriptorBufferInfo 
		// struct. This structure specifies the buffer and the region within it that contains the data for the descriptor.
		VkDescriptorBufferInfo bufferInfo = {};
		bufferInfo.buffer = uniformBuffer;
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(UniformBuffer::UniformBufferObject);

		// Bind the actual image and sampler resources to the descriptor in the descriptor set.
		VkDescriptorImageInfo imageInfo = {};
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo.imageView = textureImageView;
		imageInfo.sampler = textureSampler;

		// Update the descriptor to finalize its configuration

		std::array<VkWriteDescriptorSet, 2> descriptorWrites = {};

		descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[0].dstSet = descriptorSet;
		descriptorWrites[0].dstBinding = 0;
		descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		// It's possible to update multiple descriptors at once in an array, starting at index dstArrayElement. 
		// The descriptorCount field specifies how many array elements you want to update.
		descriptorWrites[0].descriptorCount = 1;
		// The last field references an array with descriptorCount structs that actually configure the descriptors.
		// It depends on the type of descriptor which one of the three you actually need to use.
		descriptorWrites[0].pBufferInfo = &bufferInfo;
		//descriptorWrite[0].pImageInfo = nullptr; // Optional
		//descriptorWrite[0].pTexelBufferView = nullptr; // Optional

		descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[1].dstSet = descriptorSet;
		descriptorWrites[1].dstBinding = 1;
		descriptorWrites[1].dstArrayElement = 0;
		descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrites[1].descriptorCount = 1;
		descriptorWrites[1].pImageInfo = &imageInfo;

		vkUpdateDescriptorSets(device, descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);

		/* A resource descriptor is a way for shaders to freely access resources like buffers and images.
		   We're going to set up a buffer that contains the transformation matrices and have the vertex shader 
		   access them through a descriptor. Usage of descriptors consists of three parts:
		   - Specify a descriptor layout during pipeline creation
		   - Allocate a descriptor set from a descriptor pool
		   - Bind the descriptor set during rendering

		   The descriptor layout specifies the types of resources that are going to be accessed by the pipeline, 
		   just like a render pass specifies the types of attachments that will be accessed. A descriptor set 
		   specifies the actual buffer or image resources that will be bound to the descriptors, just like a framebuffer 
		   specifies the actual image views to bind to render pass attachments. The descriptor set is then bound for the 
		   drawing commands just like the vertex buffers and framebuffer.
		*/

	}

	void createBuffer(
		VkDeviceSize size, 
		VkBufferUsageFlags usage, 
		VkMemoryPropertyFlags properties, 
		VDeleter<VkBuffer>& buffer, 
		VDeleter<VkDeviceMemory>& bufferMemory) 
	{
		VkBufferCreateInfo bufferInfo = {};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = size;
		bufferInfo.usage = usage;
		// Just like the images in the swap chain, buffers can also be owned by a specific queue 
		// family or be shared between multiple at the same time. The buffer will only be used 
		// from the graphics queue, so we can stick to exclusive access.
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		if (vkCreateBuffer(device, &bufferInfo, nullptr, buffer.replace()) != VK_SUCCESS) {
			throw std::runtime_error("failed to create buffer!");
		}

		// The buffer has been created, but it doesn't actually have any memory assigned to it yet. 
		// The first step of allocating memory for the buffer is to query its memory requirements using the 
		// aptly named vkGetBufferMemoryRequirements function.
		VkMemoryRequirements memRequirements;
		vkGetBufferMemoryRequirements(device, buffer, &memRequirements);
		// The VkMemoryRequirements struct has three fields :
		// size: The size of the required amount of memory in bytes, may differ from bufferInfo.size.
		// alignment : The offset in bytes where the buffer begins in the allocated region of memory, 
		// depends on bufferInfo.usage and bufferInfo.flags.
		// memoryTypeBits : Bit field of the memory types that are suitable for the buffer.

		VkMemoryAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

		// We now have a way to determine the right memory type, so we can actually allocate the memory 
		// by filling in the VkMemoryAllocateInfo structure.
		if (vkAllocateMemory(device, &allocInfo, nullptr, bufferMemory.replace()) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate buffer memory!");
		}

		// Associate the memory allocation with the buffer
		vkBindBufferMemory(device, buffer, bufferMemory, 0);
	}

	VkCommandBuffer beginSingleTimeCommands() {
		// Memory transfer operations are executed using command buffers, just like drawing commands. 
		// Therefore we must first allocate a temporary command buffer. 
		// TIP: You may wish to create a separate command pool for these kinds of short-lived buffers, 
		// because the implementation may be able to apply memory allocation optimizations. You should 
		// use the VK_COMMAND_POOL_CREATE_TRANSIENT_BIT flag during command pool generation in that case.
		// Not done here.
		VkCommandBufferAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = commandPool;
		allocInfo.commandBufferCount = 1;

		VkCommandBuffer commandBuffer;
		vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

		// Start recording the command buffer
		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		// The VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT flag that we used for the drawing command buffers 
		// is not necessary here, because we're only going to use the command buffer once and wait until the copy 
		// operation has finished executing. It's good practice to tell the driver about our intent using 
		// VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
		vkBeginCommandBuffer(commandBuffer, &beginInfo);

		return commandBuffer;
	}

	void endSingleTimeCommands(VkCommandBuffer commandBuffer) {

		// This command buffer only contains the copy command, so we can stop recording right after that.
		vkEndCommandBuffer(commandBuffer);

		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;

		// Unlike the draw commands, there are no events we need to wait on this time. We just want to 
		// execute the transfer on the buffers immediately. There are again two possible ways to wait on 
		// this transfer to complete.We could use a fence and wait with vkWaitForFences, or simply wait 
		// for the transfer queue to become idle with vkQueueWaitIdle. A fence would allow you to schedule 
		// multiple transfers simultaneously and wait for all of them complete, instead of executing one at 
		// a time.That may give the driver more opportunities to optimize.
		vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
		vkQueueWaitIdle(graphicsQueue);

		// Don't forget to clean up the command buffer used for the transfer operation
		vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
	}

	void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {

		VkCommandBuffer commandBuffer = beginSingleTimeCommands();

		VkBufferCopy copyRegion = {};
		copyRegion.size = size;
		vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

		endSingleTimeCommands(commandBuffer);	
	}

	uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
		// We need to combine the requirements of the buffer and our own application requirements to 
		// find the right type of memory to use.
		VkPhysicalDeviceMemoryProperties memProperties;
		// The VkPhysicalDeviceMemoryProperties structure has two arrays memoryTypes and memoryHeaps.
		// Memory heaps are distinct memory resources like dedicated VRAM and swap space in RAM for 
		// when VRAM runs out.The different types of memory exist within these heaps
		vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

		// find a memory type that is suitable for the buffer itself:
		for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
			// The typeFilter parameter will be used to specify the bit field of memory types that are suitable.
			// That means that we can find the index of a suitable memory type by simply iterating over them and 
			// checking if the corresponding bit is set to 1.
			if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
				// The properties define special features of the memory, like being able to map it 
				// so we can write to it from the CPU.This property is indicated with 
				// VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, but we also need to use the 
				// VK_MEMORY_PROPERTY_HOST_COHERENT_BIT property
				return i;
			}
		}

		throw std::runtime_error("failed to find suitable memory type!");
	}

	void createCommandPool() {
		// Each command pool can only allocate command buffers that are submitted on a single type of queue.
		QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice);

		VkCommandPoolCreateInfo poolInfo = {};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily;
		//poolInfo.flags = 0; // Optional, two possible flags described below
	    // VK_COMMAND_POOL_CREATE_TRANSIENT_BIT: Hint that command buffers are rerecorded with new commands very often
		// (may change memory allocation behavior)
		// VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT : Allow command buffers to be rerecorded individually, without 
		// this flag they all have to be reset together
		if (vkCreateCommandPool(device, &poolInfo, nullptr, commandPool.replace()) != VK_SUCCESS) {
			throw std::runtime_error("failed to create command pool!");
		}
	}

	void createCommandBuffers() {
		// If this is a case of swap chain recreation, then there is no need to recreate the command pool,
		// instead just delete the old command buffeers and recreate them
		if (commandBuffers.size() > 0) {
			vkFreeCommandBuffers(device, commandPool, commandBuffers.size(), commandBuffers.data());
		}

		// Because one of the drawing commands involves binding the right VkFramebuffer, 
		// we'll actually have to record a command buffer for every image in the swap chain once again
		commandBuffers.resize(swapChainFramebuffers.size());

		VkCommandBufferAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = commandPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		// The level parameter specifies if the allocated command buffers are primary or secondary command buffers.
		// VK_COMMAND_BUFFER_LEVEL_PRIMARY: Can be submitted to a queue for execution, but cannot be called from 
		// other command buffers.
		// VK_COMMAND_BUFFER_LEVEL_SECONDARY : Cannot be submitted directly, but can be called from primary command 
		// buffers.
		allocInfo.commandBufferCount = (uint32_t)commandBuffers.size();

		if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate command buffers!");
		}

		for (size_t i = 0; i < commandBuffers.size(); i++) {
			VkCommandBufferBeginInfo beginInfo = {};
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
			// Flags specify how we're going to use the command buffer
		    // VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT: The command buffer will be rerecorded 
			// right after executing it once.
			// VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT : This is a secondary command buffer 
			// that will be entirely within a single render pass.
			// VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT : The command buffer can be resubmitted 
			// while it is also already pending execution.

			// Start recording the commnad buffer
			vkBeginCommandBuffer(commandBuffers[i], &beginInfo);

			// Drawing starts by beginning a render pass
			VkRenderPassBeginInfo renderPassInfo = {};
			renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			renderPassInfo.renderPass = renderPass;
			renderPassInfo.framebuffer = swapChainFramebuffers[i];
			renderPassInfo.renderArea.offset = { 0, 0 };
			renderPassInfo.renderArea.extent = swapChainExtent;

			// The last two parameters define the clear values to use for VK_ATTACHMENT_LOAD_OP_CLEAR, 
			// which we used as load operation for the color attachment. 
			// Furthermore, because we now have multiple attachments with VK_ATTACHMENT_LOAD_OP_CLEAR, 
			// we also need to specify multiple clear values.

			std::array<VkClearValue, 2> clearValues = {};
			clearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
			clearValues[1].depthStencil = { 1.0f, 0 };

			renderPassInfo.clearValueCount = clearValues.size();
			renderPassInfo.pClearValues = clearValues.data();

			vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
			// VK_SUBPASS_CONTENTS_INLINE: The render pass commands will be embedded in the primary 
			// command buffer itself and no secondary command buffers will be executed.
		    // VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS: The render pass commands will be executed 
			// from secondary command buffers.

			// Bind the graphics pipeline
			vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

			// Bind vertex buffers to bindings
			VkBuffer vertexBuffers[] = { vertexBuffer };
			VkDeviceSize offsets[] = { 0 };

			vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, vertexBuffers, offsets);

			vkCmdBindIndexBuffer(commandBuffers[i], indexBuffer, 0, VK_INDEX_TYPE_UINT32);

			// Unlike vertex and index buffers, descriptor sets are not unique to graphics pipelines
			// .Therefore we need to specify if we want to bind descriptor sets to the graphics or compute pipeline.
			// The next parameter is the layout that the descriptors are based on. The next three parameters specify 
			// the index of the first descriptor set, the number of sets to bind, and the array of sets to bind.
			vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);

			// We have told vulkan which operations to execute in the graphics pipeline and which
			// attachment to use in the fragment shader, now draw the triangle
			vkCmdDrawIndexed(commandBuffers[i], VertexBuffer::indices.size(), 1, 0, 0, 0);

			
			//vkCmdDraw(commandBuffers[i], VertexBuffer::vertices.size(), 1, 0, 0);
			// instanceCount : Used for instanced rendering, use 1 if you're not doing that.
			// firstVertex : Used as an offset into the vertex buffer, defines the lowest value of gl_VertexIndex.
			// firstIntance : Used as an offset for instanced rendering, defines the lowest value of gl_InstanceIndex.

			vkCmdEndRenderPass(commandBuffers[i]);

			// Finish recording the command buffer
			if (vkEndCommandBuffer(commandBuffers[i]) != VK_SUCCESS) {
				throw std::runtime_error("failed to record command buffer!");
			}
		}
	}

	void createSemaphores() {
		VkSemaphoreCreateInfo semaphoreInfo = {};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, imageAvailableSemaphore.replace()) != VK_SUCCESS ||
			vkCreateSemaphore(device, &semaphoreInfo, nullptr, renderFinishedSemaphore.replace()) != VK_SUCCESS) {

			throw std::runtime_error("failed to create semaphores!");
		}
	}

	void updateUniformBuffer() {
		static auto startTime = std::chrono::high_resolution_clock::now();

		auto currentTime = std::chrono::high_resolution_clock::now();
		float time = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - startTime).count() / 1000.0f;

		UniformBuffer::UniformBufferObject ubo = {};
		// The model rotation will be a simple rotation around the Z - axis using the time variable :
		ubo.model = glm::rotate(glm::mat4(), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		// look at the geometry from above at a 45 degree angle
		ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		// perspective projection with a 45 degree vertical field - of - view
		ubo.proj = glm::perspective(glm::radians(45.0f), swapChainExtent.width / (float)swapChainExtent.height, 0.1f, 10.0f);
		// GLM was originally designed for OpenGL, where the Y coordinate of the clip coordinates is inverted.
		// The easiest way to compensate for that is to flip the sign on the scaling factor of the Y axis in the projection matrix
		ubo.proj[1][1] *= -1;

		void* data;
		vkMapMemory(device, uniformStagingBufferMemory, 0, sizeof(ubo), 0, &data);
		memcpy(data, &ubo, sizeof(ubo));
		vkUnmapMemory(device, uniformStagingBufferMemory);

		// Using a staging buffer and final buffer this way is not the most efficient way to pass frequently changing 
		// values to the shader.A more efficient way to pass a small buffer of data to shaders are push constants.
		copyBuffer(uniformStagingBuffer, uniformBuffer, sizeof(ubo));
	}

	void drawFrame() {
		uint32_t imageIndex;
		// First acquire an image from the swap chain
		// max 64 disables timeout, and specify the semaphore to signal when the presentation engine is finished using
		// the image so that we can start drawing on it
		VkResult result = vkAcquireNextImageKHR(device, swapChain, std::numeric_limits<uint64_t>::max(), imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

		if (result == VK_ERROR_OUT_OF_DATE_KHR) {
			// If the swap chain turns out to be out of date when attempting to acquire an image, 
			// then it is no longer possible to present to it. Therefore we should immediately recreate 
			// the swap chain and try again in the next drawFrame call.
			recreateSwapChain();
			return;
		}
		else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
			throw std::runtime_error("failed to acquire swap chain image!");
		}

		// Prepare the queue submission and synchronization
		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		// The first three parameters specify which semaphores to wait on before execution begins 
		// and in which stage(s) of the pipeline to wait. We want to wait with writing colors to 
		// the image until it's available, so we're specifying the stage of the graphics pipeline
		// that writes to the color attachment. That means that theoretically the implementation
		// can already start executing our vertex shader and such while the image is not available
		// yet. Each entry in the waitStages array corresponds to the semaphore with the same index
		// in pWaitSemaphores.
		VkSemaphore waitSemaphores[] = { imageAvailableSemaphore };
		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;

		// We should submit the command buffer that binds the swap chain image we just acquired
		// as color attachment
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffers[imageIndex];

		// The signalSemaphoreCount and pSignalSemaphores parameters specify which semaphores 
		// to signal once the command buffer(s) have finished execution
		VkSemaphore signalSemaphores[] = { renderFinishedSemaphore };
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;

		// Submit the command queue to the graphics queue
		if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS) {
			throw std::runtime_error("failed to submit draw command buffer!");
		}

		// The last step of drawing a frame is submitting the result back to the swap chain to have 
		// it eventually show up on the screen
		VkPresentInfoKHR presentInfo = {};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		// The first two parameters specify which semaphores to wait on before presentation can happen
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = signalSemaphores;

		VkSwapchainKHR swapChains[] = { swapChain };
		// The next two parameters specify the swap chains to present images to and the index of the 
		// image for each swap chain.This will almost always be a single one.
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapChains;
		presentInfo.pImageIndices = &imageIndex;

		// The vkQueuePresentKHR function submits the request to present an image to the swap chain
		result = vkQueuePresentKHR(presentQueue, &presentInfo);

		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
			recreateSwapChain();
		}
		else if (result != VK_SUCCESS) {
			throw std::runtime_error("failed to present swap chain image!");
		}
	}

	void createShaderModule(const std::vector<char>& code, VDeleter<VkShaderModule>& shaderModule) {
		VkShaderModuleCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = code.size();
		createInfo.pCode = (uint32_t*)code.data();

		if (vkCreateShaderModule(device, &createInfo, nullptr, shaderModule.replace()) != VK_SUCCESS) {
			throw std::runtime_error("failed to create shader module!");
		}
	}

	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
		// Ideally we want to use 32 bits per pixel in the BGRA order and work in the sRGB color space (gamma correction)
		// Ideally, the surface has no preferred format so we can specify what we want
		if (availableFormats.size() == 1 && availableFormats[0].format == VK_FORMAT_UNDEFINED) {
			return{ VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
		}
		// Otherwise, try to find the one we are looking for
		for (const auto& availableFormat : availableFormats) {
			if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
				return availableFormat;
			}
		}
		// If above fails the just settle for the first format available
		return availableFormats[0];
	}

	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> availablePresentModes) {
		/* This is the most important setting in the swap chain since it represents the actual conditions
		   for showing images on the screen. 4 possible modes are:
		   - VK_PRESENT_MODE_IMMEDIATE_KHR: Images submitted by your application are transferred to the screen 
		     right away, which may result in tearing.
           - VK_PRESENT_MODE_FIFO_KHR: The swap chain is a queue where the display takes an image from the front 
             of the queue on a vertical blank and the program inserts rendered images at the back of the queue. 
             If the queue is full then the program has to wait. This is most similar to vertical sync as found in modern games.
           - VK_PRESENT_MODE_FIFO_RELAXED_KHR: This mode only differs from the above one if the application is late and the 
             queue was empty at the last vertical blank. Instead of waiting for the next vertical blank, the image is transferred 
             right away when it finally arrives. This may result in visible tearing.
           - VK_PRESENT_MODE_MAILBOX_KHR: This is another variation of the FIFO mode. Instead of blocking the application when the queue
             is full, the images that are already queued are simply replaced with the newer ones. This mode can be used to implement triple
             buffering, which allows you to avoid tearing with significantly less latency issues than standard vertical sync that uses 
             double buffering.
		*/
		for (const auto& availablePresentMode : availablePresentModes) {
			if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
				return availablePresentMode;
			}
		}

		return VK_PRESENT_MODE_FIFO_KHR; // onlny implementation to be guaranteed
	}

	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
		/* The swap extent is the resolution of the swap chain images and it's almost always exactly equal to the resolution 
		   of the window that we're drawing to. The range of the possible resolutions is defined in the VkSurfaceCapabilitiesKHR 
		   structure. Vulkan tells us to match the resolution of the window by setting the width and height in the currentExtent 
		   member. However, some window managers do allow us to differ here and this is indicated by setting the width and height 
		   in currentExtent to a special value: the maximum value of uint32_t. In that case we'll pick the resolution that best 
		   matches the window within the minImageExtent and maxImageExtent bounds.
		*/
		if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
			return capabilities.currentExtent;
		}
		else {
			VkExtent2D actualExtent = { WIDTH, HEIGHT };

			actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
			actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));

			return actualExtent;
		}
	}

	SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device) {
		/* Just checking if a swap chain is available is not sufficient, because it may not actually be 
		   compatible with our window surface. Creating a swap chain also involves a lot more settings than 
		   instance and device creation, so we need to query for some more details before we're able to proceed.

           There are basically three kinds of properties we need to check:
           - Basic surface capabilities (min/max number of images in swap chain, min/max width and height of images)
           - Surface formats (pixel format, color space)
           - Available presentation modes
		*/
		SwapChainSupportDetails details;

		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

		uint32_t formatCount;
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

		if (formatCount != 0) {
			details.formats.resize(formatCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
		}

		uint32_t presentModeCount;
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

		if (presentModeCount != 0) {
			details.presentModes.resize(presentModeCount);
			vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
		}

		return details;
	}

	bool isDeviceSuitable(VkPhysicalDevice device) {

		// Example of how to query for properties and features
		/*
		VkPhysicalDeviceProperties deviceProperties;
		VkPhysicalDeviceFeatures deviceFeatures;
		vkGetPhysicalDeviceProperties(device, &deviceProperties);
		vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

		return deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
			deviceFeatures.geometryShader;
		*/
		QueueFamilyIndices indices = findQueueFamilies(device);

		bool extensionsSupported = checkDeviceExtensionSupport(device);

		bool swapChainAdequate = false;
		// Only query for chain support after verifying that the extension is available
		if (extensionsSupported) {
			SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
			// For now we are calling sufficient if there is at least one supported image format
			// and one supported presenation mode given to the window surface we have
			swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
		}

		return indices.isComplete() && extensionsSupported && swapChainAdequate;
	}

	bool checkDeviceExtensionSupport(VkPhysicalDevice device) {

		/* Since image presentation is heavily tied into the window system and the surfaces associated with windows, 
		   it is not actually part of the Vulkan core. You have to enable the VK_KHR_swapchain (VK_KHR_SWAPCHAIN_EXTENSION_NAME) 
		   device extension after querying for its support.
		*/
		uint32_t extensionCount;
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

		std::vector<VkExtensionProperties> availableExtensions(extensionCount);
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

		/* Decision was chosen to use a set of strings here to represent the unconfirmed required extensions. 
		   That way we can easily tick them off while enumerating the sequence of available extensions. Of course 
		   you can also use a nested loop like in checkValidationLayerSupport. The performance difference is 
		   irrelevant.
		*/
		std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());
		for (const auto& extension : availableExtensions) {
			requiredExtensions.erase(extension.extensionName);
		}

		return requiredExtensions.empty();
	}

	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) {

		// Interested in finding out if device supports at least
		// a graphics queue and a present queue
		QueueFamilyIndices indices;

		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

		int i = 0;
		for (const auto& queueFamily : queueFamilies) {
			if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
				indices.graphicsFamily = i;
			}

			/* Although the Vulkan implementation may support window system integration, that does not 
			   mean that every device in the system supports it.Therefore we need to extend isDeviceSuitable 
			   to ensure that a device can present images to the surface we created. Since the presentation 
			   is a queue - specific feature, the problem is actually about finding a queue family that supports 
			   presenting to the surface we created.*/
			VkBool32 presentSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

			if (queueFamily.queueCount > 0 && presentSupport) {
				indices.presentFamily = i;
			}

			if (indices.isComplete()) {
				break;
			}

			i++;
		}

		return indices;
	}

	std::vector<const char*> getRequiredExtensions() {
		std::vector<const char*> extensions;

		unsigned int glfwExtensionCount = 0;
		const char** glfwExtensions;
		// obtain required extension for vulkan to interface with our windowing 
		// system glfw
		glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

		for (unsigned int i = 0; i < glfwExtensionCount; i++) {
			extensions.push_back(glfwExtensions[i]);
		}

		// In order to relay the debug messages back to our application we need 
		// this extension in order to set up the get messages callback
		if (enableValidationLayers) {
			extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
		}

		return extensions;
	}

	bool checkValidationLayerSupport() {
		uint32_t layerCount;
		vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

		std::vector<VkLayerProperties> availableLayers(layerCount);
		vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

		// look for the existance of the lunar sdk standard validation layers
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

	static std::vector<char> readFile(const std::string& filename) {
		std::ifstream file(filename, std::ios::ate | std::ios::binary);

		if (!file.is_open()) {
			throw std::runtime_error("failed to open file!");
		}

		size_t fileSize = (size_t)file.tellg();
		std::vector<char> buffer(fileSize);

		file.seekg(0);
		file.read(buffer.data(), fileSize);

		file.close();

		return buffer;
	}

	// VKAPI_ATTR and VKAPI_CALL ensure that the function has the right signature for Vulkan to call it
	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objType, uint64_t obj, size_t location, int32_t code, const char* layerPrefix, const char* msg, void* userData) {
		std::cerr << "validation layer: " << msg << std::endl;

		return VK_FALSE;

		/* The first parameter specifies the type of message, which can be a combination of any of the following bit flags :

			VK_DEBUG_REPORT_INFORMATION_BIT_EXT
			VK_DEBUG_REPORT_WARNING_BIT_EXT
			VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT
			VK_DEBUG_REPORT_ERROR_BIT_EXT
			VK_DEBUG_REPORT_DEBUG_BIT_EXT
			The objType parameter specifies the type of object that is the subject of the message.For example if obj is a VkPhysicalDevice then objType would be VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT.This works because internally all Vulkan handles are typedef'd as uint64_t.

			The msg parameter contains the pointer to the message itself.Finally, there's a userData parameter to pass your own data to the callback.
		*/

	}
};

int main() {
	HelloTriangleApplication app;

	try {
		app.run();
	}
	catch (const std::runtime_error& e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}