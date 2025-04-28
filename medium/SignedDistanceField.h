#pragma once
#include <cstdint>
#include <fstream>
#include <string>
#include <vector>
#include <cassert>
#include <iostream>

class SignedDistanceField {
public:
    SignedDistanceField &loadFromFile(const std::string &filename, int32_t gridSize) {
        // Open the binary file
        std::ifstream file(filename, std::ios::binary);

        assert(file.is_open());

        // Calculate the total number of elements (grid_size^3 for a cubic grid)
        size_t total_elements = gridSize * gridSize * gridSize;

        // Allocate memory for the SDF grid
        field = new float[total_elements];

        // Read the SDF values into the array
        file.read(reinterpret_cast<char*>(field), total_elements * sizeof(float));

        assert(!file.fail());

        file.close();


        float min = field[0], max = field[0];
        for (int i = 0; i < total_elements; ++i) {
            if (field[i] < min) {
                min = field[i];
            }
            if (field[i] > max) {
                max = field[i];
            }
        }
        std::cout << "Min: " << min << ", Max: " << max  << "Total: " << total_elements << std::endl;

        return *this;
    }

    [[nodiscard]] float *data() const {
        return field;
    }


    void free() const {
        delete[] field;
    }

private:
    float *field;
};