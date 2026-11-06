from pathlib import Path
import unreal

ROOT='/Game/Demos/Bartending'
LIB=unreal.MaterialEditingLibrary
EA=unreal.EditorAssetLibrary
TOOLS=unreal.AssetToolsHelpers.get_asset_tools()
def node(m,c): return LIB.create_material_expression(m,c)
def connect(a,b,p='',o=''): assert LIB.connect_material_expressions(a,o,b,p)
def prop(a,p,o=''): assert LIB.connect_material_property(a,o,p)
def scalar(m,n,v):
    a=node(m,unreal.MaterialExpressionScalarParameter); a.set_editor_property('parameter_name',n); a.set_editor_property('default_value',v); return a
def vector(m,n,v):
    a=node(m,unreal.MaterialExpressionVectorParameter); a.set_editor_property('parameter_name',n); a.set_editor_property('default_value',unreal.LinearColor(*v))
    b=node(m,unreal.MaterialExpressionAppendVector); connect(a,b,'A'); connect(a,b,'B','A'); return b
def tex(m,n,asset):
    a=node(m,unreal.MaterialExpressionTextureObjectParameter); a.set_editor_property('parameter_name',n); a.set_editor_property('texture',unreal.load_asset(asset)); return a
def custom(m,code,inputs,kind=unreal.CustomMaterialOutputType.CMOT_FLOAT4):
    a=node(m,unreal.MaterialExpressionCustom); a.set_editor_property('code',code); a.set_editor_property('output_type',kind)
    slots=[]
    for name,src in inputs:
        s=unreal.CustomInput(); s.set_editor_property('input_name',name); slots.append(s)
    a.set_editor_property('inputs',slots)
    for name,src in inputs: connect(src,a,name)
    return a
def mat(n,transparent=False):
    a=unreal.load_asset(ROOT+'/Materials/'+n) or TOOLS.create_asset(n,ROOT+'/Materials',unreal.Material,unreal.MaterialFactoryNew())
    LIB.delete_all_material_expressions(a); a.set_editor_property('shading_model',unreal.MaterialShadingModel.MSM_UNLIT)
    a.set_editor_property('two_sided',True); a.set_editor_property('blend_mode',unreal.BlendMode.BLEND_TRANSLUCENT if transparent else unreal.BlendMode.BLEND_OPAQUE); return a
