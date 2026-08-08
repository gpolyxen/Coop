import unreal
import os

TILE_LEVEL = "/Game/OpenWorld/Tiles/OpenWorldTile"
WORLD_LEVEL = "/Game/OpenWorld/OpenWorld"


tile_file = os.path.join(unreal.Paths.project_content_dir(), "OpenWorld", "Tiles", "OpenWorldTile.umap")
world_file = os.path.join(unreal.Paths.project_content_dir(), "OpenWorld", "OpenWorld.umap")

if not os.path.isfile(tile_file):
    if not unreal.EditorLevelLibrary.new_level(TILE_LEVEL):
        raise RuntimeError("Could not create level: " + TILE_LEVEL)

if os.path.isfile(world_file):
    raise RuntimeError("Persistent open-world level already exists and will not be overwritten")

if not unreal.EditorLevelLibrary.new_level(WORLD_LEVEL):
    raise RuntimeError("Could not create the persistent open-world level")
unreal.log("Created streaming open world: " + WORLD_LEVEL)
unreal.log("Streaming tile template: " + TILE_LEVEL)
