#pragma once

#include <functional>
#include <cassert>
#include <complex>
#include <utility>
#include <variant>
#include <type_traits>
#include <optional>
#include <tiny_gltf.h>
#include <unordered_set>

#include <hammock/core/Device.h>
#include <hammock/core/CoreUtils.h>
#include <hammock/resources/Descriptors.h>

#include "hammock/core/ResourceManager.h"
#include "hammock/core/FrameManager.h"
#include "hammock/core/Types.h"


namespace hammock {
    typedef std::function<ResourceHandle(ResourceManager &, uint32_t frameIndex)>
    ResourceResolver;

    /**
     * RenderGraph node representing a resource
     */
    struct ResourceNode {
        enum class Type {
            UniformBuffer,
            VertexBuffer,
            IndexBuffer,
            StorageBuffer,
            PushConstantData,
            StorageImage,
            SampledImage,
            SwapChainImage,
            ColorAttachment,
            DepthStencilAttachment,
        } type;

        // Name is used for lookup
        std::string name;

        // Resolver is used to resolve the actual resource
        ResourceResolver resolver;

        // cache to store handles to avoid constant recreation
        std::vector<ResourceHandle> cachedHandles;

        // Needs recreation
        bool isDirty = true;

        /**
         * Returns handle corresponding to resource of specific frame
         * @param rm ResourceManager where resource is registered
         * @param frameIndex Frame index of the resource
         * @return Returns resolved handle
         */
        ResourceHandle resolve(ResourceManager &rm, uint32_t frameIndex) {
            if (isDirty || cachedHandles.empty()) {
                cachedHandles.resize(SwapChain::MAX_FRAMES_IN_FLIGHT);
                isDirty = false;
            }

            if (!cachedHandles[frameIndex].isValid()) {
                cachedHandles[frameIndex] = resolver(rm, frameIndex);
            }

            return cachedHandles[frameIndex];
        }


        bool isBuffer() const {
            return type == Type::UniformBuffer || type == Type::VertexBuffer || type == Type::IndexBuffer || type ==
                   Type::StorageBuffer;
        }

        bool isImage() const {
            return type == Type::ColorAttachment || type == Type::DepthStencilAttachment || type ==
                   Type::SwapChainImage ||  type == Type::StorageImage || type == Type::SampledImage;
        }

        bool isColorAttachment() const {
            return type == Type::ColorAttachment || type == Type::SwapChainImage;
        }

        bool isDepthAttachment() const {
            return type == Type::DepthStencilAttachment;
        }

        bool isSwapChainImage() const {
            return type == Type::SwapChainImage;
        }
    };

    /**
     * Describes a render pass context. This data is passed into rendering callback, and it is only infor available for rendering
     */
    struct RenderPassContext {
        ResourceManager &rm;
        VkCommandBuffer commandBuffer;
        uint32_t frameIndex;
        // TODO change this to resource handles
        std::unordered_map<std::string, ResourceNode *> inputs;
        std::unordered_map<std::string, ResourceNode *> outputs;
        std::vector<VkDescriptorSet> descriptorSets;

        RenderPassContext(ResourceManager &rm, VkCommandBuffer commandBuffer,
                          uint32_t frameIndex) : rm(rm), commandBuffer(commandBuffer), frameIndex(frameIndex) {
        }

        template<typename Type>
        Type *get(const std::string &name) {
            ASSERT(inputs.contains(name) || outputs.contains(name), "Cannot find buffer with name '" + name + "'");

            if (inputs.contains(name)) {
                auto uniformBufferNode = inputs[name];
                auto uniformBufferHandle = uniformBufferNode->resolve(rm, frameIndex);
                return rm.getResource<Type>(uniformBufferHandle);
            }

            if (outputs.contains(name)) {
                auto uniformBufferNode = outputs[name];
                auto uniformBufferHandle = uniformBufferNode->resolve(rm, frameIndex);
                return rm.getResource<Type>(uniformBufferHandle);
            }

            throw std::runtime_error("Cannot find buffer with name '" + name + "'");
        }

        void bindVertexBuffers(const std::vector<std::string> &names, const std::vector<VkDeviceSize> &offsets) {
            std::vector<VkBuffer> buffers;
            for (const auto &name: names) {
                buffers.push_back(get<Buffer>(name)->getBuffer());
            }

            vkCmdBindVertexBuffers(commandBuffer, 0, buffers.size(), buffers.data(), offsets.data());
        }

        void bindIndexBuffer(const std::string &name, VkIndexType indexType = VK_INDEX_TYPE_UINT32) {
            vkCmdBindIndexBuffer(commandBuffer, get<Buffer>(name)->getBuffer(), 0, indexType);
        }

        void bindDescriptorSet(uint32_t set, uint32_t binding, VkPipelineLayout layout,
                               VkPipelineBindPoint bindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS) {
            vkCmdBindDescriptorSets(
                commandBuffer,
                bindPoint,
                layout,
                binding, 1,
                &descriptorSets[set],
                0,
                nullptr);
        }
    };

    /**
     * Describes how the resource will be accessed
     */
    struct ResourceAccess {
        std::string resourceName; // Name of the accessed resource
        VkImageLayout requiredLayout = VK_IMAGE_LAYOUT_UNDEFINED; // Layout before rendering
        VkImageLayout finalLayout = VK_IMAGE_LAYOUT_UNDEFINED; // Layout after rendering, optional
        VkAttachmentLoadOp loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; // Load op
        VkAttachmentStoreOp storeOp = VK_ATTACHMENT_STORE_OP_STORE; // Store op
        VkPipelineStageFlags stageFlags = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
        std::string samplerName; // only for images
        CommandQueueFamily queueFamily = CommandQueueFamily::Ignored;
    };

    enum RenderPassFlags : int32_t {
        RENDER_PASS_FLAGS_NONE = 0,
        RENDER_PASS_FLAGS_SWAPCHAIN_WRITE = 1 << 0, // This render pass writes directly to swapchain image
        RENDER_PASS_FLAGS_CONTRIBUTING = 1 << 1,
        // this render pass contributes to the final image (directly or indirectly)
    };

    /**
     * RenderGraph node representing a render pass
     */
    struct RenderPassNode {
        typedef std::function<void(RenderPassContext)> ExecuteFunction;

        struct Descriptor {
            std::shared_ptr<DescriptorSetLayout> layout;
            std::vector<VkDescriptorSet> setCache;
        };

