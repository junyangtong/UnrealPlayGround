"""Create reusable Marching Squares assets, an interactive board, and a 16-case gallery.
Existing maps and the user's module set are never overwritten.
"""
import unreal

ROOT = "/Game/Demos/WFC/MarchingSquares"
MATERIAL = "/Game/Demos/WFC/Town/M_TownVertexColor"
material = unreal.load_asset(MATERIAL)
if not material:
    raise RuntimeError("Run create_town_demo.py first to create the vertex-color material")
modules = unreal.WFCSquareSampleLibrary.create_square_samples(material)
assert modules
meshes = modules.get_editor_property("modules")
assert len(meshes) == 16 and meshes[0] is None
for mask in range(1, 16):
    assert meshes[mask] and meshes[mask].get_editor_property("allow_cpu_access")
    vertices, triangles, normals, uvs, tangents = unreal.ProceduralMeshLibrary.get_section_from_static_mesh(meshes[mask], 0, 0)
    assert len(vertices) > 0 and len(triangles) > 0, mask
unreal.log("SQUARE_ASSETS_VERIFIED: 15 nonempty CPU-readable models + empty case 0")

levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
map_path = ROOT + "/L_MarchingSquares"
if not unreal.EditorAssetLibrary.does_asset_exist(map_path):
    assert levels.new_level(map_path)
    builder = actors.spawn_actor_from_class(unreal.load_class(None, "/Script/WFCRuntime.WFCTownBuilder"), unreal.Vector())
    builder.set_actor_label("Marching Squares - click vertex markers")
    builder.set_editor_property("use_marching_squares", True)
    builder.set_editor_property("square_modules", modules)
    builder.set_editor_property("surface_material", material)
    builder.set_editor_property("floor_height", 40.0)
    builder.reset_town()
    assert builder.set_vertex_occupied(0, True)
    assert builder.is_vertex_occupied(0)
    assert any(builder.get_square_mask(cell) for cell in range(builder.get_cell_count()))
    builder.undo()
    assert not builder.is_vertex_occupied(0)
    assert all(builder.get_square_mask(cell) == 0 for cell in range(builder.get_cell_count()))
    builder.redo()
    assert builder.is_vertex_occupied(0)
    assert not builder.set_vertex_occupied(-1, True)
    builder.reset_town()
    for vertex in [0, 1, 2, 3, 7, 8, 9]:
        builder.set_vertex_occupied(vertex, True)
    sun = actors.spawn_actor_from_class(unreal.DirectionalLight, unreal.Vector(0,0,1500), unreal.Rotator(pitch=-50,yaw=-35,roll=0))
    sun.light_component.set_editor_property("intensity", 4.0)
    fill = actors.spawn_actor_from_class(unreal.DirectionalLight, unreal.Vector(0,-1000,1000), unreal.Rotator(pitch=-30,yaw=90,roll=0))
    fill.set_actor_label("MS Sample Fill")
    fill.light_component.set_editor_property("intensity", 2.0)
    fill.light_component.set_editor_property("cast_shadows", False)
    sky = actors.spawn_actor_from_class(unreal.SkyLight, unreal.Vector(0,0,1000))
    sky.light_component.set_editor_property("mobility", unreal.ComponentMobility.MOVABLE)
    actors.spawn_actor_from_class(unreal.SkyAtmosphere, unreal.Vector())
    actors.spawn_actor_from_class(unreal.PlayerStart, unreal.Vector(0,0,1500))
    unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).set_level_viewport_camera_info(
        unreal.Vector(2300,-2300,2600), unreal.Rotator(pitch=-40,yaw=135,roll=0))
    assert levels.save_current_level()
    assert levels.load_level(map_path)
    loaded = next(actor for actor in actors.get_all_level_actors() if actor.get_class().get_name() == "WFCTownBuilder")
    assert loaded.is_vertex_occupied(0), "Vertex occupancy must survive map reload"
    unreal.log("SQUARE_INTERACTION_TESTS_PASSED: undo/redo, shared masks, save/reload")

gallery_path = ROOT + "/L_SquareModelGallery"
if not unreal.EditorAssetLibrary.does_asset_exist(gallery_path):
    assert levels.new_level(gallery_path)
    for mask in range(16):
        x, y = (mask % 4) * 180.0, (mask // 4) * 180.0
        if mask:
            actor = actors.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(x,y,0))
            actor.static_mesh_component.set_static_mesh(meshes[mask])
            actor.set_actor_label(f"Mask {mask:02d} - bits {mask:04b}")
        label = actors.spawn_actor_from_class(unreal.TextRenderActor, unreal.Vector(x,y-25,5), unreal.Rotator(pitch=51,yaw=-90,roll=0))
        label.text_render.set_text(f"{mask:02d} / {mask:04b}" + (" EMPTY" if mask == 0 else ""))
        label.text_render.set_world_size(16)
    sun = actors.spawn_actor_from_class(unreal.DirectionalLight, unreal.Vector(0,0,1000), unreal.Rotator(pitch=-50,yaw=-35,roll=0))
    sun.light_component.set_editor_property("intensity", 4.0)
    fill = actors.spawn_actor_from_class(unreal.DirectionalLight, unreal.Vector(0,-1000,1000), unreal.Rotator(pitch=-30,yaw=90,roll=0))
    fill.set_actor_label("MS Sample Fill")
    fill.light_component.set_editor_property("intensity", 2.0)
    fill.light_component.set_editor_property("cast_shadows", False)
    sky = actors.spawn_actor_from_class(unreal.SkyLight, unreal.Vector(0,0,1000))
    sky.light_component.set_editor_property("mobility", unreal.ComponentMobility.MOVABLE)
    actors.spawn_actor_from_class(unreal.SkyAtmosphere, unreal.Vector())
    unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).set_level_viewport_camera_info(
        unreal.Vector(320,-550,1100), unreal.Rotator(pitch=-55,yaw=90,roll=0))
    assert levels.save_current_level()
unreal.log("SQUARE_DEMOS_READY")
