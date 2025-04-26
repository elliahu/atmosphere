#pragma once

#include <cassert>
#include <functional>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <cmath>
#include <stb_image.h>
#include <stb_image_write.h>
#include <algorithm>
#include <regex>

#include "hammock/core/CoreUtils.h"
#include "hammock/core/Types.h"



namespace hammock {
    namespace Filesystem {
        inline bool fileExists(const std::string &filename) {
            std::ifstream f(filename.c_str());
            return !f.fail();
        }

        inline std::vector<char> readFile(const std::string &filePath) {
            std::ifstream file{filePath, std::ios::ate | std::ios::binary};

            if (!file.is_open()) {
                throw std::runtime_error("failed to open file: " + filePath);
            }

            size_t fileSize = static_cast<size_t>(file.tellg());
            std::vector<char> buffer(fileSize);

            file.seekg(0);
            file.read(buffer.data(), fileSize);

            file.close();
            return buffer;
        }

        inline void dump(const std::string &filename, const std::string &data) {
            std::ofstream outFile(filename);
            if (outFile.is_open()) {
                outFile << data;
                outFile.close();
            } else {
                throw std::runtime_error("could not dump into file!");
            }
        }

        inline std::vector<std::string> ls(const std::string &directoryPath) {
            std::vector<std::string> fileList;

            try {
                for (const auto &entry: std::filesystem::directory_iterator(directoryPath)) {
                    try {
                        if (entry.is_regular_file()) {
                            fileList.push_back(entry.path().string());
                        }
                    } catch (const std::filesystem::filesystem_error &e) {
                        Logger::log(LOG_LEVEL_ERROR, "Error on entry: %s | %s\n",
                                    entry.path().string().c_str(), e.what());
                    }
                }
            } catch (const std::filesystem::filesystem_error &e) {
                Logger::log(LOG_LEVEL_ERROR, "Error: Error accessing directory %s\n", directoryPath.c_str());
                throw std::runtime_error("Error: Error accessing directory");
            }

            // Custom comparator for natural sorting
            auto naturalSort = [](const std::string &a, const std::string &b) {
                std::regex numRegex(R"(.*?(\d+))"); // Extracts the first number from the filename
                std::smatch matchA, matchB;

                int numA = std::regex_search(a, matchA, numRegex) ? std::stoi(matchA[1].str()) : 0;
                int numB = std::regex_search(b, matchB, numRegex) ? std::stoi(matchB[1].str()) : 0;

                return numA < numB;
            };

            std::sort(fileList.begin(), fileList.end(), naturalSort);
            return fileList;
        }

        enum class WriteImageDefinition {
            SDR, HDR
        };

        inline void writeImage(const std::string &filename, const void *buffer, uint32_t instanceSize, uint32_t width,
                               uint32_t height, uint32_t channels,
                               WriteImageDefinition definition = WriteImageDefinition::SDR) {
            // Handle SDR and HDR separately
            if (definition == WriteImageDefinition::SDR) {
                // For SDR, normalize to 8-bit and save as PNG
                unsigned char *normalizedBuffer = nullptr;
                if (instanceSize > 1) {
                    normalizedBuffer = new unsigned char[width * height * channels];
                    const float *floatBuffer = static_cast<const float *>(buffer);
                    for (size_t i = 0; i < width * height * channels; ++i) {
                        normalizedBuffer[i] = static_cast<unsigned char>(std::clamp(
                            floatBuffer[i] * 255.0f, 0.0f, 255.0f));
                    }
                } else {
                    // If instance size is already 1 byte, cast the buffer
                    normalizedBuffer = const_cast<unsigned char *>(static_cast<const unsigned char *>(buffer));
                }

                // Use stb_image_write to save the image as PNG
                int success = stbi_write_png(filename.c_str(), width, height, channels, normalizedBuffer,
                                             width * channels);
                if (!success) {
                    Logger::log(LOG_LEVEL_ERROR, "Error: Failed to save SDR image!");
                    throw std::runtime_error("Failed to save image: " + filename);
                }

                // Clean up if normalization was done
                if (instanceSize > 1) {
                    delete[] normalizedBuffer;
                }
            } else if (definition == WriteImageDefinition::HDR) {
                // For HDR, save as HDR file using float buffer directly
                if (instanceSize != sizeof(float)) {
                    Logger::log(LOG_LEVEL_ERROR, "Error: HDR requires float data!");
                    throw std::runtime_error("HDR images require buffer with float data.");
                }

                const float *floatBuffer = static_cast<const float *>(buffer);
                int success = stbi_write_hdr(filename.c_str(), width, height, channels, floatBuffer);
                if (!success) {
                    Logger::log(LOG_LEVEL_ERROR, "Error: Failed to save HDR image!");
                    throw std::runtime_error("Failed to save HDR image: " + filename);
                }
            } else {
                Logger::log(LOG_LEVEL_ERROR, "Error: Unknown image definition!");
                throw std::invalid_argument("Unknown image definition specified.");
            }
        }

