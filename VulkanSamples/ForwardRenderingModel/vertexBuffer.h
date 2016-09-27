#pragma once
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <array>
#include <vector>

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

namespace VertexBuffer {

	const std::string MODEL_PATH = "models/chalet.obj";
	const std::string TEXTURE_PATH = "textures/chalet.jpg";

	struct Vertex {
		// GLM provides C++ types that match those use in GLSL
		glm::vec3 pos;
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
			attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
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

	void loadModel() {
		// An OBJ file consists of positions, normals, texture coordinates and faces. Faces consist of an 
		// arbitrary amount of vertices, where each vertex refers to a position, normal and / or texture 
		// coordinate by index. This makes it possible to not just reuse entire vertices, but also individual 
		// attributes.

		// The attrib container holds all of the positions, normals and texture coordinates in its attrib.vertices, 
		// attrib.normals and attrib.texcoords vectors.
		tinyobj::attrib_t attrib;
		// The shapes container contains all of the separate objects and their faces. Each face consists of an array 
		// of vertices, and each vertex contains the indices of the position, normal and texture coordinate attributes.
		std::vector<tinyobj::shape_t> shapes;
		std::vector<tinyobj::material_t> materials;
		std::string err;

		// Faces in OBJ files can actually contain an arbitrary number of vertices, whereas our application can only 
		// render triangles. Luckily the LoadObj has an optional parameter to automatically triangulate such faces, 
		// which is enabled by default
		if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &err, MODEL_PATH.c_str())) {
			throw std::runtime_error(err);
		}
	}

	// Interleaving vertex attributes
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;
}