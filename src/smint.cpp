#include "util.h"
#include "smint.h"

#define STBI_ASSERT(X) Assert(X)
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STBIW_ASSERT(X) Assert(X)
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"


b32 AreTilesEqualNoFlip(tile* A, tile* B)
{
	for (u32 Y = 0; Y < 8; Y++)
	{
		for (u32 X = 0; X < 8; X++)
		{
			pixel* PixelA = PixelAt(A, X, Y);
			pixel* PixelB = PixelAt(B, X, Y);
			if (*PixelA != *PixelB)
			{
				return false;
			}
		}
	}
	return true;
}

b32 AreTilesEqual(tile* A, tile* B)
{
	return AreTilesEqualNoFlip(A, B);
}

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
	if ((ImageWidth % 8 != 0) || (ImageHeight % 8 != 0))
	{
		fprintf(stderr, "ERROR: Image dimensions (%dx%d) do not split evenly into 8x8 tiles; please modify the image before proceeding.\n",
		        ImageWidth, ImageHeight);
		return 1;
	}

	pixel* Pixels = (pixel*)ImageData;

	tileset_image OriginalImage = {};
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
					u32 PixelIndex = TileY * OriginalImage.TileWidth * 8 * 8 + PixelY * 8 * OriginalImage.TileWidth + TileX * 8 + PixelX;
					
					pixel* PixelToRead = Pixels + PixelIndex;
					pixel* PixelToWrite = PixelAt(Tile, PixelX, PixelY);

					*PixelToWrite = *PixelToRead;
				}
			}
		}
	}

	u32 NumUniqueTiles = 0;
	tile** MinimisedTiles = (tile**)malloc(sizeof(tile*) * OriginalImage.TileWidth * OriginalImage.TileHeight);

	for (u32 TileY = 0; TileY < OriginalImage.TileHeight; TileY++)
	{
		for (u32 TileX = 0; TileX < OriginalImage.TileWidth; TileX++)
		{
			tile* Tile = TileAt(&OriginalImage, TileX, TileY);

			b32 IsTileUnique = true;
			for (u32 UniqueTileIndex = 0; UniqueTileIndex < NumUniqueTiles; UniqueTileIndex++)
			{
				tile* UnqiueTile = MinimisedTiles[UniqueTileIndex];
				if (AreTilesEqual(Tile, UnqiueTile))
				{
					IsTileUnique = false;
					break;
				}
			}
			if (IsTileUnique)
			{
				MinimisedTiles[NumUniqueTiles++] = Tile;
			}
		}
	}

	// TODO: Work out largest height of output image so width and height both evenly divide NumUniqueTiles (no blank/wasted tiles)
	s32 OutputImageWidth = (s32)NumUniqueTiles * 8;
	s32 OutputImageHeight = 1 * 8;
	Assert(OutputImageWidth > 0);

	pixel* OutputPixels = (pixel*)malloc(sizeof(pixel) * OutputImageWidth * OutputImageHeight);
	
	// TODO: This is just a temp 1D tile loop - needs rework once more ideal output dimensions worked out
	u32 TileY = 0;
	for (u32 TileX = 0; TileX < NumUniqueTiles; TileX++)
	{
		u32 TileIndex = TileX;
		tile* SourceTile = MinimisedTiles[TileIndex];

		for (u32 PixelY = 0; PixelY < 8; PixelY++)
		{
			for (u32 PixelX = 0; PixelX < 8; PixelX++)
			{
				u32 DestIndex = TileY * OutputImageWidth * 8 + PixelY * OutputImageWidth + TileX * 8 + PixelX;

				pixel* PixelToWrite = OutputPixels + DestIndex;
				pixel* PixelToRead = PixelAt(SourceTile, PixelX, PixelY);

				*PixelToWrite = *PixelToRead;
			}
		}
	}

	if (!stbi_write_bmp("out.bmp", OutputImageWidth, OutputImageHeight, 4, OutputPixels))
	{
		fprintf(stderr, "Failed to write output image.\n");
		return 1;
	}
	

	stbi_image_free(ImageData);
}