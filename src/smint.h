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

struct tile
{
	pixel Pixels[8 * 8];
};

pixel* PixelAt(tile* Tile, u32 X, u32 Y)
{
	Assert(X < 8 && Y < 8);
	pixel* Result = Tile->Pixels + Y * 8 + X;
	return Result;
}

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