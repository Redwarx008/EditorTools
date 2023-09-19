// dllmain.cpp : Defines the entry point for the DLL application.
#include "Utility.h"
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

struct Config
{
    const char* fileName;
    int tileSize;
    int tileBorderSize;
    bool isHeightmap;
    bool createNormalmap;
    float minHeight;
    float maxHeight;
    int leafQuadTreeNodeSize;
    int lodLevelCount;
};

struct ProcessInfo
{
    string outFileName;
    int width;
    int height;
    int nChannel;
    int outBitDepth;
    int tileSize;
    int borderSize;
};


bool TiledBitmapProcess(float* data, const ProcessInfo& info);
bool NormalmapProcess(float* heights, float** normalmap, int width, int height);
bool GenerateMinMaxMaps(const char* outFileName, const float* heightmap, int width, int height, int leafQuadTreeNodeSize, int nLodLevel);

float get_channel_value(const float* data, int x, int y, int width, int height, int nChannel, int channel)
{
    if (x >= width) x = width - 1;
    if (y >= height) y = height - 1;



    return data[(x + y * width) * nChannel + channel];
}

void WriteHeader(FILE* f, int width, int height, int tileSize, int borderSize, int bitDepth, int nChannel, int mipmapCount)
{
    if (f == nullptr) return;

    uint16_t sizeInfo[] = { (uint16_t)width, (uint16_t)height, (uint16_t)tileSize };
    uint8_t pixelInfo[] = { (uint8_t)bitDepth, (uint8_t)nChannel, (uint8_t)mipmapCount, (uint8_t)borderSize};
    fwrite(&sizeInfo[0], sizeof(uint16_t), sizeof(sizeInfo) / sizeof(uint16_t), f);
    fwrite(&pixelInfo[0], sizeof(uint8_t), sizeof(pixelInfo) / sizeof(uint8_t), f);

}


extern "C" __declspec(dllexport) bool GetImgInfo(const char* fileName, int& width, int& height, int& nChannel, int& bitDepth)
{
    FILE* f = OpenFile(path(fileName).string(), "rb+");
    if (f == nullptr)
    {
        std::cout << "can't open " + path(fileName).string() << std::endl;
        return false;
    }
    spng_ctx* ctx = spng_ctx_new(0);

    spng_set_png_file(ctx, f);

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
    std::string fileName = path(config.fileName).string();
    int tileSize = config.tileSize;
    int borderSize = config.tileBorderSize;

    int width, height, nChannel, bitDepth;

    void* originPixels;
    size_t pixelsSize = DecodeImage(fileName, &originPixels, &width, &height, &nChannel, &bitDepth);
    if (pixelsSize == -1)
    {
        return false;
    }

    int outBitDepth = bitDepth;
    if (config.isHeightmap)
    {
        outBitDepth = 32;
    }


    // convert to float array
    float* heights = new float[width * height * nChannel];
    if (!heights)
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

                heights[index] = config.isHeightmap ? config.minHeight + (float)value / 65535 * (config.maxHeight - config.minHeight) : (float)value;
            }
        }
    }
    free(originPixels);

    string outFileName = GetFileNameWithoutSuffix(fileName) + ".tbmp";

    ProcessInfo info;
    info.width = width;
    info.height = height;
    info.nChannel = nChannel;
    info.outBitDepth = outBitDepth;
    info.tileSize = tileSize;
    info.borderSize = borderSize;
    info.outFileName = outFileName;

    bool ret = TiledBitmapProcess(heights, info);
    if (!ret)
    {
        return false;
    }
    // generate minmax maps

    if (config.isHeightmap)
    {
        if (config.createNormalmap)
        {
            float* normalmap;
            NormalmapProcess(heights, &normalmap, width, height);
            ProcessInfo info;
            info.width = width;
            info.height = height;
            info.nChannel = 2;
            info.outBitDepth = 8;
            info.tileSize = tileSize;
            info.outFileName = GetFileNameWithoutSuffix(fileName) + "_N.tbmp";

            bool ret = TiledBitmapProcess(normalmap, info);
            delete[] normalmap;
            if (!ret)
            {
                return false;
            }
        }
        std::string outFileName = std::filesystem::path(fileName).parent_path().string() + "/minmax.bin";
        if (!GenerateMinMaxMaps(outFileName.c_str(), heights, width, height, config.leafQuadTreeNodeSize, config.lodLevelCount))
        {
            return false;
        }
    }
    delete[] heights;
    return true;
}

bool TiledBitmapProcess(float* data, const ProcessInfo& info)
{
    int width = info.width;
    int height = info.height;
    int tileSize = info.tileSize;
    int borderSize = info.borderSize;
    int outBitDepth = info.outBitDepth;
    int nChannel = info.nChannel;

    int mipLevel = 0;
    {
        int minLength = std::min(width, height);
        while (minLength > tileSize)
        {
            minLength = minLength >> 1;
            ++mipLevel;
        }
    }

    string outFileName = info.outFileName;

    FILE* f = OpenFile(outFileName, "wb");
    WriteHeader(f, width, height, tileSize, outBitDepth, nChannel, mipLevel, borderSize);

    float* inData = data;

    TiledBitmap* outTiledBitmap = TiledBitmap::Create(inData, nChannel, outBitDepth, width, height, tileSize, borderSize);
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

        outTiledBitmap = TiledBitmap::Create(outData, nChannel, outBitDepth, w, h, tileSize, borderSize);
        outTiledBitmap->SaveData(f);
        delete outTiledBitmap;
        // Input data should be released externally
        if (mip != 1)
        {
            delete[] inData;
        }
        inData = outData;
        width = w;
        height = h;
    }
    delete[] inData;
    fclose(f);
    return true;
}