        enum ReadImageLoadingFlags {
            FLIP_Y = 0x00000001
        };

        enum class ImageFormat {
            R32_SFLOAT,
            R32G32_SFLOAT,
            R32G32B32_SFLOAT,
            R32G32B32A32_SFLOAT,
            R16_SFLOAT,
            R16G16_SFLOAT,
            R16G16B16_SFLOAT,
            R16G16B16A16_SFLOAT,
            R8_UNORM,
            R8G8_UNORM,
            R8G8B8_UNORM,
            R8G8B8A8_UNORM
        };

        // Image loading function with 16-bit float support
        inline const void *readImage(const std::string &filename, int &width, int &height, int &channels,
                                     const ImageFormat format = ImageFormat::R32G32B32A32_SFLOAT, uint32_t flags = 0) {
            int desiredChannels = 4; // Default channels
            if (format == ImageFormat::R32_SFLOAT || format == ImageFormat::R16_SFLOAT || format == ImageFormat::R8_UNORM)
                desiredChannels = 1;
            else if (format == ImageFormat::R32G32_SFLOAT || format == ImageFormat::R16G16_SFLOAT || format == ImageFormat::R8G8_UNORM)
                desiredChannels = 2;
            else if (format == ImageFormat::R32G32B32_SFLOAT || format == ImageFormat::R16G16B16_SFLOAT || format ==
                     ImageFormat::R8G8B8_UNORM)
                desiredChannels = 3;

            stbi_set_flip_vertically_on_load(flags & FLIP_Y); // Handle vertical flipping

            // Load HDR (float) image
            if (format >= ImageFormat::R32_SFLOAT && format <= ImageFormat::R16G16B16A16_SFLOAT) {
                const float *data = stbi_loadf(filename.c_str(), &width, &height, &channels, 4);
                stbi_set_flip_vertically_on_load(false);

                if (!data) {
                    throw std::runtime_error("Failed to load HDR image.");
                }

                size_t pixelCount = static_cast<size_t>(width) * height;
                if (format >= ImageFormat::R16_SFLOAT && format <= ImageFormat::R16G16B16A16_SFLOAT) {
                    // Allocate buffer for 16-bit floats
                    uint16_t *processedData = new uint16_t[pixelCount * desiredChannels];

                    for (size_t i = 0; i < pixelCount; ++i) {
                        for (int c = 0; c < desiredChannels; ++c) {
                            processedData[i * desiredChannels + c] = float32float16(data[i * 4 + c]);
                        }
                    }

                    stbi_image_free(const_cast<float *>(data));
                    channels = desiredChannels;
                    return processedData; // 16-bit float buffer
                } else {
                    // Allocate buffer for 32-bit floats
                    float *processedData = new float[pixelCount * desiredChannels];

                    for (size_t i = 0; i < pixelCount; ++i) {
                        for (int c = 0; c < desiredChannels; ++c) {
                            processedData[i * desiredChannels + c] = data[i * 4 + c];
                        }
                    }

                    stbi_image_free(const_cast<float *>(data));
                    channels = desiredChannels;
                    return processedData; // 32-bit float buffer
                }
            }

            // Load standard 8-bit image
            if (format >= ImageFormat::R8_UNORM && format <= ImageFormat::R8G8B8A8_UNORM) {
                const unsigned char *data = stbi_load(filename.c_str(), &width, &height, &channels, 4);
                stbi_set_flip_vertically_on_load(false);

                if (!data) {
                    throw std::runtime_error("Failed to load SDR image.");
                }

                size_t pixelCount = static_cast<size_t>(width) * height;
                unsigned char *processedData = new unsigned char[pixelCount * desiredChannels];

                for (size_t i = 0; i < pixelCount; ++i) {
                    for (int c = 0; c < desiredChannels; ++c) {
                        processedData[i * desiredChannels + c] = data[i * 4 + c];
                    }
                }

                stbi_image_free(const_cast<unsigned char *>(data));
                channels = desiredChannels;
                return processedData;
            }

            throw std::runtime_error("Invalid format in readImage.");
        }


