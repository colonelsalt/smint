#include "util.h"
#include "smint.h"
#include <sys/stat.h>

#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/error/en.h"

#define STBI_ASSERT(X) Assert(X)
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STBIW_ASSERT(X) Assert(X)
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

enum tiled_flip_flags : u32
{
	TiledFlag_HFlip 	   = 0x80000000,
	TiledFlag_VFlip 	   = 0x40000000,
	TiledFlag_DiagonalFlip = 0x20000000,
	TiledFlag_Rotated      = 0x10000000
};

struct str_buffer
{
	u8* Data;
	u64 Size;
};

str_buffer ReadTextFile(char* FileName)
{
	str_buffer Result = {};
    FILE* File = fopen(FileName, "rt");
    if(File)
    {
		#if _WIN32
        struct __stat64 Stat;
        _stat64(FileName, &Stat);
		#else
        struct stat Stat;
        stat(FileName, &Stat);
		#endif
        
        Result.Data = (u8*)malloc(sizeof(u8) * Stat.st_size);
		Result.Size = Stat.st_size;
        if(Result.Data)
        {
			Result.Size = fread(Result.Data, sizeof(u8), Stat.st_size, File);
			if (ferror(File))
			{
				fprintf(stderr, "ERROR: Unable to read '%s'.\n", FileName);
				free(Result.Data);
				Result.Data = nullptr;
			}
			else
			{
				// Not quite sure why there seems to be a stray 'r' at the end of the string... mangled line ending..?
				Result.Data[Result.Size] = 0;
			}
        }
        
        fclose(File);
    }
    else
    {
        fprintf(stderr, "ERROR: Unable to open \"%s\".\n", FileName);
    }
    
    return Result;
}

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

b32 AreTilesEqual(unique_tile* UniqueTile, tile* TileToCheck)
{
	for (u32 VariantIndex = TileTransform_Unchanged; VariantIndex < TileTransform_Count; VariantIndex++)
	{
		tile* UniqueVariant = UniqueTile->Variants + VariantIndex;
		if (AreTilesEqualNoFlip(UniqueVariant, TileToCheck))
		{
			TileToCheck->EquivalentUniqueTile = UniqueTile;
			TileToCheck->EqualAfterTransform = (tile_transform_type)VariantIndex;
			return true;
		}
	}

	return false;
}

void CopyTransformedTile(tile* SourceTile, tile* OutTransformedTile, tile_transform_type Transform)
{
	for (u32 Y = 0; Y < 8; Y++)
	{
		for (u32 X = 0; X < 8; X++)
		{
			pixel* PixelToWrite = PixelAt(OutTransformedTile, X, Y);

			pixel* PixelToRead;
			switch (Transform)
			{
				case TileTransform_HFlip:
				{
					PixelToRead = PixelAt(SourceTile, 8 - X - 1, Y);
				} break;
				case TileTransform_VFlip:
				{
					PixelToRead = PixelAt(SourceTile, X, 8 - Y - 1);
				} break;
				case TileTransform_DiagonalFlip:
				{
					PixelToRead = PixelAt(SourceTile, 8 - X - 1, 8 - Y - 1);
				} break;
				default:
				{
					PixelToRead = PixelAt(SourceTile, X, Y);
				} break;
			}

			*PixelToWrite = *PixelToRead;
		}
	}
}

