#pragma once
#include <chrono>
#include <limits>
#include <sstream>


namespace hammock {
    class BenchmarkRunner {
    public:
        struct BenchmarkTimePoint {
            float frameTime;
        };

        struct BenchmarkResult {
            struct FPS {
                float average = 0.f;
                float min;
                float max;
            } fps;

            struct FrameTime {
                float average = 0.f;
                float min = std::numeric_limits<float>::max();
                float max = std::numeric_limits<float>::min();
            } frameTime;
        };

    protected:
        inline void startBenchmark(float length) {
            benchmarkLength = length;
            benchmarkStart = std::chrono::high_resolution_clock::now();
        }

        inline void endBenchmark() {
            if (!benchmarkEnded)
                benchmarkEnd = std::chrono::high_resolution_clock::now();

            benchmarkEnded = true;
        }

        [[nodiscard]] bool benchmarkInProgress() const {
            return std::chrono::duration_cast<std::chrono::seconds>(
                       std::chrono::high_resolution_clock::now() - benchmarkStart).count() < benchmarkLength;
        }

        inline void addBenchmarkTimePoint(BenchmarkTimePoint timePoint) {
            benchmarkTimePoints.push_back(timePoint);
        }

        inline BenchmarkResult processBenchmark() {
            BenchmarkResult result{};
            float frameTimeTotal = 0.f;

            for (auto &point: benchmarkTimePoints) {
                // frame time
                frameTimeTotal += point.frameTime;

                if (point.frameTime < result.frameTime.min)
                    result.frameTime.min = point.frameTime;

                if (point.frameTime > result.frameTime.max)
                    result.frameTime.max = point.frameTime;
            }

            //average frametime, fps
            result.frameTime.average = frameTimeTotal / static_cast<float>(benchmarkTimePoints.size());
            result.fps.average = 1.f / result.frameTime.average;
            result.fps.min = 1.f / result.frameTime.min;
            result.fps.max = 1.f / result.frameTime.max;

            return result;
        }

        inline std::string benchmarkResultToCSV(BenchmarkResult result) {
            std::stringstream csvStream;

            // Add CSV headers
            csvStream << "Frame Time (ms),FPS\n";

            // Add data for each time point
            for (const auto &timePoint: benchmarkTimePoints) {
                float fps = 1.f / timePoint.frameTime; // Convert frametime to FPS
                csvStream << timePoint.frameTime << "," << fps << "\n";
            }

            return csvStream.str();
        }


        float benchmarkLength = 0;
        bool benchmarkEnded = false;
        std::chrono::time_point<std::chrono::high_resolution_clock> benchmarkStart;
        std::chrono::time_point<std::chrono::high_resolution_clock> benchmarkEnd;
        std::vector<BenchmarkTimePoint> benchmarkTimePoints{};
    };
}
