#pragma once

struct pixel
{
	u8 R;
	u8 G;
	u8 B;
	u8 A; // Unused - only here for padding

	b32 operator==(pixel Other)
	{
		b32 Result = Other.R == R && Other.G == G && Other.B == B;
		return Result;
	}

	b32 operator!=(pixel Other)
	{
		b32 Result = !(Other == *this);
		return Result;
	}
};

struct bitmap_image
{
	s32 Width;
	s32 Height;
	pixel* Pixels;
};

enum tile_transform_type : u32
{
	Transform_Unchanged,
	Transform_HFlip,
	Transform_VFlip,
	Transform_DiagonalFlip, // i.e. both HFLIP and VFLIP

	Transform_Count
};

struct tile
{
	pixel Pixels[8 * 8];
	tile* EquivalentUniqueTile;
	tile_transform_type EqualAfterTransform; // which transform you need to apply to make this tile equal to the above
};

pixel* PixelAt(tile* Tile, u32 X, u32 Y)
{
	Assert(X < 8 && Y < 8);
	pixel* Result = Tile->Pixels + Y * 8 + X;
	return Result;
}

struct tile_variant
{
	tile HFlipped;
	tile VFlipped;
	tile DiagonalFlipped;
};

struct tileset_image
{
	u32 TileWidth;
	u32 TileHeight;
	tile* Tiles;
};

tile* TileAt(tileset_image* Image, u32 X, u32 Y)
{
	Assert(X < Image->TileWidth && Y < Image->TileHeight);
	tile* Result = Image->Tiles + Y * Image->TileWidth + X;
	return Result;
}