        struct DescriptorBinding {
            uint32_t bindingIndex;
            std::vector<std::string> bindingNames;
            VkDescriptorType descriptorType;
            VkShaderStageFlags stageFlags;
            VkDescriptorBindingFlags bindingFlags = 0;
        };

        CommandQueueFamily type; // Type of the pass
        int32_t flags = 0;
        RelativeViewPortSize viewportSize; // Size of viewport

        std::string name; // Name of the pass for debug purposes and lookup
        HmckVec4 viewport;
        std::vector<ResourceAccess> inputs; // Read accesses
        std::vector<ResourceAccess> outputs; // Write accesses
        std::unordered_map<uint32_t, std::vector<DescriptorBinding> > descriptorLayoutInfos{};


        std::unordered_map<uint32_t, Descriptor> descriptors;

        std::vector<RenderPassNode *> dependencies;

        bool autoBeginRendering = true;


        // callback for rendering
        ExecuteFunction executeFunc = nullptr;

        VkDescriptorSet resolveDescriptorSet(uint32_t setIndex, uint32_t frameIndex) {
            ASSERT(descriptors.contains(setIndex), "Invalid set index or called before build");
            Descriptor &descriptor = descriptors.at(setIndex);
            return descriptor.setCache[frameIndex];
        }

        void updateQueueFamilyOwnership(ResourceAccess &access) const {
            access.queueFamily = type;
        }


        // builder methods
        RenderPassNode &read(ResourceAccess access) {
            //updateQueueFamilyOwnership(access);
            inputs.emplace_back(access);
            return *this;
        }

        RenderPassNode &write(ResourceAccess access) {
            //updateQueueFamilyOwnership(access);
            outputs.emplace_back(access);
            return *this;
        }

        RenderPassNode &autoBeginRenderingDisabled() {
            autoBeginRendering = false;
            return *this;
        }


        // TODO ReadModifyWrite

        RenderPassNode &execute(ExecuteFunction exec) {
            executeFunc = exec;
            return *this;
        }

        RenderPassNode &descriptor(const uint32_t set, std::vector<DescriptorBinding> bindings) {
            ASSERT(!descriptorLayoutInfos.contains(set), "Descriptor already set");
            descriptorLayoutInfos[set] = std::move(bindings);
            return *this;
        }
    };


    /**
     * Wraps necessary operation needed for pipeline barrier transitions for a single resource
     */
    class PipelineBarrier {
    public:
        enum class TransitionStage {
            RequiredLayout,
            FinalLayout,
        };

        explicit PipelineBarrier(ResourceManager &rm, FrameManager &renderContext,
                                 VkCommandBuffer commandBuffer, ResourceNode &node,
                                 ResourceAccess &access,
                                 TransitionStage transitionStage) : rm(rm), renderContext(renderContext),
                                                                    commandBuffer(commandBuffer), node(node),
                                                                    access(access), transitionStage(transitionStage) {
        }

        /**
         *  Checks if resource needs barrier transition given its access
         * @return true if resource needs barrier transition, else false
         */
        bool isNeeded() {
            if (node.isImage()) {
                if (node.isSwapChainImage()) {
                    return true;
                }
                ResourceHandle handle = node.resolve(rm, renderContext.getFrameIndex());
                Image *attachment = rm.getResource<Image>(handle);

                // check for layout change
                if (transitionStage == TransitionStage::RequiredLayout) {
                    layoutChangeNeeded = attachment->getLayout() != access.requiredLayout;
                }
                if (transitionStage == TransitionStage::FinalLayout) {
                    layoutChangeNeeded = attachment->getLayout() != access.finalLayout && access.finalLayout !=
                                         VK_IMAGE_LAYOUT_UNDEFINED;
                }

                // check for ownership transfer
                ownerTransferNeeded = attachment->getQueueFamily() != access.queueFamily;

                return layoutChangeNeeded || ownerTransferNeeded;
            }
            if (node.isBuffer()) {
                // TODO support for buffer transition
                return false;
            }
            ASSERT(false, "Why am I here ??");
        }

        /**
         * Applies pre-render pipeline barrier transition
         */
        void apply() const;

    private:
        ResourceManager &rm;
        FrameManager &renderContext;
        ResourceNode &node;
        ResourceAccess &access;
        TransitionStage transitionStage;
        VkCommandBuffer commandBuffer;
        bool layoutChangeNeeded = false;
        bool ownerTransferNeeded = false;
    };

    /**
     * RenderGraph is a class representing a directed acyclic graph (DAG) that describes the process of creation of a frame.
     * It consists of resource nodes and render pass nodes. Render pass node can be dependent on some resources and
     * can itself create resources that other passes may depend on. RenderGraph analyzes these dependencies, makes adjustments when possible,
     * makes necessary transitions between resource states. These transitions are done just in time as well as resource allocation.
     *  TODO HOT rework pipeline barrier recording - currently just in time - should be forward declared along with command queue family resource acquisition and release
     *  TODO HOT support for fences for GPU-CPU sync declared by user - this way (when declaring the graph) user can select at which point the CPU would be blocked to wait for the fence
     *  TODO FUTURE support for Read Modify Write - render pass writes and reads from the same resource
     *  TODO FUTURE support for conditional resource
     *  TODO FUTURE support for swapchain dependent rendperPass
     *  TODO FUTURE support for pipeline caching
     *  TODO FUTURE support for pushConstants
     */
    class RenderGraph {
        struct SubmissionGroup {
            CommandQueueFamily queueFamily;
            std::vector<RenderPassNode *> renderPassNodes;
            std::vector<VkSemaphore> waitSemaphores; // one semaphore per frame in flight
            std::vector<VkSemaphore> signalSemaphores; // one semaphore per frame in flight
            std::vector<VkFence> fences; // one fence for each command buffer
            VkPipelineStageFlags waitStage;
            std::vector<VkCommandBuffer> commandBuffers; // one command buffer per frame in flight
            size_t globalStartIndex = 0; // The global sorted order index of the first pass in this group.
        };

        // This structure pairs a SubmissionGroup pointer with its global start index.
        struct GroupWithGlobalIndex {
            SubmissionGroup *group;
            size_t globalStartIndex;
        };

