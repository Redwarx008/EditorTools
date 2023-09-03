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
			Tile(std::vector<uint8_t> data)
				:data(data)
			{
				
			}

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
}




