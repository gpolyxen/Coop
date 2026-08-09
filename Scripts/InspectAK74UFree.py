import unreal


ROOT = "/Game/AK74UFree"
skeletal = unreal.load_asset(ROOT + "/FPS/SK_AK74U_FPS")
world = unreal.load_asset(ROOT + "/WorldParts/AK74U_Gun")

if not skeletal:
    raise RuntimeError("Missing AK74U skeletal mesh")
if not world:
    raise RuntimeError("Missing AK74U world mesh")

unreal.log("AK74U_SKELETAL_MATERIALS=" + repr([
    (str(item.get_editor_property("material_slot_name")), str(item.get_editor_property("material_interface")))
    for item in skeletal.get_editor_property("materials")
]))
unreal.log("AK74U_WORLD_MATERIALS=" + repr([
    (str(item.get_editor_property("material_slot_name")), str(item.get_editor_property("material_interface")))
    for item in world.get_editor_property("static_materials")
]))

for asset_path in unreal.EditorAssetLibrary.list_assets(ROOT + "/FPS", recursive=False, include_folder=False):
    asset = unreal.load_asset(asset_path)
    if isinstance(asset, unreal.AnimSequence):
        unreal.log("AK74U_ANIM=%s length=%.4f" % (
            asset.get_name(),
            asset.get_editor_property("sequence_length"),
        ))

unreal.log("AK74U_INSPECT_OK")
