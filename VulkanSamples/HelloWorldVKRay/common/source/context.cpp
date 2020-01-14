#include <context.h>

#include <string>
#include <vector>
#include <iostream>
#include <cassert>

//#include <ml/gfx/bitmask.h>
//#include <ml/gfx/check.h>
#include <helpers.h>
#include <log_helpers.h>
#include <scope_exit.h>

namespace vksamples {
	namespace common {
		namespace legacy {
			namespace vk {

#define CHECK_RESULT(msg)                                                                \
  if (VK_SUCCESS != result) {                                                            \
    std::cerr << " : " << vksamples::common::legacy::vk::Helpers::ToString(result).c_str() << std::endl; \
    return false;                                                                        \
  }

				Context::Context() {
				}

				Context::~Context() {
					assert(Free());
				}

				bool Context::Initialize(VkApplicationInfo const &application_info,
					VkInstance const         instance_param,
					VkPhysicalDevice const   physical_device_param,
					VkDevice const           device_param) {
					VkResult result = VK_INCOMPLETE;
					if (!vksamples::common::vk::LoadGlobalSymbols(symbols_)) {
						return false;
					}

#if _DEBUG
					std::vector<std::string> vulkan_info = { "Vulkan System Information" };
					GFX_SCOPE_EXIT(LogHelpers::LogLargeString(LogHelpers::Join(vulkan_info, "\n").c_str()));
#endif

					uint32_t count = 0;

#if _DEBUG
					bool debug_enabled = false;

					bool have_VK_LAYER_LUNARG_standard_validation = false;
					bool have_VK_LAYER_GOOGLE_threading = false;
					bool have_VK_LAYER_LUNARG_parameter_validation = false;
					bool have_VK_LAYER_LUNARG_device_limits = false;
					bool have_VK_LAYER_LUNARG_object_tracker = false;
					bool have_VK_LAYER_LUNARG_image = false;
					bool have_VK_LAYER_LUNARG_core_validation = false;
#endif

					std::vector<char const *> enabled_instance_extensions;
					std::vector<char const *> enabled_instance_layers;

					// instance extensions
					result = symbols_.vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr);
					CHECK_RESULT("vkEnumerateInstanceExtensionProperties failed")

						std::vector<VkExtensionProperties> instance_extension_properties(count);
					result = symbols_.vkEnumerateInstanceExtensionProperties(nullptr, &count, instance_extension_properties.data());
					CHECK_RESULT("vkEnumerateInstanceExtensionProperties failed")

#if _DEBUG
						vulkan_info.push_back("== Vulkan Instance Extensions ==");
#endif
					for (auto &extension_properties : instance_extension_properties) {
#if _DEBUG
						vulkan_info.push_back(LogHelpers::StringFormat(" - %s [%d]", extension_properties.extensionName, extension_properties.specVersion));
#endif

						/*if (0 == strcmp(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME, extension_properties.extensionName)) {
							capabilities_.get_physical_device_properties2 = true;
							enabled_instance_extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
						}
						if (0 == strcmp(VK_KHR_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME, extension_properties.extensionName)) {
							capabilities_.external_memory_capabilities = true;
							enabled_instance_extensions.push_back(VK_KHR_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME);
						}
						if (0 == strcmp(VK_KHR_EXTERNAL_SEMAPHORE_CAPABILITIES_EXTENSION_NAME, extension_properties.extensionName)) {
							capabilities_.external_semaphore_capabilities = true;
							enabled_instance_extensions.push_back(VK_KHR_EXTERNAL_SEMAPHORE_CAPABILITIES_EXTENSION_NAME);
						}*/
#if _DEBUG
						if (0 == strcmp(VK_EXT_DEBUG_REPORT_EXTENSION_NAME, extension_properties.extensionName)) {
							capabilities_.debug_report = true;
						}
#endif
					}

#if _DEBUG
					// instance layers
					result = symbols_.vkEnumerateInstanceLayerProperties(&count, nullptr);
					CHECK_RESULT("vkEnumerateInstanceLayerProperties failed")

						std::vector<VkLayerProperties> instance_layer_properties(count);
					result = symbols_.vkEnumerateInstanceLayerProperties(&count, instance_layer_properties.data());
					CHECK_RESULT("vkEnumerateInstanceLayerProperties failed")