void GenerateTileVariants(tile* Tile, unique_tile* OutUniqueTile)
{	
	for (u32 Transform = TileTransform_Unchanged; Transform < TileTransform_Count; Transform++)
	{
		tile* DestTile = OutUniqueTile->Variants + Transform;
		DestTile->EquivalentUniqueTile = nullptr;
		DestTile->EqualAfterTransform = TileTransform_Unchanged;

		CopyTransformedTile(Tile, DestTile, (tile_transform_type)Transform);
	}
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
			Tile->EquivalentUniqueTile = nullptr;
			Tile->EqualAfterTransform = TileTransform_Unchanged;
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
	unique_tile* MinimisedTiles = (unique_tile*)malloc(sizeof(unique_tile) * OriginalImage.TileWidth * OriginalImage.TileHeight);

	for (u32 TileY = 0; TileY < OriginalImage.TileHeight; TileY++)
	{
		for (u32 TileX = 0; TileX < OriginalImage.TileWidth; TileX++)
		{
			tile* Tile = TileAt(&OriginalImage, TileX, TileY);

			b32 IsTileUnique = true;
			for (u32 UniqueTileIndex = 0; UniqueTileIndex < NumUniqueTiles; UniqueTileIndex++)
			{
				unique_tile* UnqiueTile = MinimisedTiles + UniqueTileIndex;
				if (AreTilesEqual(UnqiueTile, Tile))
				{
					IsTileUnique = false;
					break;
				}
			}
			if (IsTileUnique)
			{
				unique_tile* NewUniqueTile = MinimisedTiles + NumUniqueTiles;
				GenerateTileVariants(Tile, MinimisedTiles + NumUniqueTiles);

				Tile->EquivalentUniqueTile = NewUniqueTile;
				Tile->EqualAfterTransform = TileTransform_Unchanged;

				NumUniqueTiles++;
			}
		}
	}

	str_buffer MapFileContents = ReadTextFile(MapFileName);
	if (!MapFileContents.Data)
	{
		return 1;
	}

	rapidjson::Document JsonDoc;
	if (JsonDoc.ParseInsitu((char*)MapFileContents.Data).HasParseError())
	{
		fprintf(stderr, "ERROR: Failed to parse '%s': %s\n", MapFileName, rapidjson::GetParseError_En(JsonDoc.GetParseError()));
		return 1;
	}

	if (!JsonDoc.HasMember("layers") || !JsonDoc["layers"].IsArray())
	{
		fprintf(stderr, "ERROR: Invalid map format - 'layers' element not found or invalid format.\n");
		return 1;
	}


	if (!JsonDoc.HasMember("tilesets") || !JsonDoc["tilesets"].IsArray())
	{
		fprintf(stderr, "ERROR: Invalid map format - 'tilesets' not found or invalid format.\n");
		return 1;
	}
	rapidjson::Value& TilesetsArray = JsonDoc["tilesets"];
	if (TilesetsArray.Size() == 0)
	{
		fprintf(stderr, "ERROR: 'tilesets' array in map file is empty.\n");
		return 1;
	}
	if (TilesetsArray.Size() > 1)
	{
		printf("WARNING: Multiple tilesets in the same map is not supported yet - only tiles from the first tileset will be minimised.\n");
	}
	rapidjson::Value& TilesetObj = TilesetsArray[0];
	if (!TilesetObj.IsObject() || !TilesetObj.HasMember("firstgid") || !TilesetObj["firstgid"].IsUint())
	{
		fprintf(stderr, "ERROR: Could not find 'firstgid' in map's 'tileset' property.\n");
		return 1;
	}
	u32 FirstTileId = TilesetObj["firstgid"].GetUint();

	rapidjson::Value& Layers = JsonDoc["layers"];
	for (u32 LayerIndex = 0; LayerIndex < Layers.Size(); LayerIndex++)
	{
		rapidjson::Value& Layer = Layers[LayerIndex];
		if (!Layer.IsObject() || !Layer.HasMember("data") || !Layer["data"].IsArray())
		{
			fprintf(stderr, "ERROR: Invalid map format - layer %u has unexpected format and/or is missing 'data' array.\n", LayerIndex);
			return 1;
		}

		rapidjson::Value& LayerData = Layer["data"];
		for (u32 DataIndex = 0; DataIndex < LayerData.Size(); DataIndex++)
		{
			u32 TileIndex = LayerData[DataIndex].GetUint();
			if (TileIndex == 0)
			{
				continue; // Blank tile
			}

			u32 FlipFlags = 0;
			if (TileIndex & TiledFlag_HFlip)
			{
				FlipFlags |= TiledFlag_HFlip;
				TileIndex &= ~TiledFlag_HFlip;
			}
			if (TileIndex & TiledFlag_VFlip)
			{
				FlipFlags |= TiledFlag_VFlip;
				TileIndex &= ~TiledFlag_VFlip;
			}
			if (TileIndex & TiledFlag_DiagonalFlip)
			{
				// tbh I don't actually know how to diagonally flip a tile in Tiled, but if we ever encounter one, let's treat it like HFLIP+VFLIP
				FlipFlags |= TiledFlag_HFlip | TiledFlag_VFlip;
				TileIndex &= ~TiledFlag_DiagonalFlip;
			}
			if (TileIndex & TiledFlag_Rotated)
			{
				printf("WARNING: (Layer %u, entry %u) Tile rotation is not supported for GBA - this entry will be unchanged in the output map.\n",
				       LayerIndex, DataIndex);
				TileIndex &= ~TiledFlag_Rotated;
			}
			TileIndex -= FirstTileId;
			Assert(TileIndex < OriginalImage.TileWidth * OriginalImage.TileHeight);

			tile* SourceTile = OriginalImage.Tiles + TileIndex;
			unique_tile* UniqueTile = SourceTile->EquivalentUniqueTile;
			u32 NewTileIndex = UniqueTile - MinimisedTiles;
			Assert(NewTileIndex < NumUniqueTiles);

			NewTileIndex += FirstTileId;

			// The output entry's flip is `OldFlip` XOR `NewFlip`
			switch (SourceTile->EqualAfterTransform)
			{
				case TileTransform_HFlip:
				{
					FlipFlags ^= TiledFlag_HFlip;
				} break;
				case TileTransform_VFlip:
				{
					FlipFlags ^= TiledFlag_VFlip;
				} break;
				case TileTransform_DiagonalFlip:
				{
					FlipFlags ^= (TiledFlag_HFlip | TiledFlag_VFlip);
				} break;
			}
			NewTileIndex |= FlipFlags;

			LayerData[DataIndex].SetUint(NewTileIndex);
		}
	}

	// Work out most square-ish dimensions of output image so width and height both evenly divide NumUniqueTiles (no blank/wasted tiles)
	u32 OutputTileWidth = (u32)sqrt(NumUniqueTiles) + 1;
	u32 OutputTileHeight = NumUniqueTiles / OutputTileWidth;
	while (!(NumUniqueTiles % OutputTileWidth == 0 && NumUniqueTiles % OutputTileHeight == 0))
	{
		OutputTileWidth--;
		OutputTileHeight = NumUniqueTiles / OutputTileWidth;
	}

	s32 OutputImageWidth = (s32)OutputTileWidth * 8;
	s32 OutputImageHeight = (s32)OutputTileHeight * 8;
	Assert(OutputImageWidth > 0 && OutputTileHeight > 0);

	pixel* OutputPixels = (pixel*)malloc(sizeof(pixel) * OutputImageWidth * OutputImageHeight);
	
	
	for (u32 TileY = 0; TileY < OutputTileHeight; TileY++)
	{
		for (u32 TileX = 0; TileX < OutputTileWidth; TileX++)
		{
			u32 TileIndex = TileY * OutputTileWidth + TileX;
			tile* SourceTile = MinimisedTiles[TileIndex].Variants + TileTransform_Unchanged;

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
	}

	if (!stbi_write_bmp("out.bmp", OutputImageWidth, OutputImageHeight, 4, OutputPixels))
	{
		fprintf(stderr, "Failed to write output image.\n");
		return 1;
	}

	rapidjson::StringBuffer OutStringBuffer(0, MapFileContents.Size);
	rapidjson::Writer<rapidjson::StringBuffer> JsonWriter(OutStringBuffer);
	JsonDoc.Accept(JsonWriter);
	
	FILE* OutMapFile = fopen("out.tmj", "wb");
	fprintf(OutMapFile, "%s", OutStringBuffer.GetString());

	stbi_image_free(ImageData);
}