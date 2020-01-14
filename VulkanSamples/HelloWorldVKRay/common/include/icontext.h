#pragma once

#include <memory>

#include <noncopyable.h>
#include <symbols.h>

namespace vksamples {
	namespace common {
		namespace legacy {
			namespace vk {

				struct VkSymbols
					: public vksamples::common::vk::GlobalSymbols
					, public vksamples::common::vk::InstanceSymbols
					, public vksamples::common::vk::DeviceSymbols {
				};

				struct VkPools {
					VkCommandPool graphics_command_pool = VK_NULL_HANDLE;
					VkCommandPool transfer_command_pool = VK_NULL_HANDLE;
				};

				struct VkQueues {
					VkQueue graphics_queue = VK_NULL_HANDLE;
					VkQueue transfer_queue = VK_NULL_HANDLE;
				};

				// clang-format off
				struct VkCapabilities {
					//bool get_physical_device_properties2 : 1;
					//bool get_memory_requirements2 : 1;
					//bool dedicated_allocation : 1;
					//bool external_memory_capabilities : 1;
					//bool external_memory : 1;
					//bool external_memory_fd : 1;
					//bool external_semaphore_capabilities : 1;
					//bool external_semaphore : 1;
					//bool external_semaphore_fd : 1;
					//bool dirty_tile_map : 1;
#ifdef _DEBUG
					bool debug_report : 1;
#endif
				};
				// clang-format on

				class IContext : public NonCopyable {
				public:
					IContext() = default;
					virtual ~IContext() = default;

					virtual VkInstance const       &Instance() const = 0;
					virtual VkPhysicalDevice const &PhysicalDevice() const = 0;
					virtual VkDevice const         &Device() const = 0;
					virtual VkQueues const         &Queues() const = 0;
					virtual VkSymbols const        &Symbols() const = 0;
					virtual VkPools const          &Pools() const = 0;
					virtual VkCapabilities const   &Capabilities() const = 0;

					virtual uint32_t GraphicsQueueFamilyIndex() const = 0;
					virtual uint32_t TransferQueueFamilyIndex() const = 0;

					//virtual uint64_t TimestampToNS(uint64_t timestamp) const = 0;

					static std::shared_ptr<IContext> Create(VkApplicationInfo const &application_info,
						VkInstance const         instance = VK_NULL_HANDLE,
						VkPhysicalDevice const   physical_device = VK_NULL_HANDLE,
						VkDevice const           logical_device = VK_NULL_HANDLE);
				};

			} // namespace vk
		} // namespace legacy
	} // namespace common
} // namespace vksamples