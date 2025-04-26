#pragma once
#include <cstdint>
#include <cstdarg>
#include <cstdio>
#include <iostream>
#include <stdexcept>
#include <string>
#include <cstdlib> // for abort
#include <algorithm>
#include <hammock/core/HandmadeMath.h>
#include <hammock/utils/Initializers.h>
#include "stb_image.h"
#include <cassert>
#include <functional>
#include <cmath>

namespace hammock {
    enum LogLevel {
        LOG_LEVEL_DEBUG,
        LOG_LEVEL_WARN,
        LOG_LEVEL_ERROR
    };

    class Logger {
    public:
#ifdef NDEBUG
        static inline LogLevel hmckMinLogLevel = LOG_LEVEL_WARN;
#else
        static inline LogLevel hmckMinLogLevel = LOG_LEVEL_DEBUG;
#endif

        static void log(const LogLevel level, const char *format, ...) {
            if (level >= hmckMinLogLevel) {
                const char* prefix = "";
                switch (level) {
                    case LOG_LEVEL_DEBUG:
                        prefix = "DEBUG: ";
                    break;
                    case LOG_LEVEL_WARN:
                        prefix = "WARNING: ";
                    break;
                    case LOG_LEVEL_ERROR:
                        prefix = "ERROR: ";
                    break;
                }

                // Print the prefix first
                printf("%s", prefix);

                // Print the formatted message
                va_list args;
                va_start(args, format);
                vprintf(format, args);
                va_end(args);
            }
        }
    };

    namespace AssertUtils {
        // Behavior options for failed assertions
        enum class AssertAction {
            Abort,
            Throw,
            Log,
        };

        // Current action; can be set dynamically
        inline AssertAction CurrentAction = AssertAction::Abort;

        // Assert handler
        inline void HandleAssert(const char *expr, const char *file, int line, const char *func,
                                 const std::string &message) {
            // Construct the debug message
            std::string debugMessage = "[ASSERT FAILED]\n";
            debugMessage += "Expression: " + std::string(expr) + "\n";
            debugMessage += "File: " + std::string(file) + "\n";
            debugMessage += "Line: " + std::to_string(line) + "\n";
            debugMessage += "Function: " + std::string(func) + "\n";
            if (!message.empty()) {
                debugMessage += "Message: " + message + "\n";
            }

            switch (CurrentAction) {
                case AssertAction::Abort:
                    std::cerr << debugMessage << std::endl;
                    std::abort();
                case AssertAction::Throw:
                    throw std::runtime_error(debugMessage);
                case AssertAction::Log:
                    // Replace with your logging system if needed
                    std::cerr << debugMessage << std::endl;
                    break;
            }
        }
    } // namespace AssertUtils

    // Custom assert macro
#ifndef ASSERT
#if defined(_MSC_VER)
#define ASSERT(expr, message)                                            \
do {                                                                 \
if (!(expr)) {                                                   \
AssertUtils::HandleAssert(#expr, __FILE__, __LINE__,         \
__FUNCTION__, message);   /* Use __FUNCTION__ for MSVC */    \
}                                                                \
} while (false)
#else
#define ASSERT(expr, message)                                            \
do {                                                                 \
if (!(expr)) {                                                   \
AssertUtils::HandleAssert(#expr, __FILE__, __LINE__,         \
__PRETTY_FUNCTION__, message);  /* Use __PRETTY_FUNCTION__ */ \
}                                                                \
} while (false)
#endif
#endif
    struct alignas(16) IntPadded {
        int32_t value;
        int32_t padding[3]; // Explicit padding to 16 bytes
    };

    struct alignas(16) FloatPadded {
        float value;
        float padding[3];
    };

    struct alignas(16) Vec3Padded {
        HmckVec3 value;
        float padding;
    };

    // Follows std140 alignment
    struct alignas(16) GlobalDataBuffer {
        static constexpr size_t MAX_MESHES = 256;

        alignas(16) HmckVec4 baseColorFactors[MAX_MESHES]; // w is padding
        alignas(16) HmckVec4 metallicRoughnessAlphaCutOffFactors[MAX_MESHES]; // w is padding