						vulkan_info.push_back("== Vulkan Instance Layers ==");
					for (auto const &layer_properties : instance_layer_properties) {
						vulkan_info.push_back(LogHelpers::StringFormat(" - %s [%d]", layer_properties.layerName, layer_properties.implementationVersion));

						result = symbols_.vkEnumerateInstanceExtensionProperties(layer_properties.layerName, &count, nullptr);
						CHECK_RESULT("vkEnumerateInstanceExtensionProperties failed")

							std::vector<VkExtensionProperties> layer_extension_properties(count);
						result = symbols_.vkEnumerateInstanceExtensionProperties(layer_properties.layerName, &count, layer_extension_properties.data());
						CHECK_RESULT("vkEnumerateInstanceExtensionProperties failed")

							vulkan_info.push_back("   Extensions:");
						for (auto const &extension_properties : layer_extension_properties) {
							vulkan_info.push_back(LogHelpers::StringFormat("    - %s [%d]", extension_properties.extensionName, extension_properties.specVersion));
						}

						if (0 == strcmp(VK_LAYER_LUNARG_STANDARD_VALIDATION_LAYER_NAME, layer_properties.layerName)) {
							have_VK_LAYER_LUNARG_standard_validation = true;
						}
						if (0 == strcmp(VK_LAYER_GOOGLE_THREADING_LAYER_NAME, layer_properties.layerName)) {
							have_VK_LAYER_GOOGLE_threading = true;
						}
						if (0 == strcmp(VK_LAYER_LUNARG_PARAMETER_VALIDATION_LAYER_NAME, layer_properties.layerName)) {
							have_VK_LAYER_LUNARG_parameter_validation = true;
						}
						if (0 == strcmp(VK_LAYER_LUNARG_DEVICE_LIMITS_LAYER_NAME, layer_properties.layerName)) {
							have_VK_LAYER_LUNARG_device_limits = true;
						}
						if (0 == strcmp(VK_LAYER_LUNARG_OBJECT_TRACKER_LAYER_NAME, layer_properties.layerName)) {
							have_VK_LAYER_LUNARG_object_tracker = true;
						}
						if (0 == strcmp(VK_LAYER_LUNARG_IMAGE_LAYER_NAME, layer_properties.layerName)) {
							have_VK_LAYER_LUNARG_image = true;
						}
						if (0 == strcmp(VK_LAYER_LUNARG_CORE_VALIDATION_LAYER_NAME, layer_properties.layerName)) {
							have_VK_LAYER_LUNARG_core_validation = true;
						}
					}

					if (capabilities_.debug_report) {
						if (have_VK_LAYER_LUNARG_standard_validation) {
							debug_enabled = true;
							enabled_instance_layers.push_back(VK_LAYER_LUNARG_STANDARD_VALIDATION_LAYER_NAME);
						}
						if (have_VK_LAYER_GOOGLE_threading) {
							debug_enabled = true;
							enabled_instance_layers.push_back(VK_LAYER_GOOGLE_THREADING_LAYER_NAME);
						}
						if (have_VK_LAYER_LUNARG_parameter_validation) {
							debug_enabled = true;
							enabled_instance_layers.push_back(VK_LAYER_LUNARG_PARAMETER_VALIDATION_LAYER_NAME);
						}
						if (have_VK_LAYER_LUNARG_device_limits) {
							debug_enabled = true;
							enabled_instance_layers.push_back(VK_LAYER_LUNARG_DEVICE_LIMITS_LAYER_NAME);
						}
						if (have_VK_LAYER_LUNARG_object_tracker) {
							debug_enabled = true;
							enabled_instance_layers.push_back(VK_LAYER_LUNARG_OBJECT_TRACKER_LAYER_NAME);
						}
						if (have_VK_LAYER_LUNARG_image) {
							debug_enabled = true;
							enabled_instance_layers.push_back(VK_LAYER_LUNARG_IMAGE_LAYER_NAME);
						}
						if (have_VK_LAYER_LUNARG_core_validation) {
							debug_enabled = true;
							enabled_instance_layers.push_back(VK_LAYER_LUNARG_CORE_VALIDATION_LAYER_NAME);
						}
						if (debug_enabled) {
							enabled_instance_extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
						}
					}
#endif

					// could use the original version as a fallback if the extension isn't available,
					// but this extension is needed for export/import
					/*if (!capabilities_.get_physical_device_properties2) {
						return false;
					}*/

