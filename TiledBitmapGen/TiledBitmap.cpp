#include "TiledBitmap.hpp"
#include <iostream>

namespace TiledBitmapGen
{

	TiledBitmap::TiledBitmap(TiledBitmapFormat format, int width, int height, int tileSize, std::vector<Tile>&& tiles)
		:_tiles(std::move(tiles))
	{
		_format = format;
		_height = height;
		_width = width;
		_tileSize = tileSize;
	}

	TiledBitmap* TiledBitmap::Create(float* data, TiledBitmapFormat format, int width, int height, int tileSize)
	{
		TiledBitmap* texture = nullptr;

		int nTileX = (width - 1) / tileSize + 1;
		int nTileY = (height - 1) / tileSize + 1;
		std::vector<Tile> tiles;

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


				std::vector<uint8_t> tileData;
				tileData.resize(tileWidth * tileHeight * pixelSize);

				for (int dstY = 0; dstY < tileHeight; ++dstY)
				{
					for (int dstX = 0; dstX < tileWidth; ++dstX)
					{
						int srcX = x + dstX;
						int srcY = y + dstY;	

						for (int c = 0; c < nChannel; ++c)
						{
							float value = data[(srcX + srcY * width) * nChannel + c];
							if (depth == 8)
							{
								((uint8_t*)&tileData[0])[(dstX + dstY * tileWidth) * nChannel + c] = (uint8_t)value;
							}
							else if (depth == 16)
							{
								((uint16_t*)&tileData[0])[(dstX + dstY * tileWidth) * nChannel + c] = (uint16_t)value;
							}
							else
							{
								((float *)&tileData[0])[(dstX + dstY * tileWidth) * nChannel + c] = value;
							}
						}
					}
				}
				tiles.push_back(Tile(tileData));
			}
		}

		texture = new TiledBitmap(format, width, height, tileSize, std::move(tiles));
		return texture;
	}

	void TiledBitmap::SaveData(FILE* f)
	{
		if (!f)
		{
			return;
		}

		int nChannel, bitDepth;
		GetPixelInfo(_format, &nChannel, &bitDepth);

		for (int i = 0; i < _tiles.size(); ++i)
		{
			fwrite(&_tiles[i].data[0], 1, _tiles[i].data.size(), f);
		}
	}
}
