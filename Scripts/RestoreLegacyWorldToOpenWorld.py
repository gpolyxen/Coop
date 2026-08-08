import os
import unreal

SOURCE_WORLD = "/Game/ThirdPersonBP/Maps/ThirdPersonExampleMap"
DESTINATION_WORLD = "/Game/OpenWorld/OpenWorld"
BACKUP_BASE = "/Game/OpenWorld/OpenWorld_EmptyStreamingBackup"

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

backup_world = next_available_backup()
current_open_world = unreal.EditorLoadingAndSavingUtils.load_map(DESTINATION_WORLD)
if not current_open_world:
    raise RuntimeError("Could not load current open-world map for backup")
if not unreal.EditorLoadingAndSavingUtils.save_map(current_open_world, backup_world):
    raise RuntimeError("Could not preserve current open-world map as " + backup_world)
unreal.log("Preserved empty streaming map as: " + backup_world)

legacy_world = unreal.EditorLoadingAndSavingUtils.load_map(SOURCE_WORLD)
if not legacy_world:
    raise RuntimeError("Could not load source world: " + SOURCE_WORLD)

world_settings = legacy_world.get_world_settings()
shooter_game_mode = unreal.load_class(None, "/Script/MyProject.ShooterGameMode")
if not shooter_game_mode:
    raise RuntimeError("Could not load the native ShooterGameMode class")
world_settings.set_editor_property("default_game_mode", shooter_game_mode)

if not unreal.EditorLoadingAndSavingUtils.save_map(legacy_world, DESTINATION_WORLD):
    raise RuntimeError("Could not save the restored open-world map")

actors = unreal.EditorLevelLibrary.get_all_level_actors()
class_counts = {}
for actor in actors:
    class_name = actor.get_class().get_name()
    class_counts[class_name] = class_counts.get(class_name, 0) + 1

unreal.log("Restored legacy scene into: " + DESTINATION_WORLD)
unreal.log("Restored actor count: {}".format(len(actors)))
for class_name in sorted(class_counts):
    lowered = class_name.lower()
    if "bot" in lowered or "weapon" in lowered or "ammo" in lowered or "playerstart" in lowered:
        unreal.log("Restored gameplay actors: {} x {}".format(class_name, class_counts[class_name]))

