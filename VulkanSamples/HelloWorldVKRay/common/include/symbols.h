#pragma once

#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>

#include <lib_loader.h>

#define VK_GLOBAL_SYMBOLS(SYM_FUNC)                \
  SYM_FUNC(vkEnumerateInstanceExtensionProperties) \
  SYM_FUNC(vkEnumerateInstanceLayerProperties)     \
  SYM_FUNC(vkCreateInstance)

#define VK_INSTANCE_SYMBOLS(SYM_FUNC)            \
  SYM_FUNC(vkDestroyInstance)                    \
  SYM_FUNC(vkEnumeratePhysicalDevices)           \
  SYM_FUNC(vkGetPhysicalDeviceFormatProperties)  \
  SYM_FUNC(vkEnumerateDeviceExtensionProperties) \
  SYM_FUNC(vkEnumerateDeviceLayerProperties)     \
  SYM_FUNC(vkCreateDevice)                       \
  SYM_FUNC(vkGetDeviceProcAddr)                  \
  SYM_FUNC(vkGetPhysicalDeviceProperties)        \
  SYM_FUNC(vkGetPhysicalDeviceMemoryProperties)  \
  SYM_FUNC(vkGetPhysicalDeviceFeatures)          \
  SYM_FUNC(vkGetPhysicalDeviceQueueFamilyProperties)

#define VK_INSTANCE_EXT_SYMBOLS(SYM_FUNC)                  \
  SYM_FUNC(vkCreateDebugReportCallbackEXT)                 \
  SYM_FUNC(vkDestroyDebugReportCallbackEXT)                \
  SYM_FUNC(vkEnumeratePhysicalDeviceGroupsKHR)             \
  SYM_FUNC(vkGetPhysicalDeviceProperties2KHR)              \
  SYM_FUNC(vkGetPhysicalDeviceMemoryProperties2KHR)        \
  SYM_FUNC(vkGetPhysicalDeviceFeatures2KHR)                \
  SYM_FUNC(vkGetPhysicalDeviceQueueFamilyProperties2KHR)   \
  SYM_FUNC(vkGetPhysicalDeviceImageFormatProperties2KHR)   \
  SYM_FUNC(vkGetPhysicalDeviceExternalBufferPropertiesKHR) \
  SYM_FUNC(vkGetPhysicalDeviceExternalSemaphorePropertiesKHR)

