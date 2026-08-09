import json
import os
import re

import unreal


PROJECT_ROOT = os.path.abspath(os.path.join(unreal.Paths.project_dir()))
SOURCE_ROOT = os.path.join(PROJECT_ROOT, "SourceArt", "P9MannyFPS")
FBX_ROOT = os.path.join(SOURCE_ROOT, "fbx")
TEXTURE_ROOT = os.path.join(SOURCE_ROOT, "textures")
MANIFEST_FILE = os.path.join(FBX_ROOT, "P9MannyImportManifest.json")

ROOT_DESTINATION = "/Game/P9MannyFPS"
MESH_DESTINATION = ROOT_DESTINATION + "/Meshes"
ANIMATION_DESTINATION = ROOT_DESTINATION + "/Animations"
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
    result = []
    for task in tasks:
        result.extend(task.get_editor_property("imported_object_paths"))
    return result


def skeletal_mesh_options():
    options = unreal.FbxImportUI()
    options.set_editor_property("automated_import_should_detect_type", False)
    options.set_editor_property("mesh_type_to_import", unreal.FBXImportType.FBXIT_SKELETAL_MESH)
    options.set_editor_property("original_import_type", unreal.FBXImportType.FBXIT_SKELETAL_MESH)
    options.set_editor_property("import_as_skeletal", True)
    options.set_editor_property("import_mesh", True)
    options.set_editor_property("import_animations", False)
    options.set_editor_property("import_materials", True)
    options.set_editor_property("import_textures", False)
    options.set_editor_property("create_physics_asset", False)

    data = options.get_editor_property("skeletal_mesh_import_data")
    data.set_editor_property("import_meshes_in_bone_hierarchy", True)
    data.set_editor_property("import_morph_targets", False)
    data.set_editor_property("preserve_smoothing_groups", True)
    return options


def static_mesh_options():
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
    data.set_editor_property("combine_meshes", True)
    data.set_editor_property("generate_lightmap_u_vs", True)
    return options


def animation_options(skeleton):
    options = unreal.FbxImportUI()
    options.set_editor_property("automated_import_should_detect_type", False)
    options.set_editor_property("mesh_type_to_import", unreal.FBXImportType.FBXIT_ANIMATION)
    options.set_editor_property("original_import_type", unreal.FBXImportType.FBXIT_ANIMATION)
    options.set_editor_property("import_as_skeletal", True)
    options.set_editor_property("import_mesh", False)
    options.set_editor_property("import_animations", True)
    options.set_editor_property("import_materials", False)
    options.set_editor_property("import_textures", False)
    options.set_editor_property("skeleton", skeleton)

    data = options.get_editor_property("anim_sequence_import_data")
    data.set_editor_property("animation_length", unreal.FBXAnimationLengthImportType.FBXALIT_EXPORTED_TIME)
    data.set_editor_property("use_default_sample_rate", False)
    data.set_editor_property("custom_sample_rate", 24)
    data.set_editor_property("remove_redundant_keys", False)
    return options


def load_texture(asset_name):
    texture = unreal.load_asset(TEXTURE_DESTINATION + "/" + asset_name)
    if not texture:
        raise RuntimeError("Missing P9 texture: " + asset_name)
    return texture


def configure_texture(texture, is_normal=False, is_mask=False):
    texture.set_editor_property("srgb", not (is_normal or is_mask))
    if is_normal:
        texture.set_editor_property(
            "compression_settings", unreal.TextureCompressionSettings.TC_NORMALMAP
        )
    elif is_mask:
        texture.set_editor_property(
            "compression_settings", unreal.TextureCompressionSettings.TC_MASKS
        )
    texture.modify()


def add_texture_sample(material, texture, x, y, sampler_type=None):
    node = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionTextureSample, x, y
    )
    node.set_editor_property("texture", texture)
    if sampler_type is not None:
        node.set_editor_property("sampler_type", sampler_type)
    return node


def build_material(
    material_name,
    base_color_name,
    normal_name,
    metallic_name,
    roughness_name,
    metallic_output="R",
    roughness_output="R",
):
    material = unreal.load_asset(MESH_DESTINATION + "/" + material_name)
    if not material:
        raise RuntimeError("Missing imported P9 material: " + material_name)

    base_color = load_texture(base_color_name)
    normal = load_texture(normal_name)
    metallic = load_texture(metallic_name)
    roughness = load_texture(roughness_name)
    configure_texture(base_color)
    configure_texture(normal, is_normal=True)
    configure_texture(metallic, is_mask=True)
    configure_texture(roughness, is_mask=True)

    unreal.MaterialEditingLibrary.delete_all_material_expressions(material)
    material.set_editor_property("two_sided", True)
    base_node = add_texture_sample(material, base_color, -640, -160)
    normal_node = add_texture_sample(
        material,
        normal,
        -640,
        80,
        unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL,
    )
    metallic_node = add_texture_sample(material, metallic, -640, 320)
    roughness_node = (
        metallic_node
        if roughness == metallic
        else add_texture_sample(material, roughness, -640, 520)
    )
    unreal.MaterialEditingLibrary.connect_material_property(
        base_node, "RGB", unreal.MaterialProperty.MP_BASE_COLOR
    )
    unreal.MaterialEditingLibrary.connect_material_property(
        normal_node, "RGB", unreal.MaterialProperty.MP_NORMAL
    )
    unreal.MaterialEditingLibrary.connect_material_property(
        metallic_node, metallic_output, unreal.MaterialProperty.MP_METALLIC
    )
    unreal.MaterialEditingLibrary.connect_material_property(
        roughness_node, roughness_output, unreal.MaterialProperty.MP_ROUGHNESS
    )
    unreal.MaterialEditingLibrary.recompile_material(material)
    unreal.EditorAssetLibrary.save_loaded_asset(material, only_if_is_dirty=False)


