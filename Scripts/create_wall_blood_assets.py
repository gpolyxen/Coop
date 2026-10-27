import unreal

SOURCE = r"C:\Program Files\Epic Games\UE_4.27\Coop 4.27 - 3\Content\ThirdPersonBP\VFX\Blood\Source\T_BloodWallSplatter_Atlas.png"
DEST = "/Game/ThirdPersonBP/VFX/Blood"

tools = unreal.AssetToolsHelpers.get_asset_tools()
task = unreal.AssetImportTask()
task.filename = SOURCE
task.destination_path = DEST
task.destination_name = "T_BloodWallSplatter_Atlas"
task.automated = True
task.replace_existing = True
task.save = True
tools.import_asset_tasks([task])

texture = unreal.load_asset(DEST + "/T_BloodWallSplatter_Atlas")
if not texture:
    raise RuntimeError("Wall blood atlas import failed")
texture.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_DEFAULT)
texture.set_editor_property("srgb", True)
texture.set_editor_property("filter", unreal.TextureFilter.TF_TRILINEAR)

material = unreal.load_asset(DEST + "/M_BloodWallSplatter_Decal")
if not material:
    material = tools.create_asset("M_BloodWallSplatter_Decal", DEST, unreal.Material, unreal.MaterialFactoryNew())
material.set_editor_property("material_domain", unreal.MaterialDomain.MD_DEFERRED_DECAL)
material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
try:
    material.set_editor_property("decal_blend_mode", unreal.DecalBlendMode.DBM_TRANSLUCENT)
except Exception:
    pass

mel = unreal.MaterialEditingLibrary
uv = mel.create_material_expression(material, unreal.MaterialExpressionTextureCoordinate, -700, 0)
uv.set_editor_property("u_tiling", 0.25)
uv.set_editor_property("v_tiling", 0.25)
offset = mel.create_material_expression(material, unreal.MaterialExpressionVectorParameter, -700, 170)
offset.set_editor_property("parameter_name", "AtlasOffset")
offset.set_editor_property("default_value", unreal.LinearColor(0.0, 0.0, 0.0, 0.0))
add = mel.create_material_expression(material, unreal.MaterialExpressionAdd, -440, 20)
sample = mel.create_material_expression(material, unreal.MaterialExpressionTextureSample, -190, 0)
sample.set_editor_property("texture", texture)
rough = mel.create_material_expression(material, unreal.MaterialExpressionScalarParameter, -180, 240)
rough.set_editor_property("parameter_name", "BloodRoughness")
rough.set_editor_property("default_value", 0.3)
mel.connect_material_expressions(uv, "", add, "A")
mel.connect_material_expressions(offset, "", add, "B")
mel.connect_material_expressions(add, "", sample, "Coordinates")
mel.connect_material_property(sample, "RGB", unreal.MaterialProperty.MP_BASE_COLOR)
mel.connect_material_property(sample, "A", unreal.MaterialProperty.MP_OPACITY)
mel.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
mel.recompile_material(material)
unreal.EditorAssetLibrary.save_loaded_asset(texture)
unreal.EditorAssetLibrary.save_loaded_asset(material)
unreal.log("CODEX_WALL_BLOOD_ASSETS_READY")
