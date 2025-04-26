#pragma once

#include <tiny_gltf.h>

#include "hammock/scene/Geometry.h"
#include "hammock/core/CoreUtils.h"
#include "hammock/scene/Vertex.h"
#include "tiny_obj_loader.h"
#include <algorithm>
#include <functional>
#include "hammock/core/Types.h"
#include "hammock/core/ResourceManager.h"

namespace hammock {
    namespace gltf = tinygltf;

    struct Loader {
        Geometry &state;
        Device &device;
        ResourceManager &rm;

        explicit Loader(Geometry &state, Device &device, ResourceManager &rm): state(state), device{device},
                                                                               rm{rm} {
        }

        Loader &loadglTF(const std::string &filename) {
            tinygltf::Model gltfModel;
            tinygltf::TinyGLTF gltfContext;

            std::string error;
            std::string warning;

            bool binary = false;
            size_t extpos = filename.rfind('.', filename.length());
            if (extpos != std::string::npos) {
                binary = (filename.substr(extpos + 1, filename.length() - extpos) == "glb");
            }

            bool fileLoaded = binary
                                  ? gltfContext.LoadBinaryFromFile(&gltfModel, &error, &warning, filename.c_str())
                                  : gltfContext.LoadASCIIFromFile(&gltfModel, &error, &warning, filename.c_str());

            if (!fileLoaded) {
                Logger::log(LOG_LEVEL_ERROR, error.c_str());
                throw std::runtime_error(error.c_str());
            }

            auto extensions = gltfModel.extensionsUsed;
            for (auto &extension: extensions) {
                Logger::log(LOG_LEVEL_DEBUG, "glTF Loader: Using extension: %s\n", extension.c_str());
                // TODO use the extensions
            }

            struct gVertex {
                HmckVec3 position{};
                HmckVec3 normal{};
                HmckVec2 uv{};
                HmckVec4 tangent{};
            };

            struct gPrimitive {
                uint32_t firstIndex;
                uint32_t indexCount;
                int32_t materialIndex;
            };

            struct gMesh {
                std::vector<gPrimitive> primitives;
            };

            struct gNode {
                gNode *parent;
                std::vector<gNode *> children;
                gMesh mesh;
                HmckMat4 matrix;
                std::string name;
                bool visible = true;

                ~gNode() {
                    for (auto &child: children) {
                        delete child;
                    }
                }
            };

            struct gMaterial {
                HmckVec3 baseColorFactor{1.0f, 1.0f, 0.0f};
                float metallicFactor = 0.0f;
                float roughnessFactor = 0.0f;
                int32_t albedoTexture = -1;; // this is not TextureHandle !
                int32_t normalTexture = -1; // this is not TextureHandle !
                int32_t metallicRoughnessTexture = -1; // this is not TextureHandle !
                int32_t occlusionTexture = -1; // this is not TextureHandle !
                std::string alphaMode = "OPAQUE";
                float alphaCutOff = 0.5f;
                bool doubleSided = false;
            };

            struct gImage {
                ResourceHandle textureHandle;
                std::string name;
            };

            struct gTexture {
                int32_t imageIndex;
            };

            std::vector<gImage> images;
            std::vector<gTexture> textures;
            std::vector<gMaterial> materials;
            std::vector<gNode *> nodes;
            std::vector<gVertex> vertexBuffer;
            std::vector<uint32_t> indexBuffer;


            // Load images
            for (size_t i = 0; i < gltfModel.images.size(); i++) {
                gltf::Image &glTFImage = gltfModel.images[i];

                // Get the image data from the glTF loader
                unsigned char *buffer = nullptr;
                // We convert RGB-only images to RGBA, as most devices don't support RGB-formats in Vulkan
                if (glTFImage.component == 3) {
                    throw std::runtime_error("3-component images not supported");
                }

                buffer = &glTFImage.image[0];

                ResourceHandle imageHandle = rm.createResource<Image>(glTFImage.name, ImageDesc{
                                                                          .width = static_cast<uint32_t>(glTFImage.
                                                                              width),
                                                                          .height = static_cast<uint32_t>(glTFImage.
                                                                              height),
                                                                          .channels = 4,
                                                                          .mips = getNumberOfMipLevels(
                                                                              static_cast<uint32_t>(glTFImage.width),
                                                                              static_cast<uint32_t>(glTFImage.height)),
                                                                          .format = VK_FORMAT_R8G8B8A8_UNORM,
                                                                          .usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                                                                              VK_IMAGE_USAGE_SAMPLED_BIT,
                                                                          .imageType = VK_IMAGE_TYPE_2D,
                                                                          .imageViewType = VK_IMAGE_VIEW_TYPE_2D,
                                                                      });

                Image *image = rm.getResource<Image>(imageHandle);
                image->queueImageLayoutTransition(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

                images.push_back(gImage{imageHandle, glTFImage.name});

                ResourceHandle stagingBufferHandle = rm.createResource<Buffer>(
                    "staging-buffer-for-gltf-image" + glTFImage.name, BufferDesc{
                        .instanceSize = sizeof(unsigned char),
                        .instanceCount = static_cast<uint32_t>(glTFImage.width * glTFImage.height * 4),
                        .usageFlags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                        .allocationFlags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
                    });
                Buffer *stagingBuffer = rm.getResource<Buffer>(stagingBufferHandle);
                stagingBuffer->map();
                stagingBuffer->writeToBuffer(buffer);

                device.copyBufferToImage(stagingBuffer->getBuffer(), image->getImage(),
                                         static_cast<uint32_t>(glTFImage.width),
                                         static_cast<uint32_t>(glTFImage.height), 1, 0, 1);

                image->queueImageLayoutTransition(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

            }

            // load textures
            textures.resize(gltfModel.textures.size());
            for (size_t i = 0; i < gltfModel.textures.size(); i++) {
                textures[i].imageIndex = gltfModel.textures[i].source;
            }

            // load materials
            for (size_t i = 0; i < gltfModel.materials.size(); i++) {
                tinygltf::Material glTFMaterial = gltfModel.materials[i];
                gMaterial material{};
                // Get the base color factor
                if (glTFMaterial.values.contains("baseColorFactor")) {
                    material.baseColorFactor = HmckVec3{
                        static_cast<float>(glTFMaterial.values["baseColorFactor"].ColorFactor()[0]),
                        static_cast<float>(glTFMaterial.values["baseColorFactor"].ColorFactor()[1]),
                        static_cast<float>(glTFMaterial.values["baseColorFactor"].ColorFactor()[2])
                    };
                }
                // Get metallic factor
                if (glTFMaterial.values.contains("metallicFactor")) {
                    material.metallicFactor = static_cast<float>(glTFMaterial.values["metallicFactor"].Factor());
                }
                // Get roughness factor
                if (glTFMaterial.values.contains("roughnessFactor")) {
                    material.roughnessFactor = static_cast<float>(glTFMaterial.values["roughnessFactor"].Factor());
                }
                // Get base color texture index
                if (glTFMaterial.values.contains("baseColorTexture")) {
                    material.albedoTexture =
                            glTFMaterial.values["baseColorTexture"].TextureIndex();
                }
                // Get normal texture index
                if (glTFMaterial.additionalValues.contains("normalTexture")) {
                    material.normalTexture =
                            glTFMaterial.additionalValues["normalTexture"].TextureIndex();
                }
                // Get rough/metal texture index
                if (glTFMaterial.values.contains("metallicRoughnessTexture")) {
                    material.metallicRoughnessTexture =
                            glTFMaterial.values["metallicRoughnessTexture"].TextureIndex();
                }
                // Get occlusion texture index
                if (glTFMaterial.additionalValues.contains("occlusionTexture")) {
                    material.occlusionTexture =
                            glTFMaterial.additionalValues["occlusionTexture"].TextureIndex();
                }
                material.alphaMode = glTFMaterial.alphaMode;
                material.alphaCutOff = static_cast<float>(glTFMaterial.alphaCutoff);
                material.doubleSided = glTFMaterial.doubleSided;

                materials.push_back(material);
            }

            // load nodes recursively
            std::function<void(const tinygltf::Node &, const tinygltf::Model &, gNode *, std::vector<uint32_t> &,
                               std::vector<gVertex> &)> loadNode;

            loadNode = [&](const tinygltf::Node &inputNode, const tinygltf::Model &input, gNode *parent,
                           std::vector<uint32_t> &indexBuffer, std::vector<gVertex> &vertexBuffer) {
                gNode *node = new gNode{};
                node->name = inputNode.name;
                node->parent = parent;

                // Get the local node matrix
                // It's either made up from translation, rotation, scale or a 4x4 matrix
                node->matrix = HmckMat4{
                    {
                        {1.0f, 0.0f, 0.0f, 0.0f},
                        {0.0f, 1.0f, 0.0f, 0.0f},
                        {0.0f, 0.0f, 1.0f, 0.0f},
                        {0.0f, 0.0f, 0.0f, 1.0f}
                    }
                };
                if (inputNode.matrix.size() == 16) {
                    node->matrix = HmckMat4{
                        static_cast<float>(inputNode.matrix[0]), static_cast<float>(inputNode.matrix[1]),
                        static_cast<float>(inputNode.matrix[2]), static_cast<float>(inputNode.matrix[3]),
                        static_cast<float>(inputNode.matrix[4]), static_cast<float>(inputNode.matrix[5]),
                        static_cast<float>(inputNode.matrix[6]), static_cast<float>(inputNode.matrix[7]),
                        static_cast<float>(inputNode.matrix[8]), static_cast<float>(inputNode.matrix[9]),
                        static_cast<float>(inputNode.matrix[10]), static_cast<float>(inputNode.matrix[11]),
                        static_cast<float>(inputNode.matrix[12]), static_cast<float>(inputNode.matrix[13]),
                        static_cast<float>(inputNode.matrix[14]), static_cast<float>(inputNode.matrix[15]),
                    };
                }
                if (inputNode.translation.size() == 3) {
                    node->matrix = HmckTranslate(HmckVec3{
                        static_cast<float>(inputNode.translation[0]),
                        static_cast<float>(inputNode.translation[1]),
                        static_cast<float>(inputNode.translation[2])
                    });
                }
                if (inputNode.rotation.size() == 4) {
                    // TODO handle rotation
                    HmckQuat quat{
                        static_cast<float>(inputNode.rotation[0]), static_cast<float>(inputNode.rotation[1]),
                        static_cast<float>(inputNode.rotation[2]), static_cast<float>(inputNode.rotation[3]),
                    };
                }
                if (inputNode.scale.size() == 3) {
                    node->matrix = HmckScale(HmckVec3{
                        static_cast<float>(inputNode.scale[0]),
                        static_cast<float>(inputNode.scale[1]),
                        static_cast<float>(inputNode.scale[2])
                    });
                }

                // Load node's children
                if (inputNode.children.size() > 0) {
                    for (size_t i = 0; i < inputNode.children.size(); i++) {
                        loadNode(input.nodes[inputNode.children[i]], input, node, indexBuffer, vertexBuffer);
                    }
                }

                // If the node contains mesh data, we load vertices and indices from the buffers
                // In glTF this is done via accessors and buffer views
                if (inputNode.mesh > -1) {
                    const tinygltf::Mesh mesh = input.meshes[inputNode.mesh];
                    // Iterate through all primitives of this node's mesh
                    for (size_t i = 0; i < mesh.primitives.size(); i++) {
                        const tinygltf::Primitive &glTFPrimitive = mesh.primitives[i];
                        uint32_t firstIndex = static_cast<uint32_t>(indexBuffer.size());
                        uint32_t vertexStart = static_cast<uint32_t>(vertexBuffer.size());
                        uint32_t indexCount = 0;
                        // Vertices
                        {
                            const float *positionBuffer = nullptr;
                            const float *normalsBuffer = nullptr;
                            const float *texCoordsBuffer = nullptr;
                            const float *tangentsBuffer = nullptr;
                            size_t vertexCount = 0;

                            // Get buffer data for vertex normals
                            if (glTFPrimitive.attributes.find("POSITION") != glTFPrimitive.attributes.end()) {
                                const tinygltf::Accessor &accessor = input.accessors[glTFPrimitive.attributes.find(
                                    "POSITION")->second];
                                const tinygltf::BufferView &view = input.bufferViews[accessor.bufferView];
                                positionBuffer = reinterpret_cast<const float *>(&(input.buffers[view.buffer].data[
                                    accessor.byteOffset + view.byteOffset]));
                                vertexCount = accessor.count;
                            }
                            // Get buffer data for vertex normals
                            if (glTFPrimitive.attributes.find("NORMAL") != glTFPrimitive.attributes.end()) {
                                const tinygltf::Accessor &accessor = input.accessors[glTFPrimitive.attributes.find(
                                    "NORMAL")->second];
                                const tinygltf::BufferView &view = input.bufferViews[accessor.bufferView];
                                normalsBuffer = reinterpret_cast<const float *>(&(input.buffers[view.buffer].data[
                                    accessor.byteOffset + view.byteOffset]));
                            }
                            // Get buffer data for vertex texture coordinates
                            // glTF supports multiple sets, we only load the first one
                            if (glTFPrimitive.attributes.find("TEXCOORD_0") != glTFPrimitive.attributes.end()) {
                                const tinygltf::Accessor &accessor = input.accessors[glTFPrimitive.attributes.find(
                                    "TEXCOORD_0")->second];
                                const tinygltf::BufferView &view = input.bufferViews[accessor.bufferView];
                                texCoordsBuffer = reinterpret_cast<const float *>(&(input.buffers[view.buffer].data[
                                    accessor.byteOffset + view.byteOffset]));
                            }
                            // POI: This sample uses normal mapping, so we also need to load the tangents from the glTF file
                            if (glTFPrimitive.attributes.find("TANGENT") != glTFPrimitive.attributes.end()) {
                                const tinygltf::Accessor &accessor = input.accessors[glTFPrimitive.attributes.find(
                                    "TANGENT")->second];
                                const tinygltf::BufferView &view = input.bufferViews[accessor.bufferView];
                                tangentsBuffer = reinterpret_cast<const float *>(&(input.buffers[view.buffer].data[
                                    accessor.byteOffset + view.byteOffset]));
                            }

                            // Append data to model's vertex buffer
                            for (size_t v = 0; v < vertexCount; v++) {
                                const float *position = &positionBuffer[v * 3];
                                const float *normal = normalsBuffer ? &normalsBuffer[v * 3] : nullptr;
                                const float *uvs = texCoordsBuffer ? &texCoordsBuffer[v * 2] : nullptr;
                                const float *tangents = tangentsBuffer ? &tangentsBuffer[v * 4] : nullptr;

                                gVertex vert{};
                                vert.position = HmckVec3{position[0], position[1], position[2]};
                                vert.normal = HmckNorm(normalsBuffer
                                                           ? HmckVec3{normal[0], normal[1], normal[2]}
                                                           : HmckVec3{0.0f, 0.0f, 0.0f});
                                vert.uv = texCoordsBuffer ? HmckVec2{uvs[0], uvs[1]} : HmckVec2{0.0f, 0.0f};
                                vert.tangent = tangentsBuffer
                                                   ? HmckVec4{tangents[0], tangents[1], tangents[2], tangents[3]}
                                                   : HmckVec4{0.0f, 0.0f, 0.0f, 0.0f};
                                vertexBuffer.push_back(vert);
                            }
                        }
                        // Indices
                        {
                            const tinygltf::Accessor &accessor = input.accessors[glTFPrimitive.indices];
                            const tinygltf::BufferView &bufferView = input.bufferViews[accessor.bufferView];
                            const tinygltf::Buffer &buffer = input.buffers[bufferView.buffer];

                            indexCount += static_cast<uint32_t>(accessor.count);

                            // glTF supports different component types of indices
                            switch (accessor.componentType) {
                                case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT: {
                                    const uint32_t *buf = reinterpret_cast<const uint32_t *>(&buffer.data[
                                        accessor.byteOffset + bufferView.byteOffset]);
                                    for (size_t index = 0; index < accessor.count; index++) {
                                        indexBuffer.push_back(buf[index] + vertexStart);
                                    }
                                    break;
                                }
                                case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT: {
                                    const uint16_t *buf = reinterpret_cast<const uint16_t *>(&buffer.data[
                                        accessor.byteOffset + bufferView.byteOffset]);
                                    for (size_t index = 0; index < accessor.count; index++) {
                                        indexBuffer.push_back(buf[index] + vertexStart);
                                    }
                                    break;
                                }
                                case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE: {
                                    const uint8_t *buf = reinterpret_cast<const uint8_t *>(&buffer.data[
                                        accessor.byteOffset + bufferView.byteOffset]);
                                    for (size_t index = 0; index < accessor.count; index++) {
                                        indexBuffer.push_back(buf[index] + vertexStart);
                                    }
                                    break;
                                }
                                default:
                                    std::cerr << "Index component type " << accessor.componentType << " not supported!"
                                            << std::endl;
                                    return;
                            }
                        }
                        gPrimitive primitive{};
                        primitive.firstIndex = firstIndex;
                        primitive.indexCount = indexCount;
                        primitive.materialIndex = glTFPrimitive.material;
                        node->mesh.primitives.push_back(primitive);
                    }
                }
                if (parent) {
                    parent->children.push_back(node);
                } else {
                    nodes.push_back(node);
                }
            };

            const tinygltf::Scene &scene = gltfModel.scenes[0];
            for (size_t i = 0; i < scene.nodes.size(); i++) {
                const tinygltf::Node node = gltfModel.nodes[scene.nodes[i]];
                loadNode(node, gltfModel, nullptr, indexBuffer, vertexBuffer);
            }

            std::function<void(gNode *)> parseNode;
            parseNode = [&](gNode *node) {
                HmckMat4 model = node->matrix;
                gNode *currentParent = node->parent;
                while (currentParent) {
                    model = currentParent->matrix * model;
                    currentParent = currentParent->parent;
                }

                int32_t visibilityFlags = Geometry::VisibilityFlags::VISIBILITY_NONE;

                if (!node->mesh.primitives.empty()) {
                    visibilityFlags |= Geometry::VisibilityFlags::VISIBILITY_VISIBLE;
                    for (gPrimitive &primitive: node->mesh.primitives) {
                        gMaterial defaultMaterial{};
                        gMaterial &material = (primitive.materialIndex > -1)
                                                  ? materials[primitive.materialIndex]
                                                  : defaultMaterial;
                        if (material.alphaMode == "OPAQUE") {
                            visibilityFlags |= Geometry::VisibilityFlags::VISIBILITY_OPAQUE;
                        }

                        if (material.alphaMode == "BLEND") {
                            visibilityFlags |= Geometry::VisibilityFlags::VISIBILITY_BLEND;
                        }

                        visibilityFlags |= Geometry::VisibilityFlags::VISIBILITY_CASTS_SHADOW |
                                Geometry::VisibilityFlags::VISIBILITY_RECEIVES_SHADOW;

                        const auto indexOffset = static_cast<int32_t>(state.textures.size());

                        int32_t baseColorTextureIndex = -1;
                        if (material.albedoTexture > -1) {
                            baseColorTextureIndex = textures[material.albedoTexture].imageIndex + indexOffset;
                        }

                        int32_t normalTextureIndex = -1;
                        if (material.normalTexture > -1) {
                            normalTextureIndex = textures[material.normalTexture].imageIndex + indexOffset;
                        }

                        int32_t metallicRoughnessTextureIndex = -1;
                        if (material.metallicRoughnessTexture > -1) {
                            metallicRoughnessTextureIndex =
                                    textures[material.metallicRoughnessTexture].imageIndex + indexOffset;
                        }

                        int32_t occlusionTextureIndex = -1;
                        if (material.occlusionTexture > -1) {
                            occlusionTextureIndex = textures[material.occlusionTexture].imageIndex + indexOffset;
                        }

                        state.renderMeshes.push_back({
                            model,
                            visibilityFlags,
                            material.baseColorFactor,
                            HmckVec3{material.metallicFactor, material.roughnessFactor, material.alphaCutOff},
                            baseColorTextureIndex,
                            normalTextureIndex,
                            metallicRoughnessTextureIndex,
                            occlusionTextureIndex,
                            primitive.firstIndex, primitive.indexCount
                        });
                    }
                } else {
                    // this render primitive is not visible
                    state.renderMeshes.push_back({
                        model,
                        visibilityFlags,
                        HmckVec3{1.0f, 1.0f, 0.0f},
                        HmckVec3{0.0f, 0.0f, 0.0f},
                        -1,
                        -1,
                        -1,
                        -1,
                        0, 0
                    });
                }

                for (auto &child: node->children) {
                    parseNode(child);
                }
            };

            // now finally parse the loaded data
            for (auto n: nodes) {
                // compute the matrix from the parent
                parseNode(n);
            }

            // TODO make this work for multiple models in row
            // add texture resources to the list
            for (int t = 0; t < images.size(); t++) {
                state.textures.push_back(images[t].textureHandle);
            }
            // add vertices and indices into buffers
            for (int v = 0; v < vertexBuffer.size(); v++) {
                state.vertices.push_back(Vertex{
                    vertexBuffer[v].position,
                    vertexBuffer[v].normal,
                    vertexBuffer[v].uv,
                    vertexBuffer[v].tangent,
                });
            }
            std::copy(indexBuffer.begin(), indexBuffer.end(), std::back_inserter(state.indices));

            Logger::log(LOG_LEVEL_DEBUG, "glTF model loaded. Vertices: %d, Indices: %d, Triangles: %d\n",
                        vertexBuffer.size(), indexBuffer.size(), vertexBuffer.size() / 3);


            // and lastly cleanup time !
            for (auto node: nodes) {
                delete node;
            }
            return *this;
        }
    };
}
