"""Small editor-only helpers. No shared runtime materials or external content."""
from pathlib import Path
import unreal
L = unreal.MaterialEditingLibrary
A = unreal.EditorAssetLibrary
T = unreal.AssetToolsHelpers.get_asset_tools()

def expr(m, cls): return L.create_material_expression(m,cls)
def scalar(m,name,value=0,cp=None):
    n=expr(m,unreal.MaterialExpressionScalarParameter)
    n.set_editor_property('parameter_name',name)
    n.set_editor_property('default_value',value)
    if cp is not None:
        n.set_editor_property('use_custom_primitive_data',True)
        n.set_editor_property('primitive_data_index',cp)
    return n
def vector(m,name,value=(1,1,1,1)):
    n=expr(m,unreal.MaterialExpressionVectorParameter)
    n.set_editor_property('parameter_name',name)
    n.set_editor_property('default_value',unreal.LinearColor(*value))
    return n
def texture(m,name,asset,linear=False):
    n=expr(m,unreal.MaterialExpressionTextureObjectParameter)
    n.set_editor_property('parameter_name',name)
    n.set_editor_property('texture',asset)
    if linear: n.set_editor_property('sampler_type',unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR)
    return n
def custom(m,code,inputs,kind=unreal.CustomMaterialOutputType.CMOT_FLOAT3):
    n=expr(m,unreal.MaterialExpressionCustom)
    n.set_editor_property('code',code)
    n.set_editor_property('output_type',kind)
    slots=[]
    for name,source,out in inputs:
        v=unreal.CustomInput(); v.set_editor_property('input_name',name); slots.append(v)
    n.set_editor_property('inputs',slots)
    for name,source,out in inputs: assert L.connect_material_expressions(source,out,n,name),name
    return n
def prop(n,p,out=''): assert L.connect_material_property(n,out,p)
def save(m):
    L.layout_material_expressions(m); L.recompile_material(m); assert A.save_loaded_asset(m)
    return m
def mat(root,name,blend=None):
    m=T.create_asset(name,root,unreal.Material,unreal.MaterialFactoryNew())
    m.set_editor_property('shading_model',unreal.MaterialShadingModel.MSM_UNLIT)
    if blend is not None:
        m.set_editor_property('blend_mode',blend)
        m.set_editor_property('translucency_pass',unreal.MaterialTranslucencyPass.MTP_BEFORE_DOF)
    return m
def import_texture(filename,dest,name):
    existing=unreal.load_asset(dest+'/'+name)
    if existing: return existing
    task=unreal.AssetImportTask()
    for k,v in dict(filename=str(filename),destination_path=dest,destination_name=name,automated=True,save=True).items(): task.set_editor_property(k,v)
    T.import_asset_tasks([task])
    asset=unreal.load_asset(dest+'/'+name); assert asset
    # Keep reimport sources relative to the public project, not the developer's disk.
    return asset
def camera(actors,location,target,fov=50):
    rotation=unreal.MathLibrary.find_look_at_rotation(unreal.Vector(*location),unreal.Vector(*target))
    c=actors.spawn_actor_from_class(unreal.CameraActor,unreal.Vector(*location),rotation)
    c.camera_component.set_editor_property('field_of_view',fov)
    c.set_editor_property('auto_activate_for_player',unreal.AutoReceiveInput.PLAYER0)
    unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).set_level_viewport_camera_info(unreal.Vector(*location),rotation)
    return c
