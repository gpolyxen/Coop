import os
import unreal

PROJECT_DIR = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
SOURCE_DIR = os.path.join(PROJECT_DIR, "Content", "ThirdPersonBP", "UI", "Generated", "Source")
DESTINATION = "/Game/ThirdPersonBP/UI/Generated"


def import_texture(filename, asset_name):
    task = unreal.AssetImportTask()
    task.filename = os.path.join(SOURCE_DIR, filename)
    task.destination_path = DESTINATION
    task.destination_name = asset_name
    task.automated = True
    task.replace_existing = True
    task.save = True
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    texture = unreal.EditorAssetLibrary.load_asset("{}/{}".format(DESTINATION, asset_name))
    if texture:
        texture.set_editor_property("srgb", True)
        unreal.EditorAssetLibrary.save_loaded_asset(texture)
        unreal.log("Imported inventory icon: {}".format(texture.get_path_name()))
    else:
        raise RuntimeError("Failed to import {}".format(filename))


import_texture("T_ItemPlank.png", "T_ItemPlank")
import_texture("T_ItemRope.png", "T_ItemRope")
