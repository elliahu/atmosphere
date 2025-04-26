#include "ParticipatingMediumScene.h"
#include "SignedDistanceField.h"

void ParticipatingMediumScene::init() {
    // Load the SDF from the disk
    int32_t grid = 258;
    ScopedMemory sdfData(SignedDistanceField().loadFromFile(assetPath("sdf/dragon"), grid).data());

    // Create the staging buffer
    ResourceHandle sdfStagingBuffer = rm.createResource<Buffer>(
        "sdf-staging-buffer", BufferDesc{
            .instanceSize = sizeof(float32_t),
            .instanceCount = static_cast<uint32_t>(grid * grid * grid * 1),
            .usageFlags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            .allocationFlags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
        }
    );

    rm.getResource<Buffer>(sdfStagingBuffer)->map();
    rm.getResource<Buffer>(sdfStagingBuffer)->writeToBuffer(sdfData.get());

    // Create the actual image resource
    sdf = rm.createResource<Image>(
        "sdf-image", ImageDesc{
            .width = static_cast<uint32_t>(grid),
            .height = static_cast<uint32_t>(grid),
            .channels = 1,
            .depth = static_cast<uint32_t>(grid),
            .format = VK_FORMAT_R32_SFLOAT,
            .usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
            .imageType = VK_IMAGE_TYPE_3D,
            .imageViewType = VK_IMAGE_VIEW_TYPE_3D,
        }
    );

    // Copy data from buffer to image
    rm.getResource<Image>(sdf)->queueImageLayoutTransition(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    rm.getResource<Image>(sdf)->queueCopyFromBuffer(rm.getResource<Buffer>(sdfStagingBuffer)->getBuffer());
    // Image will be transitioned into SHADER_READ_ONLY_OPTIMAL by the render graph automatically

    // Release the staging buffer
    rm.releaseResource(sdfStagingBuffer.getUid());

    // Load the 3D noise from the disk
    int w, h, c, d;
    ScopedMemory densityNoiseData(readVolume(Filesystem::ls(assetPath("noise/base")), w, h, c, d,
                                             Filesystem::ImageFormat::R16G16B16A16_SFLOAT));

    // Create the staging buffer
    ResourceHandle densityNoiseStagingBuffer = rm.createResource<Buffer>(
        "density-noise-staging-buffer", BufferDesc{
            .instanceSize = sizeof(float16_t),
            .instanceCount = static_cast<uint32_t>(w * h * d * c),
            .usageFlags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            .allocationFlags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
        }
    );

    rm.getResource<Buffer>(densityNoiseStagingBuffer)->map();
    rm.getResource<Buffer>(densityNoiseStagingBuffer)->writeToBuffer(densityNoiseData.get());

    // Create the actual resource
    densityNoise = rm.createResource<Image>(
        "density-noise-image", ImageDesc{
            .width = static_cast<uint32_t>(w),
            .height = static_cast<uint32_t>(h),
            .channels = static_cast<uint32_t>(c),
            .depth = static_cast<uint32_t>(d),
            .format = VK_FORMAT_R16G16B16A16_SFLOAT,
            .usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
            .imageType = VK_IMAGE_TYPE_3D,
            .imageViewType = VK_IMAGE_VIEW_TYPE_3D,
        }
    );

    // Copy data from buffer to image
    rm.getResource<Image>(densityNoise)->queueImageLayoutTransition(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    rm.getResource<Image>(densityNoise)->queueCopyFromBuffer(rm.getResource<Buffer>(densityNoiseStagingBuffer)->getBuffer());
    // Image will be transitioned into SHADER_READ_ONLY_OPTIMAL by the render graph automatically

    // Release the staging buffer
    rm.releaseResource(densityNoiseStagingBuffer.getUid());

    // Load the curl noise, we don't require any precision here so 8 bits is ok
    ScopedMemory curlNoiseData(readImage(assetPath("noise/curlNoise.png"), w, h, c,
                                         Filesystem::ImageFormat::R8G8B8A8_UNORM));

    // Create host visible staging buffer on device
    ResourceHandle curlNoiseStagingBuffer = rm.createResource<Buffer>(
        "curl-staging-buffer",
        BufferDesc{
            .instanceSize = sizeof(uchar8_t),
            .instanceCount = static_cast<uint32_t>(w * h * c),
            .usageFlags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            .allocationFlags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
        }
    );

    // Write the data into the staging buffer
    rm.getResource<Buffer>(curlNoiseStagingBuffer)->map();
    rm.getResource<Buffer>(curlNoiseStagingBuffer)->writeToBuffer(curlNoiseData.get());

    // Create image resource
    curlNoise = rm.createResource<Image>(
        "curl-noise",
        ImageDesc{
            .width = static_cast<uint32_t>(w),
            .height = static_cast<uint32_t>(h),
            .channels = static_cast<uint32_t>(c),
            .format = VK_FORMAT_R8G8B8A8_UNORM,
            .usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            .imageType = VK_IMAGE_TYPE_2D,
            .imageViewType = VK_IMAGE_VIEW_TYPE_2D,
        }
    );

    // Copy the data from buffer into the image
    rm.getResource<Image>(curlNoise)->queueImageLayoutTransition(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    rm.getResource<Image>(curlNoise)->queueCopyFromBuffer(rm.getResource<Buffer>(curlNoiseStagingBuffer)->getBuffer());
    // Image will be transitioned into SHADER_READ_ONLY_OPTIMAL by the render graph automatically

    // Release the staging buffer
    rm.releaseResource(curlNoiseStagingBuffer.getUid());

    ScopedMemory blueNoiseData(readImage(assetPath("noise/blue_noise.png"), w, h, c,
                                         Filesystem::ImageFormat::R8G8B8A8_UNORM));

    // Create host visible staging buffer on device
    ResourceHandle blueNoiseStagingBuffer = rm.createResource<Buffer>(
        "blue-staging-buffer",
        BufferDesc{
            .instanceSize = sizeof(uchar8_t),
            .instanceCount = static_cast<uint32_t>(w * h * c),
            .usageFlags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            .allocationFlags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
        }
    );

    // Write the data into the staging buffer
    rm.getResource<Buffer>(blueNoiseStagingBuffer)->map();
    rm.getResource<Buffer>(blueNoiseStagingBuffer)->writeToBuffer(blueNoiseData.get());

    // Create image resource
    blueNoise = rm.createResource<Image>(
        "blue-noise",
        ImageDesc{
            .width = static_cast<uint32_t>(w),
            .height = static_cast<uint32_t>(h),
            .channels = static_cast<uint32_t>(c),
            .format = VK_FORMAT_R8G8B8A8_UNORM,
            .usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            .imageType = VK_IMAGE_TYPE_2D,
            .imageViewType = VK_IMAGE_VIEW_TYPE_2D,
        }
    );

    // Copy the data from buffer into the image
    rm.getResource<Image>(blueNoise)->queueImageLayoutTransition(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    rm.getResource<Image>(blueNoise)->queueCopyFromBuffer(rm.getResource<Buffer>(blueNoiseStagingBuffer)->getBuffer());
    // Image will be transitioned into SHADER_READ_ONLY_OPTIMAL by the render graph automatically

    // Release the staging buffer
    rm.releaseResource(blueNoiseStagingBuffer.getUid());

    // Other resource are managed by the render graph
    buildRenderGraph();

    // Finally build the pipelines
    buildPipelines();

    // In the constructor if IScene, device is waiting after the initialization so that the queues are finished before rendering
    // No need to do it here again
}

void ParticipatingMediumScene::buildRenderGraph() {
    // We will be using single uniform buffer object that will be managed by the render graph
    renderGraph->addResource<ResourceNode::Type::UniformBuffer, Buffer, BufferDesc>(
        "ubo", BufferDesc{
            .instanceSize = sizeof(UniformBuffer),
            .instanceCount = 1,
            .usageFlags = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            .allocationFlags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
        });

    // declare static resources managed externally
    renderGraph->addStaticResource<ResourceNode::Type::SampledImage>("sdf", sdf);
    renderGraph->addStaticResource<ResourceNode::Type::SampledImage>("density-noise", densityNoise);
    renderGraph->addStaticResource<ResourceNode::Type::SampledImage>("curl-noise", curlNoise);
    renderGraph->addStaticResource<ResourceNode::Type::SampledImage>("blue-noise", blueNoise);

    // Create a default sampler that will be used to sample the images
    renderGraph->createSampler("default-sampler", SamplerDesc{});

    // And tell the graph that we will be outputting to swap chain
    renderGraph->addSwapChainImageResource("swap-color-image");


    // Next, declare the render passes and what resources each pass uses
    renderGraph->addPass<CommandQueueFamily::Graphics, RelativeViewPortSize::SwapChainRelative>("forward-pass")
            .read(ResourceAccess{
                .resourceName = "ubo",
            })
            .read(ResourceAccess{
                .resourceName = "sdf",
                .requiredLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            })
            .read(ResourceAccess{
                .resourceName = "density-noise",
                .requiredLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            })
            .read(ResourceAccess{
                .resourceName = "curl-noise",
                .requiredLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            })
            .read(ResourceAccess{
                .resourceName = "blue-noise",
                .requiredLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            })
            .descriptor(0, {
                            {0, {"ubo"}, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT},
                            {1, {"sdf"}, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT},
                            {2, {"density-noise"}, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT},
                            {3, {"curl-noise"}, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT},
                            {4, {"blue-noise"}, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT},
                        })
            .write(ResourceAccess{
                .resourceName = "swap-color-image",
                .requiredLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                .finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
            })
            .execute([this](RenderPassContext context)-> void {
                context.get<Buffer>("ubo")->writeToBuffer(&ubo);

                pipeline->bind(context.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS);
                context.bindDescriptorSet(0, 0, pipeline->pipelineLayout, VK_PIPELINE_BIND_POINT_GRAPHICS);

                vkCmdPushConstants(context.commandBuffer, pipeline->pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0,
                                   sizeof(PushConstants), &pushConstants);

                // Even though there is no vertex buffer, this call is safe as it does not actually read the vertices in the shader
                // This only triggers fullscreen effect in vert shader that runs fragment shader for each pixel of the screen
                vkCmdDraw(context.commandBuffer, 3, 1, 0, 0);
            });


    renderGraph->addPass<CommandQueueFamily::Graphics, RelativeViewPortSize::SwapChainRelative>("ui-pass")
            .autoBeginRenderingDisabled() // This will tell the render graph to not call begin rendering
            .write(ResourceAccess{
                .resourceName = "swap-color-image",
                .requiredLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, // Last pass before present
            })
            .execute([this](RenderPassContext context)-> void {
                // Clarification on why calling vkCmdBeginRenderPass:
                // The rendergraph by default uses dynamic rendering but the ImGUI vulkan backend requires valid VkRednerPass object
                // In dynamic rendering workflow, no VkRenderPasses and VkFramebuffer are created or used
                // That is why we need to tell the rendergraph to skip the dynamic rendering in this pass and begin the pass ourselves
                VkRenderPassBeginInfo renderPassInfo{};
                renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
                renderPassInfo.renderPass = fm.getSwapChain()->getRenderPass();
                renderPassInfo.framebuffer = fm.getSwapChain()->getFramebuffer(fm.getSwapChainImageIndex());
                renderPassInfo.renderArea.offset = {0, 0};
                renderPassInfo.renderArea.extent = fm.getSwapChain()->getSwapChainExtent();
                VkClearValue clearColor = {.color = {{0.0f, 0.0f, 0.0f, 1.0f}}};
                renderPassInfo.clearValueCount = 1;
                renderPassInfo.pClearValues = &clearColor;

                // Begin the render pass
                vkCmdBeginRenderPass(context.commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

                // Draw the UI
                ui.get()->beginUserInterface();

                ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
                ImGui::SetNextWindowPos({0, static_cast<float>(window.getExtent().height)}, 0, {0, 1});
                ImGui::Begin("Editor options", (bool *) false,
                             ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove |
                             ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing |
                             ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoDecoration);

                ImGui::SeparatorText("Status");
                ImGui::Text("Camera position <%.3f;%.3f;%.3f>", ubo.eye.X, ubo.eye.Y, ubo.eye.Z);

                ImGui::SeparatorText("Participating medium properties");
                ImGui::SliderFloat3("Scattering", &pushConstants.scattering.Elements[0], 0.0f, 2.0f);
                ImGui::SliderFloat3("Absorption", &pushConstants.absorption.Elements[0], 0.0f, 2.0f);
                ImGui::SliderFloat("Mie (phase) G", &pushConstants.mieG, -.99f, .99);
                ImGui::SliderFloat("Density override", &pushConstants.densityMultiplier, 1.0f, 1000.0f, "%.0f");

                ImGui::SeparatorText("Light");
                ImGui::DragFloat3("Light position", &ubo.lightPosition.Elements[0], 0.1f);
                ImGui::ColorEdit3("Light color", &ubo.lightColor.Elements[0]);

                ImGui::SeparatorText("Rendering");
                ImGui::SliderInt("Jitter", &pushConstants.jitter, 0,1);
                if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)){
                  ImGui::SetTooltip("Randomly offset ray origin to eliminate banding");
                }
                ImGui::SliderFloat("Jitter strength", &pushConstants.jitterStrength, 0.0f,10.0f);

                ImGui::SeparatorText("Noise");
                ImGui::SliderFloat("Scale", &pushConstants.densityScale, 0.5f, 10.0f);
                ImGui::SliderInt("Light steps", &pushConstants.lightSteps, 1, 100);
                ImGui::SliderFloat("Light step size", &pushConstants.lightStepSize, 0.0001f, .1f, "%.4f");


                ImGui::PopStyleVar();
                ImGui::End();

                ui.get()->endUserInterface(context.commandBuffer);

                // End the render pass
                vkCmdEndRenderPass(context.commandBuffer);
            });

    renderGraph->build();
}

void ParticipatingMediumScene::buildPipelines() {
    pipeline = GraphicsPipeline::create({
        .debugName = "forward-pipeline",
        .device = device,
        .vertexShader
        // Fullscreen vertex shader
        {.byteCode = Filesystem::readFile(compiledShaderPath("medium.vert")),},
        .fragmentShader
        // Fragment shader samples storage texture and writes it to swapchain image
        {.byteCode = Filesystem::readFile(compiledShaderPath("medium.frag")),},
        .descriptorSetLayouts = {renderGraph->getDescriptorSetLayouts("forward-pass")},
        .pushConstantRanges{{VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushConstants)}},
        .graphicsState{
            // We disable cull so that the vkCmdDraw command is not skipped
            .cullMode = VK_CULL_MODE_NONE,
            .blendAtaAttachmentStates{Init::pipelineColorBlendAttachmentState(0xf, VK_TRUE)},
            .vertexBufferBindings{}
        },
        .dynamicRendering = {
            // Render graph requires by default dynamic rendering
            .enabled = true,
            .colorAttachmentCount = 1,
            .colorAttachmentFormats = {fm.getSwapChain()->getSwapChainImageFormat()},
        }
    });
}

void ParticipatingMediumScene::update() {
    if (progressTime) {
        ubo.elapsedTime += deltaTime;
    }

    ubo.resX = static_cast<float>(window.getExtent().width);
    ubo.resY = static_cast<float>(window.getExtent().height);

    float speed = 1.0f;
    if (window.isKeyDown(Surfer::KeyCode::KeyA)) azimuth += speed * deltaTime;
    if (window.isKeyDown(Surfer::KeyCode::KeyD)) azimuth -= speed * deltaTime;
    if (window.isKeyDown(Surfer::KeyCode::KeyW)) elevation += speed * deltaTime;
    if (window.isKeyDown(Surfer::KeyCode::KeyS)) elevation -= speed * deltaTime;
    if (window.isKeyDown(Surfer::KeyCode::ArrowUp)) radius -= speed * deltaTime;
    if (window.isKeyDown(Surfer::KeyCode::ArrowDown)) radius += speed * deltaTime;

    ubo.eye.XYZ = Math::orbitalPosition(HmckVec3{.0f, .0f, .0f}, radius, azimuth, elevation);
    //ubo.eye.XYZ = HmckVec3{0.0f, 0.0f, 2.0f};
    ubo.view = Projection().view(ubo.eye.XYZ, {0.f, 0.f, 0.f}, Projection().upPosY());
    ubo.proj = Projection().perspective(45.0f, fm.getAspectRatio(), 0.1f, 64.0f);
}

void ParticipatingMediumScene::render() {
    auto currentTime = std::chrono::high_resolution_clock::now();
    while (!window.shouldClose()) {
        // Timing
        auto newTime = std::chrono::high_resolution_clock::now();
        deltaTime = std::chrono::duration<float, std::chrono::seconds::period>(newTime - currentTime).count();
        currentTime = newTime;

        // Poll for events
        window.pollEvents();

        // Update the data for the frame
        update();

        // Execute the render graph
        renderGraph->execute();
    }
    // Wait for queues to finish before deallocating resources
    device.waitIdle();
}
