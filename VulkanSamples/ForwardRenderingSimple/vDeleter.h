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

	// overloading the address of operator, useful for 
	// making wrapper transparent
	const T* operator &() const {
		return &object;
	}

	// It overloads the address - of, assignment, comparison and casting operators to make 
	// the wrapper as transparent as possible. When the wrapped object goes out of scope, 
	// the destructor is invoked, which in turn calls the cleanup function we specified.

	// The address - of operator returns a constant pointer to make sure that the object 
	// within the wrapper is not unexpectedly changed. If you want to replace the handle 
	// within the wrapper through a pointer, then you should use the replace() function 
	// instead. It will invoke the cleanup function for the existing handle so that you 
	// can safely overwrite it afterwards.

	//	There is also a default constructor with a dummy deleter function that can be used 
	// to initialize it later, which will be useful for lists of deleters.

	T* replace() {
		cleanup();
		return &object;
	}

	void operator=(T rhs) {
		cleanup();
		object = rhs;
	}

	template<typename V>
	bool operator==(V rhs) {
		return object == T(rhs);
	}

	// overloading the casting or conversion operator. 
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
