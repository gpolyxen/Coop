import bpy
import json
import sys
from pathlib import Path


def argument_after_double_dash():
    args = sys.argv
    if "--" not in args:
        raise RuntimeError("Expected the GLB path after --")
    return Path(args[args.index("--") + 1]).resolve()


source = argument_after_double_dash()

bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.gltf(filepath=str(source), import_pack_images=False)

summary = {
    "objects": [
        {
            "name": obj.name,
            "type": obj.type,
            "parent": obj.parent.name if obj.parent else None,
            "data": obj.data.name if obj.data else None,
            "animation_action": (
                obj.animation_data.action.name
                if obj.animation_data and obj.animation_data.action
                else None
            ),
            "parent_type": obj.parent_type,
            "vertex_groups": len(obj.vertex_groups) if obj.type == "MESH" else None,
            "modifiers": [
                {
                    "name": modifier.name,
                    "type": modifier.type,
                    "object": (
                        modifier.object.name
                        if hasattr(modifier, "object") and modifier.object
                        else None
                    ),
                }
                for modifier in obj.modifiers
            ],
            "nla_tracks": [
                {
                    "name": track.name,
                    "strips": [
                        {
                            "name": strip.name,
                            "action": strip.action.name if strip.action else None,
                            "start": strip.frame_start,
                            "end": strip.frame_end,
                        }
                        for strip in track.strips
                    ],
                }
                for track in (obj.animation_data.nla_tracks if obj.animation_data else [])
            ],
        }
        for obj in bpy.context.scene.objects
    ],
    "actions": [
        {
            "name": action.name,
            "start": action.frame_range[0],
            "end": action.frame_range[1],
            "slots": len(action.slots) if hasattr(action, "slots") else None,
        }
        for action in bpy.data.actions
    ],
    "root_bones": [
        bone.name
        for armature in [obj for obj in bpy.context.scene.objects if obj.type == "ARMATURE"]
        for bone in armature.data.bones
        if bone.parent is None
    ],
}

print("P9_MANNY_SUMMARY_BEGIN")
print(json.dumps(summary, indent=2))
print("P9_MANNY_SUMMARY_END")
