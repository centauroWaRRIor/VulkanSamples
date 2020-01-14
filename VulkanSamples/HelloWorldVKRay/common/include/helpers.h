#pragma once

#include <vector>
#include <iostream>

//#include <ml/gfx/log_helpers.h>
#include <unused.h>

#include <vulkan/vulkan.h>

static char const *const VK_LAYER_LUNARG_STANDARD_VALIDATION_LAYER_NAME = "VK_LAYER_LUNARG_standard_validation";
static char const *const VK_LAYER_LUNARG_API_DUMP_LAYER_NAME = "VK_LAYER_LUNARG_api_dump";
static char const *const VK_LAYER_GOOGLE_THREADING_LAYER_NAME = "VK_LAYER_GOOGLE_threading";
static char const *const VK_LAYER_LUNARG_PARAMETER_VALIDATION_LAYER_NAME = "VK_LAYER_LUNARG_parameter_validation";
static char const *const VK_LAYER_LUNARG_DEVICE_LIMITS_LAYER_NAME = "VK_LAYER_LUNARG_device_limits";
static char const *const VK_LAYER_LUNARG_OBJECT_TRACKER_LAYER_NAME = "VK_LAYER_LUNARG_object_tracker";
static char const *const VK_LAYER_LUNARG_IMAGE_LAYER_NAME = "VK_LAYER_LUNARG_image";
static char const *const VK_LAYER_LUNARG_CORE_VALIDATION_LAYER_NAME = "VK_LAYER_LUNARG_core_validation";
static char const *const VK_LAYER_LUNARG_SWAPCHAIN_LAYER_NAME = "VK_LAYER_LUNARG_swapchain";
static char const *const VK_LAYER_GOOGLE_UNIQUE_OBJECTS_LAYER_NAME = "VK_LAYER_GOOGLE_unique_objects";

namespace vksamples {
	namespace common {

		struct MemoryAllocation;

		namespace legacy {
			namespace vk {

				class IContext;

				struct MemoryAllocationVK {
					uint64_t       size = 0;
					uint32_t       memory_type_index = 0;
					bool           dedicated = false;
					VkDeviceMemory device_memory = VK_NULL_HANDLE;
				};

				struct DebugReportLoggingCallbackContext {
					bool log_vk_mem = false;
				};

				class Helpers {
				public:
					static bool AllocateMemory(vksamples::common::legacy::vk::IContext const &vulkan_context,
						VkMemoryRequirements const          &memory_requirements,
						VkMemoryPropertyFlags const         &required_memory_properties,
						VkImage const                       &dedicated_image,
						VkBuffer const                      &dedicated_buffer,
						bool const                          &make_exportable,
						MemoryAllocationVK                  &out_memory_allocation);
					/*static bool ImportMemory(vksamples::common::legacy::vk::IContext const &vulkan_context,
						MemoryAllocation const              &shared_memory_allocation,
						VkImage const                       &dedicated_image,
						VkBuffer const                      &dedicated_buffer,
						MemoryAllocationVK                  &out_memory_allocation);*/

					static uint32_t GetMemoryTypeIndex(VkPhysicalDeviceMemoryProperties const &memory_properties, uint32_t memory_type_bits, VkMemoryPropertyFlags properties) {
						for (uint32_t memory_type_index = 0; memory_type_index < memory_properties.memoryTypeCount; ++memory_type_index) {
							if ((memory_type_bits & 1) == 1 && (memory_properties.memoryTypes[memory_type_index].propertyFlags & properties) == properties) {
								return memory_type_index;
							}
							memory_type_bits >>= 1;
						}
						return VK_MAX_MEMORY_TYPES;
					}

					static VkDeviceSize AlignedOffset(VkDeviceSize current_offset, VkDeviceSize alignment) {
						return (current_offset + alignment - 1) / alignment * alignment;
					}

