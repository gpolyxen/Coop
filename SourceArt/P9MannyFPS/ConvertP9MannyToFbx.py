"""Convert the P9 Manny GLB package into UE4.27-friendly FBX files.

Run with Blender in background mode:
  blender --background --python ConvertP9MannyToFbx.py -- input.glb output_directory
"""

import bpy
import json
import re
import sys
from pathlib import Path


def script_arguments():
    if "--" not in sys.argv:
        raise RuntimeError("Expected input GLB and output directory after --")
    args = sys.argv[sys.argv.index("--") + 1 :]
    if len(args) != 2:
        raise RuntimeError("Usage: -- input.glb output_directory")
    return Path(args[0]).resolve(), Path(args[1]).resolve()


def select_only(objects):
    bpy.ops.object.select_all(action="DESELECT")
    for obj in objects:
        obj.hide_set(False)
        obj.hide_viewport = False
        obj.hide_render = False
        obj.select_set(True)
    bpy.context.view_layer.objects.active = objects[0]


def safe_asset_name(name):
    name = name.removesuffix("_Armature")
    return re.sub(r"[^A-Za-z0-9_]+", "_", name).strip("_")


source_path, output_path = script_arguments()
output_path.mkdir(parents=True, exist_ok=True)

bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.gltf(filepath=str(source_path), import_pack_images=False)

scene = bpy.context.scene
scene.render.fps = 24
scene.render.fps_base = 1.0

armatures = [obj for obj in scene.objects if obj.type == "ARMATURE"]
if len(armatures) != 1:
    raise RuntimeError(f"Expected one armature, found {len(armatures)}")

armature = armatures[0]
armature.name = "Armature"
armature.data.name = "SK_P9_MannyFPS_Skeleton"

# The source rig has three top-level roots: Manny, the control rig and P9.
# UE4 accepts only one root in a skeletal hierarchy. Keep Manny's real `root`
# and parent the two auxiliary roots to it without connecting their positions.
bpy.context.view_layer.objects.active = armature
armature.select_set(True)
bpy.ops.object.mode_set(mode="EDIT")
main_root = armature.data.edit_bones.get("root")
if not main_root:
    raise RuntimeError("The Manny root bone was not found")
for auxiliary_name in ("CB_root", "DEF_Root_Bone"):
    auxiliary_root = armature.data.edit_bones.get(auxiliary_name)
    if auxiliary_root:
        auxiliary_root.parent = main_root
        auxiliary_root.use_connect = False
bpy.ops.object.mode_set(mode="OBJECT")

arms = next((obj for obj in scene.objects if obj.name == "SK_Manny_Arms"), None)
pistol = next((obj for obj in scene.objects if obj.name == "P9"), None)
if not arms or not pistol:
    raise RuntimeError("Could not find the expected SK_Manny_Arms and P9 meshes")

arms.name = "SK_P9_MannyFPS_Arms"
arms.data.name = "SK_P9_MannyFPS_Arms"
pistol.name = "SK_P9_MannyFPS_Pistol"
pistol.data.name = "SK_P9_MannyFPS_Pistol"

# The source contains a helper icosphere which is not part of the FPS asset.
for obj in list(scene.objects):
    if obj not in {armature, arms, pistol}:
        bpy.data.objects.remove(obj, do_unlink=True)

if not armature.animation_data:
    armature.animation_data_create()

for track in armature.animation_data.nla_tracks:
    track.mute = True

actions = sorted(list(bpy.data.actions), key=lambda action: action.name)
ref_action = next((action for action in actions if action.name.startswith("WEP_Refpose")), None)

# Export the combined arms + animated pistol as one skeletal mesh in its rest pose.
armature.data.pose_position = "REST"
armature.animation_data.action = ref_action
select_only([armature, arms, pistol])
base_fbx = output_path / "SK_P9_MannyFPS.fbx"
bpy.ops.export_scene.fbx(
    filepath=str(base_fbx),
    use_selection=True,
    object_types={"ARMATURE", "MESH"},
    global_scale=1.0,
    apply_unit_scale=True,
    apply_scale_options="FBX_SCALE_ALL",
    axis_forward="-Z",
    axis_up="Y",
    use_mesh_modifiers=True,
    mesh_smooth_type="FACE",
    add_leaf_bones=False,
    primary_bone_axis="Y",
    secondary_bone_axis="X",
    bake_anim=False,
    path_mode="AUTO",
)

# Create a true static copy of the P9 for world pickups and third-person display.
bpy.context.view_layer.update()
evaluated_pistol = pistol.evaluated_get(bpy.context.evaluated_depsgraph_get())
static_mesh = bpy.data.meshes.new_from_object(
    evaluated_pistol,
    preserve_all_data_layers=True,
    depsgraph=bpy.context.evaluated_depsgraph_get(),
)
static_pistol = bpy.data.objects.new("SM_P9", static_mesh)
scene.collection.objects.link(static_pistol)
static_pistol.matrix_world = pistol.matrix_world.copy()
for vertex_group in list(static_pistol.vertex_groups):
    static_pistol.vertex_groups.remove(vertex_group)
select_only([static_pistol])
static_fbx = output_path / "SM_P9.fbx"
bpy.ops.export_scene.fbx(
    filepath=str(static_fbx),
    use_selection=True,
    object_types={"MESH"},
    global_scale=1.0,
    apply_unit_scale=True,
    apply_scale_options="FBX_SCALE_ALL",
    axis_forward="-Z",
    axis_up="Y",
    use_mesh_modifiers=True,
    mesh_smooth_type="FACE",
    bake_anim=False,
    path_mode="AUTO",
)

# Export one animation per FBX. UE4 imports these onto the skeleton from the base FBX.
armature.data.pose_position = "POSE"
exported_animations = []
for action in actions:
    armature.animation_data.action = action
    start = int(action.frame_range[0])
    end = max(start + 1, int(round(action.frame_range[1])))
    scene.frame_start = start
    scene.frame_end = end
    scene.frame_set(start)

    animation_name = safe_asset_name(action.name)
    animation_fbx = output_path / f"A_P9_{animation_name}.fbx"
    select_only([armature])
    bpy.ops.export_scene.fbx(
        filepath=str(animation_fbx),
        use_selection=True,
        object_types={"ARMATURE"},
        global_scale=1.0,
        apply_unit_scale=True,
        apply_scale_options="FBX_SCALE_ALL",
        axis_forward="-Z",
        axis_up="Y",
        add_leaf_bones=False,
        primary_bone_axis="Y",
        secondary_bone_axis="X",
        bake_anim=True,
        bake_anim_use_all_bones=True,
        bake_anim_use_nla_strips=False,
        bake_anim_use_all_actions=False,
        bake_anim_force_startend_keying=True,
        bake_anim_step=1.0,
        bake_anim_simplify_factor=0.0,
        path_mode="AUTO",
    )
    exported_animations.append(
        {
            "name": animation_name,
            "file": animation_fbx.name,
            "start_frame": start,
            "end_frame": end,
        }
    )

manifest = {
    "source": str(source_path),
    "skeletal_mesh": base_fbx.name,
    "static_pistol": static_fbx.name,
    "animations": exported_animations,
    "fps": scene.render.fps,
}
(output_path / "P9MannyImportManifest.json").write_text(
    json.dumps(manifest, indent=2), encoding="utf-8"
)
print(json.dumps(manifest, indent=2))