        IntPadded baseColorTextureIndexes[MAX_MESHES];
        IntPadded normalTextureIndexes[MAX_MESHES];
        IntPadded metallicRoughnessTextureIndexes[MAX_MESHES];
        IntPadded occlusionTextureIndexes[MAX_MESHES];
        IntPadded visibilityFlags[MAX_MESHES];
    };

    // Projection buffer bound every frame
    struct FrameDataBuffer {
        HmckMat4 projectionMat{};
        HmckMat4 viewMat{};
        HmckMat4 inverseViewMat{};
        HmckVec4 exposureGammaWhitePoint{4.5f, 1.0f, 11.0f};
    };

    // Push block pushed for each mesh
    struct PushBlockDataBuffer {
        HmckMat4 modelMat{};
        uint32_t meshIndex = -1;
    };

    class NonCopyable {
    protected:
        // Protected default constructor and destructor
        // Allows instantiation by derived classes, but not directly
        NonCopyable() = default;

        ~NonCopyable() = default;

        // Deleted copy constructor and copy assignment operator
        NonCopyable(const NonCopyable &) = delete;

        NonCopyable &operator=(const NonCopyable &) = delete;
    };

    // TODO FIXME this is total shit! it needs to know the size of the allocation and also if it should use free() for C style or delete[]
    class [[deprecated("Use AutoDele class")]] ScopedMemory {
    public:
        // Constructor to take ownership of the pointer
        explicit ScopedMemory(const void *ptr = nullptr)
            : memory_(ptr) {
        }

        // Destructor automatically frees the memory
        ~ScopedMemory() {
            clear();
        }

        // Move constructor to allow transferring ownership
        ScopedMemory(ScopedMemory &&other) noexcept
            : memory_(other.memory_) {
            other.memory_ = nullptr; // Release ownership from the source
        }

        // Move assignment operator for ownership transfer
        ScopedMemory &operator=(ScopedMemory &&other) noexcept {
            if (this != &other) {
                clear(); // Free any existing memory
                memory_ = other.memory_;
                other.memory_ = nullptr; // Release ownership from the source
            }
            return *this;
        }

        // Deleted copy constructor and copy assignment to prevent copying
        ScopedMemory(const ScopedMemory &) = delete;

        ScopedMemory &operator=(const ScopedMemory &) = delete;

        // Free the memory manually (if needed)
        void clear() {
            if (memory_) {
                stbi_image_free(const_cast<void *>(memory_));
                memory_ = nullptr;
            }
        }

        // Retrieve the pointer
        const void *get() const {
            return memory_;
        }

        // Access the pointer with [] syntax (useful for arrays)
        const void *operator[](size_t index) const {
            return static_cast<const char *>(memory_) + index;
        }

    private:
        const void *memory_; // Pointer to the managed memory
    };

    class AutoDelete {
    public:
        using Deleter = std::function<void(const void *)>;

        explicit AutoDelete(const void *ptr = nullptr, Deleter deleter = nullptr)
            : memory_(ptr), deleter_(std::move(deleter)) {}

        ~AutoDelete() { clear(); }

        AutoDelete(AutoDelete &&other) noexcept
            : memory_(other.memory_), deleter_(std::move(other.deleter_)) {
            other.memory_ = nullptr;
        }

        AutoDelete &operator=(AutoDelete &&other) noexcept {
            if (this != &other) {
                clear();
                memory_ = other.memory_;
                deleter_ = std::move(other.deleter_);
                other.memory_ = nullptr;
            }
            return *this;
        }

        AutoDelete(const AutoDelete &) = delete;
        AutoDelete &operator=(const AutoDelete &) = delete;

        void clear() {
            if (memory_) {
                if (deleter_) deleter_(memory_);
                memory_ = nullptr;
            }
        }

        [[nodiscard]] const void *get() const { return memory_; }

    private:
        const void *memory_;
        Deleter deleter_;
    };

