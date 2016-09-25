#pragma once
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <array>
#include <vector>

namespace VertexBuffer {

	struct Vertex {
		// GLM provides C++ types that match those use in GLSL
		glm::vec2 pos;
		glm::vec3 color;
		glm::vec2 texCoord;

		// Need to tell Vulkan how to pass this data into the vertex
		// shader once its been uploaded into GPU memory. Two methods
		// below achieve that.

		static VkVertexInputBindingDescription getBindingDescription() {
			// A vertex binding describes at which rate to load data from memory throughout 
			// the vertices. It specifies the number of bytes between data entries and whether 
			// to move to the next data entry after each vertex or after each instance.
			VkVertexInputBindingDescription bindingDescription = {};
			bindingDescription.binding = 0;
			bindingDescription.stride = sizeof(Vertex);
			bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
			// All of our per - vertex data is packed together in one array, so we're only going to 
			// have one binding. The binding parameter specifies the index of the binding in the array of bindings. 
			// The stride parameter specifies the number of bytes from one entry to the next, and the inputRate parameter 
			// can have one of the following values:
			// VK_VERTEX_INPUT_RATE_VERTEX: Move to the next data entry after each vertex
			// VK_VERTEX_INPUT_RATE_INSTANCE : Move to the next data entry after each instance
			return bindingDescription;
		}

		static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions() {
			std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions = {};
			// An attribute description struct describes how to extract a vertex attribute from 
			// a chunk of vertex data originating from a binding description. We have two attributes, 
			// position and color, so we need two attribute description structs.
			attributeDescriptions[0].binding = 0;
			attributeDescriptions[0].location = 0;
			attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
			attributeDescriptions[0].offset = offsetof(Vertex, pos);
			// The binding parameter tells Vulkan from which binding the per - vertex data comes.
			// The location parameter references the location directive of the input in the vertex shader.
			attributeDescriptions[1].binding = 0;
			attributeDescriptions[1].location = 1;
			attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
			attributeDescriptions[1].offset = offsetof(Vertex, color);
			// The format parameter implicitly defines the byte size of attribute data and the offset 
			// parameter specifies the number of bytes since the start of the per - vertex data to read from.
			attributeDescriptions[2].binding = 0;
			attributeDescriptions[2].location = 2;
			attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
			attributeDescriptions[2].offset = offsetof(Vertex, texCoord);
			return attributeDescriptions;
		}
	};

	// Interleaving vertex attributes
	const std::vector<Vertex> vertices = {
		{ { -0.5f, -0.5f },{ 1.0f, 0.0f, 0.0f },{ 0.0f, 0.0f } },
		{ { 0.5f, -0.5f },{ 0.0f, 1.0f, 0.0f },{ 1.0f, 0.0f } },
		{ { 0.5f, 0.5f },{ 0.0f, 0.0f, 1.0f },{ 1.0f, 1.0f } },
		{ { -0.5f, 0.5f },{ 1.0f, 1.0f, 1.0f },{ 0.0f, 1.0f } }
	};

	const std::vector<uint16_t> indices = {
		0, 1, 2, 2, 3, 0
	};

}