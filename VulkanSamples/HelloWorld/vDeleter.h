#pragma once
#include <functional> // used for lambda functions in the resource management 

// Thin vulkan objects wrapper to facilitate its resource management. This model
// follows the RAII principle as it cleans up the object the moment it goes out of
// scope
template <typename T>
class VDeleter {
public:
	// default constructor with dummy deleter useful for list of deleters
	VDeleter() : VDeleter([](T, VkAllocationCallbacks*) {}) {}

	VDeleter(std::function<void(T, VkAllocationCallbacks*)> deletef) {
		this->deleter = [=](T obj) { deletef(obj, nullptr); };
	}

	VDeleter(const VDeleter<VkInstance>& instance, std::function<void(VkInstance, T, VkAllocationCallbacks*)> deletef) {
		this->deleter = [&instance, deletef](T obj) { deletef(instance, obj, nullptr); };
	}

	VDeleter(const VDeleter<VkDevice>& device, std::function<void(VkDevice, T, VkAllocationCallbacks*)> deletef) {
		this->deleter = [&device, deletef](T obj) { deletef(device, obj, nullptr); };
	}

	~VDeleter() {
		cleanup();
	}

	// overloading the address of operator, useful for re-assinging an object like this
	// *&swapChain = newSwapChain; 
	// where:
	// VDeleter<VkSwapchainKHR> swapChain{ device, vkDestroySwapchainKHR };
	// VkSwapchainKHR newSwapChain;
	T* operator &() {
		cleanup();
		return &object;
	}

	// overloading the casting or conversion operator. This is to make the wrapper
	// transparent
	operator T() const {
		return object;
	}

private:
	T object{ VK_NULL_HANDLE };
	std::function<void(T)> deleter;

	void cleanup() {
		if (object != VK_NULL_HANDLE) {
			deleter(object);
		}
		object = VK_NULL_HANDLE;
	}
};
