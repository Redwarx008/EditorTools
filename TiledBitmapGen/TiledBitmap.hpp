#pragma once

#include <vector>

namespace TiledBitmapGen
{
	enum TiledBitmapFormat : uint8_t
	{
		R8UInt,
		RG8UInt,
		RGBA8UInt,
		R16UInt,
		R32SFloat,
		Unkown
	};

	inline void GetPixelInfo(TiledBitmapFormat format, int* nchannel, int* depth)
	{
		switch (format)
		{
		case R8UInt:
			*nchannel = 1;
			*depth = 1;
			break;
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

	struct Color
	{
		float r;
		float g;
		float b;
		float a;
	};

	class TiledBitmap
	{
	public:
		static TiledBitmap* Create(float* data, TiledBitmapFormat format, int width, int height, int tileSize);
		void SetPixel(int x, int y, Color color);
		void SaveData(FILE* f);

		struct Tile
		{
			std::vector<uint8_t> data;
		};
	private:
		TiledBitmap(TiledBitmapFormat format, int width, int height, int tileSize, std::vector<Tile>&& tiles);
		int _width;
		int _height;
		int _tileSize;
		TiledBitmapFormat _format;
		std::vector<Tile> _tiles;
	};

	template<typename ChannelFormat, size_t TileSize>
	class TiledTexture
	{
	public:
		TiledTexture(float* data, int width, int height, int nChannel)
		{
			int nTileX = ceilf((float)width / TileSize);
			int nTileY = ceilf((float)height / TileSize);

			std::vector<Tile> tiles;
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

								(ChannelFormat*)& tileData[0])[(dstX + dstY * tileWidth) * nChannel + c] = (ChannelFormat)value;
							}
						}
					}
					tiles.push_back(Tile{ tileData });
				}
			}
		}
		void SaveData(FILE* f)
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
	private:
		struct Tile
		{
			std::vector<uint8_t> data;
		};
		std::vector<Tile> _tiles;
	};
}