bool NormalmapProcess(float* heights, float** normalmap, int width, int height)
{
    size_t size = static_cast<size_t>(width) * height;
    *normalmap = new float[size * 2]; //r8g8 texture
    if (normalmap == nullptr)
    {
        std::cout << "There is not enough available memory to export normalmap.\n";
        return false;
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

        float x = ((normal.x + 1) / 2) * 255;
        float y = ((normal.y + 1) / 2) * 255;

        (*normalmap)[index * 2] = x;
        (*normalmap)[index * 2 + 1] = y;
    }
    return true;
}

bool GenerateMinMaxMaps(const char* outFileName, const float* heightmap, int width, int height, int leafQuadTreeNodeSize, int nLodLevel)
{
    using MinMaxMaps = std::vector<std::vector<float>>;
    MinMaxMaps minMaxMaps;
    minMaxMaps.resize(nLodLevel);

    int topNodeSize = leafQuadTreeNodeSize * pow(2, nLodLevel - 1);
    int nodeCount = 0;
    for (int level = 0; level < nLodLevel; ++level)
    {
        int size = leafQuadTreeNodeSize << level;
        int nBlockX = ceil((float)width / size);
        int nBlockY = ceil((float)height / size);
        minMaxMaps[level].resize(nBlockX * nBlockY * 2);
        nodeCount += minMaxMaps[level].size();
    }


    struct Desc
    {
        int width;
        int height;
        const float* heightmap;
    };
    struct Node
    {
        Node()
        {
            Min = FLT_MIN;
            Max = FLT_MAX;
            Level = UINT16_MAX;
            X = UINT16_MAX;
            Y = UINT16_MAX;
            Size = UINT16_MAX;
        }
        Node(int x, int y, int size, int level, const Desc& desc, std::vector<Node>& nodes)
        {
            X = (uint16_t)x;
            Y = (uint16_t)y;
            Size = (uint16_t)size;
            Level = (uint16_t)level;
            if (level == 0)
            {
                int sizeX = std::min(desc.width, x + size + 1) - x;
                int sizeY = std::min(desc.height, y + size + 1) - y;

                float min = 999.0;
                float max = -999.0;
                for (int blockY = 0; blockY < sizeY; ++blockY)
                {
                    for (int blockX = 0; blockX < sizeX; ++blockX)
                    {
                        float h = desc.heightmap[blockX + x + (blockY + y) * sizeX];
                        min = std::min(min, h);
                        max = std::max(max, h);
                    }
                }
                this->Min = min;
                this->Max = max;
            }
            else
            {
                int subSize = size / 2;
                // top left
                auto node = Node(x, y, subSize, level - 1, desc, nodes);
                this->Min = node.Min;
                this->Max = node.Max;
                nodes.push_back(node);
                // top right
                if ((x + subSize) < desc.width)
                {
                    auto node = Node(x + subSize, y, subSize, level - 1, desc, nodes);
                    this->Min = std::min(this->Min, node.Min);
                    this->Max = std::max(this->Max, node.Max);
                    nodes.push_back(node);
                }

                // bottom left
                if ((y + subSize) < desc.height)
                {
                    auto node = Node(x, y + subSize, subSize, level - 1, desc, nodes);
                    this->Min = std::min(this->Min, node.Min);
                    this->Max = std::max(this->Max, node.Max);
                    nodes.push_back(node);
                }

                //bottom right
                if (((x + subSize) < desc.width) && ((y + subSize) < desc.height))
                {
                    auto node = Node(x + subSize, y + subSize, subSize, level - 1, desc, nodes);
                    this->Min = std::min(this->Min, node.Min);
                    this->Max = std::max(this->Max, node.Max);
                    nodes.push_back(node);
                }
            }

        }
        float Min;
        float Max;
        uint16_t Level;
        uint16_t X;
        uint16_t Y;
        uint16_t Size;
    };

    std::vector<Node> nodes;
    nodes.resize(nodeCount);
    int nTopNodeX = ceil((float)width / topNodeSize);
    int nTopNodeY = ceil((float)height / topNodeSize);
    Desc desc{};
    desc.heightmap = heightmap;
    desc.width = width;
    desc.height = height;
    for (int y = 0; y < nTopNodeY; ++y)
    {
        for (int x = 0; x < nTopNodeX; ++x)
        {
            auto node = Node(x * topNodeSize, y * topNodeSize, topNodeSize, nLodLevel - 1, desc, nodes);
            nodes.push_back(node);
        }
    }

    for (auto& node : nodes)
    {
        int index = node.X / node.Size + (node.Y / node.Size) * ceil((float)width / node.Size);
        minMaxMaps[node.Level][index * 2 + 0] = node.Min;
        minMaxMaps[node.Level][index * 2 + 1] = node.Max;
    }

    FILE* f = OpenFile(outFileName, "wb");
    for (int i = 0; i < minMaxMaps.size(); ++i)
    {
        fwrite(&minMaxMaps[i][0], sizeof(float), minMaxMaps[i].size(), f);
    }
    fclose(f);
    return true;
}
