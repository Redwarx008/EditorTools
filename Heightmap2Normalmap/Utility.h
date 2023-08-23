#pragma once
#include <math.h>
#include <string>
#include <spng.h>

struct Vector3
{
    float x, y, z;
    Vector3(float x, float y, float z)
    {
        this->x = x;
        this->y = y;
        this->z = z;
    }
    Vector3 Normalized()
    {
        Vector3 v = *this;
        double squared = v.x * v.x + v.y * v.y + v.z * v.z;
        if (squared == 0)
        {
            v.x = v.y = v.z = 0;
        }
        else
        {
            double length = sqrt(squared);
            v.x /= length;
            v.y /= length;
            v.z /= length;
        }
        return v;
    }
};

int GetChannelCount(spng_color_type color_type)
{
    switch (color_type)
    {
    case SPNG_COLOR_TYPE_GRAYSCALE:
        return 1;
    case SPNG_COLOR_TYPE_TRUECOLOR:
        return 3;
    case SPNG_COLOR_TYPE_INDEXED:
        return 3;
    case SPNG_COLOR_TYPE_GRAYSCALE_ALPHA:
        return 2;
    case SPNG_COLOR_TYPE_TRUECOLOR_ALPHA:
        return 4;
    default:
        return -1;
    }
}

__forceinline std::string GetFileNameWithoutSuffix(const std::string& fileName)
{
    return fileName.substr(0, fileName.find_last_of('.'));
}

int GetImageInfo(spng_ctx* ctx, int* width, int* height, int* nChannel, int* bitDepth)
{
    spng_ihdr ihdr;
    int ret = spng_get_ihdr(ctx, &ihdr);
    *height = ihdr.height;
    *width = ihdr.width;
    *nChannel = GetChannelCount((spng_color_type)ihdr.color_type);
    *bitDepth = ihdr.bit_depth / *nChannel;
    return ret;
}

FILE* OpenFile(const std::string& fileName, const char* mode)
{
    FILE* f;
#if defined(_MSC_VER) && _MSC_VER >= 1400
    if (0 != fopen_s(&f, fileName.c_str(), mode))
    {
        return nullptr;
    }
#else 
    f = fopen(fileName.c_str(), mode);
    if (f == nullptr)
    {
        return nullptr;
    }
#endif // (_MSC_VER) && _MSC_VER >= 1400
    return f;
}