        // Vulkan device
        Device &device;
        // Rendering context
        FrameManager &fm;
        ResourceManager &rm;
        DescriptorPool &pool;
        // Holds all the resources
        std::unordered_map<std::string, ResourceNode> resources;
        // Holds all the render passes
        std::vector<RenderPassNode> passes;
        // Holds all the samplers
        std::unordered_map<std::string, ResourceHandle> samplers;

        std::vector<RenderPassNode *> topologicallySortedPasses; // contains topologically sorted passes
        std::unordered_map<CommandQueueFamily, std::vector<SubmissionGroup> > groupsByQueue;
        // submission groups by queue family
        std::vector<GroupWithGlobalIndex> allGroups; // all groups in execution order

    public:
        RenderGraph(Device &device, ResourceManager &rm,
                    FrameManager &fm, DescriptorPool &pool): device(device), rm(rm), fm(fm), pool(pool) {
            Logger::log(LOG_LEVEL_DEBUG, "Creating rendegraph\n");
        }

        ~RenderGraph() {
            Logger::log(LOG_LEVEL_DEBUG, "Releasing rendegraph\n");
            passes.clear();
            resources.clear();

            // Free command buffer and destroy sync objects
            for (auto &group: allGroups) {
                if (group.group->queueFamily == CommandQueueFamily::Graphics) {
                    vkFreeCommandBuffers(device.device(), device.getGraphicsCommandPool(),
                                         group.group->commandBuffers.size(), group.group->commandBuffers.data());
                }

                if (group.group->queueFamily == CommandQueueFamily::Compute) {
                    vkFreeCommandBuffers(device.device(), device.getComputeCommandPool(),
                                         group.group->commandBuffers.size(), group.group->commandBuffers.data());
                }

                if (group.group->queueFamily == CommandQueueFamily::Transfer) {
                    vkFreeCommandBuffers(device.device(), device.getTransferCommandPool(),
                                         group.group->commandBuffers.size(), group.group->commandBuffers.data());
                }

                for (auto &signal: group.group->signalSemaphores) {
                    vkDestroySemaphore(device.device(), signal, nullptr);
                }

                for (auto &wait: group.group->waitSemaphores) {
                    vkDestroySemaphore(device.device(), wait, nullptr);
                }

                for (auto& fence: group.group->fences) {
                    vkDestroyFence(device.device(), fence, nullptr);
                }
            }
        }


        /**
         * Creates a resource in the graph
         * @tparam Type Type of the resource Node
         * @tparam ResourceType Type of the actual resource
         * @tparam DescriptionType Type of the description of the resource based on the type of the resource
         * @param name Name of the resource for lookup and reference
         * @param desc Description of the resource
         */
        template<ResourceNode::Type Type, typename ResourceType, typename DescriptionType>
        void addResource(const std::string &name, const DescriptionType &desc) {
            ResourceNode node;
            node.type = Type;
            node.name = name;
            node.resolver = [name, desc](ResourceManager &rm, uint32_t frameIndex) {
                return rm.createResource<ResourceType>(name, desc);
            };
            resources[name] = std::move(node);
        }

        /**
         * Create a static (external, non-buffered) resource
         * @tparam Type Type of the resource node
         * @param name Name of the resource
         * @param handle Handle of the actuall resource
         */
        template<ResourceNode::Type Type>
        void addStaticResource(const std::string &name, ResourceHandle handle) {
            ResourceNode node;
            node.type = Type;
            node.name = name;
            node.resolver = [handle](ResourceManager &rm, uint32_t frameIndex) {
                return handle;
            };
            resources[name] = std::move(node);
        }


        /**
         * Create a resource that dependes on swapchain size
         * @tparam Type Type of the dependant resource node
         * @tparam ResourceType Type of the actual resource
         * @tparam DescriptionType Type of the resource description
         * @param name Name of the resource
         * @param modifier Callback to create a description of the dependant resource based on the swapchain size
         */
        template<ResourceNode::Type Type, typename ResourceType, typename DescriptionType>
        void addSwapChainDependentResource(const std::string &name,
                                           std::function<DescriptionType(VkExtent2D)> modifier) {
            ResourceNode node;
            node.type = Type;
            node.name = name;
            node.resolver = [this,name, modifier](ResourceManager &rm, uint32_t frameIndex) {
                VkExtent2D swapChainExtent = fm.getSwapChain()->getSwapChainExtent();
                ASSERT(modifier, "Modifier is null!");
                DescriptionType depDesc = modifier(swapChainExtent);
                return rm.createResource<ResourceType>(name, depDesc);
            };
            resources[name] = std::move(node);
        }

        /**
         * Creates a resource that dependes on another resource
         * @tparam Type Type of the resource node
         * @tparam ResourceType Type of the actual resource
         * @tparam DescriptionType Type of the resource description
         * @param name Name of the dependent resource
         * @param dependency Name of dependency resource
         * @param modifier Call back that creates dependent description base on the dependency description
         */
        template<ResourceNode::Type Type, typename ResourceType, typename DescriptionType>
        void addDependentResource(const std::string &name, const std::string &dependency,
                                  std::function<DescriptionType(ResourceHandle)> modifier) {
            ResourceNode node;
            node.type = Type;
            node.name = name;
            node.resolver = [this, name, dependency,modifier](ResourceManager &rm, uint32_t frameIndex) {
                auto depHandle = resources.at(dependency).resolve(rm, frameIndex);
                ASSERT(modifier, "Modifier is null!");
                auto newDesc = modifier(depHandle);
                return rm.createResource<ResourceType>(name, newDesc);
            };
            resources[name] = std::move(node);
        }

        /**
         * Creates a resource that represents swapchain image. This way you can reference to swapchain image using the resource name.
         * @tparam Type Type of the SwapChain attachment (SwapChainColorAttachment or SwapChainDepthStencilAttachment)
         * @param name Name of the resource
         */
        void addSwapChainImageResource(const std::string &name) {
            ResourceNode node;
            node.name = name;
            node.type = ResourceNode::Type::SwapChainImage;
            node.resolver = nullptr;
            resources[name] = std::move(node);
        }

        /**
         * Creates a new resource node that represents another resources state in a previous frame
         * @param name Name of the resource
         * @param refName Name of the resource that this resource references
         */
        void addResourceFromPreviousFrame(const std::string &name, const std::string &refName) {
            ResourceNode node;
            node.type = resources.at(refName).type;
            node.name = name;
            node.resolver = [this, refName](ResourceManager &rm, uint32_t frameIndex) {
                uint32_t previousFrameIndex = (frameIndex + SwapChain::MAX_FRAMES_IN_FLIGHT - 1) % SwapChain::MAX_FRAMES_IN_FLIGHT;
                return resources.at(refName).resolve(rm, previousFrameIndex);
            };
            resources[name] = std::move(node);
        }


