# smint
The smart tilemap image minifier for [Tiled](https://www.mapeditor.org/) & GBA.

### Intro
smint takes in a Tiled tilemap file (.tmj) and produces an identical output map, but with every tileset image in the map minimised to take up the least possible space on GBA. The minification removes all duplicate tiles (inclusive of HFLIP and/or VFLIP), but leaves the final map looking identical.

This way, you can download a nice-looking tileset off the internet and build a map with it directly,
without having to first minimise the tiles into an ugly mess which might fit in VRAM, but is annoying to work with from a level design perspective.

Suggested to use alongside the [tiled-to-gba-export plugin](https://github.com/djedditt/tiled-to-gba-export).

### Example usage

```sh
./smint ./CastleMap.tmj

Reduced number of tiles in 'castle.tsj': 2312->526 (-77%)
Wrote minimised tile image to 'castle_min.png'.

Map 'CastleMap.tmj' successfully minimised to 'CastleMap_min.tmj'.
```

![demo_image](https://i.imgur.com/UcV3uVw.png)
*Tileset pictured is by Jason Perry from [timefantasy.net](usage_demo.png)*.

smint parses the map file (.tmj), and finds all tileset (.tsj) files pointed to by it, and each image file pointed to in turn by the tilesets. Each image is scanned for duplicate 8x8 tiles, and an output image (always .png) is produced where all tiles are unique. A new tileset file is created that points to the new image. Finally, a new map file is created that updates all tileset paths and Tile IDs to correspond to the new minimised tilesets.

Any tileset in the input map that is already minimal (i.e. contains no duplicate tiles) is left untouched, and if this is the case for all tilesets in the map, no output map is produced.
### Download
There's a Windows x86_64 binary on the [releases](https://github.com/colonelsalt/smint/releases) page.

### Building

#### Windows
Run `build.bat` in a shell initialised with the VS build environment (e.g. Developer Powershell for VS).

#### Unix-like
```sh
chmod +x ./build.sh
./build.sh
```

Unix support has been much less tested, but seems to run fine in my WSL1 environment.

### Limitations
- Only supports JSON-based Tiled tilemaps (.tmj) and tilesets (.tsj) at the moment.
- Only supports path names up to Windows's default MAX_PATH of 260 characters.
- If an image contains an alpha channel, this is ignored when checking for tile duplicates, but will still be present in the output image. (GBA does not support alpha so seemed like a sensible solution, but might be worth keeping in mind.)
### Libraries used
- [stb_image](https://github.com/nothings/stb)
- [rapidjson](https://github.com/Tencent/rapidjson)