					VKAPI_ATTR static VkBool32 VKAPI_CALL DebugReportLoggingCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT object_type, uint64_t object, size_t location, int32_t message_code, const char *layer_prefix, const char *message, void *user_data) {
						ML_UNUSED(object_type);
						ML_UNUSED(object);
						ML_UNUSED(location);
						ML_UNUSED(message_code);
						ML_UNUSED(user_data);
#if !_DEBUG
						ML_UNUSED(message);
#endif

						DebugReportLoggingCallbackContext const *context = static_cast<DebugReportLoggingCallbackContext *>(user_data);

						// MEM is too verbose
						bool const log_vk_mem = context && context->log_vk_mem;
						if (!log_vk_mem && (0 == strcmp("MEM", layer_prefix))) {
							return VK_FALSE;
						}

						switch (flags) {
						case VK_DEBUG_REPORT_INFORMATION_BIT_EXT:
						case VK_DEBUG_REPORT_DEBUG_BIT_EXT:
						case VK_DEBUG_REPORT_WARNING_BIT_EXT:
						case VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT:
						case VK_DEBUG_REPORT_ERROR_BIT_EXT:
						default:
							std::cout << "Vulkan [" << layer_prefix << "] " << message << std::endl;
							break;
						}

						return VK_FALSE;
					}

					static std::string ToString(VkResult value) {
						switch (value) {
						case VK_SUCCESS:
							return "Success";
						case VK_NOT_READY:
							return "Not Ready";
						case VK_TIMEOUT:
							return "Timeout";
						case VK_EVENT_SET:
							return "Event Set";
						case VK_EVENT_RESET:
							return "Event Reset";
						case VK_INCOMPLETE:
							return "Incomplete";
						case VK_ERROR_OUT_OF_HOST_MEMORY:
							return "Out Of Host Memory";
						case VK_ERROR_OUT_OF_DEVICE_MEMORY:
							return "Out Of Device Memory";
						case VK_ERROR_INITIALIZATION_FAILED:
							return "Initialization Failed";
						case VK_ERROR_DEVICE_LOST:
							return "Device Lost";
						case VK_ERROR_MEMORY_MAP_FAILED:
							return "Memory Map Failed";
						case VK_ERROR_LAYER_NOT_PRESENT:
							return "Layer Not Present";
						case VK_ERROR_EXTENSION_NOT_PRESENT:
							return "Extension Not Present";
						case VK_ERROR_FEATURE_NOT_PRESENT:
							return "Feature Not Present";
						case VK_ERROR_INCOMPATIBLE_DRIVER:
							return "Incompatible Driver";
						case VK_ERROR_TOO_MANY_OBJECTS:
							return "Too Many Objects";
						case VK_ERROR_FORMAT_NOT_SUPPORTED:
							return "Format Not Supported";
						case VK_ERROR_FRAGMENTED_POOL:
							return "Fragmented Pool";
						case VK_ERROR_SURFACE_LOST_KHR:
							return "Surface Lost";
						case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
							return "Native Window In Use";
						case VK_SUBOPTIMAL_KHR:
							return "Suboptimal";
						case VK_ERROR_OUT_OF_DATE_KHR:
							return "Out Of Date";
						case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR:
							return "Incompatible Display";
						case VK_ERROR_VALIDATION_FAILED_EXT:
							return "Validation Failed";
						case VK_ERROR_INVALID_SHADER_NV:
							return "Invalid Shader";
						case VK_ERROR_OUT_OF_POOL_MEMORY_KHR:
							return "Out Of Pool Memory";
						case VK_ERROR_INVALID_EXTERNAL_HANDLE_KHR:
							return "Invalid External Handle";
						default:
							return "invalid";
						}
					}

					static std::string ToString(VkPhysicalDeviceType value) {
						switch (value) {
						case VK_PHYSICAL_DEVICE_TYPE_OTHER:
							return "Other";
						case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
							return "Integrated GPU";
						case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
							return "Discrete GPU";
						case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
							return "Virtual GPU";
						case VK_PHYSICAL_DEVICE_TYPE_CPU:
							return "CPU";
						default:
							return "invalid";
						}
					}

