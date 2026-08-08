import os
import unreal

SOURCE_WORLD = "/Game/OpenWorld/OpenWorld_EmptyStreamingBackup"
DESTINATION_WORLD = "/Game/OpenWorld/OpenWorld"
BACKUP_BASE = "/Game/OpenWorld/OpenWorld_LegacySceneBackup"

def map_file(asset_path):
    relative_path = asset_path.replace("/Game/", "", 1).replace("/", os.sep)
    return os.path.join(unreal.Paths.project_content_dir(), relative_path + ".umap")

def next_available_backup():
    candidate = BACKUP_BASE
    suffix = 2
    while os.path.isfile(map_file(candidate)):
        candidate = "{}_{}".format(BACKUP_BASE, suffix)
        suffix += 1
    return candidate

if not os.path.isfile(map_file(SOURCE_WORLD)):
    raise RuntimeError("Streaming-world backup does not exist: " + SOURCE_WORLD)

backup_world = next_available_backup()
current_world = unreal.EditorLoadingAndSavingUtils.load_map(DESTINATION_WORLD)
if not current_world:
    raise RuntimeError("Could not load current OpenWorld for backup")
if not unreal.EditorLoadingAndSavingUtils.save_map(current_world, backup_world):
    raise RuntimeError("Could not preserve legacy scene as " + backup_world)
unreal.log("Preserved legacy scene as: " + backup_world)

streaming_world = unreal.EditorLoadingAndSavingUtils.load_map(SOURCE_WORLD)
if not streaming_world:
    raise RuntimeError("Could not load streaming-world backup")

shooter_game_mode = unreal.load_class(None, "/Script/MyProject.ShooterGameMode")
if not shooter_game_mode:
    raise RuntimeError("Could not load native ShooterGameMode")
streaming_world.get_world_settings().set_editor_property("default_game_mode", shooter_game_mode)

if not unreal.EditorLoadingAndSavingUtils.save_map(streaming_world, DESTINATION_WORLD):
    raise RuntimeError("Could not restore lightweight streaming OpenWorld")
unreal.log("Restored lightweight grass-and-rock streaming world: " + DESTINATION_WORLD)