def rgba(m,a):
    b=node(m,unreal.MaterialExpressionComponentMask)
    for c in ['r','g','b']: b.set_editor_property(c,True)
    connect(a,b); prop(b,unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    c=node(m,unreal.MaterialExpressionComponentMask); c.set_editor_property('r',False); c.set_editor_property('a',True); connect(a,c); prop(c,unreal.MaterialProperty.MP_OPACITY)
def save(m):
    LIB.layout_material_expressions(m); LIB.recompile_material(m); EA.save_loaded_asset(m); return m

liquid=mat('M_LayeredLiquid',True)
height=node(liquid,unreal.MaterialExpressionTextureCoordinate); height.set_editor_property('coordinate_index',1)
inputs=[('UV',node(liquid,unreal.MaterialExpressionTextureCoordinate)),('HeightUV',height),('Normal',node(liquid,unreal.MaterialExpressionPixelNormalWS)),('View',node(liquid,unreal.MaterialExpressionCameraVectorWS))]
for n,v in [('Fill',1),('LayerCount',5),('Clock',0),('Swirl',0)]: inputs.append((n,scalar(liquid,n,v)))
for i in range(5):
    inputs.extend([(f'Weight{i}',vector(liquid,f'Weight{i}',tuple(1 if i==j else 0 for j in range(4)))),(f'Tail{i}',scalar(liquid,f'Tail{i}',1 if i==4 else 0)),(f'Color{i}',vector(liquid,f'Color{i}',(.5,.1,.3,.8))),(f'Detail{i}',vector(liquid,f'Detail{i}',(.3,0,0,0))),(f'Mask{i}',tex(liquid,f'Mask{i}','/Engine/EngineResources/Black'))])
for n,t in [('BubbleTex','Bubble'),('NoiseTex','Noise_perlin'),('Environment','BarEnv_00')]: inputs.append((n,tex(liquid,n,ROOT+'/Textures/'+t)))
shader=Path(unreal.Paths.project_dir())/'Plugins/Bartending/Shaders/LayeredLiquid.hlsl'
rgba(liquid,custom(liquid,shader.read_text(encoding='utf-8'),inputs)); save(liquid)

for name,is_ice in [('M_Glass',False),('M_Ice',True)]:
    m=mat(name,True)
    inputs=[('N',node(m,unreal.MaterialExpressionPixelNormalWS)),('V',node(m,unreal.MaterialExpressionCameraVectorWS)),('UV',node(m,unreal.MaterialExpressionTextureCoordinate)),('NormalMap',tex(m,'NormalMap',ROOT+'/Textures/'+('Ice_normal' if is_ice else 'Goblet_normal'))),('Matcap',tex(m,'Matcap',ROOT+'/Textures/'+('Ice_matcap' if is_ice else 'Matcap_00'))),('Env',tex(m,'Env',ROOT+'/Textures/BarEnv_00'))]
    code='''float3 perturb=Texture2DSample(NormalMap,NormalMapSampler,UV).xyz*2-1;
float3 n=normalize(N+perturb*.16); float f=pow(1-saturate(abs(dot(n,V))),1.1);
float3 reflected=TextureCubeSample(Env,EnvSampler,reflect(-V,n)).rgb;
float3 vn=mul(n,(float3x3)ResolvedView.TranslatedWorldToView);
float3 mc=Texture2DSample(Matcap,MatcapSampler,vn.xy*.5+.5).rgb;
'''+('return float4(lerp(reflected,mc,.502),.32+f*.5);' if is_ice else 'return float4(reflected*.7+mc*.3,.035+f*.3);')
    rgba(m,custom(m,code,inputs)); save(m)
base=mat('M_CupBase')
inputs=[('N',node(base,unreal.MaterialExpressionPixelNormalWS)),('V',node(base,unreal.MaterialExpressionCameraVectorWS)),('Env',tex(base,'Environment',ROOT+'/Textures/BarEnv_00'))]
prop(custom(base,'return TextureCubeSample(Env,EnvSampler,reflect(-normalize(V),normalize(N))).rgb*float3(1,.84431744,.7012578);',inputs,unreal.CustomMaterialOutputType.CMOT_FLOAT3),unreal.MaterialProperty.MP_EMISSIVE_COLOR); save(base)
bar=mat('M_BarBackdrop'); sample=node(bar,unreal.MaterialExpressionTextureSample); sample.set_editor_property('texture',unreal.load_asset(ROOT+'/Textures/Bar')); prop(sample,unreal.MaterialProperty.MP_EMISSIVE_COLOR); save(bar)

levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem); actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
assert levels.load_level(ROOT+'/Maps/L_Bartending')
demo=next(a for a in actors.get_all_level_actors() if isinstance(a,unreal.BartendingDemo))
for key,name in [('cup_source_mesh','Cup'),('liquid_source_mesh','Liquid'),('ice_source_mesh','Ice')]: demo.set_editor_property(key,unreal.load_asset(ROOT+'/Meshes/'+name))
for key,name in [('liquid_material','M_LayeredLiquid'),('glass_material','M_Glass'),('ice_material','M_Ice'),('cup_base_material','M_CupBase')]: demo.set_editor_property(key,unreal.load_asset(ROOT+'/Materials/'+name))
values=[((.9811321,.31760314,.14501001,1),.3,0,1,.05,True),((1,.2980432,.09748411,.9137255),.3,.408,.348,0,True),((1,.43081754,.60042244,.5019608),.3,6.26,0,0,False),((.39937097,.47386762,1,.56078434),.4,6.14,0,0,False),((.014,0,.014,1),.3,0,1,.27,False)]
presets=[]
for color,blend,bubble,warp,noise,mask in values:
    p=unreal.CocktailLayer(); p.set_editor_property('color',unreal.LinearColor(*color))
    for n,v in [('blend_range',blend),('bubbles',bubble),('warp',warp),('noise_scale',noise)]: p.set_editor_property(n,v)
    if mask: p.set_editor_property('mask',unreal.load_asset(ROOT+'/Textures/Mask_00'))
    presets.append(p)
demo.set_editor_property('presets',presets); demo.set_editor_property('pour_duration',5); demo.set_editor_property('stir_duration',5); demo.preview_full()
for a in actors.get_all_level_actors():
    if isinstance(a,unreal.TextRenderActor): actors.destroy_actor(a)
    elif isinstance(a,unreal.StaticMeshActor): actors.destroy_actor(a)
    elif isinstance(a,unreal.PostProcessVolume):
        s=a.get_editor_property('settings'); s.set_editor_property('auto_exposure_bias',0); s.set_editor_property('override_auto_exposure_apply_physical_camera_exposure',True); s.set_editor_property('auto_exposure_apply_physical_camera_exposure',False); a.set_editor_property('settings',s)
# Use the source bar photograph as a background, facing the demo camera.
back=actors.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(-220,350,230),unreal.Rotator(pitch=0,yaw=122.6,roll=90))
back.set_actor_rotation(unreal.MathLibrary.make_rot_from_z(unreal.Vector(690,-1080,600)-back.get_actor_location()),False); back.set_actor_label('Unity original bar backdrop'); back.static_mesh_component.set_static_mesh(unreal.load_asset('/Engine/BasicShapes/Plane')); back.static_mesh_component.set_material(0,bar); back.set_actor_scale3d(unreal.Vector(16,12,1))
assert levels.save_current_level()
unreal.log('ORIGINAL_BARTENDING_BOUND')
