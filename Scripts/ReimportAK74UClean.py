import os

import unreal


project_root = os.path.abspath(unreal.Paths.project_dir())
source_file = os.path.join(
    project_root, "SourceArt", "AK74UFree", "source", "AK74U_UE4_Clean.fbx"
)
if not os.path.isfile(source_file):
    raise RuntimeError("Missing cleaned AK74U FBX: " + source_file)

options = unreal.FbxImportUI()
options.set_editor_property("automated_import_should_detect_type", False)
options.set_editor_property("mesh_type_to_import", unreal.FBXImportType.FBXIT_SKELETAL_MESH)
options.set_editor_property("original_import_type", unreal.FBXImportType.FBXIT_SKELETAL_MESH)
options.set_editor_property("import_as_skeletal", True)
options.set_editor_property("import_mesh", True)
options.set_editor_property("import_animations", True)
options.set_editor_property("import_materials", True)
options.set_editor_property("import_textures", False)
options.set_editor_property("create_physics_asset", False)

mesh_data = options.get_editor_property("skeletal_mesh_import_data")
mesh_data.set_editor_property("import_meshes_in_bone_hierarchy", False)
mesh_data.set_editor_property("import_morph_targets", False)
mesh_data.set_editor_property("preserve_smoothing_groups", True)

anim_data = options.get_editor_property("anim_sequence_import_data")
anim_data.set_editor_property(
    "animation_length", unreal.FBXAnimationLengthImportType.FBXALIT_EXPORTED_TIME
)
anim_data.set_editor_property("use_default_sample_rate", False)
anim_data.set_editor_property("custom_sample_rate", 30)
anim_data.set_editor_property("remove_redundant_keys", False)

task = unreal.AssetImportTask()
task.set_editor_property("automated", True)
task.set_editor_property("filename", source_file)
task.set_editor_property("destination_path", "/Game/AK74UFree/FPS")
task.set_editor_property("destination_name", "SK_AK74U_FPS")
task.set_editor_property("replace_existing", True)
task.set_editor_property("save", True)
task.set_editor_property("options", options)

unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
unreal.EditorAssetLibrary.save_directory(
    "/Game/AK74UFree/FPS", only_if_is_dirty=False, recursive=True
)
unreal.log("AK74U_CLEAN_REIMPORT_OK: " + repr(task.get_editor_property("imported_object_paths")))
