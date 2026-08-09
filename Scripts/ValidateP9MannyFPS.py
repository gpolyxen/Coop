import unreal


ROOT = "/Game/P9MannyFPS"
MESH = ROOT + "/Meshes/SK_P9_MannyFPS"
STATIC = ROOT + "/Meshes/SM_P9"
SKELETON = ROOT + "/Meshes/SK_P9_MannyFPS_Skeleton"
ANIMATIONS = (
    "A_P9_WEP_Draw",
    "A_P9_WEP_Fire",
    "A_P9_WEP_Idle",
    "A_P9_WEP_Inspect_01",
    "A_P9_WEP_Refpose",
    "A_P9_WEP_Reload_01",
    "A_P9_WEP_Walk",
)
MATERIALS = ("MI_Manny_01", "MI_Manny_02", "p220", "p220_mag")


def require(asset_path, class_name):
    asset = unreal.load_asset(asset_path)
    if not asset:
        raise RuntimeError("Missing asset: " + asset_path)
    actual_class = asset.get_class().get_name()
    if actual_class != class_name:
        raise RuntimeError(
            "Wrong asset type for {}: expected {}, found {}".format(
                asset_path, class_name, actual_class
            )
        )
    return asset


skeletal_mesh = require(MESH, "SkeletalMesh")
static_mesh = require(STATIC, "StaticMesh")
skeleton = require(SKELETON, "Skeleton")

mesh_skeleton = skeletal_mesh.get_editor_property("skeleton")
if mesh_skeleton != skeleton:
    raise RuntimeError("The imported skeletal mesh uses the wrong skeleton")

for material_name in MATERIALS:
    require(ROOT + "/Meshes/" + material_name, "Material")

animation_report = []
for animation_name in ANIMATIONS:
    animation = require(ROOT + "/Animations/" + animation_name, "AnimSequence")
    if animation.get_editor_property("skeleton") != skeleton:
        raise RuntimeError(animation_name + " uses the wrong skeleton")
    animation_report.append(
        {
            "name": animation_name,
            "seconds": animation.get_editor_property("sequence_length"),
        }
    )

unreal.log("P9_MANNY_VALIDATION_OK")
unreal.log("Skeletal mesh: " + skeletal_mesh.get_path_name())
unreal.log("Static mesh: " + static_mesh.get_path_name())
unreal.log("Skeleton: " + skeleton.get_path_name())
unreal.log("Animations: " + repr(animation_report))
