#pragma once
#include <cstdint>
#include <vector>
#include <hammock/core/HandmadeMath.h>

#include "hammock/core/Types.h"
#include "hammock/scene/Vertex.h"

namespace hammock {
    struct Geometry {

        enum VisibilityFlags : int32_t {
            VISIBILITY_NONE = 0,
            VISIBILITY_VISIBLE = 1 << 0,
            VISIBILITY_OPAQUE = 1 << 1,
            VISIBILITY_BLEND = 1 << 2,
            VISIBILITY_CASTS_SHADOW = 1 << 3,
            VISIBILITY_RECEIVES_SHADOW = 1 << 4,
        };

        struct MeshInstance {
            typedef int32_t Index;

            HmckMat4 transform;
            int32_t visibilityFlags;
            HmckVec3 baseColorFactor;
            HmckVec3 metallicRoughnessAlphaCutOffFactor;
            Index baseColorTextureIndex;
            Index normalTextureIndex;
            Index metallicRoughnessTextureIndex;
            Index occlusionTextureIndex;
            uint32_t firstIndex;
            uint32_t indexCount;
        };

        std::vector<MeshInstance> renderMeshes;
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
        std::vector<ResourceHandle> textures;

    };
}
