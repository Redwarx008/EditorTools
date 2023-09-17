#include <iostream>
#include "TiledBitmap.h"
#include <filesystem>
using std::cout;
using std::filesystem::path;

inline FILE* OpenFile(const std::string& fileName, const std::string& mode)
{
	FILE* f;
#if defined(_WIN32)

#if defined(_MSC_VER) && _MSC_VER >= 1400
	if (0 != _wfopen_s(&f, path(fileName).wstring().c_str(), path(mode).wstring().c_str()))
		f = 0;
#else
	f = _wfopen(path(fileName).wstring().c_str(), path(mode).wstring().c_str());
#endif

#elif defined(_MSC_VER) && _MSC_VER >= 1400
	if (0 != fopen_s(&f, filename, mode))
		f = 0;
#else
	f = fopen(filename, mode);
#endif
	return f;
}


int main(int argc, char* argv[])
{
	if (argc != 2)
	{
		cout << "usage: [fileName]\n";
		return -1;
	}

	const char* fileName = argv[1];
	TiledBitmap tiledBitmap;
	if (!tiledBitmap.Open(fileName))
	{
		cout << "cant't open file\n";
		return -1;
	}

	if (tiledBitmap.GetBitDepth() != 32 || tiledBitmap.GetChannelNum() != 1)
	{
		cout << "unsupport format\n";
		return -1;
	}

	int width = tiledBitmap.GetWidth();
	int height = tiledBitmap.GetHeight();
	int tileSize = tiledBitmap.GetTileSize();
	int nMipmap = tiledBitmap.GetMipmapNum();

	std::vector<float> out;

	for (int n = 0; n < nMipmap; ++n)
	{
		int nTileX = ceil(static_cast<float>(width >> n) / tileSize);
		int nTileY = ceil(static_cast<float>(height >> n) / tileSize);

		for (int tileY = 0; tileY < nTileY; ++tileY)
		{
			for (int tileX = 0; tileX < nTileX; ++tileX)
			{
				TiledBitmap::Tile tile = tiledBitmap.LoadTile(n, tileX, tileY);
				float* data = reinterpret_cast<float*> (&tile.data[0]);
				int w = tile.width;
				int h = tile.height;
				float min = 99999999.0;
				float max = -99999999.0;
				for (int y = 0; y < h; ++y)
				{
					for (int x = 0; x < w; ++x)
					{
						min = std::min(min, data[x + y * w]);
						max = std::max(max, data[x + y * w]);
					}
				}
				out.push_back(min);
				out.push_back(max);
			}
		}
	}
	std::string filePath = std::filesystem::path(fileName).parent_path().string() + "/minmax.bin";
	FILE* f = OpenFile(filePath.c_str(), "wb");
	fwrite(&out[0], sizeof(float), out.size(), f);
	fclose(f);
	return 0;
};