import os
import unreal

ROOT = os.path.abspath(os.path.join(unreal.Paths.project_dir(), "Intermediate", "WoodAxeImport"))
AXE_DEST = "/Game/ThirdPersonBP/Player_0/Weapon/WoodAxe"
ANIM_DEST = "/Game/ThirdPersonBP/Player_0/Anim/Two_Hands_weapon"
SKELETON = unreal.load_asset("/Game/ThirdPersonBP/Player_0/UE4_Mannequin/Mesh/UE4_Mannequin_Skeleton")


def run_task(filename, destination, name, options):
    task = unreal.AssetImportTask()
    task.filename = filename
    task.destination_path = destination
    task.destination_name = name
    task.automated = True
    task.replace_existing = True
    task.save = True
    task.options = options
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    unreal.log("IMPORT {} -> {} ({})".format(filename, name, task.imported_object_paths))
    return task.imported_object_paths


texture_assets = {}
texture_names = {
    "Material_Base_Color.png": "T_WoodAxe_BaseColor",
    "Material_Metallic.png": "T_WoodAxe_Metallic",
    "Material_Mixed_AO.png": "T_WoodAxe_AO",
    "Material_Normal.png": "T_WoodAxe_Normal",
    "Material_Roughness.png": "T_WoodAxe_Roughness",
}
for source_name, asset_name in texture_names.items():
    paths = run_task(os.path.join(ROOT, "textures", source_name), AXE_DEST, asset_name, None)
    if paths:
        texture_assets[asset_name] = unreal.load_asset(paths[0])

mesh_options = unreal.FbxImportUI()
mesh_options.import_as_skeletal = False
mesh_options.import_mesh = True
mesh_options.import_materials = False
mesh_options.import_textures = False
mesh_options.automated_import_should_detect_type = False
mesh_options.mesh_type_to_import = unreal.FBXImportType.FBXIT_STATIC_MESH
mesh_options.static_mesh_import_data.combine_meshes = True
run_task(os.path.join(ROOT, "source", "Axe.fbx"), AXE_DEST, "SM_WoodAxe", mesh_options)

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
material = unreal.load_asset(AXE_DEST + "/M_WoodAxeColored")
if not material:
    # UE 4.27 may assert while deleting expressions from a loaded/rooted material.
    # A dedicated project-owned material is safe to create, save and cook.
    material = asset_tools.create_asset("M_WoodAxeColored", AXE_DEST, unreal.Material, unreal.MaterialFactoryNew())


def connect_texture(texture, prop, x, y):
    if not texture:
        return
    node = None
    try:
        for expression in material.get_editor_property("expressions"):
            if isinstance(expression, unreal.MaterialExpressionTextureSample) and expression.texture == texture:
                node = expression
                break
    except Exception:
        node = None
    if not node:
        node = unreal.MaterialEditingLibrary.create_material_expression(material, unreal.MaterialExpressionTextureSample, x, y)
        node.texture = texture
    unreal.MaterialEditingLibrary.connect_material_property(node, "RGB", prop)


connect_texture(texture_assets.get("T_WoodAxe_BaseColor"), unreal.MaterialProperty.MP_BASE_COLOR, -600, -200)
connect_texture(texture_assets.get("T_WoodAxe_Normal"), unreal.MaterialProperty.MP_NORMAL, -600, 0)
connect_texture(texture_assets.get("T_WoodAxe_Roughness"), unreal.MaterialProperty.MP_ROUGHNESS, -600, 200)
connect_texture(texture_assets.get("T_WoodAxe_Metallic"), unreal.MaterialProperty.MP_METALLIC, -600, 400)
connect_texture(texture_assets.get("T_WoodAxe_AO"), unreal.MaterialProperty.MP_AMBIENT_OCCLUSION, -600, 600)
unreal.MaterialEditingLibrary.recompile_material(material)
unreal.EditorAssetLibrary.save_loaded_asset(material)
axe_mesh = unreal.load_asset(AXE_DEST + "/SM_WoodAxe")
if axe_mesh:
    axe_mesh.set_material(0, material)
    unreal.EditorAssetLibrary.save_loaded_asset(axe_mesh)

if not SKELETON:
    raise RuntimeError("UE4 mannequin skeleton was not found")

animation_sources = {
    "Axe_Attack_1": "Great Sword Slash.fbx",
    "Axe_Attack_2": "Stable Sword Inward Slash.fbx",
    "Axe_Attack_3": "Stable Sword Outward Slash.fbx",
    "TwoHand_Idle": "Great Sword Idle.fbx",
    "TwoHand_Jump": "Great Sword Jump.fbx",
    "TwoHand_Run": "Great Sword Run.fbx",
    "TwoHand_Strafe": "Great Sword Strafe.fbx",
    "TwoHand_Walk": "Great Sword Walk.fbx",
}
desktop = os.path.join(os.path.expanduser("~"), "Desktop")
for asset_name, source_name in animation_sources.items():
    options = unreal.FbxImportUI()
    options.import_as_skeletal = True
    options.import_mesh = False
    options.import_animations = True
    options.import_materials = False
    options.import_textures = False
    options.automated_import_should_detect_type = False
    options.mesh_type_to_import = unreal.FBXImportType.FBXIT_ANIMATION
    options.skeleton = SKELETON
    run_task(os.path.join(desktop, source_name), ANIM_DEST, asset_name, options)

unreal.EditorAssetLibrary.save_directory(AXE_DEST, only_if_is_dirty=False, recursive=True)
unreal.EditorAssetLibrary.save_directory(ANIM_DEST, only_if_is_dirty=False, recursive=True)
unreal.log("WOOD AXE IMPORT COMPLETE")
