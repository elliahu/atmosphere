#pragma once
#include <vector>
#include <vulkan/vulkan.h>
#include "hammock/core/HandmadeMath.h"

namespace hammock {
    struct Vertex {
        HmckVec3 position{};
        HmckVec3 normal{};
        HmckVec2 uv{};
        HmckVec4 tangent{};

        static std::vector<VkVertexInputAttributeDescription> vertexInputAttributeDescriptions() {
            return {
                    {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, position)},
                    {1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal)},
                    {2, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, uv)},
                    {3, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, tangent)}
            };
        }

        static std::vector<VkVertexInputBindingDescription> vertexInputBindingDescriptions() {
            return {
                    {
                        .binding = 0,
                        .stride = sizeof(Vertex),
                        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
                    }
            };
        }
    };

    struct Triangle {
        Vertex v0,v1,v2;
    };

}
