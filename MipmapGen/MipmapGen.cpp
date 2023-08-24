// MipmapGen.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <string>
#include <spng/spng.h>
#include <Utility.h>

using std::string;

inline uint16_t get_channel_value(void* pixels, int bitDepth,
    int x, int y, int width, int height, int nChannel, int channel)
{
    if (x >= width) x = width - 1;
    if (y >= height) y = height - 1;

    uint16_t value;
    if (bitDepth == 16)
    {
        //uint8_t high = pixels[(x + (size_t)y * width) * 2];
        //uint8_t low = pixels[(x + (size_t)y * width) * 2 + 1];
        //value = (high << 8) | low;

        uint16_t* data = static_cast<uint16_t*>(pixels);
        value = data[(x + (y * width)) * nChannel + channel];
    }
    else
    {
        uint8_t* data = static_cast<uint8_t*>(pixels);
        value = data[(x + (y * width)) * nChannel + channel];
    }

    return value;
}

inline void set_channel_value(void* pixels, uint16_t value, int bitDepth,
    int x, int y, int width, int nChannel, int channel)
{
    if (bitDepth == 16)
    {
        //uint8_t high = (value >> 8) & 0xff;
        //uint8_t low = value & 0xff;
        //pixels->at((x + (size_t)y * width) * 2) = high;
        //pixels->at((x + (size_t)y * width) * 2 + 1) = low;

        uint16_t* data = static_cast<uint16_t*>(pixels);
        data[(x + (y * width)) * nChannel + channel] = value;
    }
    else
    {
        uint8_t* data = static_cast<uint8_t*>(pixels);
        data[(x + (y * width)) * nChannel + channel] = value;
    }
}

void WriteHeader(FILE* f, int width, int height, int bitDepth, int nChannel, int mipmapCount)
{
    if (f == nullptr) return;

    uint16_t sizeInfo[] = { (uint16_t)width, (uint16_t)height};
    uint8_t pixelInfo[] = { (uint8_t)bitDepth, (uint8_t)nChannel, (uint8_t)mipmapCount};
    fwrite(&sizeInfo[0], sizeof(uint16_t), sizeof(sizeInfo) / sizeof(uint16_t), f);
    fwrite(&pixelInfo[0], sizeof(uint8_t), sizeof(pixelInfo) / sizeof(uint8_t), f);

}

void WriteContent(FILE* f, void* pixels, size_t size)
{
    if (f == nullptr) return;

    fwrite(pixels, 1, size, f);
}

int main(int argc, char* argv[])
{
    if (argc != 3)
    {
        std::cout << "usage: [file] [max mip level].\n";
        return 22;
    }

    string fileName = argv[1];
    int maxMipLevel = atoi(argv[2]);

    int width, height, nChannel, bitDepth;

    void* originPixels;
    size_t pixelsSize = DecodeImage(fileName.c_str(), &originPixels, &width, &height, &nChannel, &bitDepth);
    if (pixelsSize == -1)
    {
        return 1;
    }

    bool is16Bit = bitDepth == 16 ? true : false;

    if (maxMipLevel < 1 || maxMipLevel > std::max(1, (int)std::log2(std::min(width, height))))
    {
        std::cout << "max mip level is out of range.\n";
        return 2;
    }

    string outFileName = GetFileNameWithoutSuffix(fileName) + ".tbin";

    FILE* f = OpenFile(outFileName, "wb");
    WriteHeader(f, width, height, bitDepth, nChannel, maxMipLevel);

    void* inPixels = originPixels;
    for (int mip = 1; mip <= maxMipLevel; ++mip)
    {
        int h = floor((float)height / 2);
        int w = floor((float)width / 2);

        size_t outSize = static_cast<size_t>(h * w * bitDepth / 8) * nChannel;
        void* outPixels = malloc(outSize);
        if (outPixels == nullptr)
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
                    float y0 = wx0 * get_channel_value(inPixels, bitDepth, 2 * x, 2 * y, width, height, nChannel, c) +
                        wx1 * get_channel_value(inPixels, bitDepth, 2 * x + 1, 2 * y, width, height, nChannel, c) +
                        wx2 * get_channel_value(inPixels, bitDepth, 2 * x + 2, 2 * y, width, height, nChannel, c);
                    float y1 = wx0 * get_channel_value(inPixels, bitDepth, 2 * x, 2 * y + 1, width, height, nChannel, c) +
                        wx1 * get_channel_value(inPixels, bitDepth, 2 * x + 1, 2 * y + 1, width, height, nChannel, c) +
                        wx2 * get_channel_value(inPixels, bitDepth, 2 * x + 2, 2 * y + 1, width, height, nChannel, c);
                    float y2 = wx0 * get_channel_value(inPixels, bitDepth, 2 * x, 2 * y + 2, width, height, nChannel, c) +
                        wx1 * get_channel_value(inPixels, bitDepth, 2 * x + 1, 2 * y + 2, width, height, nChannel, c) +
                        wx2 * get_channel_value(inPixels, bitDepth, 2 * x + 2, 2 * y + 2, width, height, nChannel, c);

                    float mixValue = wy0 * y0 + wy1 * y1 + wy2 * y2;
                    set_channel_value(outPixels, mixValue, bitDepth, x, y, w, nChannel, c);
                }
            }
        }

        WriteContent(f, outPixels, outSize);
        //void* outBuffer;
        //size_t size = EncodeImage(outPixels, w, h, nChannel, bitDepth, &outBuffer);
        //FILE* wf = OpenFile("d" + std::to_string(mip) + ".png", "wb");
        //fwrite(outBuffer, 1, size, wf);
        //free(outBuffer);
        //fclose(wf);
        free(inPixels);
        inPixels = outPixels;
        width = w;
        height = h;
    }
    free(inPixels);
    fclose(f);
    return 0;
}
