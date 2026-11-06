"""Run with UnrealEditor-Cmd -run=pythonscript -script=<absolute script path>.
Creates new demo assets only; never overwrites an existing demo map.
"""
import unreal

ROOT = "/Game/Demos/WFC/Town"
MAP = ROOT + "/L_TownPrototype"
MATERIAL = ROOT + "/M_TownVertexColor"

if unreal.EditorAssetLibrary.does_asset_exist(MAP):
    unreal.log("Town demo already exists; leaving it unchanged.")
else:
    material = unreal.load_asset(MATERIAL)
    if not material:
        material = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            "M_TownVertexColor", ROOT, unreal.Material, unreal.MaterialFactoryNew()
        )
        color = unreal.MaterialEditingLibrary.create_material_expression(material, unreal.MaterialExpressionVertexColor)
        unreal.MaterialEditingLibrary.connect_material_property(color, "RGB", unreal.MaterialProperty.MP_BASE_COLOR)
        roughness = unreal.MaterialEditingLibrary.create_material_expression(material, unreal.MaterialExpressionConstant)
        roughness.set_editor_property("r", 0.85)
        unreal.MaterialEditingLibrary.connect_material_property(roughness, "", unreal.MaterialProperty.MP_ROUGHNESS)
        unreal.MaterialEditingLibrary.recompile_material(material)
        unreal.EditorAssetLibrary.save_loaded_asset(material)

    levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    if not levels.new_level(MAP):
        raise RuntimeError("Unable to create town prototype map")
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    builder_class = unreal.load_class(None, "/Script/WFCRuntime.WFCTownBuilder")
    if not builder_class:
        raise RuntimeError("Build WFCRuntime before generating the demo")
    builder = actors.spawn_actor_from_class(builder_class, unreal.Vector(0, 0, 0))
    builder.set_actor_label("Town Builder - Play to build")
    builder.set_editor_property("surface_material", material)
    builder.reset_town()
    assert not builder.edit_cell(-1, 1)
    assert not builder.edit_cell(builder.get_cell_count(), 1)
    assert not builder.edit_cell(0, -1)
    assert builder.edit_cell(0, 1)
    assert builder.get_cell_height(0) == 1
    builder.undo()
    assert builder.get_cell_height(0) == 0
    builder.redo()
    assert builder.get_cell_height(0) == 1
    for _ in range(20):
        builder.edit_cell(0, 1)
    assert builder.get_cell_height(0) == builder.get_editor_property("max_floors")
    assert not builder.edit_cell(0, 1)
    builder.reset_town()
    unreal.log("TOWN_EDIT_TESTS_PASSED")
    # A small sample skyline; cell IDs are deterministic for the default seed.
    count = builder.get_cell_count()
    for cell, height in [(0, 1), (1, 2), (2, 3), (3, 1), (4, 2), (5, 1)]:
        if cell < count:
            for _ in range(height):
                builder.edit_cell(cell, 1)
    sun = actors.spawn_actor_from_class(unreal.DirectionalLight, unreal.Vector(0, 0, 1500), unreal.Rotator(pitch=-50,yaw=-35,roll=0))
    sun.set_actor_label("Town Sun")
    sun.light_component.set_editor_property("intensity", 4.0)
    sky = actors.spawn_actor_from_class(unreal.SkyLight, unreal.Vector(0, 0, 1000))
    sky.light_component.set_editor_property("mobility", unreal.ComponentMobility.MOVABLE)
    sky.light_component.set_editor_property("intensity", 0.8)
    actors.spawn_actor_from_class(unreal.SkyAtmosphere, unreal.Vector(0, 0, 0))
    actors.spawn_actor_from_class(unreal.PlayerStart, unreal.Vector(0, 0, 1500))
    unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).set_level_viewport_camera_info(
        unreal.Vector(2300, -2300, 2600), unreal.Rotator(pitch=-40,yaw=135,roll=0))
    if not levels.save_current_level():
        raise RuntimeError("Unable to save town prototype map")
    unreal.log("TOWN_DEMO_CREATED: " + MAP + " cells=" + str(count))