					VkInstanceCreateInfo instance_create_info;
					instance_create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
					instance_create_info.pNext = nullptr;
					instance_create_info.flags = 0;
					instance_create_info.pApplicationInfo = &application_info;
					instance_create_info.enabledLayerCount = static_cast<uint32_t>(enabled_instance_layers.size());
					instance_create_info.ppEnabledLayerNames = enabled_instance_layers.data();
					instance_create_info.enabledExtensionCount = static_cast<uint32_t>(enabled_instance_extensions.size());
					instance_create_info.ppEnabledExtensionNames = enabled_instance_extensions.data();

#if _DEBUG
					VkDebugReportCallbackCreateInfoEXT debug_report_callback_create_info;
					debug_report_callback_create_info.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
					debug_report_callback_create_info.pNext = nullptr;
					debug_report_callback_create_info.flags = VK_DEBUG_REPORT_INFORMATION_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT | VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_DEBUG_BIT_EXT;
					debug_report_callback_create_info.pfnCallback = vksamples::common::legacy::vk::Helpers::DebugReportLoggingCallback;
					debug_report_callback_create_info.pUserData = nullptr;

					if (debug_enabled) {
						// appending callback create info to our instance create info allows debugging instance creation
						// but is not persistent, so still need to create the callback below
						instance_create_info.pNext = &debug_report_callback_create_info;
					}
#endif

					if (VK_NULL_HANDLE == instance_param) {
						// create a new vulkan instance
						result = symbols_.vkCreateInstance(&instance_create_info, nullptr, &instance_);
						CHECK_RESULT("vkCreateInstance failed")
					}
					else {
						// reuse the instance passed in
						instance_ = instance_param;
						instance_is_local_ = true;
					}

					if (!vksamples::common::vk::LoadInstanceSymbols(instance_, symbols_.vkGetInstanceProcAddr, symbols_)) {
						std::cout << "Failed to load instance symbols" << std::endl;
						return false;
					}

#if _DEBUG
					if (debug_enabled && !instance_is_local_) {
						result = symbols_.vkCreateDebugReportCallbackEXT(instance_, &debug_report_callback_create_info, nullptr, &debug_report_callback_);
						CHECK_RESULT("vkCreateDebugReportCallbackEXT failed")
					}
#endif

					std::vector<VkPhysicalDevice> physical_devices;

