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

	s32 TilesetWidth, TilesetHeight, TilesetNumComponents;
	u8* ImageData = stbi_load(TilesetFileName, &TilesetWidth, &TilesetHeight, &TilesetNumComponents, 4);
	if (!ImageData)
	{
		const char* FailReason = stbi_failure_reason();
		fprintf(stderr, "ERROR: Failed to load image file %s: %s\n", TilesetFileName, FailReason);
		return 1;
	}

	Assert((TilesetWidth * TilesetHeight) % sizeof(pixel) == 0);
	pixel* Pixels = (pixel*)ImageData;

	printf("First pixel is R=%u, G=%u, B=%u\n", Pixels[0].R, Pixels[0].G, Pixels[0].B);
	printf("Second pixel is R=%u, G=%u, B=%u\n", Pixels[1].R, Pixels[1].G, Pixels[1].B);

	pixel* Pixel11 = Pixels + TilesetWidth + 1;
	printf("Pixel[1][1] is R=%u, G=%u, B=%u\n", Pixel11->R, Pixel11->G, Pixel11->B);

	pixel* LastPixel = Pixels + TilesetHeight * TilesetWidth - 1;
	printf("Last pixel is R=%u, G=%u, B=%u\n", LastPixel->R, LastPixel->G, LastPixel->B);

	stbi_image_free(ImageData);
}