        /**
         * Creates a sampler that can be used to sample attachments
         * @param name Name of the sampler
         * @param desc Optional description to customize sampler
         */
        void createSampler(const std::string &name, SamplerDesc desc = {}) {
            ASSERT(!samplers.contains(name), "Sampler already exists!");
            samplers.emplace(name, rm.createResource<Sampler>(name, desc));
        }

        void addSampler(const std::string &name, ResourceHandle handle) {
            samplers.emplace(name, handle);
        }

        /**
         *
         * @tparam QueueFamily Type of the Pass (Graphics, Transfer, Compute)
         * @tparam ViewPortSize ViewPortSize class (SwapChainRelative, Fixed)
         * @tparam ViewPortWidth ViewPort width multiplier (if Fixed, then absolute width)
         * @tparam ViewPortHeight ViewPort height multiplier (if Fixed, then absolute height)
         * @tparam ViewPortDepthMin Min depth value
         * @tparam ViewPortDepthMax Max depth value
         * @param name Name of the render pass
         * @return Returns reference to the RenderPassNode node that can be used to set additional parameters. See RenderPassNode definition.
         */
        template<CommandQueueFamily QueueFamily, RelativeViewPortSize ViewPortSize =
                    RelativeViewPortSize::SwapChainRelative, float
            ViewPortWidth = 1.0f, float ViewPortHeight = 1.0f
            , float ViewPortDepthMin = 0.f, float ViewPortDepthMax = 1.f>
        RenderPassNode &addPass(const std::string &name) {
            RenderPassNode node;
            node.name = name;
            node.type = QueueFamily;
            node.viewportSize = ViewPortSize;
            node.viewport = HmckVec4{ViewPortWidth, ViewPortHeight, ViewPortDepthMin, ViewPortDepthMax};
            passes.push_back(node);
            return passes.back();
        }

        /**
         * Returns list of descriptor set layouts used in the render pass
         * @param passName Name of the render pass
         * @return Vector of descriptor set layouts used in the render pass
         */
        std::vector<VkDescriptorSetLayout> getDescriptorSetLayouts(const std::string &passName) {
            std::vector<VkDescriptorSetLayout> layouts = {};
            for (auto &pass: passes) {
                if (pass.name == passName) {
                    for (const auto &descriptor: pass.descriptors) {
                        layouts.push_back(descriptor.second.layout->getDescriptorSetLayout());
                    }
                    return layouts;
                }
            }
            return layouts;
        }