					if (VK_NULL_HANDLE == physical_device_param) {
						result = symbols_.vkEnumeratePhysicalDevices(instance_, &count, nullptr);
						CHECK_RESULT("vkEnumeratePhysicalDevices failed")
							physical_devices.resize(count);
						result = symbols_.vkEnumeratePhysicalDevices(instance_, &count, physical_devices.data());
						CHECK_RESULT("vkEnumeratePhysicalDevices failed")
					}
					else {
						physical_devices.push_back(physical_device_param);
					}

#if _DEBUG
					vulkan_info.push_back("== Vulkan Physical Devices ==");
#endif
					for (auto &physical_device : physical_devices) {
						std::vector<char const *> enabled_device_extensions;

						//VkPhysicalDeviceProperties2KHR physical_device_properties;
						//physical_device_properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2_KHR;
						//physical_device_properties.pNext = nullptr;
						// Because certain extension were not found I can't use this extension way of querying for phyical device
						//symbols_.vkGetPhysicalDeviceProperties2KHR(physical_device, &physical_device_properties);

						VkPhysicalDeviceProperties physical_device_properties;
						symbols_.vkGetPhysicalDeviceProperties(physical_device, &physical_device_properties);


						//timestamp_period_ = physical_device_properties.properties.limits.timestampPeriod;

#if _DEBUG
						//vulkan_info.push_back(LogHelpers::StringFormat(" - %s: %s", vksamples::common::legacy::vk::Helpers::ToString(physical_device_properties.properties.deviceType).c_str(), physical_device_properties.properties.deviceName));
						//vulkan_info.push_back(LogHelpers::StringFormat("   Timestamp Period: %f", physical_device_properties.properties.limits.timestampPeriod));
			            vulkan_info.push_back(LogHelpers::StringFormat(" - %s: %s", vksamples::common::legacy::vk::Helpers::ToString(physical_device_properties.deviceType).c_str(), physical_device_properties.deviceName));

						//VkPhysicalDeviceMemoryProperties2KHR physical_device_memory_properties;
						//physical_device_memory_properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2_KHR;
						//physical_device_memory_properties.pNext = nullptr;
						//symbols_.vkGetPhysicalDeviceMemoryProperties2KHR(physical_device, &physical_device_memory_properties);

						VkPhysicalDeviceMemoryProperties physical_device_memory_properties;
						symbols_.vkGetPhysicalDeviceMemoryProperties(physical_device, &physical_device_memory_properties);

						vulkan_info.push_back("   Memory Types:");
						//for (size_t i = 0; i < physical_device_memory_properties.memoryProperties.memoryTypeCount; ++i) {
							//auto const &memory_type = physical_device_memory_properties.memoryProperties.memoryTypes[i];
						for (size_t i = 0; i < physical_device_memory_properties.memoryTypeCount; ++i) {
							auto const &memory_type = physical_device_memory_properties.memoryTypes[i];
							vulkan_info.push_back(LogHelpers::StringFormat("    - %s, Heap: %d", vksamples::common::legacy::vk::Helpers::VkMemoryPropertyFlagsToString(memory_type.propertyFlags).c_str(), memory_type.heapIndex));
						}
						vulkan_info.push_back("   Memory Heaps:");
						//for (size_t i = 0; i < physical_device_memory_properties.memoryProperties.memoryHeapCount; ++i) {
							//auto const &memory_heap = physical_device_memory_properties.memoryProperties.memoryHeaps[i];
						for (size_t i = 0; i < physical_device_memory_properties.memoryHeapCount; ++i) {
							auto const &memory_heap = physical_device_memory_properties.memoryHeaps[i];
							vulkan_info.push_back(LogHelpers::StringFormat("    - %d) %s, %s", i, vksamples::common::legacy::vk::Helpers::VkMemoryHeapFlagsToString(memory_heap.flags).c_str(), LogHelpers::FormatBytes(memory_heap.size).c_str()));
						}

						// TODO: retrieve and print available physical device features
#endif

						//symbols_.vkGetPhysicalDeviceQueueFamilyProperties2KHR(physical_device, &count, nullptr);
						//std::vector<VkQueueFamilyProperties2KHR> queue_family_properties(count);
						//for (auto &queue_family : queue_family_properties) {
						//	queue_family.sType = VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2_KHR;
						//	queue_family.pNext = nullptr;
						//}
						//symbols_.vkGetPhysicalDeviceQueueFamilyProperties2KHR(physical_device, &count, queue_family_properties.data());

						symbols_.vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &count, nullptr);
						std::vector<VkQueueFamilyProperties> queue_family_properties(count);
						symbols_.vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &count, queue_family_properties.data());

						uint32_t graphics_queue_family_index = static_cast<uint32_t>(queue_family_properties.size());
						uint32_t transfer_queue_family_index = static_cast<uint32_t>(queue_family_properties.size());
						uint32_t dedicated_transfer_queue_family_index = static_cast<uint32_t>(queue_family_properties.size());

#if _DEBUG
						vulkan_info.push_back("   Queue Families:");
#endif
						for (uint32_t i = 0; queue_family_properties.size() > i; ++i) {
							auto const &queue_family = queue_family_properties[i];
#if _DEBUG
							//vulkan_info.push_back(LogHelpers::StringFormat("    - %s: %d", vksamples::common::legacy::vk::Helpers::VkQueueFlagsToString(queue_family.queueFamilyProperties.queueFlags).c_str(), queue_family.queueFamilyProperties.queueCount));
							vulkan_info.push_back(LogHelpers::StringFormat("    - %s: %d", vksamples::common::legacy::vk::Helpers::VkQueueFlagsToString(queue_family.queueFlags).c_str(), queue_family.queueCount));
#endif

							/*if ((0 < queue_family.queueFamilyProperties.queueCount) &&
								(VK_QUEUE_GRAPHICS_BIT == (VK_QUEUE_GRAPHICS_BIT & queue_family.queueFamilyProperties.queueFlags)) &&
								(queue_family.queueFamilyProperties.timestampValidBits > 0)) {
								graphics_queue_family_index = std::min(graphics_queue_family_index, i);
							}
							// queueCount must be at least 2 if we have already selected this family for graphics otherwise queueCount must be at least 1
							if ((((graphics_queue_family_index != i) && (0 < queue_family.queueFamilyProperties.queueCount)) || (1 < queue_family.queueFamilyProperties.queueCount)) &&
								(VK_QUEUE_TRANSFER_BIT == (VK_QUEUE_TRANSFER_BIT & queue_family.queueFamilyProperties.queueFlags))) {
								transfer_queue_family_index = std::min(transfer_queue_family_index, i);
								if ((0 == (VK_QUEUE_GRAPHICS_BIT & queue_family.queueFamilyProperties.queueFlags)) &&
									(0 == (VK_QUEUE_COMPUTE_BIT & queue_family.queueFamilyProperties.queueFlags))) {
									dedicated_transfer_queue_family_index = std::min(dedicated_transfer_queue_family_index, i);
								}
							}*/

							if ((0 < queue_family.queueCount) &&
								(VK_QUEUE_GRAPHICS_BIT == (VK_QUEUE_GRAPHICS_BIT & queue_family.queueFlags)) &&
								(queue_family.timestampValidBits > 0)) {
								graphics_queue_family_index = std::min(graphics_queue_family_index, i);
							}
							// queueCount must be at least 2 if we have already selected this family for graphics otherwise queueCount must be at least 1
							if ((((graphics_queue_family_index != i) && (0 < queue_family.queueCount)) || (1 < queue_family.queueCount)) &&
								(VK_QUEUE_TRANSFER_BIT == (VK_QUEUE_TRANSFER_BIT & queue_family.queueFlags))) {
								transfer_queue_family_index = std::min(transfer_queue_family_index, i);
								if ((0 == (VK_QUEUE_GRAPHICS_BIT & queue_family.queueFlags)) &&
									(0 == (VK_QUEUE_COMPUTE_BIT & queue_family.queueFlags))) {
									dedicated_transfer_queue_family_index = std::min(dedicated_transfer_queue_family_index, i);
								}
							}
						}

