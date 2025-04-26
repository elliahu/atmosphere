#include <hammock/hammock.h>

using namespace hammock;

std::string assetPath(const std::string &asset) {
    return "../../../data/" + asset;
}

std::string compiledShaderPath(const std::string &shader) {
    return "../../../src/hammock/shaders/compiled/" + shader + ".spv";
}

int main() {
    VulkanInstance instance{};
    hammock::Window window{instance, "Render Graph", 1280, 720};
    Device device{instance, window.getSurface()};
    ResourceManager rm{device};
    // TODO decouple context and window
    FrameManager fm{window, device};
    std::unique_ptr<DescriptorPool> descriptorPool = DescriptorPool::Builder(device)
            .setMaxSets(20000)
            .setPoolFlags(VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT)
            .addPoolSize(VK_DESCRIPTOR_TYPE_SAMPLER, 10000)
            .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 10000)
            .addPoolSize(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 10000)
            .addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 10000)
            .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 10000)
            .addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 10000)
            .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 10000)
            .addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 10000)
            .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 10000)
            .addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 10000)
            .addPoolSize(VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 10000)
            .build();
    UserInterface ui{device, fm.getSwapChain()->getRenderPass() , descriptorPool->descriptorPool, window};



    struct UniformBuffer {
        HmckVec3 lightPosition = {1.0f, 1.0f, 1.0f};
        float aspectRatio;
        HmckVec4 fogColor = {0.f, 0.f, 0.f, 0.f};

        struct {
            HmckVec3 pos = {0.0f, 0.0f, 4.0f};
            HmckVec3 lookat = {0.0f, 0.5f, 0.0f};
            float fov = 10.0f;
        } camera;

        HmckMat4 padding;
    } uniformData;
    uniformData.aspectRatio = fm.getAspectRatio();

    enum class SceneObjectType { Sphere = 0, Plane = 1 };

    union SceneObjectProperty {
        HmckVec4 positionAndRadius;
        HmckVec4 normalAndDistance;
    };

    struct SceneObject {
        SceneObjectProperty objectProperties;
        HmckVec3 diffuse;
        float specular{1.0f};
        uint32_t id{0};
        uint32_t objectType{0};
        // Due to alignment rules we need to pad to make the element align at 16-bytes
        HmckVec2 _pad;
    };

    uint32_t currentId = 0;

    std::vector<SceneObject> sceneObjects{};

    // Add some spheres to the scene
    //std::vector<Sphere> spheres;
    // Lambda to simplify object creation
    auto addSphere = [&sceneObjects, &currentId](HmckVec3 pos, float radius, HmckVec3 diffuse, float specular) {
        SceneObject sphere{};
        sphere.id = currentId++;
        sphere.objectProperties.positionAndRadius = HmckVec4{pos, radius};
        sphere.diffuse = diffuse;
        sphere.specular = specular;
        sphere.objectType = (uint32_t) SceneObjectType::Sphere;
        sceneObjects.push_back(sphere);
    };

    auto addPlane = [&sceneObjects, &currentId](HmckVec3 normal, float distance, HmckVec3 diffuse, float specular) {
        SceneObject plane{};
        plane.id = currentId++;
        plane.objectProperties.normalAndDistance = HmckVec4{normal, distance};
        plane.diffuse = diffuse;
        plane.specular = specular;
        plane.objectType = (uint32_t) SceneObjectType::Plane;
        sceneObjects.push_back(plane);
    };

    addSphere({1.75f, -0.5f, 0.0f}, 1.0f, {0.0f, 1.0f, 0.0f}, 32.0f);
    addSphere({0.0f, 1.0f, -0.5f}, 1.0f, {0.65f, 0.77f, 0.97f}, 32.0f);
    addSphere({-1.75f, -0.75f, -0.5f}, 1.25f, {0.9f, 0.76f, 0.46f}, 32.0f);

    const float roomDim = 4.0f;
    addPlane({0.0f, 1.0f, 0.0f}, roomDim, {1.0f, 1.0f, 1.0f}, 32.0f);
    addPlane({0.0f, -1.0f, 0.0f}, roomDim, {1.0f, 1.0f, 1.0f}, 32.0f);
    addPlane({0.0f, 0.0f, 1.0f}, roomDim, {1.0f, 1.0f, 1.0f}, 32.0f);
    addPlane({0.0f, 0.0f, -1.0f}, roomDim, {.0f, .0f, .0f}, 32.0f);
    addPlane({-1.0f, 0.0f, 0.0f}, roomDim, {1.0f, 0.0f, 0.0f}, 32.0f);
    addPlane({1.0f, 0.0f, 0.0f}, roomDim, {0.0f, 1.0f, 0.0f}, 32.0f);


    VkDeviceSize storageBufferSize = sceneObjects.size() * sizeof(SceneObject);

    ResourceHandle stagingBuffer = rm.createResource<Buffer>("staging-buffer", BufferDesc{
                                                                 .instanceSize = sizeof(SceneObject),
                                                                 .instanceCount = static_cast<uint32_t>(sceneObjects.
                                                                     size()),
                                                                 .usageFlags =
                                                                 VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                                                 .allocationFlags =
                                                                 VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
                                                                 // CPU & GPU
                                                                 .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
                                                             });
    rm.getResource<Buffer>(stagingBuffer)->map();
    rm.getResource<Buffer>(stagingBuffer)->writeToBuffer(sceneObjects.data());
    ResourceHandle storageBuffer = rm.createResource<Buffer>("compute-storage-buffer", BufferDesc{
                                                                 .instanceSize = sizeof(SceneObject),
                                                                 .instanceCount = static_cast<uint32_t>(sceneObjects.
                                                                     size()),
                                                                 .usageFlags =
                                                                 VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                                                                 VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                                                 .allocationFlags =
                                                                 VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT, // GPU only
                                                                 .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
                                                             });
    device.copyBuffer(rm.getResource<Buffer>(stagingBuffer)->getBuffer(),
                      rm.getResource<Buffer>(storageBuffer)->getBuffer(), storageBufferSize);
    rm.releaseResource(stagingBuffer.getUid());


    // Forward declare pipelines
    std::unique_ptr<ComputePipeline> computePipeline = nullptr;
    std::unique_ptr<GraphicsPipeline> presentPipeline = nullptr;

    // Create the actual render graph
    const auto renderGraph = std::make_unique<RenderGraph>(device, rm, fm, *descriptorPool);
    renderGraph->addResource<ResourceNode::Type::UniformBuffer, Buffer, BufferDesc>(
        "compute-uniform-buffer", BufferDesc{
            .instanceSize = sizeof(UniformBuffer),
            .instanceCount = 1,
            .usageFlags =
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            .allocationFlags =
            VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        });
    renderGraph->addStaticResource<ResourceNode::Type::StorageBuffer>(
        "compute-storage-buffer", storageBuffer);
    renderGraph->addResource<ResourceNode::Type::StorageImage, Image, ImageDesc>(
        "compute-storage-image", ImageDesc{
            .width = window.getExtent().width,
            .height = window.getExtent().height,
            .channels = 4,
            .format = VK_FORMAT_R8G8B8A8_UNORM,
            .usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
            .imageType = VK_IMAGE_TYPE_2D,
            .imageViewType = VK_IMAGE_VIEW_TYPE_2D,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE
        }
    );
    renderGraph->createSampler("default-sampler");
    renderGraph->addSwapChainImageResource("swap-color-image");


    renderGraph->addPass<CommandQueueFamily::Compute>("compute-pass")
            .read(ResourceAccess{
                .resourceName = "compute-uniform-buffer",
            })
            .read(ResourceAccess{
                .resourceName = "compute-storage-buffer",
            })
            .descriptor(0, {
                            {0, {"compute-storage-image"},VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT},
                            {1, {"compute-uniform-buffer"},VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT},
                            {2, {"compute-storage-buffer"},VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT}
                        })
            .write(ResourceAccess{
                .resourceName = "compute-storage-image",
                .requiredLayout = VK_IMAGE_LAYOUT_GENERAL,
            })
            .execute([&](RenderPassContext context)-> void {
                computePipeline->bind(context.commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE);
                context.bindDescriptorSet(0, 0, computePipeline->pipelineLayout,
                                          VK_PIPELINE_BIND_POINT_COMPUTE);
                context.get<Buffer>("compute-uniform-buffer")->writeToBuffer(&uniformData);
                vkCmdDispatch(context.commandBuffer, window.getExtent().width / 16, window.getExtent().height / 16, 1);
            });


    renderGraph->addPass<CommandQueueFamily::Graphics, RelativeViewPortSize::SwapChainRelative>("present-pass")
            .read(ResourceAccess{
                .resourceName = "compute-storage-image",
                .requiredLayout = VK_IMAGE_LAYOUT_GENERAL,
                .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD
            })
            .descriptor(0, {
                            {0, {"compute-storage-image"},VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT},
                        })
            .write(ResourceAccess{
                .resourceName = "swap-color-image",
                .requiredLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            })
            .execute([&](RenderPassContext context)-> void {
                presentPipeline->bind(context.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS);
                context.bindDescriptorSet(0, 0, presentPipeline->pipelineLayout,
                                          VK_PIPELINE_BIND_POINT_GRAPHICS);
                vkCmdDraw(context.commandBuffer, 3, 1, 0, 0);
            });

    renderGraph->addPass<CommandQueueFamily::Graphics, RelativeViewPortSize::SwapChainRelative>("user-interface-pass")
            .autoBeginRenderingDisabled() // ui uses custom render pass
            .write(ResourceAccess{
                .resourceName = "swap-color-image",
                .requiredLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            })
            .execute([&](RenderPassContext context)-> void {
                VkRenderPassBeginInfo renderPassInfo{};
                renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
                renderPassInfo.renderPass = fm.getSwapChain()->getRenderPass();
                renderPassInfo.framebuffer = fm.getSwapChain()->getFramebuffer(fm.getSwapChainImageIndex());
                renderPassInfo.renderArea.offset = {0, 0};
                renderPassInfo.renderArea.extent = fm.getSwapChain()->getSwapChainExtent();
                // Define a clear color (e.g., black with full alpha)
                VkClearValue clearColor = {.color = {{0.0f, 0.0f, 0.0f, 1.0f}}};
                renderPassInfo.clearValueCount = 1;
                renderPassInfo.pClearValues = &clearColor;

                // Begin the render pass
                vkCmdBeginRenderPass(context.commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

                ui.beginUserInterface();
                ui.showDemoWindow();
                ui.endUserInterface(context.commandBuffer);
                vkCmdEndRenderPass(context.commandBuffer);
            });

    renderGraph->build();

    computePipeline = ComputePipeline::create({
        .debugName = "compute-pipeline",
        .device = device,
        .computeShader{.byteCode = Filesystem::readFile(compiledShaderPath("raytrace.comp")),},
        .descriptorSetLayouts = {renderGraph->getDescriptorSetLayouts("compute-pass")},
        .pushConstantRanges{}
    });

    presentPipeline = GraphicsPipeline::create({
        .debugName = "present-pipeline",
        .device = device,
        .vertexShader
        {.byteCode = Filesystem::readFile(compiledShaderPath("fullscreen_headless.vert")),},
        .fragmentShader
        {.byteCode = Filesystem::readFile(compiledShaderPath("render_graph.frag")),},
        .descriptorSetLayouts = {renderGraph->getDescriptorSetLayouts("present-pass")},
        .pushConstantRanges{},
        .graphicsState{
            .cullMode = VK_CULL_MODE_NONE,
            .vertexBufferBindings{}
        },
        .dynamicRendering = {
            .enabled = true,
            .colorAttachmentCount = 1,
            .colorAttachmentFormats = {fm.getSwapChain()->getSwapChainImageFormat()},
        }
    });


    device.waitIdle();
    while (!window.shouldClose()) {
        window.pollEvents();

        renderGraph->execute();
    }
    device.waitIdle();
}
