#include "PagedTexture.hpp"
#include <iostream>
#include <algorithm>

PagedTexture::PagedTexture(int width, int height, int tileSize, std::vector<Tile>&& tiles)
	:_tiles(std::move(tiles))
{
	_height = height;
	_width = width;
	_tileSize = tileSize;
}

PagedTexture* PagedTexture::Create(float* data, int nChannel, int bitDepth, int width, 
	int height, int tileSize, int borderWidth)
{
	PagedTexture* texture = nullptr;

	int nTileX = ceil((float)width / tileSize);
	int nTileY = ceil((float)height / tileSize);
	std::vector<Tile> tiles;

	int pixelSize = nChannel * bitDepth / 8;

	for (int y = 0; y < height; y += tileSize)
	{
		for (int x = 0; x < width; x += tileSize)
		{
			int tileWidth = std::min(tileSize, width - x);
			int tileHeight = std::min(tileSize, height - y);

			int pageWidth = tileWidth + borderWidth * 2;
			int pageHeight = tileHeight + borderWidth * 2;

			std::vector<uint8_t> tileData;
			tileData.resize((uint64_t)pageWidth * pageHeight * pixelSize);

			for (int dstY = 0; dstY < pageHeight; ++dstY)
			{
				for (int dstX = 0; dstX < pageWidth; ++dstX)
				{
					int srcX = std::clamp(x + dstX - borderWidth, 0, width - 1);
					int srcY = std::clamp(y + dstY - borderWidth, 0, height - 1);

					for (int c = 0; c < nChannel; ++c)
					{
						float value = data[(srcX + srcY * width) * nChannel + c];
						if (bitDepth == 8)
						{
							((uint8_t*)&tileData[0])[(dstX + dstY * pageWidth) * nChannel + c] = (uint8_t)value;
						}
						else if (bitDepth == 16)
						{
							((uint16_t*)&tileData[0])[(dstX + dstY * pageWidth) * nChannel + c] = (uint16_t)value;
						}
						else
						{
							((float*)&tileData[0])[(dstX + dstY * pageWidth) * nChannel + c] = value;
						}
					}
				}
			}
			tiles.emplace_back(Tile(std::move(tileData)));
		}
	}

	texture = new PagedTexture(width, height, tileSize, std::move(tiles));
	return texture;
}

void PagedTexture::SaveData(FILE* f)
{
	if (!f)
	{
		return;
	}

	for (int i = 0; i < _tiles.size(); ++i)
	{
		fwrite(&_tiles[i].data[0], 1, _tiles[i].data.size(), f);
	}
}