						if (queue_family_properties.size() > dedicated_transfer_queue_family_index) {
							transfer_queue_family_index = dedicated_transfer_queue_family_index;
						}

						result = symbols_.vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &count, nullptr);
						CHECK_RESULT("vkEnumerateDeviceExtensionProperties failed")

							std::vector<VkExtensionProperties> device_extension_properties(count);
						result = symbols_.vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &count, device_extension_properties.data());
						CHECK_RESULT("vkEnumerateDeviceExtensionProperties failed")

#if _DEBUG
							vulkan_info.push_back("   Extensions:");
#endif
						for (auto const &extension_properties : device_extension_properties) {
#if _DEBUG
							vulkan_info.push_back(LogHelpers::StringFormat("    - %s [%d]", extension_properties.extensionName, extension_properties.specVersion));
#endif

							// enable this minor fixes
							if (0 == strcmp(VK_KHR_MAINTENANCE1_EXTENSION_NAME, extension_properties.extensionName)) {
								enabled_device_extensions.push_back(VK_KHR_MAINTENANCE1_EXTENSION_NAME);
							}
							if (0 == strcmp(VK_KHR_MAINTENANCE2_EXTENSION_NAME, extension_properties.extensionName)) {
								enabled_device_extensions.push_back(VK_KHR_MAINTENANCE2_EXTENSION_NAME);
							}

							/*if (0 == strcmp(VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME, extension_properties.extensionName)) {
								capabilities_.get_memory_requirements2 = true;
								enabled_device_extensions.push_back(VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME);
							}
							if (0 == strcmp(VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME, extension_properties.extensionName)) {
								capabilities_.dedicated_allocation = true;
								enabled_device_extensions.push_back(VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME);
							}
							if (0 == strcmp(VK_KHR_EXTERNAL_MEMORY_EXTENSION_NAME, extension_properties.extensionName)) {
								capabilities_.external_memory = true;
								enabled_device_extensions.push_back(VK_KHR_EXTERNAL_MEMORY_EXTENSION_NAME);
							}
							if (0 == strcmp(VK_KHR_EXTERNAL_MEMORY_FD_EXTENSION_NAME, extension_properties.extensionName)) {
								capabilities_.external_memory_fd = true;
								enabled_device_extensions.push_back(VK_KHR_EXTERNAL_MEMORY_FD_EXTENSION_NAME);
							}
							if (0 == strcmp(VK_KHR_EXTERNAL_SEMAPHORE_EXTENSION_NAME, extension_properties.extensionName)) {
								capabilities_.external_semaphore = true;
								enabled_device_extensions.push_back(VK_KHR_EXTERNAL_SEMAPHORE_EXTENSION_NAME);
							}
							if (0 == strcmp(VK_KHR_EXTERNAL_SEMAPHORE_FD_EXTENSION_NAME, extension_properties.extensionName)) {
								capabilities_.external_semaphore_fd = true;
								enabled_device_extensions.push_back(VK_KHR_EXTERNAL_SEMAPHORE_FD_EXTENSION_NAME);
							}
							/*if (0 == strcmp(VK_NV_TEXTURE_DIRTY_TILE_MAP_EXTENSION_NAME, extension_properties.extensionName)) {
								capabilities_.dirty_tile_map = true;
								enabled_device_extensions.push_back(VK_NV_TEXTURE_DIRTY_TILE_MAP_EXTENSION_NAME);
							}*/
						}

#if _DEBUG
						result = symbols_.vkEnumerateDeviceLayerProperties(physical_device, &count, nullptr);
						CHECK_RESULT("vkEnumerateDeviceLayerProperties failed")

