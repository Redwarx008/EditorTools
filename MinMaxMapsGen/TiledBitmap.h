#pragma once

#include <vector>
#include <iostream>

using std::cout;

class TiledBitmap
{
public:
	struct Tile
	{
		int width;
		int height;
		std::vector<uint8_t> data;
	};

	TiledBitmap() {}
	bool Open(const char* fileName)
	{
		if (fopen_s(&_f, fileName, "rb") != 0)
		{
			cout << "open file failed\n";
			return false;
		}

		_width = Read16();
		_height = Read16();
		_tileSize = Read16();
		_bitDepth = Read8();
		_nChannel = Read8();
		_nMipmap = Read8();
		return true;
	}
	Tile LoadTile(int mipmapLevel, int tileX, int tileY)
	{
		Tile tile;

		if (mipmapLevel >= _nMipmap || mipmapLevel < 0)
		{
			//ERR_FAIL_V_MSG(tile, "invalid mipmapLevel.");
			cout << "invalid mipmapLevel.\n";
			return tile;
		}

		int64_t offset = GetTileStartPos(mipmapLevel, tileX, tileY);
		if (!Seek(offset))
		{
			//ERR_FAIL_V_MSG(tile, "set seek failed.");
			cout << "set seek failed.\n";
			return tile;
		}

		int width;
		int height;
		{
			int nTileX = ceil((float)(_width >> mipmapLevel) / _tileSize);
			int nTileY = ceil((float)(_height >> mipmapLevel) / _tileSize);

			width = tileX == nTileX - 1 ? nTileX * _tileSize - (nTileX - 1) * _tileSize : _tileSize;
			height = tileY == nTileY - 1 ? nTileY * _tileSize - (nTileY - 1) * _tileSize : _tileSize;
		}

		int pixelSize = _nChannel * _bitDepth / 8;

		tile.data = Read(width * height * pixelSize);
		tile.width = width;
		tile.height = height;
		return tile;
	}
	int GetBitDepth() const
	{
		return _bitDepth;
	}
	int GetChannelNum() const
	{
		return _nChannel;
	}
	int GetMipmapNum() const
	{
		return _nMipmap;
	}
	int GetWidth() const
	{
		return _width;
	}
	int GetHeight() const
	{
		return _height;
	}
	int GetTileSize() const
	{
		return _tileSize;
	}
	~TiledBitmap()
	{
		if (_f)
		{
			fclose(_f);
		}
	}
private:

	FILE* _f = nullptr;
	int _width = 0;
	int _height = 0;
	int _tileSize = 0;
	int _nChannel = 0;
	int _bitDepth = 0;
	int _nMipmap = 0;
	int _headerSize = 9;


	int64_t GetTileStartPos(int mipmapLevel, int tileX, int tileY) const
	{
		int pixelSize = _nChannel * _bitDepth / 8;

		int64_t pos = _headerSize;

		for (int i = 0; i < mipmapLevel; ++i)
		{
			int width = _width >> i;
			int height = _height >> i;
			pos += width * height * pixelSize;
		}

		int mipWidth = _width >> mipmapLevel;
		int mipHeight = _height >> mipmapLevel;

		int nTileX = ceil((float)mipWidth / _tileSize);
		int nTileY = ceil((float)mipHeight / _tileSize);

		if (tileX >= nTileX || tileX < 0)
		{
			//ERR_FAIL_V_MSG(-1, "invalid tileX.");'
			cout << "invalid tileX.\n";
			return -1;
		}

		if (tileY >= nTileY || tileY < 0)
		{
			//ERR_FAIL_V_MSG(-1, "invalid tileY.");
			cout << "invalid tileY.\n";
			return -1;
		}

		int edgeTileWidth = nTileX * _tileSize - (nTileX - 1) * _tileSize;
		int edgeTileHeight = nTileY * _tileSize - (nTileY - 1) * _tileSize;

		pos += (int64_t)tileY * (nTileX - 1) * _tileSize * _tileSize * pixelSize; // except the last column and row.
		pos += (int64_t)tileY * edgeTileWidth * _tileSize * pixelSize; // except the last row in the last column.

		if (tileY == nTileY - 1)
		{
			pos += (int64_t)tileX * edgeTileHeight * _tileSize * pixelSize;
		}
		else
		{
			pos += (int64_t)tileX * _tileSize * _tileSize * pixelSize;
		}

		return pos;
	}

	uint8_t Read8()
	{
		uint8_t ret;
		if (0 == fread_s(&ret, 1, 1, 1, _f))
		{
			ret = '\0';
		}
		return ret;
	}
	uint16_t Read16()
	{
		//uint8_t low = Read8();
		//uint8_t high = Read8();
		//uint16_t ret = (uint16_t)high << 8 | (uint16_t)low;
		uint16_t ret;
		fread_s(&ret, sizeof(ret), sizeof(uint16_t), 1, _f);
		return ret;
	}
	float ReadFloat()
	{
		float ret;
		fread_s(&ret, sizeof(ret), sizeof(float), 1, _f);
		return ret;
	}
	std::vector<uint8_t> Read(size_t length)
	{
		std::vector<uint8_t> data;
		if (length < 0)
		{
			return data;
		}

		data.resize(length);
		size_t count = fread_s(&data[0], data.size(), 1, length, _f);
		if (count < length)
		{
			data.resize(count);
		}
		return data;
	}
	bool Seek(size_t pos)
	{
		return _fseeki64(_f, pos, SEEK_SET) == 0;
	}
};