					static std::string VkMemoryPropertyFlagsToString(VkMemoryPropertyFlags value) {
						if (!value)
							return "{}";
						std::string result;
						if (value & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
							result += "DeviceLocal | ";
						if (value & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
							result += "HostVisible | ";
						if (value & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
							result += "HostCoherent | ";
						if (value & VK_MEMORY_PROPERTY_HOST_CACHED_BIT)
							result += "HostCached | ";
						if (value & VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT)
							result += "LazilyAllocated | ";
						return "{" + result.substr(0, result.size() - 3) + "}";
					}

					static std::string VkMemoryHeapFlagsToString(VkMemoryHeapFlags value) {
						if (!value)
							return "{}";
						std::string result;
						if (value & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)
							result += "DeviceLocal | ";
						if (value & VK_MEMORY_HEAP_MULTI_INSTANCE_BIT_KHR)
							result += "MultiInstanceKHR | ";
						return "{" + result.substr(0, result.size() - 3) + "}";
					}

					static std::string VkQueueFlagsToString(VkQueueFlags value) {
						if (!value)
							return "{}";
						std::string result;
						if (value & VK_QUEUE_GRAPHICS_BIT)
							result += "Graphics | ";
						if (value & VK_QUEUE_COMPUTE_BIT)
							result += "Compute | ";
						if (value & VK_QUEUE_TRANSFER_BIT)
							result += "Transfer | ";
						if (value & VK_QUEUE_SPARSE_BINDING_BIT)
							result += "SparseBinding | ";
						return "{" + result.substr(0, result.size() - 3) + "}";
					}

					static std::string VkFormatToString(const VkFormat format) {
						switch (format) {
						case VK_FORMAT_R8G8B8A8_UNORM:
							return "VK_FORMAT_R8G8B8A8_UNORM";
						case VK_FORMAT_R8G8B8A8_SRGB:
							return "VK_FORMAT_R8G8B8A8_SRGB";
						case VK_FORMAT_A2R10G10B10_UNORM_PACK32:
							return "VK_FORMAT_A2R10G10B10_UNORM_PACK32";
						case VK_FORMAT_R16G16B16A16_SFLOAT:
							return "VK_FORMAT_R16G16B16A16_SFLOAT";
						case VK_FORMAT_D32_SFLOAT:
							return "VK_FORMAT_D32_SFLOAT";
						case VK_FORMAT_D24_UNORM_S8_UINT:
							return "VK_FORMAT_D24_UNORM_S8_UINT";
						case VK_FORMAT_D32_SFLOAT_S8_UINT:
							return "VK_FORMAT_D32_SFLOAT_S8_UINT";
						case VK_FORMAT_R8_UNORM:
							return "VK_FORMAT_R8_UNORM";
						case VK_FORMAT_R8_UINT:
							return "VK_FORMAT_R8_UINT";
						case VK_FORMAT_R16_UINT:
							return "VK_FORMAT_R16_UINT";
						case VK_FORMAT_R16_SFLOAT:
							return "VK_FORMAT_R16_SFLOAT";
						case VK_FORMAT_R32_SFLOAT:
							return "VK_FORMAT_R32_SFLOAT";
						case VK_FORMAT_R16G16_SFLOAT:
							return "VK_FORMAT_R16G16_SFLOAT";
						case VK_FORMAT_R8G8B8A8_UINT:
							return "VK_FORMAT_R8G8B8A8_UINT";
						default:
							return "VK_FORMAT_UNDEFINED";
						}
					}
				};

#define VK_RETURN_ON_ERROR(f)                                               \
  {                                                                         \
    VkResult const res = (f);                                               \
    if (VK_SUCCESS != res) {                                                \
      std::cerr << "Failed a Vulkan API call. VkResult is " << (int)res << std::endl; \
      return false;                                                         \
    }                                                                       \
  }

			} // namespace vk
		} // namespace legacy
	} // namespace common
} // namespace vksamples