#pragma once
#include <numeric>
#include <vector>

#define CIRCULAR_BUFFER_SIZE 1500

// Basic circular buffer that is used to collect CIRCULAR_BUFFER_SIZE frames
class CircularBuffer {
public:
    CircularBuffer(size_t maxFrames)
        : maxSize(maxFrames), buffer(maxFrames), head(0), full(false) {
    }

    void add(float timeMs) {
        buffer[head] = timeMs;
        head = (head + 1) % maxSize;
        if (head == 0) full = true;
    }

    [[nodiscard]] std::vector<float> getSnapshot() const {
        std::vector<float> snapshot;
        if (full) {
            snapshot.insert(snapshot.end(), buffer.begin() + head, buffer.end());
            snapshot.insert(snapshot.end(), buffer.begin(), buffer.begin() + head);
        } else {
            snapshot.insert(snapshot.end(), buffer.begin(), buffer.begin() + head);
        }
        return snapshot;
    }

    size_t size() const {
        return full ? maxSize : head;
    }

private:
    size_t maxSize;
    std::vector<float> buffer;
    size_t head;
    bool full;
};

// Stores pass length in ms for each frame (indexed by frame number)
struct BenchmarkResult {
    CircularBuffer depthPrePass{CIRCULAR_BUFFER_SIZE};
    CircularBuffer cloudComputePass{CIRCULAR_BUFFER_SIZE};
    CircularBuffer transmittanceLUT{CIRCULAR_BUFFER_SIZE};
    CircularBuffer multipleScatteringLUT{CIRCULAR_BUFFER_SIZE};
    CircularBuffer skyViewLUT{CIRCULAR_BUFFER_SIZE};
    CircularBuffer aerialPerspectiveLUT{CIRCULAR_BUFFER_SIZE};
    CircularBuffer godRaysMask{CIRCULAR_BUFFER_SIZE};
    CircularBuffer godRaysBlur{CIRCULAR_BUFFER_SIZE};
    CircularBuffer skyUpsample{CIRCULAR_BUFFER_SIZE};
    CircularBuffer composition{CIRCULAR_BUFFER_SIZE};
    CircularBuffer post{CIRCULAR_BUFFER_SIZE};
    CircularBuffer terrainDraw{CIRCULAR_BUFFER_SIZE};
    CircularBuffer total{CIRCULAR_BUFFER_SIZE};

    static float average(std::vector<float> values) {
        float sum = std::accumulate(values.begin(), values.end(), 0.0f);
        return sum / values.size();
    }
};
