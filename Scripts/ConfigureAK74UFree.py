import unreal


ROOT = "/Game/AK74UFree"
MATERIAL_ROOT = ROOT + "/FPS"
TEXTURE_ROOT = ROOT + "/Textures"


def load(path):
    asset = unreal.load_asset(path)
    if not asset:
        raise RuntimeError("Missing AK74U asset: " + path)
    return asset


def texture(name, normal=False, mask=False):
    asset = load(TEXTURE_ROOT + "/" + name)
    asset.set_editor_property("srgb", not (normal or mask))
    if normal:
        asset.set_editor_property(
            "compression_settings", unreal.TextureCompressionSettings.TC_NORMALMAP
        )
    elif mask:
        asset.set_editor_property(
            "compression_settings", unreal.TextureCompressionSettings.TC_MASKS
        )
    asset.modify()
    unreal.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False)
    return asset


def sample(material, source, x, y, sampler=None):
    node = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionTextureSample, x, y
    )
    node.set_editor_property("texture", source)
    if sampler is not None:
        node.set_editor_property("sampler_type", sampler)
    return node


def rebuild_weapon_material(name, color_name, normal_name, metal_name, rough_name, ao_name=None):
    material = load(MATERIAL_ROOT + "/" + name)
    if unreal.MaterialEditingLibrary.get_material_property_input_node(
        material, unreal.MaterialProperty.MP_BASE_COLOR
    ):
        return material
    color = texture(color_name)
    normal = texture(normal_name, normal=True)
    metal = texture(metal_name, mask=True)
    rough = texture(rough_name, mask=True)
    ao = texture(ao_name, mask=True) if ao_name else None

    unreal.MaterialEditingLibrary.delete_all_material_expressions(material)
    base_node = sample(material, color, -620, -220)
    normal_node = sample(
        material, normal, -620, 20, unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL
    )
    metal_node = sample(material, metal, -620, 260)
    rough_node = sample(material, rough, -620, 460)
    unreal.MaterialEditingLibrary.connect_material_property(
        base_node, "RGB", unreal.MaterialProperty.MP_BASE_COLOR
    )
    unreal.MaterialEditingLibrary.connect_material_property(
        normal_node, "RGB", unreal.MaterialProperty.MP_NORMAL
    )
    unreal.MaterialEditingLibrary.connect_material_property(
        metal_node, "R", unreal.MaterialProperty.MP_METALLIC
    )
    unreal.MaterialEditingLibrary.connect_material_property(
        rough_node, "R", unreal.MaterialProperty.MP_ROUGHNESS
    )
    if ao:
        ao_node = sample(material, ao, -620, 660)
        unreal.MaterialEditingLibrary.connect_material_property(
            ao_node, "R", unreal.MaterialProperty.MP_AMBIENT_OCCLUSION
        )
    unreal.MaterialEditingLibrary.recompile_material(material)
    unreal.EditorAssetLibrary.save_loaded_asset(material, only_if_is_dirty=False)
    return material


def rebuild_character_material(name, color_name, normal_name, gloss_name):
    material = load(MATERIAL_ROOT + "/" + name)
    if unreal.MaterialEditingLibrary.get_material_property_input_node(
        material, unreal.MaterialProperty.MP_BASE_COLOR
    ):
        return material
    color = texture(color_name)
    normal = texture(normal_name, normal=True)
    gloss = texture(gloss_name, mask=True)

    unreal.MaterialEditingLibrary.delete_all_material_expressions(material)
    material.set_editor_property("two_sided", True)
    base_node = sample(material, color, -620, -180)
    normal_node = sample(
        material, normal, -620, 80, unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL
    )
    gloss_node = sample(material, gloss, -620, 340)
    one_minus = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionOneMinus, -330, 340
    )
    unreal.MaterialEditingLibrary.connect_material_expressions(gloss_node, "R", one_minus, "Input")
    unreal.MaterialEditingLibrary.connect_material_property(
        base_node, "RGB", unreal.MaterialProperty.MP_BASE_COLOR
    )
    unreal.MaterialEditingLibrary.connect_material_property(
        normal_node, "RGB", unreal.MaterialProperty.MP_NORMAL
    )
    unreal.MaterialEditingLibrary.connect_material_property(
        one_minus, "", unreal.MaterialProperty.MP_ROUGHNESS
    )
    unreal.MaterialEditingLibrary.recompile_material(material)
    unreal.EditorAssetLibrary.save_loaded_asset(material, only_if_is_dirty=False)
    return material


def rebuild_null_material():
    material = load(MATERIAL_ROOT + "/Null_001")
    if unreal.MaterialEditingLibrary.get_material_property_input_node(
        material, unreal.MaterialProperty.MP_BASE_COLOR
    ):
        return material
    unreal.MaterialEditingLibrary.delete_all_material_expressions(material)
    color = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionConstant3Vector, -300, -40
    )
    color.set_editor_property("constant", unreal.LinearColor(0.015, 0.015, 0.015, 1.0))
    roughness = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionConstant, -300, 140
    )
    roughness.set_editor_property("r", 0.65)
    unreal.MaterialEditingLibrary.connect_material_property(
        color, "", unreal.MaterialProperty.MP_BASE_COLOR
    )
    unreal.MaterialEditingLibrary.connect_material_property(
        roughness, "", unreal.MaterialProperty.MP_ROUGHNESS
    )
    unreal.MaterialEditingLibrary.recompile_material(material)
    unreal.EditorAssetLibrary.save_loaded_asset(material, only_if_is_dirty=False)
    return material


body_1 = rebuild_character_material(
    "Ch08_body", "Ch08_1001_Diffuse", "Ch08_1001_Normal", "Ch08_1001_Glossiness"
)
body_2 = rebuild_character_material(
    "Ch08_body1", "Ch08_1002_Diffuse", "Ch08_1002_Normal", "Ch08_1002_Glossiness"
)
gun = rebuild_weapon_material(
    "Krinkov", "Krinkov_Color", "Krinkov_NRM_Fix", "Krinkov_Metal", "Krinkov_Rough", "Krinkov_AO"
)
magazine = rebuild_weapon_material(
    "Magazine", "Magazine_Color", "Magazine_NRM", "Magazine_Metal", "Magazine_Rough"
)
null_material = rebuild_null_material()

world_gun = load(ROOT + "/WorldParts/AK74U_Gun")
world_gun.set_material(0, gun)
world_gun.set_material(1, null_material)
world_gun.set_material(2, magazine)
unreal.EditorStaticMeshLibrary.remove_collisions(world_gun)
unreal.EditorStaticMeshLibrary.add_simple_collisions(
    world_gun, unreal.ScriptingCollisionShapeType.BOX
)
unreal.EditorAssetLibrary.save_loaded_asset(world_gun, only_if_is_dirty=False)

unreal.EditorAssetLibrary.save_directory(ROOT, only_if_is_dirty=False, recursive=True)
unreal.log("AK74U_MATERIALS_OK")
