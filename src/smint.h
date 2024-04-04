#pragma once

struct pixel
{
	u8 R;
	u8 G;
	u8 B;
	u8 A; // Unused - only here for padding
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

struct image
{
	u32 TileWidth;
	u32 TileHeight;
	tile* Tiles;
};

tile* TileAt(image* Image, u32 X, u32 Y)
{
	Assert(X < Image->TileWidth && Y < Image->TileHeight);
	tile* Result = Image->Tiles + Y * Image->TileWidth + X;
	return Result;
}
