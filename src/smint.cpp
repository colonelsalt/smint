#include "util.h"
#include "smint.h"
#define STBI_ASSERT(X) Assert(X)

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"


int main(int ArgC, char** ArgV)
{
	if (ArgC != 3)
	{
		printf("Usage: smint.exe tileset_image tiled_map\n");
		return 1;
	}

	char* TilesetFileName = ArgV[1];
	char* MapFileName = ArgV[2];

	s32 ImageWidth, ImageHeight, ImageNumComponents;
	u8* ImageData = stbi_load(TilesetFileName, &ImageWidth, &ImageHeight, &ImageNumComponents, 4);
	if (!ImageData)
	{
		const char* FailReason = stbi_failure_reason();
		fprintf(stderr, "ERROR: Failed to load image file %s: %s\n", TilesetFileName, FailReason);
		return 1;
	}
	Assert((ImageWidth * ImageHeight) % sizeof(pixel) == 0);
	Assert((ImageWidth % 8 == 0) && (ImageHeight % 8 == 0));

	pixel* Pixels = (pixel*)ImageData;

	image OriginalImage = {};
	OriginalImage.TileWidth = (u32)ImageWidth / 8;
	OriginalImage.TileHeight = (u32)ImageHeight / 8;
	OriginalImage.Tiles = (tile*)malloc(sizeof(tile) * OriginalImage.TileWidth * OriginalImage.TileHeight);

	// Copy pixels into tiles
	for (u32 TileY = 0; TileY < OriginalImage.TileHeight; TileY++)
	{
		for (u32 TileX = 0; TileX < OriginalImage.TileWidth; TileX++)
		{
			tile* Tile = TileAt(&OriginalImage, TileX, TileY);
			for (u32 PixelY = 0; PixelY < 8; PixelY++)
			{
				for (u32 PixelX = 0; PixelX < 8; PixelX++)
				{
					u32 PixelIndex = TileY * OriginalImage.TileWidth * 8 + PixelY * 8 * OriginalImage.TileWidth + TileX * 8 + PixelX;
					
					pixel* PixelToRead = Pixels + PixelIndex;
					pixel* PixelToWrite = PixelAt(Tile, PixelX, PixelY);

					*PixelToWrite = *PixelToRead;
				}
			}
		}
	}

	stbi_image_free(ImageData);
}