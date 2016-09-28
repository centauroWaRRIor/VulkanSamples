#pragma once
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>
#include <array>
#include <vector>
#include <unordered_map>

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

		// Required for using a vertex as a key in a hash table (unordered map)
		bool operator==(const Vertex& other) const {
			return pos == other.pos && color == other.color && texCoord == other.texCoord;
		}
	};

	// Interleaving vertex attributes
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;

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

		// We should keep only the unique vertices and use the index buffer to reuse them whenever they come up
		std::unordered_map<Vertex, int> uniqueVertices = {};

		// We're going to combine all of the faces in the file into a single model, so just iterate over all of the shapes:
		for (const auto& shape : shapes) {
			for (const auto& index : shape.mesh.indices) {
				Vertex vertex = {};

				// The triangulation feature has already made sure that there are three vertices per face.
				vertex.pos = {
					attrib.vertices[3 * index.vertex_index + 0],
					attrib.vertices[3 * index.vertex_index + 1],
					attrib.vertices[3 * index.vertex_index + 2]
				};

				vertex.texCoord = {
					attrib.texcoords[2 * index.texcoord_index + 0],
					// The problem is that the origin of texture coordinates in Vulkan is the top-left corner, whereas 
					// the OBJ format assumes the bottom-left corner. Solve this by flipping the vertical component 
					// of the texture coordinates:
					1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
				};

				
				// We check if we've already seen a vertex with the exact same position and texture coordinates before. 
				// If not, we add it to vertices and store its index in the uniqueVertices container. 
				// After that we add the index of the new vertex to indices. If we've seen the exact same vertex before, 
				// then we look up its index in uniqueVertices and store that index in indices.
				if (uniqueVertices.count(vertex) == 0) {
					uniqueVertices[vertex] = vertices.size();
					vertices.push_back(vertex);
				}

				indices.push_back(uniqueVertices[vertex]);
			}
		}
	}
}

// custom specialization of std::hash can be injected in namespace std
// required for using a vertex as a key in a hash table
namespace std {
	template<> struct hash<VertexBuffer::Vertex> {
		size_t operator()(VertexBuffer::Vertex const& vertex) const {
			return ((hash<glm::vec3>()(vertex.pos) ^ (hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^ (hash<glm::vec2>()(vertex.texCoord) << 1);
		}
	};
}