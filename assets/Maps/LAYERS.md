# Map layers and their tilesets

Each map is a folder of CSVs, one per layer: `<map>/<map>_<Layer>.csv`.
Every layer uses exactly **one** tileset. Values are 0-based tile IDs inside that PNG,
cut 16x16 from the top-left (`columns = width / 16`, `id = row * columns + col`). `-1` = empty.

Layers are listed in draw order (first = bottom). Draw entities/player where marked.
`Collision` is never drawn: any value other than `-1` is non-walkable.

All tilesets live in `assets/Backgrounds/Tilesets/`.
Each map folder also has `<map>_preview.png`: a crisp 2x render (32 px per tile) without the collision layer.

## camino_sur (41 x 60)

| # | Layer | Tileset | Columns |
|---|---|---|---|
| 1 | Ground | `TilesetFloor.png` | 22 |
| 2 | Decor | `TilesetFloorDetail.png` | 16 |
| 3 | Props | `TilesetElement.png` | 16 |
| 4 | Trees | `TilesetNature.png` | 24 |
| — | *entities / player* | | |
| 5 | AbovePlayer | `TilesetNature.png` | 24 |
| 6 | Collision | `TilesetLogic.png` | 8 |

Spawn: tile (20, 51). Exit to Valdoria: north edge, road at x = 19..21.

## valdoria_plaza (91 x 90)

| # | Layer | Tileset | Columns |
|---|---|---|---|
| 1 | Ground | `TilesetFloor.png` | 22 |
| 2 | Decor | `TilesetFloorDetail.png` | 16 |
| 3 | Water | `TilesetWater.png` | 28 |
| 4 | Props | `TilesetElement.png` | 16 |
| 5 | Buildings | `TilesetHouse.png` | 33 |
| 6 | Ruins | `TilesetVillageAbandoned.png` | 20 |
| 7 | Trees | `TilesetNature.png` | 24 |
| — | *entities / player* | | |
| 8 | BuildingsAbove | `TilesetHouse.png` | 33 |
| 9 | RuinsAbove | `TilesetVillageAbandoned.png` | 20 |
| 10 | AbovePlayer | `TilesetNature.png` | 24 |
| 11 | Collision | `TilesetLogic.png` | 8 |

Entry from camino_sur: south edge, road at x = 44..46. West/east roads are blocked for now.

## Unique tilesets to load

`TilesetFloor`, `TilesetFloorDetail`, `TilesetElement`, `TilesetNature`, `TilesetLogic`
(both maps), plus `TilesetWater`, `TilesetHouse`, `TilesetVillageAbandoned` (Valdoria).
`TilesetLogic` is only needed if you want to draw collision in debug mode.

Note: `TilesetFloor.png` is 417 px tall, so its last pixel row is ignored (26 rows).
