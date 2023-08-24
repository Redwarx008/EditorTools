// Heightmap2Normalmap.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <spng/spng.h>
#include <Utility.h>

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


    int width, height, nChannel, bitDepth;

    void* heightmapData;

    size_t pixelsSize = DecodeImage(fileName.c_str(), &heightmapData, &width, &height, &nChannel, &bitDepth);
    if (pixelsSize == -1)
    {
        return 1;
    }

    bool is16Bit = bitDepth == 16 ? true : false;


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
            heights[i] = static_cast<uint16_t*>(heightmapData)[i] / 65535.0 * heightMultiplier;
        }
    }
    else
    {
        for (int i = 0; i < size; ++i)
        {
            heights[i] = static_cast<uint8_t*>(heightmapData)[i] / 255.0 * heightMultiplier;
        }
    }
    
    free(heightmapData);

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

    void* outBuffer;
    size_t outSize = EncodeImage(normalmap, width, height, 2, 8, &outBuffer);
    delete[] normalmap;

    FILE* f = OpenFile(GetFileNameWithoutSuffix(fileName) + "_normal.png", "wb");
    fwrite(outBuffer, 1, outSize, f);
    fclose(f);
    free(outBuffer);
    //stbi_write_png("sts.png", width, height, nChannel, heightmapData, width * nChannel);
    return 0;
}