if not os.path.isfile(MANIFEST_FILE):
    raise RuntimeError("Missing conversion manifest: " + MANIFEST_FILE)

with open(MANIFEST_FILE, "r", encoding="utf-8") as manifest_stream:
    manifest = json.load(manifest_stream)

texture_tasks = []
for texture_file in sorted(os.listdir(TEXTURE_ROOT)):
    source_file = os.path.join(TEXTURE_ROOT, texture_file)
    if os.path.isfile(source_file):
        texture_tasks.append(
            make_task(source_file, TEXTURE_DESTINATION, safe_name(texture_file), None)
        )
texture_assets = import_tasks(texture_tasks)

# Clean up only the two known static-mesh artifacts produced by an interrupted
# skeletal import. They are generated files from this package, never user assets.
for accidental_asset in (
    MESH_DESTINATION + "/SK_P9_MannyFPS_SK_P9_MannyFPS_Arms",
    MESH_DESTINATION + "/SK_P9_MannyFPS_SK_P9_MannyFPS_Pistol",
):
    if unreal.EditorAssetLibrary.does_asset_exist(accidental_asset):
        unreal.EditorAssetLibrary.delete_asset(accidental_asset)

skeletal_source = os.path.join(FBX_ROOT, manifest["skeletal_mesh"])
skeletal_task = make_task(
    skeletal_source,
    MESH_DESTINATION,
    "SK_P9_MannyFPS",
    skeletal_mesh_options(),
)
skeletal_assets = import_tasks([skeletal_task])

skeletal_mesh = unreal.load_asset(MESH_DESTINATION + "/SK_P9_MannyFPS")
if not skeletal_mesh:
    raise RuntimeError("The P9 Manny skeletal mesh was not imported")
skeleton = skeletal_mesh.get_editor_property("skeleton")
if not skeleton:
    raise RuntimeError("The P9 Manny skeleton was not created")

static_source = os.path.join(FBX_ROOT, manifest["static_pistol"])
static_task = make_task(
    static_source,
    MESH_DESTINATION,
    "SM_P9",
    static_mesh_options(),
)
static_assets = import_tasks([static_task])

animation_tasks = []
for animation in manifest["animations"]:
    source_file = os.path.join(FBX_ROOT, animation["file"])
    asset_name = "A_P9_" + animation["name"]
    animation_tasks.append(
        make_task(
            source_file,
            ANIMATION_DESTINATION,
            asset_name,
            animation_options(skeleton),
        )
    )
animation_assets = import_tasks(animation_tasks)

build_material(
    "MI_Manny_01",
    "T_Manny_01_D_TGA_1",
    "T_Manny_01_N_TGA_0",
    "T_Manny_01_MSR_MSK_TGA_2_channels_B",
    "T_Manny_01_MSR_MSK_TGA_2_channels_G",
)
build_material(
    "MI_Manny_02",
    "T_Manny_02_D_TGA_4",
    "T_Manny_02_N_TGA_3",
    "T_Manny_02_MSR_MSK_TGA_5_channels_B",
    "T_Manny_02_MSR_MSK_TGA_5_channels_G",
)
build_material(
    "p220",
    "minebea_p9_BaseColor_7",
    "minebea_p9_Normal_opengl_6",
    "minebea_p9_Metallic_minebea_p9_Roughness_8_channels_B",
    "minebea_p9_Metallic_minebea_p9_Roughness_8_channels_G",
)
build_material(
    "p220_mag",
    "minebea_p9_mag_BaseColor_10",
    "minebea_p9_mag_Normal_opengl_9",
    "minebea_p9_mag_Metallic_minebea_p9_mag_Roughness_11_channels",
    "minebea_p9_mag_Metallic_minebea_p9_mag_Roughness_11_channels",
    metallic_output="B",
    roughness_output="G",
)

static_pistol = unreal.load_asset(MESH_DESTINATION + "/SM_P9")
if static_pistol:
    static_pistol.set_material(0, unreal.load_asset(MESH_DESTINATION + "/p220"))
    static_pistol.set_material(1, unreal.load_asset(MESH_DESTINATION + "/p220_mag"))
    unreal.EditorAssetLibrary.save_loaded_asset(static_pistol, only_if_is_dirty=False)

unreal.EditorAssetLibrary.save_directory(ROOT_DESTINATION, only_if_is_dirty=False, recursive=True)
unreal.log("P9 Manny FPS package imported successfully")
unreal.log("Skeletal assets: " + repr(skeletal_assets))
unreal.log("Static assets: " + repr(static_assets))
unreal.log("Animation assets: " + repr(animation_assets))
unreal.log("Texture assets: " + repr(texture_assets))
