#include "util.h"
#include "smint.h"

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

#include "smint_io.cpp"
#include "smint_tileset.cpp"

enum tiled_flip_flags : u32
{
	TiledFlag_HFlip 	   = 0x80000000,
	TiledFlag_VFlip 	   = 0x40000000,
	TiledFlag_DiagonalFlip = 0x20000000,
	TiledFlag_Rotated      = 0x10000000
};

int main(int ArgC, char** ArgV)
{
	if (ArgC != 2)
	{
		printf("Usage: smint.exe tiled_map.tmj\n");
		return 1;
	}

	char* MapFilePath = ArgV[1];
	char MapFileExtension[16];
	GetFileExtension(MapFilePath, MapFileExtension);
	if (strcmp(MapFileExtension, ".tmj") != 0 && strcmp(MapFileExtension, ".json"))
	{
		fprintf(stderr, "ERROR: Unsupported map file format '%s'; please supply a .tmj/.json file.\n", MapFileExtension);
		return 1;
	}

	str_buffer MapFileContents = ReadTextFile(MapFilePath);
	if (!MapFileContents.Data)
	{
		return 1;
	}

	rapidjson::Document JsonDoc;
	if (JsonDoc.ParseInsitu(MapFileContents.Data).HasParseError())
	{
		fprintf(stderr, "ERROR: Failed to parse map '%s': %s\n", MapFilePath, rapidjson::GetParseError_En(JsonDoc.GetParseError()));
		return 1;
	}

	if (!JsonDoc.HasMember("layers") || !JsonDoc["layers"].IsArray())
	{
		fprintf(stderr, "ERROR: Invalid map format - 'layers' element not found or invalid format.\n");
		return 1;
	}
	rapidjson::Value& Layers = JsonDoc["layers"];

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
	
	char MapWorkingDir[MAX_PATH];
	StripFileName(MapFilePath, MapWorkingDir);
	if (*MapWorkingDir)
	{
		// Map is in a different directory to where we are
		if (!ChangeWorkingDir(MapWorkingDir))
		{
			return 1;
		}
	}

	for (u32 TilesetIndex = 0; TilesetIndex < TilesetsArray.Size(); TilesetIndex++)
	{
		rapidjson::Value& TilesetObj = TilesetsArray[TilesetIndex];
		if (!TilesetObj.IsObject() || !TilesetObj.HasMember("firstgid") || !TilesetObj["firstgid"].IsUint() || 
			!TilesetObj.HasMember("source") || !TilesetObj["source"].IsString())
		{
			fprintf(stderr, "ERROR: Invalid format of tileset %u in map file.\n", TilesetIndex);
			return 1;
		}
		u32 FirstTileId = TilesetObj["firstgid"].GetUint();
		const char* TilesetPath = TilesetObj["source"].GetString();

		minimised_tileset MinTiles = MinimiseTileset(TilesetPath);
		if (MinTiles.Error)
		{
			return 1;
		}

		TilesetObj["source"].SetString(MinTiles.NewTilesetPath, strlen(MinTiles.NewTilesetPath));

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
					FlipFlags |= (TiledFlag_HFlip | TiledFlag_VFlip);
					TileIndex &= ~TiledFlag_DiagonalFlip;
				}
				if (TileIndex & TiledFlag_Rotated)
				{
					printf("WARNING: (Layer %u, entry %u) Tile rotation is not supported for GBA - this entry will be unchanged in the output map.\n",
					       LayerIndex, DataIndex);
					TileIndex &= ~TiledFlag_Rotated;
				}
				TileIndex -= FirstTileId;
				Assert(TileIndex < MinTiles.OriginalImage.TileWidth * MinTiles.OriginalImage.TileHeight);

				tile* SourceTile = MinTiles.OriginalImage.Tiles + TileIndex;
				unique_tile* UniqueTile = SourceTile->EquivalentUniqueTile;
				u32 NewTileIndex = UniqueTile - MinTiles.MinimisedTiles;
				Assert(NewTileIndex < MinTiles.NumUniqueTiles);

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
	}

	char MapOutPath[PATH_MAX];
	AppendToFilePath(MapFilePath, "_min", MapOutPath);

	if (!WriteJsonToFile(&JsonDoc, MapOutPath, MapFileContents.Size))
	{
		return 1;
	}

	return 0;
}