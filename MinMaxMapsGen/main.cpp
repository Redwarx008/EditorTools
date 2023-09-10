#include <iostream>

#include "TiledBitmap.h"

using std::cout;

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

	int nTileX = ceil((float)width / tileSize);
	int nTileY = ceil((float)height / tileSize);

	std::vector<float> out;

	for (int n = 0; n < nMipmap; ++n)
	{
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
#if defined(_WIN32)
	FILE* f;
	fopen_s(&f, "minmax.bin", "wb");
#else
	FILE* f = fopen("minmax.bin", "wb");
#endif // WIN32
	fwrite(&out[0], sizeof(float), out.size(), f);
	fclose(f);
	return 0;
};