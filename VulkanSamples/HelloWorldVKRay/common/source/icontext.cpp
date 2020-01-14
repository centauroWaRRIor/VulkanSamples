#include <context.h>

namespace vksamples {
	namespace common {
		namespace legacy {
			namespace vk {

				std::shared_ptr<IContext> IContext::Create(VkApplicationInfo const &application_info,
					VkInstance const         instance,
					VkPhysicalDevice const   physical_device,
					VkDevice const           logical_device) {
					// factory create the context
					auto vk_context = std::make_shared<Context>();

					// only if creation succeeds and we can initialize the context did the creation
					//  suceed
					if (vk_context && vk_context->Initialize(application_info, instance, physical_device, logical_device)) {
						return vk_context;
					}

					return nullptr;
				}

			} // namespace vk
		} // namespace legacy
	} // namespace common
} // namespace vksamples