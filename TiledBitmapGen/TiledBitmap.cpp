#include "TiledBitmap.hpp"
#include <iostream>

namespace TiledBitmapGen
{
	void GetPixelInfo(TiledBitmapFormat format, int* nchannel, int* depth)
	{
		switch (format)
		{
		case RG8UInt:
			*nchannel = 2;
			*depth = 1;
			break;
		case RGBA8UInt:
			*nchannel = 4;
			*depth = 1;
			break;
		case R16UInt:
			*nchannel = 1;
			*depth = 2;
			break;
		case R32SFloat:
			*nchannel = 1;
			*depth = 4;
			break;
		default:
			break;
		}
	}


	TiledBitmap::TiledBitmap(TiledBitmapFormat format, int width, int height, int tileSize, std::vector<Tile>&& tiles)
		:_tiles(std::move(tiles))
	{
		_format = format;
		_height = height;
		_width = width;
		_tileSize = tileSize;
	}

	TiledBitmap* TiledBitmap::Create(void* pixels, TiledBitmapFormat format, int width, int height, int tileSize)
	{
		TiledBitmap* texture = nullptr;

		int nTileX = (width - 1) / tileSize + 1;
		int nTileY = (height - 1) / tileSize + 1;
		std::vector<Tile> tiles(nTileX * nTileY);

		int nChannel;
		int depth;
		GetPixelInfo(format, &nChannel, &depth);
		int pixelSize = nChannel * depth;

		for (int y = 0; y < height; y += tileSize)
		{
			for (int x = 0; x < width; x += tileSize)
			{
				int tileWidth = std::min(tileSize, width - x);
				int tileHeight = std::min(tileSize, height - y);

				uint8_t* tileData = new uint8_t[tileWidth * tileHeight * pixelSize];
				if (tileData == nullptr)
				{
					std::cout << "There is not enough available memory.\n";
					return texture;
				}

				for (int dstY = 0; dstY < tileHeight; ++dstY)
				{
					for (int dstX = 0; dstX < tileWidth; ++dstX)
					{
						int srcX = x + dstX;
						int srcY = y + dstY;
						const uint8_t* src = &static_cast<uint8_t*>(pixels)[(srcX + srcY * width) * pixelSize];
						uint8_t* dst = &tileData[(dstX + dstY * tileWidth) * pixelSize];
						for (int i = 0; i < pixelSize; ++i)
						{
							dst[i] = src[i];
						}
					}
				}
				tiles.push_back(Tile(tileData));
			}
		}

		texture = new TiledBitmap(format, width, height, tileSize, std::move(tiles));
		return texture;
	}

	void TiledBitmap::Save(FILE* f)
	{

	}
}
