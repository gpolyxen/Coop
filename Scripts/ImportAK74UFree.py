import os
import re

import unreal


PROJECT_ROOT = os.path.abspath(unreal.Paths.project_dir())
SOURCE_ROOT = os.path.join(PROJECT_ROOT, "SourceArt", "AK74UFree")
FBX_FILE = os.path.join(SOURCE_ROOT, "source", "AK74U_UE4.fbx")
TEXTURE_ROOT = os.path.join(SOURCE_ROOT, "textures")

ROOT_DESTINATION = "/Game/AK74UFree"
FPS_DESTINATION = ROOT_DESTINATION + "/FPS"
WORLD_DESTINATION = ROOT_DESTINATION + "/WorldParts"
TEXTURE_DESTINATION = ROOT_DESTINATION + "/Textures"


def safe_name(filename):
    stem = os.path.splitext(os.path.basename(filename))[0]
    return re.sub(r"[^A-Za-z0-9_]+", "_", stem).strip("_")


def make_task(filename, destination_path, destination_name, options):
    task = unreal.AssetImportTask()
    task.set_editor_property("automated", True)
    task.set_editor_property("filename", filename)
    task.set_editor_property("destination_path", destination_path)
    task.set_editor_property("destination_name", destination_name)
    task.set_editor_property("replace_existing", False)
    task.set_editor_property("save", True)
    if options:
        task.set_editor_property("options", options)
    return task


def import_tasks(tasks):
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
    imported = []
    for task in tasks:
        imported.extend(task.get_editor_property("imported_object_paths"))
    return imported


def skeletal_options():
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
    # The source includes weapon/helper meshes under the armature. Treating those
    # meshes as bones gives UE4 several apparent roots, so keep only real joints.
    mesh_data.set_editor_property("import_meshes_in_bone_hierarchy", False)
    mesh_data.set_editor_property("import_morph_targets", False)
    mesh_data.set_editor_property("preserve_smoothing_groups", True)

    anim_data = options.get_editor_property("anim_sequence_import_data")
    anim_data.set_editor_property("animation_length", unreal.FBXAnimationLengthImportType.FBXALIT_EXPORTED_TIME)
    anim_data.set_editor_property("use_default_sample_rate", False)
    anim_data.set_editor_property("custom_sample_rate", 30)
    anim_data.set_editor_property("remove_redundant_keys", False)
    return options


def static_options():
    options = unreal.FbxImportUI()
    options.set_editor_property("automated_import_should_detect_type", False)
    options.set_editor_property("mesh_type_to_import", unreal.FBXImportType.FBXIT_STATIC_MESH)
    options.set_editor_property("original_import_type", unreal.FBXImportType.FBXIT_STATIC_MESH)
    options.set_editor_property("import_as_skeletal", False)
    options.set_editor_property("import_mesh", True)
    options.set_editor_property("import_animations", False)
    options.set_editor_property("import_materials", False)
    options.set_editor_property("import_textures", False)
    data = options.get_editor_property("static_mesh_import_data")
    data.set_editor_property("combine_meshes", False)
    data.set_editor_property("generate_lightmap_u_vs", True)
    data.set_editor_property("auto_generate_collision", False)
    return options


if not os.path.isfile(FBX_FILE):
    raise RuntimeError("Missing AK74U FBX: " + FBX_FILE)

texture_tasks = []
for texture_file in sorted(os.listdir(TEXTURE_ROOT)):
    source_file = os.path.join(TEXTURE_ROOT, texture_file)
    if os.path.isfile(source_file):
        texture_tasks.append(make_task(source_file, TEXTURE_DESTINATION, safe_name(texture_file), None))

texture_assets = import_tasks(texture_tasks)
fps_assets = import_tasks([
    make_task(FBX_FILE, FPS_DESTINATION, "SK_AK74U_FPS", skeletal_options())
])
world_assets = import_tasks([
    make_task(FBX_FILE, WORLD_DESTINATION, "AK74U", static_options())
])

unreal.EditorAssetLibrary.save_directory(ROOT_DESTINATION, only_if_is_dirty=False, recursive=True)
unreal.log("AK74U_FREE_IMPORT_OK")
unreal.log("FPS assets: " + repr(fps_assets))
unreal.log("World-part assets: " + repr(world_assets))
unreal.log("Texture assets: " + repr(texture_assets))
