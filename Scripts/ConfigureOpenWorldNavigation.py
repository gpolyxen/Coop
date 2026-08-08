import unreal

WORLD_LEVEL = "/Game/OpenWorld/OpenWorld"
NAV_LABEL = "OpenWorldNavigationBounds"

world = unreal.EditorLoadingAndSavingUtils.load_map(WORLD_LEVEL)
if not world:
    raise RuntimeError("Could not load " + WORLD_LEVEL)

nav_bounds = None
recast_nav_mesh = None
for actor in unreal.EditorLevelLibrary.get_all_level_actors():
    if isinstance(actor, unreal.NavMeshBoundsVolume):
        nav_bounds = actor
    if isinstance(actor, unreal.RecastNavMesh):
        recast_nav_mesh = actor

if nav_bounds is None:
    nav_bounds = unreal.EditorLevelLibrary.spawn_actor_from_class(
        unreal.NavMeshBoundsVolume,
        unreal.Vector(0.0, 0.0, 0.0),
        unreal.Rotator(0.0, 0.0, 0.0),
    )

if nav_bounds is None:
    raise RuntimeError("Could not create NavMeshBoundsVolume")

# The default volume brush is 200 cm across. These scales cover the reduced
# 6.6 km world, while navigation invokers limit actual runtime generation to
# the areas around the player and zombies.
nav_bounds.set_actor_label(NAV_LABEL)
nav_bounds.set_actor_location(unreal.Vector(0.0, 0.0, 0.0), False, False)
nav_bounds.set_actor_scale3d(unreal.Vector(3300.0, 3300.0, 10.0))

origin, extent = nav_bounds.get_actor_bounds(False)
unreal.log(
    "Configured {} origin={} extent={}".format(NAV_LABEL, origin, extent)
)

if recast_nav_mesh is None:
    recast_nav_mesh = unreal.EditorLevelLibrary.spawn_actor_from_class(
        unreal.RecastNavMesh,
        unreal.Vector(0.0, 0.0, 0.0),
        unreal.Rotator(0.0, 0.0, 0.0),
    )

if recast_nav_mesh is None:
    raise RuntimeError("Could not create RecastNavMesh")

recast_nav_mesh.set_actor_label("OpenWorldRecastNavMesh")
recast_nav_mesh.set_editor_property("runtime_generation", unreal.RuntimeGenerationType.DYNAMIC)
recast_nav_mesh.set_editor_property("force_rebuild_on_load", True)
recast_nav_mesh.set_editor_property("nav_mesh_origin_offset", unreal.Vector(0.0, 0.0, 0.0))
unreal.log("Configured persistent RecastNavMesh")

if not unreal.EditorLevelLibrary.save_current_level():
    raise RuntimeError("Could not save " + WORLD_LEVEL)

unreal.log("Saved persistent open-world navigation bounds")
