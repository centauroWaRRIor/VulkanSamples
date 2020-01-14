#pragma once

#include <map>
#include <mutex>
#include <thread>

#include <icontext.h>

namespace vksamples {
	namespace common {
		namespace legacy {
			namespace vk {

				class Context : public vksamples::common::legacy::vk::IContext {
				public:
					Context();
					~Context();

					bool Initialize(VkApplicationInfo const &application_info,
						VkInstance const         instance_param = VK_NULL_HANDLE,
						VkPhysicalDevice const   physical_device_param = VK_NULL_HANDLE,
						VkDevice const           device_param = VK_NULL_HANDLE);

					VkInstance const                          &Instance() const override { return instance_; }
					VkPhysicalDevice const                    &PhysicalDevice() const override { return physical_device_; }
					VkDevice const                            &Device() const override { return device_; }
					vksamples::common::legacy::vk::VkQueues const       &Queues() const override { return queues_; }
					vksamples::common::legacy::vk::VkSymbols const      &Symbols() const override { return symbols_; }
					vksamples::common::legacy::vk::VkPools const        &Pools() const override;
					vksamples::common::legacy::vk::VkCapabilities const &Capabilities() const override { return capabilities_; }

					uint32_t GraphicsQueueFamilyIndex() const override { return graphics_queue_family_index_; }
					uint32_t TransferQueueFamilyIndex() const override { return transfer_queue_family_index_; }

					//uint64_t TimestampToNS(uint64_t timestamp) const override;

				private:
					bool Free();

					VkInstance                          instance_ = VK_NULL_HANDLE;
					VkPhysicalDevice                    physical_device_ = VK_NULL_HANDLE;
					VkDevice                            device_ = VK_NULL_HANDLE;
					vksamples::common::legacy::vk::VkQueues       queues_ = {};
					vksamples::common::legacy::vk::VkSymbols      symbols_ = {};
					vksamples::common::legacy::vk::VkCapabilities capabilities_ = {};
#if _DEBUG
					VkDebugReportCallbackEXT debug_report_callback_ = VK_NULL_HANDLE;
#endif

					uint32_t graphics_queue_family_index_ = 0;
					uint32_t transfer_queue_family_index_ = 0;

					//uint64_t timestamp_bitmask_ = 0;
					//float    timestamp_period_ = 0;

					bool instance_is_local_ = false;
					bool device_is_local_ = false;

					mutable std::map<std::thread::id, VkPools> pools_;
					mutable std::mutex                         pool_create_mutex_;
				};

			} // namespace vk
		} // namespace legacy
	} // namespace common
} // namespace vksamples