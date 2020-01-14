#include <helpers.h>

#include <mutex>

#include <icontext.h>

//#include <ml/gfx/shared_gfx_memory.h>

namespace vksamples {
	namespace common {
		namespace legacy {
			namespace vk {

				static bool       memory_properties_fetched = false;
				static std::mutex memory_properties_fetch_mutex;

				static VkPhysicalDeviceMemoryProperties2KHR physical_device_memory_properties = {
				  /*.sType =*/ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2_KHR,
				  /*.pNext =*/ NULL };

				static VkMemoryDedicatedAllocateInfoKHR memory_dedicated_allocate_info = {
				  /*.sType =*/ VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO_KHR,
				  /*.pNext =*/ NULL,
				  /*.image =*/ VK_NULL_HANDLE };

				static VkExportMemoryAllocateInfoKHR export_memory_allocate_info = {
				  /*.sType =*/ VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO_KHR,
				  /*.pNext =*/ NULL,
				  /*.handleTypes =*/ VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT_KHR };

				static VkImportMemoryFdInfoKHR import_memory_fd_info = {
				  /*.sType =*/ VK_STRUCTURE_TYPE_IMPORT_MEMORY_FD_INFO_KHR,
				  /*.pNext =*/ NULL,
				  /*.handleType =*/ VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT_KHR,
				};

				static VkMemoryAllocateInfo memory_allocate_info = {
				  /*.sType =*/ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };

				bool vksamples::common::legacy::vk::Helpers::AllocateMemory(vksamples::common::legacy::vk::IContext const &vulkan_context,
					VkMemoryRequirements const          &memory_requirements,
					VkMemoryPropertyFlags const         &required_memory_properties,
					VkImage const                       &dedicated_image,
					VkBuffer const                      &dedicated_buffer,
					bool const                          &make_exportable,
					MemoryAllocationVK                  &out_memory_allocation) {
					if (!memory_properties_fetched) {
						std::lock_guard<std::mutex> lock(memory_properties_fetch_mutex);
						if (!memory_properties_fetched) {
							vulkan_context.Symbols().vkGetPhysicalDeviceMemoryProperties2KHR(vulkan_context.PhysicalDevice(), &physical_device_memory_properties);
						}
					}

					if (VK_NULL_HANDLE != dedicated_image && VK_NULL_HANDLE != dedicated_buffer) {
						std::cerr << "memory can either be dedicated for an image or a buffer but not both" << std::endl;
						return false;
					}

					out_memory_allocation.size = memory_requirements.size;
					out_memory_allocation.memory_type_index = GetMemoryTypeIndex(physical_device_memory_properties.memoryProperties,
						memory_requirements.memoryTypeBits,
						required_memory_properties);
					out_memory_allocation.dedicated = (VK_NULL_HANDLE != dedicated_image || VK_NULL_HANDLE != dedicated_buffer);

					if (make_exportable) {
						memory_allocate_info.pNext = &export_memory_allocate_info;
						export_memory_allocate_info.pNext = out_memory_allocation.dedicated ? &memory_dedicated_allocate_info : NULL;
					}
					else {
						memory_allocate_info.pNext = out_memory_allocation.dedicated ? &memory_dedicated_allocate_info : NULL;
					}

					memory_allocate_info.allocationSize = out_memory_allocation.size;
					memory_allocate_info.memoryTypeIndex = out_memory_allocation.memory_type_index;

					memory_dedicated_allocate_info.image = dedicated_image;
					memory_dedicated_allocate_info.buffer = dedicated_buffer;

					VkResult result = vulkan_context.Symbols().vkAllocateMemory(vulkan_context.Device(),
						&memory_allocate_info,
						NULL,
						&out_memory_allocation.device_memory);
					if (VK_SUCCESS != result) {
						return false;
					}

					return true;
				}

				/*bool vksamples::common::legacy::vk::Helpers::ImportMemory(vksamples::common::legacy::vk::IContext const &vulkan_context,
					MemoryAllocation const              &shared_memory_allocation,
					VkImage const                       &dedicated_image,
					VkBuffer const                      &dedicated_buffer,
					MemoryAllocationVK                  &out_memory_allocation) {
					out_memory_allocation.size = shared_memory_allocation.size_;
					out_memory_allocation.memory_type_index = shared_memory_allocation.memory_type_index_;
					out_memory_allocation.dedicated = shared_memory_allocation.dedicated_;

					import_memory_fd_info.fd = shared_memory_allocation.fd_;

					memory_allocate_info.pNext = &import_memory_fd_info;
					import_memory_fd_info.pNext = out_memory_allocation.dedicated ? &memory_dedicated_allocate_info : NULL;

					memory_allocate_info.allocationSize = out_memory_allocation.size;
					memory_allocate_info.memoryTypeIndex = out_memory_allocation.memory_type_index;

					memory_dedicated_allocate_info.image = dedicated_image;
					memory_dedicated_allocate_info.buffer = dedicated_buffer;

					VkResult result = vulkan_context.Symbols().vkAllocateMemory(vulkan_context.Device(),
						&memory_allocate_info,
						NULL,
						&out_memory_allocation.device_memory);
					if (VK_SUCCESS != result) {
						return false;
					}

					return true;
				}*/

			} // namespace vk
		} // namespace legacy
	} // namespace common
} // namespace vksamples