// Heightmap2Normalmap.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#define _CRT_SECURE_NO_WARNINGS
#include <iostream>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#include "Utility.h"
using std::string;

int main(int argc, char* argv[])
{
    if (argc < 3)
    {
        std::cout << "usage: [heightmap.png] [heightMultiplier]\n";
        return 22;
    }
    
    string fileName = argv[1];
    int heightMultiplier = atoi(argv[2]);
    if (heightMultiplier <= 0)
    {
        std::cout << "heightMultiplier can't less than 0.\n";
        return 2;
    }

    int width, height, nChannel;
    int ok = stbi_info(fileName.c_str(), &width, &height, &nChannel);
    if (ok == 0)
    {
        std::cout << stbi_failure_reason() << std::endl;
        return 3;
    }

    bool is16Bit = stbi_is_16_bit(fileName.c_str()) == 1 ? true : false;
    int bitDepth = is16Bit ? 16 : 8;

    void* inBuffer;
    if (is16Bit)
    {
        inBuffer = stbi_load_16(fileName.c_str(), &width, &height, &nChannel, 0);
    }
    else
    {
        inBuffer = stbi_load(fileName.c_str(), &width, &height, &nChannel, 0);
    }

    // convert to float
    float* heights = new float[width * height];
    if (heights == nullptr)
    {
        std::cout << "There is not enough available memory.\n";
        return 12;
    }
    int size = height * width;
    if (is16Bit)
    {
        for (int i = 0; i < size; ++i)
        {
            heights[i] = static_cast<uint16_t*>(inBuffer)[i] / 65535.0 * heightMultiplier;
        }
    }
    else
    {
        for (int i = 0; i < size; ++i)
        {
            heights[i] = static_cast<uint8_t*>(inBuffer)[i] / 255.0 * heightMultiplier;
        }
    }

    uint8_t* normalmap = new uint8_t[size * 2]; //r8g8 texture
    if (normalmap == nullptr)
    {
        std::cout << "There is not enough available memory to export normalmap.\n";
        return 12;
    }
    for (int index = 0; index < size; ++index)
    {
        int upIndex = index - width < 0 ? index : index - width;
        int downIndex = index + width > size - 1 ? index : index + width;
        int leftIndex = index - 1 < 0 ? index : index - 1;
        int rightIndex = index + 1 > size - 1 ? index : index + 1;
        float up = heights[upIndex];
        float down = heights[downIndex];
        float left = heights[leftIndex];
        float right = heights[rightIndex];

        Vector3 normal = { left - right, up - down, 2.0f };
        normal = normal.Normalized();

        uint8_t x = ((normal.x + 1) / 2) * 255;
        uint8_t y = ((normal.y + 1) / 2) * 255;

        normalmap[index * 2] = x;
        normalmap[index * 2 + 1] = y;
    }
    delete[] heights;
    stbi_write_png((GetFileNameWithoutSuffix(fileName) + "_normal.png").c_str(), width, height, 2, normalmap, 2 * width);
    delete[] normalmap;
    return 0;
}

