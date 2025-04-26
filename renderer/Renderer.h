#pragma once
#include <hammock/hammock.h>
#include "Camera.h"
#include "atmosphere/AtmospherePass.h"
#include "geometry/GeometryPass.h"
#include "clouds/CloudsPass.h"
#include "composition/CompositionPass.h"
#include "composition/PostProcessingPass.h"
#include "composition/GodRaysPass.h"
#include "depth/DepthPass.h"
#include "ui/UserInterface.h"
#include "Profiler.h"
#include "BenchmarkResult.h"


using namespace hammock;

/**
 * This is the main renderer class
 * It uses hammock engine under the hood, which is my custom Vulkan abstraction layer
 * TODO do a depth prepass so that we can perform depth culling in cloud compute
 * TODO store non-linear cloud depth so we can then blend them using areal perspective
 * TODO light shafts should be performed after the composition by blending clouds mask, and terrain depth
 */
class Renderer final{
    // Vulkan instance
    VulkanInstance instance{};
    // Window class, uses hammock's window class which stands on top of VulkanSurfer lib
    hammock::Window window;
    // Physical device (GPU)
    Device device;
    // Resource manages is responsible for resource alloc/release on device
    ResourceManager resourceManager;
    // Frame manager handles queue submission and contains swap chain abstraction
    FrameManager frameManager;
    // Profiler is used to measure time of render passes / dispatches, see definition
    Profiler profiler;
    // Descriptor pool is used to allocate descriptor sets and layouts
    std::unique_ptr<DescriptorPool> descriptorPool;
    // CPU Thread pool
    ThreadPool threadPool{};
    uint32_t processorCount{0};

    // Deletion queue is used to queue resources that should be deleted
    std::queue<ResourceHandle> deletionQueue;

    // Launch dimensions
    uint32_t lWidth, lHeight;

    // Results of benchmark
    BenchmarkResult benchmarkResult;

    // Benchmarking
    float deltaTime{0.f}, elapsedTime{0.f};
    static constexpr int FRAMETIME_BUFFER_SIZE{512}; // Number of frames to track
    float frameTimes[FRAMETIME_BUFFER_SIZE] = {0.0f};
    int frameTimeFrameIndex{0};
    bool progressTime{true};
    int frameIndex{0};

    // Perspective camera
    Camera camera{
        HmckVec3{35.397, 4.296, 67.394},
        static_cast<float>(lWidth) /  static_cast<float>(lHeight),
        HmckToRad(HmckAngleDeg(65.f)), 0.1f, 300.f, 34.671, 0.300,};

    // Movement
    const float movementSpeed = 1.0f; // Units per frame
    const float rotationSpeed = 1.0f; // Radians per frame

    // Light

    // Geometry buffers
    ResourceHandle vertexBuffer;
    ResourceHandle indexBuffer;

    // Actual geometry
    Geometry geometry;


    // Command buffers
    // There is one command buffer per group per frame,
    // so that the CPU can record commands for next frame while gpu processes commands from current frame (2 frames in flight)
    struct {
        std::array<VkCommandBuffer, SwapChain::MAX_FRAMES_IN_FLIGHT> clouds; // Clouds and blur
        std::array<VkCommandBuffer, SwapChain::MAX_FRAMES_IN_FLIGHT> atmosphere;
        std::array<VkCommandBuffer, SwapChain::MAX_FRAMES_IN_FLIGHT> depth;
        std::array<VkCommandBuffer, SwapChain::MAX_FRAMES_IN_FLIGHT> terrain;
        std::array<VkCommandBuffer, SwapChain::MAX_FRAMES_IN_FLIGHT> composition; // Composition and postprocess

        std::array<VkCommandBuffer, SwapChain::MAX_FRAMES_IN_FLIGHT> graphicsToComputeTransferRelease;
        std::array<VkCommandBuffer, SwapChain::MAX_FRAMES_IN_FLIGHT> computeToGraphicsTransferRelease;

