#pragma once

#include <vector>

class TiledBitmap
{
public:
	static TiledBitmap* Create(float* data, int nChannel, int depth, int width, 
		int height, int tileSize, int borderSize);
	void SaveData(FILE* f);
	struct Tile
	{
		Tile(std::vector<uint8_t>&& data)
			:data(std::move(data))
		{

		}
		std::vector<uint8_t> data;
	};
private:
	TiledBitmap(int width, int height, int tileSize, std::vector<Tile>&& tiles);
	int _width;
	int _height;
	int _tileSize;
	std::vector<Tile> _tiles;
};




