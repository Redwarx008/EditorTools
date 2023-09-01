#pragma once

#include <vector>

namespace TiledBitmapGen
{
	struct Color
	{
		float r;
		float g;
		float b;
		float a;
	};
	enum TiledBitmapFormat
	{
		RG8UInt,
		RGBA8UInt,
		R16UInt,
		R32SFloat
	};
	class TiledBitmap
	{
	public:
		static TiledBitmap* Create(void* pixels, TiledBitmapFormat format, int width, int height, int tileSize);
		void SetPixel(int x, int y, Color color);
		void Save(FILE* f);

		struct Tile
		{
			Tile(uint8_t* data)
			{
				this->data = data;
			}
			~Tile()
			{
				delete[] data;
			}
			uint8_t* data;
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




