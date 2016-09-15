#define GLFW_INCLUDE_VULKAN // replaces //#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h> 

#include <iostream>
#include <stdexcept>
#include <fstream>
#include <algorithm>
#include <vector>
#include <cstring>
#include <set>

#include "vdeleter.h"

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
	VDeleter<VkPipelineLayout> pipelineLayout{ device, vkDestroyPipelineLayout };
	VDeleter<VkPipeline> graphicsPipeline{ device, vkDestroyPipeline };

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
		createGraphicsPipeline();
		createFramebuffers();
		createCommandPool();
		createCommandBuffers();
		createSemaphores();
	}

	void mainLoop() {
		while (!glfwWindowShouldClose(window)) {
			glfwPollEvents();
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

		if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
			throw std::runtime_error("failed to create instance!");
		}
	}

	void setupDebugCallback() {
		if (!enableValidationLayers) return;

		VkDebugReportCallbackCreateInfoEXT createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
		createInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
		createInfo.pfnCallback = debugCallback;

		if (CreateDebugReportCallbackEXT(instance, &createInfo, nullptr, &callback) != VK_SUCCESS) {
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

		if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
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
		if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS) {
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
		// Hint, look at the address off overloading in VDeleter.h
		*&swapChain = newSwapChain;

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
			VkImageViewCreateInfo createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			createInfo.image = swapChainImages[i];
			// 2D texture
			createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			createInfo.format = swapChainImageFormat;
			// stick to default mapping  (you could for example map all of the channels to red in a monochrome texture)
			createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
			// the subresourceRange field describes the image's purpose and how to access
			// In this case this will be used as a color target without any mipmapping or layers
			createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			createInfo.subresourceRange.baseMipLevel = 0;
			createInfo.subresourceRange.levelCount = 1;
			createInfo.subresourceRange.baseArrayLayer = 0;
			createInfo.subresourceRange.layerCount = 1;

			if (vkCreateImageView(device, &createInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS) {
				// Note that &swapChainImageViews[i] uses of address of operator overload so it will 
				// automatically release old resource and acquire new one by the magic of its wrapper
				// vDeleter in case this is a swap chain recreation scenario
				throw std::runtime_error("failed to create image views!");
			}
			// An image view is sufficient to start using an image as a texture but not enough for using it
			// as a render target. That requires one more step of indirection known as a framebuffer
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

		// Every subpass references one or more of the attachments that we've described.
		// These references are themselves VkAttachmentReference
		VkAttachmentReference colorAttachmentRef = {};
		colorAttachmentRef.attachment = 0;
		// The layout specifies which layout we would like the attachment to have during a subpass that uses this reference.
		// Vulkan will automatically transition the attachment to this layout when the subpass is started
		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		// A single render pass can consist of multiple subpasses. Subpasses are subsequent rendering operations 
		// that depend on the contents of framebuffers in previous passes, for example a sequence of post - processing 
		// effects that are applied one after another. If you group these rendering operations into one render pass, 
		// then Vulkan is able to reorder the operations and conserve memory bandwidth for possibly better performance.
		// For our very first triangle, however, we'll stick to a single subpass.
		VkSubpassDescription subPass = {};
		subPass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS; // Vulkan will support compute here so need to be explicit
		subPass.colorAttachmentCount = 1;
		subPass.pColorAttachments = &colorAttachmentRef;
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

		VkRenderPassCreateInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = 1;
		renderPassInfo.pAttachments = &colorAttachment;
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subPass;
		renderPassInfo.dependencyCount = 1;
		renderPassInfo.pDependencies = &dependency;

		if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
			// Note that &renderPass uses of address of operator overload so it will 
			// automatically release old resource and acquire new one by the magic of its wrapper
			// vDeleter in case this is a swap chain recreation scenario.
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
		// spacing between data and whether data is per-vertex or per-instance
		vertexInputInfo.vertexBindingDescriptionCount = 0;
		// type of the attributes passed to the vertex shader, which binding to load them from and at which offset
		vertexInputInfo.vertexAttributeDescriptionCount = 0;

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
		rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
		rasterizer.depthBiasEnable = VK_FALSE;

		// Multisampling (anti-aliasing) is turned off in this sample
		VkPipelineMultisampleStateCreateInfo multisampling = {};
		multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampling.sampleShadingEnable = VK_FALSE;
		multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		
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

		// This structure is used to specify the layout of shader uniforms in future samples
		VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 0;
		pipelineLayoutInfo.pushConstantRangeCount = 0;

		if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
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
		if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
			// Note that &graphicsPipeline uses of address of operator overload so it will 
			// automatically release old resource and acquire new one by the magic of its wrapper
			// vDeleter in case this is a swap chain recreation scenario.
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
			VkImageView attachments[] = {
				swapChainImageViews[i]
			};

			VkFramebufferCreateInfo framebufferInfo = {};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass = renderPass;
			framebufferInfo.attachmentCount = 1;
			framebufferInfo.pAttachments = attachments;
			framebufferInfo.width = swapChainExtent.width;
			framebufferInfo.height = swapChainExtent.height;
			framebufferInfo.layers = 1;

			if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS) {
				// Note that &swapChainFramebuffers[i] uses of address of operator overload so it will 
				// automatically release old resource and acquire new one by the magic of its wrapper
				// vDeleter in case this is a swap chain recreation scenario.
				throw std::runtime_error("failed to create framebuffer!");
			}
		}
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
		if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
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
			// which we used as load operation for the color attachment
			VkClearValue clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
			renderPassInfo.clearValueCount = 1;
			renderPassInfo.pClearValues = &clearColor;

			vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
			// VK_SUBPASS_CONTENTS_INLINE: The render pass commands will be embedded in the primary 
			// command buffer itself and no secondary command buffers will be executed.
		    // VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS: The render pass commands will be executed 
			// from secondary command buffers.

			// Bind the graphics pipeline
			vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

			// We have told vulkan which operations to execute in the graphics pipeline and which
			// attachment to use in the fragment shader, now draw the triangle
			vkCmdDraw(commandBuffers[i], 3, 1, 0, 0);
			// vertexCount: Even though we don't have a vertex buffer, we technically still have 3 vertices to draw.
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

		if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphore) != VK_SUCCESS ||
			vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphore) != VK_SUCCESS) {

			throw std::runtime_error("failed to create semaphores!");
		}
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

		if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
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