							std::vector<VkLayerProperties> device_layer_properties(count);
						result = symbols_.vkEnumerateDeviceLayerProperties(physical_device, &count, device_layer_properties.data());
						CHECK_RESULT("vkEnumerateDeviceLayerProperties failed")

							vulkan_info.push_back("   Layers:");
						;
						for (auto const &layer_properties : device_layer_properties) {
							vulkan_info.push_back(LogHelpers::StringFormat("    - %s [%d]", layer_properties.layerName, layer_properties.implementationVersion));

							result = symbols_.vkEnumerateDeviceExtensionProperties(physical_device, layer_properties.layerName, &count, nullptr);
							CHECK_RESULT("vkEnumerateDeviceExtensionProperties failed")

								std::vector<VkExtensionProperties> layer_extension_properties(count);
							result = symbols_.vkEnumerateDeviceExtensionProperties(physical_device, layer_properties.layerName, &count, layer_extension_properties.data());
							CHECK_RESULT("vkEnumerateDeviceExtensionProperties failed")

								vulkan_info.push_back("      Extensions:");
							for (auto const &extension_properties : layer_extension_properties) {
								vulkan_info.push_back(LogHelpers::StringFormat("       - %s [%d]", extension_properties.extensionName, extension_properties.specVersion));
							}
						}
#endif

