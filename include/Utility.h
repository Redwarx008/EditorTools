#pragma once
#include <math.h>
#include <string>
#include "spng/spng.h"

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
    case SPNG_COLOR_TYPE_GRAYSCALE_ALPHA:
        return 2;
    case SPNG_COLOR_TYPE_TRUECOLOR_ALPHA:
        return 4;
    default:
        return -1;
    }
}

spng_color_type GetColorType(int nChannel)
{
    switch (nChannel)
    {
    case 1:
        return SPNG_COLOR_TYPE_GRAYSCALE;
    case 2:
        return SPNG_COLOR_TYPE_GRAYSCALE_ALPHA;
    case 3:
        return SPNG_COLOR_TYPE_TRUECOLOR;
    case 4:
        return SPNG_COLOR_TYPE_TRUECOLOR_ALPHA;
    default: //unreachable
        break;
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
    *bitDepth = ihdr.bit_depth;
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

size_t DecodeImage(const char* fileName, void** out, int* width, int* height, int* nChannel, int* bitDepth)
{
    
    FILE* f = OpenFile(fileName, "rb+");
    if (f == nullptr)
    {
        std::cout << "can't open " + std::string(fileName) << std::endl;
        return -1;
    }
    spng_ctx* ctx = spng_ctx_new(0);

    spng_set_png_file(ctx, f);

    int ret = GetImageInfo(ctx, width, height, nChannel, bitDepth);
    if (ret != 0)
    {
        std::cout << std::string("GetImageInfo error: ") + spng_strerror(ret) << std::endl;
        fclose(f);
        return -1;
    }


    spng_format fmt = SPNG_FMT_PNG;

    size_t inSize;
    ret = spng_decoded_image_size(ctx, fmt, &inSize);
    if (ret != 0)
    {
        std::cout << std::string("spng_decoded_image_size error: ") + spng_strerror(ret) << std::endl;
        fclose(f);
        return -1;
    }

    *out = malloc(inSize);
    if (*out == nullptr)
    {
        std::cout << "Cannot allocate enough memory.\n";
        fclose(f);
        return -1;
    }

    ret = spng_decode_image(ctx, *out, inSize, fmt, 0);
    spng_ctx_free(ctx);
    fclose(f);
    return inSize;
}

size_t EncodeImage(void* in, int width, int height, int nChannel, int bitDepth, void** outBuffer)
{
    spng_ctx* ctx = spng_ctx_new(SPNG_CTX_ENCODER);

    spng_set_option(ctx, SPNG_ENCODE_TO_BUFFER, 1);

    spng_ihdr ihdr = {0};
    ihdr.width = width;
    ihdr.height = height;
    ihdr.bit_depth = bitDepth; //mark
    ihdr.color_type = GetColorType(nChannel);

    spng_set_ihdr(ctx, &ihdr);

    spng_format fmt = SPNG_FMT_PNG;

    size_t inSize = width * height * nChannel * bitDepth / 8;
    int ret = spng_encode_image(ctx, in, inSize, fmt, SPNG_ENCODE_FINALIZE);

    if (ret)
    {
        std::cout << "spng_encode_image() error: " << spng_strerror(ret) << std::endl;
        spng_ctx_free(ctx);
        return -1;
    }

    size_t outSize;
    *outBuffer = spng_get_png_buffer(ctx, &outSize, &ret);
    if (*outBuffer == nullptr)
    {
        std::cout << "spng_get_png_buffer() error: " << spng_strerror(ret) << std::endl;
        spng_ctx_free(ctx);
        return -1;
    }
    spng_ctx_free(ctx);
    return outSize;
}