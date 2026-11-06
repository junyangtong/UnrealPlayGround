"""Run in UE's Python console: exec(open(r'<project>/Plugins/GrassInteraction/Scripts/create_grass_demo.py', encoding='utf-8').read()).
Or UnrealEditor-Cmd <project> -run=pythonscript -script=<this file> -AllowCommandletRendering.
Creates only /Game/Demos/Grass/Materials; existing demo assets are kept.
"""
from pathlib import Path
import unreal

ROOT = "/Game/Demos/Grass/Materials"
MAP = '/Game/Demos/Grass/Maps/L_GrassInteraction'
SHADERS = Path(unreal.Paths.project_dir()) / "Plugins/GrassInteraction/Shaders"
LIB = unreal.MaterialEditingLibrary
ASSETS = unreal.EditorAssetLibrary
TOOLS = unreal.AssetToolsHelpers.get_asset_tools()


def expression(material, cls, x=0, y=0):
    return LIB.create_material_expression(material, cls, x, y)


def custom(material, code, inputs, x=0, y=0):
    node = expression(material, unreal.MaterialExpressionCustom, x, y)
    node.set_editor_property("code", code)
    node.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT3)
    custom_inputs = []
    for name, source, output in inputs:
        item = unreal.CustomInput()
        item.set_editor_property("input_name", name)
        custom_inputs.append(item)
    node.set_editor_property("inputs", custom_inputs)
    for name, source, output in inputs:
        assert LIB.connect_material_expressions(source, output, node, name), name
    return node


def material(name):
    mat = TOOLS.create_asset(name, ROOT, unreal.Material, unreal.MaterialFactoryNew())
    mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    mat.set_editor_property("two_sided", True)
    return mat


def state_inputs(mat):
    texture = expression(mat, unreal.MaterialExpressionTextureObjectParameter, -700, 0)
    texture.set_editor_property("parameter_name", "StateTexture")
    texture.set_editor_property("texture", unreal.load_asset("/Engine/EngineResources/Black"))
    texture.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR)
    uv = expression(mat, unreal.MaterialExpressionTextureCoordinate, -700, 180)
    return texture, uv


def save(mat):
    LIB.recompile_material(mat)
    assert ASSETS.save_loaded_asset(mat)
    return mat


grass_mat = unreal.load_asset(ROOT + "/M_Grass")
if not grass_mat:
    grass_mat = material("M_Grass")
    texture, uv = state_inputs(grass_mat)
    color = expression(grass_mat, unreal.MaterialExpressionVertexColor, -700, 350)
    wpo = custom(grass_mat, (SHADERS / "GrassWPO.hlsl").read_text(),
                 [("StateTexture", texture, ""), ("UV", uv, ""), ("Height", color, "R")], -300, 0)
    assert LIB.connect_material_property(wpo, "", unreal.MaterialProperty.MP_WORLD_POSITION_OFFSET)
    tint = custom(grass_mat,
                  "return lerp(float3(0.012, 0.045, 0.006), float3(0.19, 0.48, 0.045), C.r) * lerp(0.65, 1.15, C.g);",
                  [("C", color, "")], -300, 350)
    assert LIB.connect_material_property(tint, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    save(grass_mat)

debug_mat = unreal.load_asset(ROOT + "/M_StateDebug")
if not debug_mat:
    debug_mat = material("M_StateDebug")
    texture, uv = state_inputs(debug_mat)
    mode = expression(debug_mat, unreal.MaterialExpressionScalarParameter, -700, 350)
    mode.set_editor_property("parameter_name", "ShowVelocity")
    result = custom(debug_mat, (SHADERS / "GrassDebug.hlsl").read_text(),
                    [("StateTexture", texture, ""), ("UV", uv, ""), ("ShowVelocity", mode, "")], -300, 0)
    assert LIB.connect_material_property(result, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    save(debug_mat)

ground_mat = unreal.load_asset(ROOT + "/M_Ground")
if not ground_mat:
    ground_mat = material("M_Ground")
    color = expression(ground_mat, unreal.MaterialExpressionConstant3Vector)
    color.set_editor_property("constant", unreal.LinearColor(0.017, 0.027, 0.012, 1))
    assert LIB.connect_material_property(color, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    save(ground_mat)

demo_class = unreal.load_class(None, "/Script/GrassInteraction.GrassInteractionDemo")
assert demo_class, "Build UnrealPlayGroundEditor first, then restart the editor."
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

if not ASSETS.does_asset_exist(MAP):
    assert levels.new_level(MAP)
    demo = actors.spawn_actor_from_class(demo_class, unreal.Vector())
    demo.set_actor_label("Grass Demo - Space: release / LMB: push / R: reset")
    demo.set_editor_property("grass_material", grass_mat)
    demo.set_editor_property("debug_material", debug_mat)
    # Trigger construction after setting the material references.
    demo.set_actor_location(unreal.Vector(), False, False)
    for component in demo.get_components_by_class(unreal.ProceduralMeshComponent):
        component.set_material(0, grass_mat)

    ground = actors.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(0, 0, -3))
    ground.set_actor_label("Ground")
    ground.static_mesh_component.set_static_mesh(unreal.load_asset("/Engine/BasicShapes/Cube"))
    ground.set_actor_scale3d(unreal.Vector(12.5, 12.5, 0.05))
    ground.static_mesh_component.set_material(0, ground_mat)
    ground.static_mesh_component.set_editor_property("cast_shadow", False)
    sun = actors.spawn_actor_from_class(unreal.DirectionalLight, unreal.Vector(0, 0, 1200), unreal.Rotator(pitch=-55,yaw=-30,roll=0))
    sun.light_component.set_editor_property("intensity", 5.0)
    sun.light_component.set_editor_property("cast_shadows", False)

    for text, x in [("DISPLACEMENT  RG", -70), ("VELOCITY  BA", 370)]:
        label = actors.spawn_actor_from_class(unreal.TextRenderActor, unreal.Vector(x, -1040, 10),
                                             unreal.Rotator(pitch=90,yaw=90,roll=0))
        label.text_render.set_text(text)
        label.text_render.set_world_size(21)
        label.set_actor_label(text)
    start = actors.spawn_actor_from_class(unreal.PlayerStart, unreal.Vector(0, -1400, 500))
    unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).set_level_viewport_camera_info(
        unreal.Vector(1150, -1550, 1350), unreal.Rotator(pitch=-36.7,yaw=129.4,roll=0))
    assert levels.save_current_level()
    unreal.log("GRASS_DEMO_CREATED: " + MAP)
else:
    unreal.log("GRASS_DEMO_EXISTS: " + MAP + " (kept unchanged)")

unreal.log("GRASS_DEMO_READY: open " + MAP + " and press Play")