						if ((VK_NULL_HANDLE == device_) &&
							(queue_family_properties.size() > graphics_queue_family_index) &&
							(queue_family_properties.size() > transfer_queue_family_index)) {
							//(queue_family_properties.size() > transfer_queue_family_index) &&
							//capabilities_.get_memory_requirements2 &&
							//capabilities_.external_memory &&
							//capabilities_.external_memory_fd &&
							//capabilities_.external_semaphore &&
							//capabilities_.external_semaphore_fd) {
							physical_device_ = physical_device;
							graphics_queue_family_index_ = graphics_queue_family_index;
							transfer_queue_family_index_ = (queue_family_properties.size() > transfer_queue_family_index) ? transfer_queue_family_index : graphics_queue_family_index;
							//timestamp_bitmask_ = BitMask(queue_family_properties[graphics_queue_family_index_].queueFamilyProperties.timestampValidBits - 1);

							std::vector<VkDeviceQueueCreateInfo> device_queue_create_infos;
							float const                          priorities[] = { 1.0, 0.0 };

							VkDeviceQueueCreateInfo device_queue_create_info;
							device_queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
							device_queue_create_info.pNext = nullptr;
							device_queue_create_info.flags = 0;
							device_queue_create_info.queueFamilyIndex = graphics_queue_family_index_;
							device_queue_create_info.pQueuePriorities = priorities;

							if (graphics_queue_family_index_ == transfer_queue_family_index_) {
								device_queue_create_info.queueCount = 2;
							}
							else {
								device_queue_create_info.queueCount = 1;
								device_queue_create_infos.push_back(device_queue_create_info);
								device_queue_create_info.queueFamilyIndex = transfer_queue_family_index_;
								device_queue_create_info.pQueuePriorities = &priorities[1];
							}
							device_queue_create_infos.push_back(device_queue_create_info);

							VkDeviceCreateInfo device_create_info;
							device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
							device_create_info.pNext = nullptr;
							device_create_info.flags = 0;
							device_create_info.queueCreateInfoCount = static_cast<uint32_t>(device_queue_create_infos.size());
							device_create_info.pQueueCreateInfos = device_queue_create_infos.data();
							device_create_info.enabledLayerCount = 0;
							device_create_info.ppEnabledLayerNames = nullptr;
							device_create_info.enabledExtensionCount = static_cast<uint32_t>(enabled_device_extensions.size());
							device_create_info.ppEnabledExtensionNames = enabled_device_extensions.data();
							device_create_info.pEnabledFeatures = nullptr;

							if (VK_NULL_HANDLE == device_param) {
								result = symbols_.vkCreateDevice(physical_device_, &device_create_info, nullptr, &device_);
								CHECK_RESULT("vkCreateDevice failed")
							}
							else {
								device_ = device_param;
								device_is_local_ = true;
							}

							if (!vksamples::common::vk::LoadDeviceSymbols(device_, symbols_.vkGetDeviceProcAddr, symbols_)) {
								std::cerr << "Failed to load device symbols" << std::endl;
								return false;
							}

							if (!device_is_local_) {
								symbols_.vkGetDeviceQueue(device_, graphics_queue_family_index_, 0, &queues_.graphics_queue);
								if (graphics_queue_family_index_ == transfer_queue_family_index_) {
									symbols_.vkGetDeviceQueue(device_, transfer_queue_family_index_, 1, &queues_.transfer_queue);
								}
								else {
									symbols_.vkGetDeviceQueue(device_, transfer_queue_family_index_, 0, &queues_.transfer_queue);
								}
								CHECK_RESULT("vkGetDeviceQueue failed")
							}
						}
					}

					if (VK_NULL_HANDLE == device_) {
						std::cerr << "Could not find usable physical device!" << std::endl;
						return false;
					}
					return true;
				}

				bool Context::Free() {
					// make sure no pools are created during free process
					std::lock_guard<std::mutex> guard(pool_create_mutex_);

					if (!device_is_local_ && (VK_NULL_HANDLE != device_)) {
						symbols_.vkDeviceWaitIdle(device_);

						for (auto &pools_entry : pools_) {
							symbols_.vkDestroyCommandPool(device_, pools_entry.second.graphics_command_pool, NULL);
							symbols_.vkDestroyCommandPool(device_, pools_entry.second.transfer_command_pool, NULL);
						}
						pools_.clear();

						symbols_.vkDestroyDevice(device_, NULL);
						device_ = VK_NULL_HANDLE;
						device_is_local_ = false;
					}
#if _DEBUG
					if (VK_NULL_HANDLE != debug_report_callback_) {
						symbols_.vkDestroyDebugReportCallbackEXT(instance_, debug_report_callback_, NULL);
						debug_report_callback_ = VK_NULL_HANDLE;
					}
#endif
					if (!instance_is_local_ && (VK_NULL_HANDLE != instance_)) {
						symbols_.vkDestroyInstance(instance_, NULL);
						instance_ = VK_NULL_HANDLE;
						instance_is_local_ = false;
					}

					return true;
				}

				//uint64_t Context::TimestampToNS(uint64_t timestamp) const {
					//return static_cast<uint64_t>(static_cast<long double>(timestamp & timestamp_bitmask_) * timestamp_period_);
				//}

				VkPools const &Context::Pools() const {
					//if the app gave us inst/phys/dev then we never made queues (and never made pools)
					if (device_is_local_) {
						static VkPools errorPools;
						errorPools.graphics_command_pool = VK_NULL_HANDLE;
						errorPools.transfer_command_pool = VK_NULL_HANDLE;
						return errorPools;
					}

					std::thread::id current_thread = std::this_thread::get_id();

					// not a const ref so it can be updated after the lock_guard
					auto it = pools_.find(current_thread);
					if (it == pools_.end()) {
						std::lock_guard<std::mutex> guard(pool_create_mutex_);
						it = pools_.find(current_thread);
						if (it == pools_.end()) {
							auto                   &new_pools = pools_[current_thread];
							VkCommandPoolCreateInfo command_pool_create_info;
							command_pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
							command_pool_create_info.pNext = NULL,
							command_pool_create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
							command_pool_create_info.queueFamilyIndex = graphics_queue_family_index_;
							symbols_.vkCreateCommandPool(device_, &command_pool_create_info, NULL, &new_pools.graphics_command_pool);
							command_pool_create_info.queueFamilyIndex = transfer_queue_family_index_;
							symbols_.vkCreateCommandPool(device_, &command_pool_create_info, NULL, &new_pools.transfer_command_pool);

							return new_pools;
						}
					}
					return it->second;
				}

			} // namespace vk
		} // namespace legacy
	} // namespace common
} // namespace vksamples