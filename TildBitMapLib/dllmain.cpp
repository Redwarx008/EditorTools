// dllmain.cpp : Defines the entry point for the DLL application.
#include "Utility.h"
#include "TiledBitmapLoader.h"
#include "TiledBitmap.hpp"
#include <iostream>
#include <string>

using std::string;
using std::wstring;
using std::filesystem::path;
//BOOL APIENTRY DllMain( HMODULE hModule,
//                       DWORD  ul_reason_for_call,
//                       LPVOID lpReserved
//                     )
//{
//    switch (ul_reason_for_call)
//    {
//    case DLL_PROCESS_ATTACH:
//    case DLL_THREAD_ATTACH:
//    case DLL_THREAD_DETACH:
//    case DLL_PROCESS_DETACH:
//        break;
//    }
//    return TRUE;
//}

float get_channel_value(const float* data, int x, int y, int width, int height, int nChannel, int channel)
{
    if (x >= width) x = width - 1;
    if (y >= height) y = height - 1;



    return data[(x + y * width) * nChannel + channel];
}

void WriteHeader(FILE* f, int width, int height, int tileSize, int bitDepth, int nChannel, int mipmapCount)
{
    if (f == nullptr) return;

    uint16_t sizeInfo[] = { (uint16_t)width, (uint16_t)height, (uint16_t)tileSize };
    uint8_t pixelInfo[] = { (uint8_t)bitDepth, (uint8_t)nChannel, (uint8_t)mipmapCount };
    fwrite(&sizeInfo[0], sizeof(uint16_t), sizeof(sizeInfo) / sizeof(uint16_t), f);
    fwrite(&pixelInfo[0], sizeof(uint8_t), sizeof(pixelInfo) / sizeof(uint8_t), f);

}

bool GenerateMinMaxMaps(const char* fileName)
{
    TiledBitmapLoader tiledBitmap;
    if (!tiledBitmap.Open(fileName))
    {
        cout << "cant't open file in GenerateMinMaxMaps\n";
        return false;
    }

    if (tiledBitmap.GetBitDepth() != 32 || tiledBitmap.GetChannelNum() != 1)
    {
        cout << "unsupport format in GenerateMinMaxMaps\n";
        return false;
    }

    int width = tiledBitmap.GetWidth();
    int height = tiledBitmap.GetHeight();
    int tileSize = tiledBitmap.GetTileSize();
    int nMipmap = tiledBitmap.GetMipmapNum();

    int nTileX = ceil((float)width / tileSize);
    int nTileY = ceil((float)height / tileSize);

    std::vector<float> out;

    for (int n = 0; n < nMipmap; ++n)
    {
        for (int tileY = 0; tileY < nTileY; ++tileY)
        {
            for (int tileX = 0; tileX < nTileX; ++tileX)
            {
                TiledBitmapLoader::Tile tile = tiledBitmap.LoadTile(n, tileX, tileY);
                float* data = reinterpret_cast<float*> (&tile.data[0]);
                int w = tile.width;
                int h = tile.height;
                float min = 99999999.0;
                float max = -99999999.0;
                for (int y = 0; y < h; ++y)
                {
                    for (int x = 0; x < w; ++x)
                    {
                        min = std::min(min, data[x + y * w]);
                        max = std::max(max, data[x + y * w]);
                    }
                }
                out.push_back(min);
                out.push_back(max);
            }
        }
    }
#if defined(_WIN32)
    FILE* f;
    fopen_s(&f, "minmax.bin", "wb");
#else
    FILE* f = fopen("minmax.bin", "wb");
#endif // WIN32
    fwrite(&out[0], sizeof(float), out.size(), f);
    fclose(f);
    return true;
}

struct Config
{
    const wchar_t* FileName;
    int TileSize;
    bool IsHeightmap;
    float MinHeight;
    float MaxHeight;
};

extern "C" __declspec(dllexport) bool GetImgInfo(const wchar_t* fileName, int& nChannel, int& bitDepth)
{
    FILE* f = OpenFile(path(fileName).string(), "rb+");
    if (f == nullptr)
    {
        std::cout << "can't open " + path(fileName).string() << std::endl;
        return false;
    }
    spng_ctx* ctx = spng_ctx_new(0);

    spng_set_png_file(ctx, f);

    int width, height;
    int ret = GetImageInfo(ctx, &width, &height, &nChannel, &bitDepth);
    if (ret != 0)
    {
        std::cout << std::string("GetImageInfo error: ") + spng_strerror(ret) << std::endl;
        fclose(f);
        spng_ctx_free(ctx);
        return false;
    }

    fclose(f);
    spng_ctx_free(ctx);
    return true;
}

extern "C" __declspec(dllexport) bool Create(Config config)
{
    std::string fileName = path(config.FileName).string();
    int tileSize = config.TileSize;

    int width, height, nChannel, bitDepth;

    void* originPixels;
    size_t pixelsSize = DecodeImage(fileName, &originPixels, &width, &height, &nChannel, &bitDepth);
    if (pixelsSize == -1)
    {
        return false;
    }

    int outBitDepth = bitDepth;
    if (config.IsHeightmap)
    {
        outBitDepth = 32;
    }


    // convert to float array
    float* formattedData = new float[width * height * nChannel];
    if (!formattedData)
    {
        std::cout << "There is not enough available memory\n";
        return false;
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

    int mipLevel = 0;
    {
        int minLength = std::min(width, height);
        while (minLength > tileSize)
        {
            minLength = minLength >> 1;
            ++mipLevel;
        }
    }

    string outFileName = GetFileNameWithoutSuffix(fileName) + ".tbmp";

    FILE* f = OpenFile(outFileName, "wb");
    WriteHeader(f, width, height, tileSize, outBitDepth, nChannel, mipLevel);

    float* inData = formattedData;

    TiledBitmap* outTiledBitmap = TiledBitmap::Create(inData, nChannel, outBitDepth, width, height, tileSize, config.MinHeight, config.MaxHeight);
    outTiledBitmap->SaveData(f);
    delete outTiledBitmap;

    for (int mip = 1; mip <= mipLevel; ++mip)
    {
        int h = floor((float)height / 2);
        int w = floor((float)width / 2);

        float* outData = new float[w * h * nChannel];
        if (outData == nullptr)
        {
            std::cout << "There is not enough available memory to create mip" << mip << '\n';
            return false;
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

        outTiledBitmap = TiledBitmap::Create(outData, nChannel, outBitDepth, w, h, tileSize, config.MinHeight, config.MaxHeight);
        outTiledBitmap->SaveData(f);
        delete outTiledBitmap;

        delete[] inData;
        inData = outData;
        width = w;
        height = h;
    }
    delete[] inData;
    fclose(f);

    // generate minmax maps

    if (config.IsHeightmap)
    {
        if (!GenerateMinMaxMaps(outFileName.c_str()))
        {
            return false;
        }
    }
    return true;
}