    // dark magic from: https://stackoverflow.com/a/57595105
    template<typename T, typename... Rest>
    void hashCombine(std::size_t &seed, const T &v, const Rest &... rest) {
        seed ^= std::hash<T>{}(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        (hashCombine(seed, rest), ...);
    };

    inline void checkResult(VkResult result) {
        assert(result == VK_SUCCESS && "Vulkan API assertion failed! This may indicates a bug in the application.");
    }

    // Check if enity of type P is derived from T
    template<typename T, typename P>
    bool isInstanceOf(std::shared_ptr<T> entity) {
        std::shared_ptr<P> derived = std::dynamic_pointer_cast<P>(entity);
        return derived != nullptr;
    }

    // tries cast T to P
    template<typename T, typename P>
    std::shared_ptr<P> cast(std::shared_ptr<T> entity) {
        std::shared_ptr<P> derived = std::dynamic_pointer_cast<P>(entity);
        if (!derived) {
            throw std::runtime_error("Dynamic cast failed: Cannot cast object to specified derived type.");
        }
        return derived;
    }

    inline bool hasExtension(std::string fullString, std::string ending) {
        if (fullString.length() >= ending.length()) {
            return (0 == fullString.compare(fullString.length() - ending.length(), ending.length(), ending));
        } else {
            return false;
        }
    }

    inline uint32_t getNumberOfMipLevels(const uint32_t width, const uint32_t height) {
        return static_cast<uint32_t>(std::floor(std::log2(std::min(width, height)))) + 1;
    }

    inline void transitionImageLayout(
        VkCommandBuffer cmdbuffer,
        VkImage image,
        VkImageLayout oldImageLayout,
        VkImageLayout newImageLayout,
        VkImageSubresourceRange subresourceRange,
        VkPipelineStageFlags srcStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        VkPipelineStageFlags dstStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        uint32_t srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        uint32_t dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED) {
        // Create an image barrier object
        VkImageMemoryBarrier imageMemoryBarrier = Init::imageMemoryBarrier();
        imageMemoryBarrier.oldLayout = oldImageLayout;
        imageMemoryBarrier.newLayout = newImageLayout;
        imageMemoryBarrier.image = image;
        imageMemoryBarrier.subresourceRange = subresourceRange;
        imageMemoryBarrier.srcQueueFamilyIndex = srcQueueFamilyIndex;
        imageMemoryBarrier.dstQueueFamilyIndex = dstQueueFamilyIndex;

        // Source layouts (old)
        // Source access mask controls actions that have to be finished on the old layout
        // before it will be transitioned to the new layout
        switch (oldImageLayout) {
            case VK_IMAGE_LAYOUT_UNDEFINED:
                // Image layout is undefined (or does not matter)
                // Only valid as initial layout
                // No flags required, listed only for completeness
                imageMemoryBarrier.srcAccessMask = 0;
                break;

            case VK_IMAGE_LAYOUT_PREINITIALIZED:
                // Image is preinitialized
                // Only valid as initial layout for linear images, preserves memory contents
                // Make sure host writes have been finished
                imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
                break;

            case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
                // Image is a color attachment
                // Make sure any writes to the color buffer have been finished
                imageMemoryBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                break;

            case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
                // Image is a depth/stencil attachment
                // Make sure any writes to the depth/stencil buffer have been finished
                imageMemoryBarrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                break;

            case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
                // Image is a transfer source
                // Make sure any reads from the image have been finished
                imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
                break;

            case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
                // Image is a transfer destination
                // Make sure any writes to the image have been finished
                imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                break;

            case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
                // Image is read by a shader
                // Make sure any shader reads from the image have been finished
                imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
                break;
            default:
                // Other source layouts aren't handled (yet)
                break;
        }

        // Target layouts (new)
        // Destination access mask controls the dependency for the new image layout
        switch (newImageLayout) {
            case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
                // Image will be used as a transfer destination
                // Make sure any writes to the image have been finished
                imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                break;

            case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
                // Image will be used as a transfer source
                // Make sure any reads from the image have been finished
                imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
                break;

            case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
                // Image will be used as a color attachment
                // Make sure any writes to the color buffer have been finished
                imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                break;

            case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
                // Image layout will be used as a depth/stencil attachment
                // Make sure any writes to depth/stencil buffer have been finished
                imageMemoryBarrier.dstAccessMask =
                        imageMemoryBarrier.dstAccessMask | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                break;

            case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
                // Image will be read in a shader (sampler, input attachment)
                // Make sure any writes to the image have been finished
                if (imageMemoryBarrier.srcAccessMask == 0) {
                    imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
                }
                imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
                break;
            default:
                // Other source layouts aren't handled (yet)
                break;
        }

        // Put barrier inside setup command buffer
        vkCmdPipelineBarrier(
            cmdbuffer,
            srcStageMask,
            dstStageMask,
            0,
            0, nullptr,
            0, nullptr,
            1, &imageMemoryBarrier);
    }

    // Fixed sub resource on first mip level and layer
    inline void transitionImageLayout(
        VkCommandBuffer cmdbuffer,
        VkImage image,
        VkImageAspectFlags aspectMask,
        VkImageLayout oldImageLayout,
        VkImageLayout newImageLayout,
        VkPipelineStageFlags srcStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        VkPipelineStageFlags dstStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT) {
        VkImageSubresourceRange subresourceRange = {};
        subresourceRange.aspectMask = aspectMask;
        subresourceRange.baseMipLevel = 0;
        subresourceRange.levelCount = 1;
        subresourceRange.layerCount = 1;
        transitionImageLayout(cmdbuffer, image, oldImageLayout, newImageLayout, subresourceRange, srcStageMask, dstStageMask);
    }

    inline size_t alignSize(size_t size, size_t alignment) {
        return (size + alignment - 1) & ~(alignment - 1);
    }

    inline bool isDepthFormat(VkFormat format) {
        std::vector<VkFormat> formats =
        {
            VK_FORMAT_D16_UNORM,
            VK_FORMAT_X8_D24_UNORM_PACK32,
            VK_FORMAT_D32_SFLOAT,
            VK_FORMAT_D16_UNORM_S8_UINT,
            VK_FORMAT_D24_UNORM_S8_UINT,
            VK_FORMAT_D32_SFLOAT_S8_UINT,
        };
        return std::ranges::find(formats, format) != std::end(formats);
    }

    inline bool isStencilFormat(VkFormat format) {
        std::vector<VkFormat> formats =
        {
            VK_FORMAT_S8_UINT,
            VK_FORMAT_D16_UNORM_S8_UINT,
            VK_FORMAT_D24_UNORM_S8_UINT,
            VK_FORMAT_D32_SFLOAT_S8_UINT,
        };
        return std::ranges::find(formats, format) != std::end(formats);
    }

    inline bool isDepthStencil(VkFormat format) {
        return (isDepthFormat(format) || isStencilFormat(format));
    }

    inline uint16_t float32float16(float f) {
        uint32_t f32 = *(uint32_t *) &f;
        uint16_t f16 = 0;

        uint32_t sign = (f32 >> 16) & 0x8000; // Extract sign bit
        uint32_t exponent = ((f32 >> 23) & 0xFF) - 112; // Adjust exponent bias
        uint32_t mantissa = (f32 & 0x007FFFFF) >> 13; // Truncate mantissa

        if (exponent <= 0) {
            // Underflow case (denormals or zero)
            f16 = sign;
        } else if (exponent >= 31) {
            // Overflow case (inf or NaN)
            f16 = sign | 0x7C00 | (mantissa ? 1 : 0);
        } else {
            // Normal conversion
            f16 = sign | (exponent << 10) | mantissa;
        }

        return f16;
    }

    //Reference: https://en.wikipedia.org/wiki/Halton_sequence
    inline float haltonSequenceAt(int index, int base)
    {
        float f = 1.0f;
        float r = 0.0f;

        while (index > 0)
        {
            f = f / float(base);
            r += f*(index%base);
            index = int(std::floor(index / base));
        }

        return r;
    }

}