        void prepareResources() {
            for (auto &pass: passes) {
                for (auto &input: pass.inputs) {
                    ResourceNode &resource = resources[input.resourceName];
                    for (int frameIndex = 0; frameIndex < SwapChain::MAX_FRAMES_IN_FLIGHT; frameIndex++) {
                        if (resource.isImage()) {
                            Image *image = rm.getResource<Image>(
                                resource.resolve(rm, frameIndex));
                            if (resource.type == ResourceNode::Type::StorageImage) {
                                image->queueImageLayoutTransition(VK_IMAGE_LAYOUT_GENERAL);
                            } else {
                                image->queueImageLayoutTransition(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
                            }
                        }
                        if (resource.type == ResourceNode::Type::UniformBuffer) {
                            Buffer *buffer = rm.getResource<Buffer>(
                                resource.resolve(rm, frameIndex));
                            buffer->map();
                        }
                    }
                }
            }
        }

        void purgeNonContributingPasses() {
            size_t originalSize = passes.size();
            // Identify swapchain writers and initialize queue
            std::queue<size_t> worklist;
            for (int i = 0; i < passes.size(); i++) {
                for (auto &write: passes[i].outputs) {
                    ResourceNode &resource = resources[write.resourceName];
                    if (resource.isSwapChainImage()) {
                        passes[i].flags |= RENDER_PASS_FLAGS_SWAPCHAIN_WRITE;
                        passes[i].flags |= RENDER_PASS_FLAGS_CONTRIBUTING;
                        worklist.push(i);
                    }
                }
            }

            // Traverse upwards marking contributing passes
            while (!worklist.empty()) {
                size_t passIndex = worklist.front();
                worklist.pop();

                const auto &pass = passes[passIndex];

                // Iterate over the inputs of this pass
                for (const auto &input: pass.inputs) {
                    auto it = resources.find(input.resourceName);
                    if (it != resources.end()) {
                        // Find the passes that write to this resource
                        for (int i = 0; i < passes.size(); i++) {
                            for (auto &write: passes[i].outputs) {
                                if (write.resourceName == input.resourceName) {
                                    ResourceNode &resource = resources[write.resourceName];
                                    if (!(passes[i].flags & RENDER_PASS_FLAGS_CONTRIBUTING)) {
                                        passes[i].flags |= RENDER_PASS_FLAGS_CONTRIBUTING;
                                        worklist.push(i);
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // Remove non-contributing passes
            std::erase_if(passes, [](const RenderPassNode &pass) {
                return !(pass.flags & RENDER_PASS_FLAGS_CONTRIBUTING);
            });

            Logger::log(LOG_LEVEL_DEBUG, "Purged %d non-contributing passes\n", originalSize - passes.size());
        }

        void detectDependencies() {
            // for each pass
            for (int passIdx = 0; passIdx < passes.size(); passIdx++) {
                // for each input of the current pass
                for (auto &input: passes[passIdx].inputs) {
                    // find which passes write to the input
                    for (int i = 0; i < passes.size(); i++) {
                        for (auto &write: passes[i].outputs) {
                            if (write.resourceName == input.resourceName) {
                                passes[passIdx].dependencies.push_back(&passes[i]);
                            }
                        }
                    }
                }
            }
        }

        void topologicallySortPasses() {
            // Clear the sorted list.
            topologicallySortedPasses.clear();

            // Map each pass to its in-degree (number of incoming dependencies).
            std::unordered_map<std::string, int> inDegree;
            for (auto &node: passes) {
                inDegree[node.name] = 0;
            }

            // For each node, for each dependency it has, increment its in-degree.
            // (If node A depends on B then B must run before A, so A’s in-degree increases.)
            for (auto &node: passes) {
                for (auto dep: node.dependencies) {
                    inDegree[node.name]++;
                }
            }

            // Use a set to mark nodes that have been processed.
            std::unordered_set<std::string> processed;

            // We repeatedly scan the passes in their original order,
            // picking those with in-degree zero (i.e. all dependencies satisfied).
            bool foundNodeInPass = true;
            while (processed.size() < passes.size() && foundNodeInPass) {
                foundNodeInPass = false;
                // Iterate over passes in original order.
                for (auto &node: passes) {
                    // If this node has not been processed and has in-degree zero, process it.
                    if (processed.find(node.name) == processed.end() && inDegree[node.name] == 0) {
                        topologicallySortedPasses.push_back(&node);
                        processed.insert(node.name);
                        foundNodeInPass = true;
                        // "Remove" this node: for every other node, if it depends on the current node, decrement its in-degree.
                        for (auto &other: passes) {
                            // Check if 'other' depends on 'node'
                            for (auto *dep: other.dependencies) {
                                if (dep->name == node.name) {
                                    inDegree[other.name]--;
                                }
                            }
                        }
                    }
                }
            }

            // If we haven't processed all nodes, there is a cycle.
            ASSERT(processed.size() == passes.size(), "This graf is NOT ACYCLIC! This should not happen!");

            // Log the resulting order.
            Logger::log(LOG_LEVEL_DEBUG, "Topological order: ");
            for (size_t i = 0; i < topologicallySortedPasses.size(); i++) {
                Logger::log(LOG_LEVEL_DEBUG, " \"%s\" -> ", topologicallySortedPasses[i]->name.c_str());
            }
            Logger::log(LOG_LEVEL_DEBUG, " PRESENT\n");
        }

        bool needsNewGroup(const SubmissionGroup &currentGroup,
                           RenderPassNode *newPass,
                           size_t globalIndex) {
            // For the very first pass (globalIndex==0), no group exists yet so no need to split.
            if (globalIndex == 0)
                return false;

            // Rule 1: Check if the immediately preceding pass in global order is of a different queue.
            // If so, newPass is not contiguous with the previous work on the same queue.
            if (topologicallySortedPasses[globalIndex - 1]->type != newPass->type)
                return true;

            // Rule 2: Check if newPass depends on any pass that is NOT in the current submission group.
            // (This would signal that newPass is waiting on work that was separated by an inter-queue dependency.)
            for (RenderPassNode *dep: newPass->dependencies) {
                // If a dependency is not found among the passes already recorded in the current group,
                // then we require a new submission group.
                if (std::find(currentGroup.renderPassNodes.begin(),
                              currentGroup.renderPassNodes.end(),
                              dep) == currentGroup.renderPassNodes.end()) {
                    return true;
                }
            }

            // Otherwise, the candidate pass can be merged into the current group.
            return false;
        }

        void groupPassesBySubmissionGroup() {
            // Iterate through the global sorted list.
            for (size_t i = 0; i < topologicallySortedPasses.size(); ++i) {
                RenderPassNode *pass = topologicallySortedPasses[i];
                CommandQueueFamily qf = pass->type;

                // Create an initial group for this queue family if needed.
                if (groupsByQueue[qf].empty()) {
                    SubmissionGroup newGroup;
                    newGroup.queueFamily = qf;
                    newGroup.globalStartIndex = i; // first pass in this group.
                    groupsByQueue[qf].push_back(newGroup);
                } else {
                    // Get the last submission group for this queue family.
                    SubmissionGroup &currentGroup = groupsByQueue[qf].back();
                    if (needsNewGroup(currentGroup, pass, i)) {
                        SubmissionGroup newGroup;
                        newGroup.queueFamily = qf;
                        newGroup.globalStartIndex = i;
                        groupsByQueue[qf].push_back(newGroup);
                    }
                }

                // Add the current pass to the latest submission group for its queue.
                groupsByQueue[qf].back().renderPassNodes.push_back(pass);
            }
        }

        void sortPassesByExecutionOrder() {
            for (auto &kv: groupsByQueue) {
                for (auto &group: kv.second) {
                    allGroups.push_back({&group, group.globalStartIndex});
                }
            }

            // Sort the groups by the global start index so that they follow the topological order.
            std::sort(allGroups.begin(), allGroups.end(),
                      [](const GroupWithGlobalIndex &a, const GroupWithGlobalIndex &b) {
                          return a.globalStartIndex < b.globalStartIndex;
                      });


            Logger::log(LOG_LEVEL_DEBUG, "Execution order: ");
            for (int i = 0; i < allGroups.size(); i++) {
                Logger::log(LOG_LEVEL_DEBUG, " EXECUTION_GROUP{ ");
                for (int j = 0; j < allGroups[i].group->renderPassNodes.size(); j++) {
                    Logger::log(LOG_LEVEL_DEBUG, " \"%s\" -> ", allGroups[i].group->renderPassNodes[j]->name.c_str());
                }
                Logger::log(LOG_LEVEL_DEBUG, " } -> ");
            }
            Logger::log(LOG_LEVEL_DEBUG, " PRESENT\n");

            VkSemaphoreCreateInfo semaphoreInfo = {};
            semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

            VkFenceCreateInfo fenceInfo = {};
            fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
            fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

            // Iterate over the groups in order of execution
            for (const auto &groupWithIndex: allGroups) {
                SubmissionGroup *group = groupWithIndex.group;


                group->commandBuffers.resize(SwapChain::MAX_FRAMES_IN_FLIGHT);
                group->signalSemaphores.resize(SwapChain::MAX_FRAMES_IN_FLIGHT);
                group->waitSemaphores.resize(SwapChain::MAX_FRAMES_IN_FLIGHT);
                group->fences.resize(SwapChain::MAX_FRAMES_IN_FLIGHT);


                for (int frameIdx = 0; frameIdx < SwapChain::MAX_FRAMES_IN_FLIGHT; frameIdx++) {
                    // First, create a command buffer for the group
                    // conservative approach for wait stage
                    group->waitStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

                    // Allocate the command buffers
                    if (group->queueFamily == CommandQueueFamily::Graphics) {
                        group->commandBuffers[frameIdx] = fm.createCommandBuffer<CommandQueueFamily::Graphics>();
                    }

                    if (group->queueFamily == CommandQueueFamily::Compute) {
                        group->commandBuffers[frameIdx] = fm.createCommandBuffer<CommandQueueFamily::Compute>();
                    }

                    if (group->queueFamily == CommandQueueFamily::Transfer) {
                        group->commandBuffers[frameIdx] = fm.createCommandBuffer<CommandQueueFamily::Transfer>();
                    }

                    // Create fence for each command buffer
                    ASSERT(vkCreateFence(device.device(), &fenceInfo, nullptr, &group->fences[frameIdx]) == VK_SUCCESS,
                           "Could not create fences");

                    // If this is a first and only group, we don!t create any semaphores
                    if (group->globalStartIndex == 0 && allGroups.size() == 1) {
                        continue;
                    }

                    // If this is the first group, we only create a signal semaphore
                    if (group->globalStartIndex == 0) {
                        ASSERT(
                            vkCreateSemaphore(device.device(), &semaphoreInfo, nullptr, &group->signalSemaphores[
                                frameIdx]) == VK_SUCCESS, "Could not create semaphore");
                        continue;
                    }

                    // If this is the last group, we only create wait semaphore
                    if (group->globalStartIndex == allGroups.size() - 1) {
                        ASSERT(
                            vkCreateSemaphore(device.device(), &semaphoreInfo, nullptr, &group->waitSemaphores[frameIdx]
                            ) == VK_SUCCESS, "Could not create semaphore");
                        continue;
                    }

                    // Else we create both wait and signal semaphores
                    ASSERT(
                        vkCreateSemaphore(device.device(), &semaphoreInfo, nullptr, &group->waitSemaphores[frameIdx]) ==
                        VK_SUCCESS, "Could not create semaphore");
                    ASSERT(
                        vkCreateSemaphore(device.device(), &semaphoreInfo, nullptr, &group->signalSemaphores[frameIdx])
                        == VK_SUCCESS, "Could not create semaphore");
                }
            }
        }

        /**
         * Compiles the render graph - analyzes dependencies and makes optimization
         */
        void build() {
            // First we purge passes that do not contribute to the final image
            purgeNonContributingPasses();

            // Detect and fill dependencies between passes
            detectDependencies();

            // Topologically sort the passes
            topologicallySortPasses();

            // Group passes into submission groups
            groupPassesBySubmissionGroup();

            // Sort passes in optimal execution order
            sortPassesByExecutionOrder();

            // Transition all input images to required layouts and map all uniform buffers
            prepareResources();

            // Finally we need to build the descriptor layouts for each pass
            buildDescriptors();
        }


        /**
         * Creates all descriptor layouts and sets. One set per frame in flight is created. One layout per set is created
         * TODO cache the descriptor layouts
         * TODO descriptor arrays
         */
        void buildDescriptors() {
            // Create descriptors
            for (int passIdx = 0; passIdx < passes.size(); passIdx++) {
                RenderPassNode &passNode = passes[passIdx];
                for (auto &descInfo: passNode.descriptorLayoutInfos) {
                    // Create a descriptor set layout
                    uint32_t setIndex = descInfo.first;
                    auto descriptorSetLayoutBuilder = DescriptorSetLayout::Builder(device);
                    for (auto &binding: descInfo.second) {
                        // No arrays yet
                        ASSERT(binding.bindingNames.size() == 1,
                               "Only one binding name per binding supported as of now!");
                        ASSERT(resources.contains(binding.bindingNames.at(0)),
                               "Binding name does not reference existing resource!");

                        ResourceNode &resourceNode = resources[binding.bindingNames.at(0)];

                        descriptorSetLayoutBuilder.addBinding(binding.bindingIndex, binding.descriptorType,
                                                              binding.stageFlags,
                                                              binding.bindingNames.size(), binding.bindingFlags);
                    }

                    auto descriptorSetLayout = descriptorSetLayoutBuilder.build();
                    passNode.descriptors.emplace(setIndex, RenderPassNode::Descriptor{
                                                     .layout = std::move(descriptorSetLayout), .setCache = {}
                                                 });

                    // Create descriptor set for each frame in flight
                    passNode.descriptors.at(setIndex).setCache.resize(SwapChain::MAX_FRAMES_IN_FLIGHT);
                    for (int frameInFlight = 0; frameInFlight < SwapChain::MAX_FRAMES_IN_FLIGHT; frameInFlight++) {
                        auto &desc = passNode.descriptorLayoutInfos.at(setIndex);
                        auto writer = DescriptorWriter(*passNode.descriptors[setIndex].layout, pool);
                        VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
                        std::unordered_map<uint32_t, VkDescriptorBufferInfo> bufferInfos;
                        std::unordered_map<uint32_t, VkDescriptorImageInfo> imageInfos;

                        for (auto &binding: desc) {
                            ResourceNode &resourceNode = resources[binding.bindingNames.at(0)];

                            if (resourceNode.isBuffer()) {
                                auto *buffer = rm.getResource<Buffer>(
                                    resourceNode.resolve(rm, frameInFlight));
                                bufferInfos.emplace(binding.bindingIndex, buffer->descriptorInfo());
                            }

                            if (resourceNode.isImage()) {
                                const auto *image = rm.getResource<Image>(
                                    resourceNode.resolve(rm, (frameInFlight + SwapChain::MAX_FRAMES_IN_FLIGHT) % SwapChain::MAX_FRAMES_IN_FLIGHT));

                                ASSERT(samplers.size() > 0,
                                       "There are no samplers that can be used to sample the attachment. Did you forget to call createSampler() or addSampler()?")
                                ;

                                // Find the sampler that should be used
                                auto result = std::find_if(passNode.inputs.begin(), passNode.inputs.end(),
                                                           [&](ResourceAccess access) {
                                                               return access.resourceName == resourceNode.name;
                                                           });
                                if (result == passNode.inputs.end()) {
                                    result = std::find_if(passNode.outputs.begin(), passNode.outputs.end(),
                                                          [&](ResourceAccess access) {
                                                              return access.resourceName == resourceNode.name;
                                                          });
                                    ASSERT(result != passNode.outputs.end(), "WTF?");
                                }

                                VkSampler sampler = VK_NULL_HANDLE;
                                if (result->samplerName.empty()) {
                                    // Use default sampler (the first one)
                                    sampler = rm.getResource<Sampler>(samplers.begin()->second)->
                                            getSampler();
                                } else {
                                    sampler = rm.getResource<Sampler>(samplers.at(result->samplerName))->
                                            getSampler();
                                }
                                imageInfos.emplace(binding.bindingIndex, image->getDescriptorImageInfo(sampler));
                            }
                        }

                        for (auto &bufferInfo: bufferInfos) {
                            writer.writeBuffer(bufferInfo.first, &bufferInfo.second);
                        }
                        for (auto &imageInfo: imageInfos) {
                            writer.writeImage(imageInfo.first, &imageInfo.second);
                        }

                        ASSERT(writer.build(descriptorSet), "Failed to build descriptor set!");
                        passNode.descriptors.at(setIndex).setCache[frameInFlight] = descriptorSet;
                    }
                }
            }
        }

        /**
         * Applies pipeline barrier transition where it is needed based on the stage
         * @param pass Render pass
         * @param stage Stage of the barrier
         */
        void applyPipelineBarriers(RenderPassNode *pass, VkCommandBuffer commandBuffer,
                                   PipelineBarrier::TransitionStage stage) {
            for (auto &access: pass->outputs) {
                ASSERT(resources.find(access.resourceName) != resources.end(),
                       "Could not find the output");

                ResourceNode &resourceNode = resources[access.resourceName];
                PipelineBarrier barrier(rm, fm, commandBuffer, resourceNode, access, stage);
                if (barrier.isNeeded()) {
                    barrier.apply();
                }
            }
            for (auto &access: pass->inputs) {
                ASSERT(resources.find(access.resourceName) != resources.end(),
                       "Could not find the output");

                ResourceNode &resourceNode = resources[access.resourceName];
                PipelineBarrier barrier(rm, fm, commandBuffer, resourceNode, access, stage);
                if (barrier.isNeeded()) {
                    barrier.apply();
                }
            }
        }

        /**
         * Returns list of color attachments of a render pass
         * @param pass Render pass
         * @return Returns list of render passes color attachments
         */
        std::vector<VkRenderingAttachmentInfo> collectColorAttachmentInfos(RenderPassNode *pass) {
            std::vector<VkRenderingAttachmentInfo> colorAttachments;
            for (auto &access: pass->outputs) {
                ResourceNode &node = resources[access.resourceName];
                if (node.isSwapChainImage() && node.isColorAttachment()) {
                    VkRenderingAttachmentInfo attachmentInfo{};
                    attachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
                    attachmentInfo.imageView = fm.getSwapChain()->
                            getImageView(fm.getSwapChainImageIndex());
                    attachmentInfo.imageLayout = access.requiredLayout;
                    attachmentInfo.clearValue = {};
                    attachmentInfo.loadOp = access.loadOp;
                    attachmentInfo.storeOp = access.storeOp;
                    colorAttachments.push_back(attachmentInfo);
                } else if (node.isColorAttachment()) {
                    ASSERT(node.resolver, "Resolver is nullptr!");
                    ResourceHandle handle = node.resolve(rm, fm.getFrameIndex());
                    Image *image = rm.getResource<Image>(handle);
                    VkRenderingAttachmentInfo attachmentInfo = image->getRenderingAttachmentInfo();
                    attachmentInfo.loadOp = access.loadOp;
                    attachmentInfo.storeOp = access.storeOp;
                    colorAttachments.push_back(attachmentInfo);
                }
            }

            return colorAttachments;
        }

        /**
         * Returns depth attachment of a render pass
         * @param pass Render pass
         * @return Returns VkRenderingAttachmentInfo of depth attachment from a pass
         */
        std::optional<VkRenderingAttachmentInfo> collectDepthStencilAttachmentInfo(RenderPassNode *pass) {
            for (auto &access: pass->outputs) {
                ResourceNode &node = resources[access.resourceName];
                if (node.isDepthAttachment()) {
                    ASSERT(node.resolver, "Resolver is nullptr!");
                    ResourceHandle handle = node.resolve(rm, fm.getFrameIndex());
                    Image *image = rm.getResource<Image>(handle);
                    VkRenderingAttachmentInfo attachmentInfo = image->getRenderingAttachmentInfo();
                    attachmentInfo.loadOp = access.loadOp;
                    attachmentInfo.storeOp = access.storeOp;
                    return attachmentInfo;
                }
            }

            return std::nullopt;
        }


        RenderPassContext createRenderPassContext(RenderPassNode *pass, VkCommandBuffer commandBuffer) {
            RenderPassContext context{rm, commandBuffer, static_cast<uint32_t>(fm.getFrameIndex())};

            // Fill the context with input resources
            for (auto &access: pass->inputs) {
                ASSERT(resources.find(access.resourceName) != resources.end(),
                       "Could not find the input");
                ResourceNode &resourceNode = resources[access.resourceName];
                // TODO automatically find out next render pass using this resource and set final layout of the attachment to the requiredLayout to save one barrier
                context.inputs.emplace(resourceNode.name, &resources[access.resourceName]);
            }

            // fill context with input resources
            for (auto &access: pass->outputs) {
                ASSERT(resources.find(access.resourceName) != resources.end(),
                       "Could not find the output");
                ResourceNode &resourceNode = resources[access.resourceName];
                context.outputs.emplace(resourceNode.name, &resources[access.resourceName]);
            }

            // fill context with descriptor sets
            for (const auto &desc: pass->descriptors) {
                context.descriptorSets.push_back(pass->resolveDescriptorSet(desc.first, fm.getFrameIndex()));
            }

            return context;
        }

        /**
         * Begins the rendering using the dynamic rendering extension
         * @param pass Current Render pass
         */
        void beginRendering(RenderPassNode *pass, VkCommandBuffer commandBuffer) {
            // Collect attachments
            std::vector<VkRenderingAttachmentInfo> colorAttachments = collectColorAttachmentInfos(pass);
            auto depthStencilOptional = collectDepthStencilAttachmentInfo(pass);

            VkRenderingInfo renderingInfo{VK_STRUCTURE_TYPE_RENDERING_INFO_KHR};
            if(pass->viewportSize == RelativeViewPortSize::SwapChainRelative) {
                renderingInfo.renderArea = {
                    0, 0, static_cast<uint32_t>(fm.getSwapChain()->getSwapChainExtent().width * pass->viewport.X),
                    static_cast<uint32_t>(fm.getSwapChain()->getSwapChainExtent().height * pass->viewport.Y)
                };
            }
            else {
                renderingInfo.renderArea = {
                    0, 0, fm.getSwapChain()->getSwapChainExtent().width,
                    fm.getSwapChain()->getSwapChainExtent().height
                };
            }
            renderingInfo.layerCount = 1;
            renderingInfo.colorAttachmentCount = colorAttachments.size();
            renderingInfo.pColorAttachments = colorAttachments.data();
            renderingInfo.pDepthAttachment = depthStencilOptional.has_value()
                                                 ? &depthStencilOptional.value()
                                                 : VK_NULL_HANDLE;
            renderingInfo.pStencilAttachment = VK_NULL_HANDLE;

            // Start a dynamic rendering
            vkCmdBeginRendering(commandBuffer, &renderingInfo);

            // Set viewport and scissors
            VkViewport viewport = Init::viewport(pass->viewport.X, pass->viewport.Y, pass->viewport.Z,
                                                 pass->viewport.W);
            if (pass->viewportSize == RelativeViewPortSize::SwapChainRelative) {
                viewport = Init::viewport(pass->viewport.X * fm.getSwapChain()->getSwapChainExtent().width,
                                          pass->viewport.Y * fm.getSwapChain()->getSwapChainExtent().height,
                                          pass->viewport.Z, pass->viewport.W);
            }
            VkRect2D scissor{
                {0, 0}, VkExtent2D{static_cast<uint32_t>(viewport.width), static_cast<uint32_t>(viewport.height)}
            };
            vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
            vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
        }

        /**
         * Ends the rendering for current renderpass
         */
        void endRendering(VkCommandBuffer commandBuffer) {
            vkCmdEndRendering(commandBuffer);
        }

        void recordRenderPass(RenderPassNode *pass, VkCommandBuffer commandBuffer) {
            if (pass->type == CommandQueueFamily::Graphics && pass->autoBeginRendering) {
                beginRendering(pass, commandBuffer);
            }
            // Create context
            RenderPassContext renderPassContext = createRenderPassContext(pass, commandBuffer);

            // Dispatch the render pass callback
            ASSERT(pass->executeFunc, "Execute function is not set!");
            pass->executeFunc(renderPassContext);

            if (pass->type == CommandQueueFamily::Graphics && pass->autoBeginRendering) {
                endRendering(commandBuffer);
            }
        }

        void releaseGroupResources(SubmissionGroup *group, VkCommandBuffer commandBuffer,
                                   CommandQueueFamily releaseToFamily) {
            for (auto &pass: group->renderPassNodes) {
                for (auto &resource: pass->inputs) {
                    resource.queueFamily = releaseToFamily;
                }
                for (auto &resource: pass->outputs) {
                    resource.queueFamily = releaseToFamily;
                }

                applyPipelineBarriers(pass, commandBuffer, PipelineBarrier::TransitionStage::FinalLayout);
            }
        }


        /**
         * Executes the graph and makes necessary barrier transitions.
         */
        void execute() {
            // Begin frame
            if (fm.beginFrame()) {
                uint32_t frameIdx = fm.getFrameIndex();
                // Track command queue families
                CommandQueueFamily previousFamily = CommandQueueFamily::Ignored;

                // Iterate over the groups in order of execution
                for (const auto &groupWithIndex: allGroups) {
                    SubmissionGroup *group = groupWithIndex.group;
                    VkCommandBuffer commandBuffer = group->commandBuffers[fm.getFrameIndex()];

                    // Begin command buffer for the group
                    fm.beginCommandBuffer(commandBuffer);

                    // Exectue all passes from the group
                    for (int i = 0; i < group->renderPassNodes.size(); i++) {
                        // Pre pass barriers
                        applyPipelineBarriers(group->renderPassNodes[i], commandBuffer,
                                              PipelineBarrier::TransitionStage::RequiredLayout);

                        // Record the render pass to the command buffer
                        recordRenderPass(group->renderPassNodes[i], commandBuffer);

                        // post pass barrier
                        applyPipelineBarriers(group->renderPassNodes[i], commandBuffer,
                                              PipelineBarrier::TransitionStage::FinalLayout);
                    }

                    // release resources owned by the group queue
                    //releaseGroupResources(group, commandBuffer, previousFamily);
                    previousFamily = group->queueFamily;

                    // Submit command buffer
                    // 3. Handle submission synchronization
                    if (group->globalStartIndex == allGroups.size() - 1) {
                        // Last group - let SwapChain handle presentation sync
                        if (group->globalStartIndex != 0) {
                             VkSemaphore waitSemaphore = allGroups[group->globalStartIndex - 1].group->signalSemaphores[frameIdx];
                            fm.submitPresentCommandBuffer(commandBuffer, {waitSemaphore}, {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT});
                        }
                        else {
                            fm.submitPresentCommandBuffer(commandBuffer, {}, {});
                        }

                    } else {
                        // Other groups - handle inter-group sync
                        std::vector<VkSemaphore> waitSemaphores{};
                        std::vector<VkSemaphore> signalSemaphores{};


                        // Wait on the previous group's signal semaphore
                        if (group->globalStartIndex > 0) {
                            auto prevGroup = allGroups[group->globalStartIndex - 1].group;
                            if (prevGroup->signalSemaphores[frameIdx] != VK_NULL_HANDLE) {
                                waitSemaphores.push_back(prevGroup->signalSemaphores[frameIdx]);
                            }
                        }

                        // Signal to the next group
                        if (group->globalStartIndex < allGroups.size() - 1) {
                            if (group->signalSemaphores[frameIdx] != VK_NULL_HANDLE) {
                                signalSemaphores.push_back(group->signalSemaphores[frameIdx]);
                            }
                        }

                        // Submit to appropriate queue
                        switch (group->queueFamily) {
                            case CommandQueueFamily::Graphics:
                                fm.submitCommandBuffer<CommandQueueFamily::Graphics>(
                                    commandBuffer, waitSemaphores, signalSemaphores,
                                    {group->waitStage});
                                break;
                            case CommandQueueFamily::Compute:
                                fm.submitCommandBuffer<CommandQueueFamily::Compute>(
                                    commandBuffer, waitSemaphores, signalSemaphores,
                                    {group->waitStage});
                                break;
                            case CommandQueueFamily::Transfer:
                                fm.submitCommandBuffer<CommandQueueFamily::Transfer>(
                                    commandBuffer, waitSemaphores, signalSemaphores,
                                    {group->waitStage});
                                break;
                        }
                    }
                }

                fm.endFrame();
            }
        }
    };
};