#define VK_DEVICE_SYMBOLS(SYM_FUNC)        \
  SYM_FUNC(vkDestroyDevice)                \
  SYM_FUNC(vkGetDeviceQueue)               \
  SYM_FUNC(vkCreateFence)                  \
  SYM_FUNC(vkDestroyFence)                 \
  SYM_FUNC(vkGetFenceStatus)               \
  SYM_FUNC(vkResetFences)                  \
  SYM_FUNC(vkWaitForFences)                \
  SYM_FUNC(vkCreateSemaphore)              \
  SYM_FUNC(vkDestroySemaphore)             \
  SYM_FUNC(vkCreateBuffer)                 \
  SYM_FUNC(vkDestroyBuffer)                \
  SYM_FUNC(vkCreateImage)                  \
  SYM_FUNC(vkDestroyImage)                 \
  SYM_FUNC(vkAllocateMemory)               \
  SYM_FUNC(vkBindBufferMemory)             \
  SYM_FUNC(vkBindImageMemory)              \
  SYM_FUNC(vkFreeMemory)                   \
  SYM_FUNC(vkMapMemory)                    \
  SYM_FUNC(vkUnmapMemory)                  \
  SYM_FUNC(vkInvalidateMappedMemoryRanges) \
  SYM_FUNC(vkCreateCommandPool)            \
  SYM_FUNC(vkDestroyCommandPool)           \
  SYM_FUNC(vkResetCommandPool)             \
  SYM_FUNC(vkCreateQueryPool)              \
  SYM_FUNC(vkDestroyQueryPool)             \
  SYM_FUNC(vkGetQueryPoolResults)          \
  SYM_FUNC(vkAllocateCommandBuffers)       \
  SYM_FUNC(vkFreeCommandBuffers)           \
  SYM_FUNC(vkBeginCommandBuffer)           \
  SYM_FUNC(vkEndCommandBuffer)             \
  SYM_FUNC(vkResetCommandBuffer)           \
  SYM_FUNC(vkQueueSubmit)                  \
  SYM_FUNC(vkCmdBeginRenderPass)           \
  SYM_FUNC(vkCmdEndRenderPass)             \
  SYM_FUNC(vkCmdPipelineBarrier)           \
  SYM_FUNC(vkCmdUpdateBuffer)              \
  SYM_FUNC(vkCmdClearColorImage)           \
  SYM_FUNC(vkCmdCopyImage)                 \
  SYM_FUNC(vkCmdResetQueryPool)            \
  SYM_FUNC(vkCmdWriteTimestamp)            \
  SYM_FUNC(vkDeviceWaitIdle)               \
  SYM_FUNC(vkCreatePipelineCache)          \
  SYM_FUNC(vkDestroyPipelineCache)         \
  SYM_FUNC(vkGetPipelineCacheData)         \
  SYM_FUNC(vkCreateRenderPass)             \
  SYM_FUNC(vkDestroyRenderPass)            \
  SYM_FUNC(vkCreateImageView)              \
  SYM_FUNC(vkDestroyImageView)             \
  SYM_FUNC(vkCreateFramebuffer)            \
  SYM_FUNC(vkDestroyFramebuffer)           \
  SYM_FUNC(vkCreateShaderModule)           \
  SYM_FUNC(vkDestroyShaderModule)          \
  SYM_FUNC(vkCreateDescriptorSetLayout)    \
  SYM_FUNC(vkDestroyDescriptorSetLayout)   \
  SYM_FUNC(vkCreatePipelineLayout)         \
  SYM_FUNC(vkDestroyPipelineLayout)        \
  SYM_FUNC(vkCreateGraphicsPipelines)      \
  SYM_FUNC(vkDestroyPipeline)              \
  SYM_FUNC(vkGetBufferMemoryRequirements)  \
  SYM_FUNC(vkFlushMappedMemoryRanges)      \
  SYM_FUNC(vkCmdBindVertexBuffers)         \
  SYM_FUNC(vkCmdBindPipeline)              \
  SYM_FUNC(vkCmdPushConstants)             \
  SYM_FUNC(vkCmdSetViewport)               \
  SYM_FUNC(vkCmdSetScissor)                \
  SYM_FUNC(vkCreateDescriptorPool)         \
  SYM_FUNC(vkDestroyDescriptorPool)        \
  SYM_FUNC(vkCmdCopyBuffer)                \
  SYM_FUNC(vkCmdDraw)                      \
  SYM_FUNC(vkUpdateDescriptorSets)         \
  SYM_FUNC(vkAllocateDescriptorSets)       \
  SYM_FUNC(vkFreeDescriptorSets)           \
  SYM_FUNC(vkCmdBindDescriptorSets)        \
  SYM_FUNC(vkQueueWaitIdle)                \
  SYM_FUNC(vkCreateSampler)                \
  SYM_FUNC(vkDestroySampler)

#define VK_DEVICE_EXT_SYMBOLS(SYM_FUNC)       \
  SYM_FUNC(vkGetMemoryFdKHR)                  \
  SYM_FUNC(vkGetSemaphoreFdKHR)               \
  SYM_FUNC(vkImportSemaphoreFdKHR)            \
  SYM_FUNC(vkGetBufferMemoryRequirements2KHR) \
  SYM_FUNC(vkGetImageMemoryRequirements2KHR)

#define VK_DECLARE_SYMBOL(SYM_NAME) \
  PFN_##SYM_NAME SYM_NAME = nullptr;

namespace vksamples {
	namespace common {
		namespace vk {

			struct GlobalSymbols {
				std::shared_ptr<Library> VulkanLibrary = nullptr;
				VK_DECLARE_SYMBOL(vkGetInstanceProcAddr)
				VK_GLOBAL_SYMBOLS(VK_DECLARE_SYMBOL)
			};

			struct InstanceSymbols {
				VK_INSTANCE_SYMBOLS(VK_DECLARE_SYMBOL)
				VK_INSTANCE_EXT_SYMBOLS(VK_DECLARE_SYMBOL)
			};

			struct DeviceSymbols {
				VK_DEVICE_SYMBOLS(VK_DECLARE_SYMBOL)
				VK_DEVICE_EXT_SYMBOLS(VK_DECLARE_SYMBOL)
			};

			bool LoadGlobalSymbols(GlobalSymbols &out_symbols);
			bool LoadInstanceSymbols(VkInstance const vk_instance, PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr, InstanceSymbols &out_symbols);
			bool LoadDeviceSymbols(VkDevice const vk_device, PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr, DeviceSymbols &out_symbols);

		} // namespace vk
	} // namespace common
} // namespace vksamples