        // Can also be used to read cube map faces
        inline const void *readVolume(const std::vector<std::string> &slices, int &width, int &height, int &channels,
                                       int &depth, const ImageFormat format = ImageFormat::R32G32B32A32_SFLOAT,
                                       uint32_t flags = 0) {
            if (slices.empty()) {
                Logger::log(LOG_LEVEL_ERROR, "Error: No slice file paths provided\n");
                throw std::runtime_error("Error: No slice file paths provided.");
            }

            if (format == ImageFormat::R32G32B32A32_SFLOAT) {
                // Read the first slice to determine width, height, and channels
                const float *firstSlice = static_cast<const float *>(readImage(
                    slices[0], width, height, channels, format, flags));
                depth = slices.size(); // The number of slices determines the depth

                // Allocate memory for the 3D texture
                size_t sliceSize = width * height * channels;
                float *volumeData = new float[sliceSize * depth];

                // Copy the first slice into the buffer
                std::copy(firstSlice, firstSlice + sliceSize, volumeData);
                delete[] firstSlice;// Free the first slice

                // Read and copy the remaining slices
                for (size_t i = 1; i < slices.size(); ++i) {
                    int currentWidth, currentHeight, currentChannels;
                    const float *sliceData = static_cast<const float *>(readImage(
                        slices[i], currentWidth, currentHeight, currentChannels, format, flags));

                    // Validate dimensions match
                    if (currentWidth != width || currentHeight != height || currentChannels != channels) {
                        delete[] volumeData;
                        delete[] sliceData;
                        Logger::log(LOG_LEVEL_ERROR, "Error: Slice dimensions or channels mismatch in slice %d\n", i);
                        throw std::runtime_error("Error: Slice dimensions or channels mismatch!");
                    }

                    // Copy the slice into the correct position in the 3D buffer
                    std::copy(sliceData, sliceData + sliceSize, volumeData + i * sliceSize);
                    delete[] sliceData; // Free the current slice
                }

                return volumeData;
            }

            if (format == ImageFormat::R16G16B16A16_SFLOAT) {
                // Read the first slice to determine width, height, and channels
                const float16_t *firstSlice = static_cast<const float16_t *>(readImage(
                    slices[0], width, height, channels, format, flags));
                depth = slices.size(); // The number of slices determines the depth

                // Allocate memory for the 3D texture
                size_t sliceSize = width * height * channels;
                float16_t *volumeData = new float16_t[sliceSize * depth];

                // Copy the first slice into the buffer
                std::copy(firstSlice, firstSlice + sliceSize, volumeData);
                delete[] firstSlice;

                // Read and copy the remaining slices
                for (size_t i = 1; i < slices.size(); ++i) {
                    int currentWidth, currentHeight, currentChannels;
                    const float16_t *sliceData = static_cast<const float16_t *>(readImage(
                        slices[i], currentWidth, currentHeight, currentChannels, format, flags));

                    // Validate dimensions match
                    if (currentWidth != width || currentHeight != height || currentChannels != channels) {
                        delete[] volumeData;
                        delete[] sliceData;
                        Logger::log(LOG_LEVEL_ERROR, "Error: Slice dimensions or channels mismatch in slice %d\n", i);
                        throw std::runtime_error("Error: Slice dimensions or channels mismatch!");
                    }

                    // Copy the slice into the correct position in the 3D buffer
                    std::copy(sliceData, sliceData + sliceSize, volumeData + i * sliceSize);
                    delete[] sliceData;
                }

                return volumeData;
            }
        }
    } // namespace Filesystem
}
