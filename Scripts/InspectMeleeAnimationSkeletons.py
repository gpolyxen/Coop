import unreal

assets = [
    "/Game/Mannequin/Character/Mesh/SK_Mannequin",
    "/Game/ThirdPersonBP/Player_0/Anim/Great_Sword_Slash_Anim_mixamo_com",
    "/Game/ThirdPersonBP/Player_0/Anim/Stable_Sword_Inward_Slash_Anim_mixamo_com",
    "/Game/ThirdPersonBP/Player_0/Anim/Stable_Sword_Outward_Slash_Anim_mixamo_com",
    "/Game/ThirdPersonBP/Player_0/Anim/Two_Hands_weapon/Great_Sword_Slash_Anim_mixamo_com",
    "/Game/ThirdPersonBP/Player_0/Anim/Two_Hands_weapon/Stable_Sword_Inward_Slash_Anim_mixamo_com",
    "/Game/ThirdPersonBP/Player_0/Anim/Two_Hands_weapon/Stable_Sword_Outward_Slash_Anim_mixamo_com",
]

for path in assets:
    asset = unreal.load_asset(path)
    if not asset:
        unreal.log_error("MELEE_INSPECT missing: {}".format(path))
        continue
    skeleton = asset.get_editor_property("skeleton")
    length = asset.get_editor_property("sequence_length") if isinstance(asset, unreal.AnimSequence) else -1.0
    unreal.log("MELEE_INSPECT asset={} class={} skeleton={} length={}".format(
        path, asset.get_class().get_name(), skeleton.get_path_name() if skeleton else "None", length))
