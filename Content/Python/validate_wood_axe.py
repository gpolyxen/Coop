import unreal

paths = [
    "/Game/ThirdPersonBP/Player_0/UE4_Mannequin/Mesh/SK_Mannequin",
    "/Game/ThirdPersonBP/Player_0/UE4_Mannequin/Mesh/UE4_Mannequin_Skeleton",
    "/Game/ThirdPersonBP/Player_0/Anim/UE4ASP_HeroTPP_AnimBlueprint",
    "/Game/ThirdPersonBP/Player_0/Anim/Two_Hands_weapon/Axe_Attack_1_mixamo_com",
    "/Game/ThirdPersonBP/Player_0/Anim/Two_Hands_weapon/Axe_Attack_2_mixamo_com",
    "/Game/ThirdPersonBP/Player_0/Anim/Two_Hands_weapon/Axe_Attack_3_mixamo_com",
    "/Game/ThirdPersonBP/Player_0/Anim/Two_Hands_weapon/TwoHand_Idle_mixamo_com",
    "/Game/ThirdPersonBP/Player_0/Anim/Two_Hands_weapon/TwoHand_Walk_mixamo_com",
    "/Game/ThirdPersonBP/Player_0/Anim/Two_Hands_weapon/TwoHand_Run_mixamo_com",
    "/Game/ThirdPersonBP/Player_0/Anim/Two_Hands_weapon/TwoHand_Strafe_mixamo_com",
    "/Game/ThirdPersonBP/Player_0/Anim/Two_Hands_weapon/TwoHand_Jump_mixamo_com",
]
for path in paths:
    asset = unreal.load_asset(path)
    if not asset:
        raise RuntimeError("Missing required asset: " + path)
    unreal.log("VALID_ASSET {} class={}".format(path, asset.get_class().get_name()))
    if isinstance(asset, unreal.AnimSequence):
        anim_skeleton = asset.get_editor_property("skeleton")
        unreal.log("VALID_ANIM {} length={} skeleton={}".format(
            path, asset.sequence_length,
            anim_skeleton.get_path_name() if anim_skeleton else "None"))
        unreal.log("VALID_ANIM_DETAIL {} frames={} tracks={}".format(
            path, asset.get_editor_property("number_of_sampled_keys"),
            len(asset.get_editor_property("raw_animation_data"))))

mesh = unreal.load_asset(paths[0])
skeleton = unreal.load_asset(paths[1])
if mesh.skeleton != skeleton:
    raise RuntimeError("Character mesh and requested animation skeleton do not match")
anim_blueprint = unreal.load_asset(paths[2])
target_skeleton = anim_blueprint.get_editor_property("target_skeleton")
unreal.log("VALID_ANIM_BP skeleton={}".format(
    target_skeleton.get_path_name() if target_skeleton else "None"))

axe_mesh = unreal.load_asset("/Game/ThirdPersonBP/Player_0/Weapon/WoodAxe/SM_WoodAxe")
material = unreal.load_asset("/Game/ThirdPersonBP/Player_0/Weapon/WoodAxe/M_WoodAxeColored")
unreal.log("AXE_MESH slots={} materials={}".format(
    len(axe_mesh.static_materials),
    [slot.material_interface.get_path_name() if slot.material_interface else "None" for slot in axe_mesh.static_materials]))
unreal.log("AXE_MATERIAL expressions={}".format([
    "{}:{}".format(expr.get_class().get_name(), expr.texture.get_path_name() if hasattr(expr, "texture") and expr.texture else "None")
    for expr in material.get_editor_property("expressions")]))
unreal.log("WOOD AXE ASSET VALIDATION COMPLETE")
