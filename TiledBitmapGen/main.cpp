#include <iostream>
#include <string>

#include "Utility.h"
#include "TiledBitmap.hpp"

using std::cout;
using std::endl;
using std::string;

using TiledBitmapGen::TiledBitmapFormat;
using TiledBitmapGen::TiledBitmap;

const static int TILE_SIZE = 256;

inline float get_channel_value(const float* data, int x, int y, int width, int height, int nChannel, int channel)
{
    if (x >= width) x = width - 1;
    if (y >= height) y = height - 1;



    return data[(x + y * width) * nChannel + channel];
}

void WriteHeader(FILE* f, int width, int height, int tileSize, int bitDepth, int nChannel, int mipmapCount)
{
    if (f == nullptr) return;

    uint16_t sizeInfo[] = { (uint16_t)width, (uint16_t)height, (uint16_t)tileSize};
    uint8_t pixelInfo[] = { (uint8_t)bitDepth, (uint8_t)nChannel, (uint8_t)mipmapCount };
    fwrite(&sizeInfo[0], sizeof(uint16_t), sizeof(sizeInfo) / sizeof(uint16_t), f);
    fwrite(&pixelInfo[0], sizeof(uint8_t), sizeof(pixelInfo) / sizeof(uint8_t), f);

}

TiledBitmapFormat GetTiledBitmapFormat(int bitDepth, int nChannel)
{
    if (nChannel == 4)
    {
        return TiledBitmapFormat::RGBA8UInt;
    }

    if (nChannel == 1)
    {
        if (bitDepth == 8)
        {
            return TiledBitmapFormat::R8UInt;
        }
        else if (bitDepth == 16)
        {
            return TiledBitmapFormat::R16UInt;
        }
        else
        {
            return TiledBitmapFormat::R32SFloat;
        }
    }

    if (nChannel == 2)
    {
        return TiledBitmapFormat::RG8UInt;
    }

    return TiledBitmapFormat::Unkown;
}

int main(int argc, char* argv[])
{
    if (argc < 3 || argc > 4)
    {
        cout << "usage: [filename] [miplevel] Option:[format]\n";
    }

    string fileName = argv[1];
    int mipLevel = atoi(argv[2]);
    int outBitDepth;
    if (strcmp(argv[3], "UInt8") == 0)
    {
        outBitDepth = 8;
    }
    else if (strcmp(argv[3], "UInt16") == 0)
    {
        outBitDepth = 16;
    }
    else
    {
        outBitDepth = 32; // float
    }

    int width, height, nChannel, bitDepth;

    void* originPixels;
    size_t pixelsSize = DecodeImage(fileName.c_str(), &originPixels, &width, &height, &nChannel, &bitDepth);
    if (pixelsSize == -1)
    {
        return 1;
    }

    // convert to float
    float* formattedData = new float[width * height * nChannel];
    if (!formattedData)
    {
        std::cout << "There is not enough available memory\n";
        return 12;
    }
    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            for (int c = 0; c < nChannel; ++c)
            {
                uint32_t index = (x + (y * width)) * nChannel + c;
                uint16_t value;
                if (bitDepth == 16)
                {
                    uint16_t* data = static_cast<uint16_t*>(originPixels);
                    value = data[index];
                }
                else
                {
                    uint8_t* data = static_cast<uint8_t*>(originPixels);
                    value = data[index];
                }

                formattedData[index] = (float)value;
            }
        }
    }
    free(originPixels);

    if (mipLevel < 1 || mipLevel > std::max(1, (int)std::log2(std::min(width, height))))
    {
        std::cout << "max mip level is out of range.\n";
        return 2;
    }

    string outFileName = GetFileNameWithoutSuffix(fileName) + ".tbmp";

    FILE* f = OpenFile(outFileName, "wb");
    WriteHeader(f, width, height, TILE_SIZE, bitDepth, nChannel, mipLevel);

    float* inData = formattedData;
    for (int mip = 1; mip <= mipLevel; ++mip)
    {
        int h = floor((float)height / 2);
        int w = floor((float)width / 2);

        float* outData = new float[w * h * nChannel];
        if (outData == nullptr)
        {
            std::cout << "There is not enough available memory to create mip" << mip << '\n';
            return 12;
        }

        for (int y = 0; y < h; ++y)
        {
            for (int x = 0; x < w; ++x)
            {
                float wx0, wx1, wx2;
                float wy0, wy1, wy2;

                if (height & 1)
                {
                    wy0 = (float)(h - y) / (2 * h + 1);
                    wy1 = (float)h / (2 * h + 1);
                    wy2 = (float)(y + 1) / (2 * h + 1);
                }
                else
                {
                    wy0 = wy1 = 0.5;
                    wy2 = 0;
                }
                if (width & 1)
                {
                    wx0 = (float)(w - x) / (2 * w + 1);
                    wx1 = (float)w / (2 * w + 1);
                    wx2 = (float)(x + 1) / (2 * w + 1);
                }
                else
                {
                    wx0 = wx1 = 0.5;
                    wx2 = 0;
                }

                for (int c = 0; c < nChannel; ++c)
                {
                    float y0 = wx0 * get_channel_value(inData, 2 * x, 2 * y, width, height, nChannel, c) +
                        wx1 * get_channel_value(inData, 2 * x + 1, 2 * y, width, height, nChannel, c) +
                        wx2 * get_channel_value(inData, 2 * x + 2, 2 * y, width, height, nChannel, c);
                    float y1 = wx0 * get_channel_value(inData, 2 * x, 2 * y + 1, width, height, nChannel, c) +
                        wx1 * get_channel_value(inData, 2 * x + 1, 2 * y + 1, width, height, nChannel, c) +
                        wx2 * get_channel_value(inData, 2 * x + 2, 2 * y + 1, width, height, nChannel, c);
                    float y2 = wx0 * get_channel_value(inData, 2 * x, 2 * y + 2, width, height, nChannel, c) +
                        wx1 * get_channel_value(inData, 2 * x + 1, 2 * y + 2, width, height, nChannel, c) +
                        wx2 * get_channel_value(inData, 2 * x + 2, 2 * y + 2, width, height, nChannel, c);

                    float mixValue = wy0 * y0 + wy1 * y1 + wy2 * y2;
                    outData[(x + y * w) * nChannel + c] = mixValue;
                }
            }
        }

        TiledBitmap* outTiledBitmap = TiledBitmap::Create(outData, GetTiledBitmapFormat(outBitDepth, nChannel), w, h, TILE_SIZE);
        outTiledBitmap->SaveData(f);

        delete[] inData;
        inData = outData;
        width = w;
        height = h;
    }
    delete[] inData;
    fclose(f);
    return 0;
}