        std::array<VkCommandBuffer, SwapChain::MAX_FRAMES_IN_FLIGHT> graphicsToComputeTransferAcquire;
        std::array<VkCommandBuffer, SwapChain::MAX_FRAMES_IN_FLIGHT> computeToGraphicsTransferAcquire;

    } commandBuffers;

    // Semaphores signal that the command buffer is finished so that different command buffer waiting for its result can start
    struct {
        std::array<VkSemaphore, SwapChain::MAX_FRAMES_IN_FLIGHT> depthReady;
        std::array<VkSemaphore, SwapChain::MAX_FRAMES_IN_FLIGHT> cloudsReady;
        std::array<VkSemaphore, SwapChain::MAX_FRAMES_IN_FLIGHT> atmosphereReady;
        std::array<VkSemaphore, SwapChain::MAX_FRAMES_IN_FLIGHT> terrainColorReady;

        std::array<VkSemaphore, SwapChain::MAX_FRAMES_IN_FLIGHT> graphicsToComputeTransferClouds;
        std::array<VkSemaphore, SwapChain::MAX_FRAMES_IN_FLIGHT> graphicsToComputeTransferAtmosphere;
        std::array<VkSemaphore, SwapChain::MAX_FRAMES_IN_FLIGHT> graphicsToComputeTransferGeometry;
        std::array<VkSemaphore, SwapChain::MAX_FRAMES_IN_FLIGHT> computeToGraphicsTransfer;
        std::array<VkSemaphore, SwapChain::MAX_FRAMES_IN_FLIGHT> graphicsToComputeSync;
        std::array<VkSemaphore, SwapChain::MAX_FRAMES_IN_FLIGHT> computeToGraphicsSync;
        std::array<VkSemaphore, SwapChain::MAX_FRAMES_IN_FLIGHT> computeToComputeSync;
    } semaphores;

    // Synchronization of the frames in flight (waiting until framesInFlight + 1 frame is acquired) is handled internally by the swap chain object

    // Render passes
    DepthPass depthPass;
    GeometryPass geometryPass;
    CloudsPass cloudsPass;
    AtmospherePass atmospherePass;
    GodRaysPass godRaysPass;
    CompositionPass compositionPass;
    PostProcessingPass postProcessingPass;

    // User interface system
    std::unique_ptr<::UserInterface> ui;

    /**
     * Queues resource for deletion
     * @param resource Resource that will be deleted
     * @return ResourceHandle
     */
    ResourceHandle queueForDeletion(ResourceHandle resource) {
        deletionQueue.push(resource);
        return resource;
    }

    /**
     * Deletes all items in the queue
     */
    void processDeletionQueue();

    /**
     * Allocates command buffer one per frame per queue
     */
    void allocateCommandBuffers();

    /**
     * Destroys all command buffers
     */
    void destroyCommandBuffers();

    /**
     * Creates synchronization primitives
     */
    void createSyncObjects();

    /**
     * Destroys synchronization primitives
     */
    void destroySyncObjects();

    void prepareGeometry();

    /**
     * Initializes the renderer
     */
    void init();

    /**
     * Helper method that queues a transition of given swap chain image on the composition queue
     * @param from original layout
     * @param to new layout
     * @param frameIndex index of the current frame the image corresponds to
     * @param imageIndex index of the current swap image (there is more images then frames in flight)
     */
    void recordSwapChainImageTransition(VkImageLayout from, VkImageLayout to, uint32_t frameIndex, uint32_t imageIndex);

    void recordGraphicsToComputeTransfers(uint32_t frameIndex);
    void recordComputeToGraphicsTransfers(uint32_t frameIndex);



    /**
     * All input related code is in here
     */
    void handleInput();

    /**
     * Called every frame before the draw
     * this is used to update the buffers etc.
     */
    void update();




public:
    // Constructor
    Renderer(const int32_t width, const int32_t height);
    // Destructor
    ~Renderer();

    /**
     * Initiates the rendering loop
     */
    void render();

    BenchmarkResult& getBenchmarkResult() { return benchmarkResult; }
};