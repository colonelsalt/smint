
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


struct minimised_tileset
{
	tileset_image OriginalImage;
	unique_tile* MinimisedTiles;
	u32 NumUniqueTiles;
	b32 Error;
	b32 IsUnchanged;
};

minimised_tileset MinimiseTileset(const char* TilesetPath, char* OutNewTilesetPath, char* CurrentWorkingDir = nullptr)
{
	minimised_tileset Result = {};

	char FileExtension[16];
	GetFileExtension(TilesetPath, FileExtension);
	if (strcmp(FileExtension, ".tsj") != 0 && strcmp(FileExtension, ".json") != 0)
	{
		fprintf(stderr, "ERROR: Tileset file '%s' has unsupported extension '%s' - must be .tsj/.json\n", TilesetPath, FileExtension);
		Result.Error = true;
		return Result;
	}

	// Parse tileset .tsj file
	rapidjson::Document JsonDoc;
	str_buffer TilesetSpecStr = ReadTextFile(TilesetPath);
	if (JsonDoc.ParseInsitu(TilesetSpecStr.Data).HasParseError())
	{
		fprintf(stderr, "ERROR: Failed to parse tileset '%s': %s\n", TilesetPath, rapidjson::GetParseError_En(JsonDoc.GetParseError()));
		Result.Error = true;
		return Result;
	}
	if (!JsonDoc.HasMember("image") || !JsonDoc["image"].IsString())
	{
		fprintf(stderr, "ERROR: Could not find 'image' field in tileset file.\n");
		Result.Error = true;
		return Result;
	}
	if (JsonDoc.HasMember("tilewidth") && JsonDoc["tilewidth"].IsUint() &&
		JsonDoc.HasMember("tileheight") && JsonDoc["tileheight"].IsUint())
	{
		if (JsonDoc["tilewidth"].GetUint() != 8 || JsonDoc["tileheight"].GetUint() != 8)
		{
			fprintf(stderr, "ERROR: Tile dimensions in tileset '%s' are not 8x8 - cannot minimise.\n", TilesetPath);
			Result.Error = true;
			return Result;
		}
	}
	else
	{
		printf("WARNING: No tile dimensions found in tileset file '%s'; proceeding on the assumption that tiles are 8x8\n", TilesetPath);
	}

	const char* ImagePath = JsonDoc["image"].GetString();
	char TilesetWorkingDir[PATH_MAX];
	StripFileName(TilesetPath, TilesetWorkingDir);

	if (*TilesetWorkingDir)
	{
		// Change working directory to where the .tsj file is so we can resolve the relative path to the image file
		if (!ChangeWorkingDir(TilesetWorkingDir))
		{
			Result.Error = true;
			return Result;
		}
	}

	s32 ImageWidth, ImageHeight, ImageNumComponents;
	u8* ImageData = stbi_load(ImagePath, &ImageWidth, &ImageHeight, &ImageNumComponents, 4);
	if (!ImageData)
	{
		const char* FailReason = stbi_failure_reason();
		fprintf(stderr, "ERROR: Failed to load image file '%s': %s\n", ImagePath, FailReason);
		Result.Error = true;
		return Result;
	}
	Assert((ImageWidth * ImageHeight) % sizeof(pixel) == 0);
	if ((ImageWidth % 8 != 0) || (ImageHeight % 8 != 0))
	{
		fprintf(stderr, "ERROR: Image dimensions (%dx%d) do not split evenly into 8x8 tiles; please modify the image before proceeding.\n",
		        ImageWidth, ImageHeight);
		Result.Error = true;
		return Result;
	}

	pixel* Pixels = (pixel*)ImageData;

	tileset_image* OriginalImage = &Result.OriginalImage;
	OriginalImage->TileWidth = (u32)ImageWidth / 8;
	OriginalImage->TileHeight = (u32)ImageHeight / 8;
	OriginalImage->Tiles = (tile*)malloc(sizeof(tile) * OriginalImage->TileWidth * OriginalImage->TileHeight);

	// Copy pixels into tiles
	for (u32 TileY = 0; TileY < OriginalImage->TileHeight; TileY++)
	{
		for (u32 TileX = 0; TileX < OriginalImage->TileWidth; TileX++)
		{
			tile* Tile = TileAt(OriginalImage, TileX, TileY);
			Tile->EquivalentUniqueTile = nullptr;
			Tile->EqualAfterTransform = TileTransform_Unchanged;
			for (u32 PixelY = 0; PixelY < 8; PixelY++)
			{
				for (u32 PixelX = 0; PixelX < 8; PixelX++)
				{
					u32 PixelIndex = TileY * OriginalImage->TileWidth * 8 * 8 + PixelY * 8 * OriginalImage->TileWidth + TileX * 8 + PixelX;
					
					pixel* PixelToRead = Pixels + PixelIndex;
					pixel* PixelToWrite = PixelAt(Tile, PixelX, PixelY);

					*PixelToWrite = *PixelToRead;
				}
			}
		}
	}

	// Find all unique tiles
	Result.MinimisedTiles = (unique_tile*)malloc(sizeof(unique_tile) * OriginalImage->TileWidth * OriginalImage->TileHeight);
	unique_tile* MinimisedTiles = Result.MinimisedTiles;

	for (u32 TileY = 0; TileY < OriginalImage->TileHeight; TileY++)
	{
		for (u32 TileX = 0; TileX < OriginalImage->TileWidth; TileX++)
		{
			tile* Tile = TileAt(OriginalImage, TileX, TileY);

			b32 IsTileUnique = true;
			for (u32 UniqueTileIndex = 0; UniqueTileIndex < Result.NumUniqueTiles; UniqueTileIndex++)
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
				unique_tile* NewUniqueTile = MinimisedTiles + Result.NumUniqueTiles;
				GenerateTileVariants(Tile, NewUniqueTile);

				Tile->EquivalentUniqueTile = NewUniqueTile;
				Tile->EqualAfterTransform = TileTransform_Unchanged;

				Result.NumUniqueTiles++;
			}
		}
	}
	if (Result.NumUniqueTiles == OriginalImage->TileWidth * OriginalImage->TileHeight)
	{
		printf("Tileset '%s' is already minimised; nothing to do.\n", TilesetPath);
		Result.IsUnchanged = true;
		return Result;
	}

	// Work out most square-ish dimensions of output image so width and height both evenly divide NumUniqueTiles (no blank/wasted tiles)
	u32 OutputTileWidth = (u32)sqrt(Result.NumUniqueTiles) + 1;
	u32 OutputTileHeight = Result.NumUniqueTiles / OutputTileWidth;
	while (!(Result.NumUniqueTiles % OutputTileWidth == 0 && Result.NumUniqueTiles % OutputTileHeight == 0))
	{
		OutputTileWidth--;
		OutputTileHeight = Result.NumUniqueTiles / OutputTileWidth;
	}

	// Write back out minimised tileset image
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

	char NewName[PATH_MAX];
	*NewName = 0;
	if (JsonDoc.HasMember("name") && JsonDoc["name"].IsString())
	{
		const char* OriginalName = JsonDoc["name"].GetString();
		strcat(NewName, OriginalName);
		strcat(NewName, "_min");

		JsonDoc["name"].SetString(NewName, strlen(NewName), JsonDoc.GetAllocator());
	}
	else
	{
		printf("WARNING: Could not find 'name' field in tileset file %s.\n", TilesetPath);
	}

	if (JsonDoc.HasMember("imagewidth") && JsonDoc["imagewidth"].IsUint() &&
		JsonDoc.HasMember("imageheight") && JsonDoc["imageheight"].IsUint())
	{
		JsonDoc["imagewidth"].SetUint((u32)OutputImageWidth);
		JsonDoc["imageheight"].SetUint((u32)OutputImageHeight);
	}
	else
	{
		printf("WARNING: Could not find 'imagewidth'/'imageheight' field(s) in tileset file %s\n", TilesetPath);
	}
	if (JsonDoc.HasMember("tilecount") && JsonDoc["tilecount"].IsUint())
	{
		JsonDoc["tilecount"].SetUint(Result.NumUniqueTiles);
	}
	else
	{
		printf("WARNING: Could not find 'tilecount' field in tileset file %s\n", TilesetPath);
	}
	if (JsonDoc.HasMember("columns") && JsonDoc["columns"].IsUint())
	{
		JsonDoc["columns"].SetUint(OutputTileWidth);
	}
	else
	{
		printf("WARNING: Could not find 'columns' field in tileset file %s\n", TilesetPath);
	}


	char ImageOutPath[PATH_MAX];
	StripFileExtension(ImagePath, ImageOutPath);
	strcat(ImageOutPath, "_min.bmp");

	if (!stbi_write_bmp(ImageOutPath, OutputImageWidth, OutputImageHeight, 4, OutputPixels))
	{
		fprintf(stderr, "ERROR: Failed to write output image '%s'.\n", ImageOutPath);
		Result.Error = true;
		return Result;
	}

	JsonDoc["image"].SetString(ImageOutPath, strlen(ImageOutPath), JsonDoc.GetAllocator());

	if (CurrentWorkingDir)
	{
		// Change working directory back to where the .tmj file is
		if (!ChangeWorkingDir(CurrentWorkingDir))
		{
			Result.Error = true;
			return Result;
		}
	}
	AppendToFilePath(TilesetPath, "_min", OutNewTilesetPath);
	if (!WriteJsonToFile(&JsonDoc, OutNewTilesetPath, TilesetSpecStr.Size))
	{
		Result.Error = true;
		return Result;
	}

	u32 StartNumTiles = OriginalImage->TileWidth * OriginalImage->TileHeight;
	f32 Pst = roundf((((f32)StartNumTiles - (f32)Result.NumUniqueTiles) / (f32)StartNumTiles) * 100.0f);
	printf("Reduced number of tiles in '%s': %u->%u (-%.0f%%)\n", TilesetPath, StartNumTiles, Result.NumUniqueTiles, Pst);

	stbi_image_free(ImageData);
	return Result;
}