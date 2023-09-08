#pragma once

#include <vector>
#include <iostream>

using std::cout;

class TiledBitmap
{
public:
	TiledBitmap() {}
	bool Open(const char* fileName)
	{
		if (fopen_s(&f, fileName, "rb") != 0)
		{
			cout << "open file failed\n";
			return false;
		}

		width = Read16();
		height = Read16();
		tileSize = Read16();
	}
	~TiledBitmap()
	{
		if (f)
		{
			fclose(f);
		}
	}
private:
	struct Tile
	{
		std::vector<uint8_t> data;
	};
	FILE* f;
	int width;
	int height;
	int tileSize;
	std::vector<Tile> _tiles;

	uint8_t Read8()
	{
		uint8_t ret;
		if (0 == fread_s(&ret, 1, 1, 1, f))
		{
			ret = '\0';
		}
		return ret;
	}
	uint16_t Read16()
	{
		uint8_t low = Read8();
		uint8_t high = Read8();
		uint16_t ret = (uint16_t)high << 8 | (uint16_t)low;
		return ret;
	}
	bool Seek(size_t pos)
	{
		return _fseeki64(f, pos, SEEK_SET) != 0;
	}
};

