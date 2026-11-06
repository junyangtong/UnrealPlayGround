"""Rebuild the cocktail materials, then apply the reference lighting setup."""
from pathlib import Path
import unreal
root=Path(unreal.Paths.project_dir())
exec((root/'Plugins/Bartending/Scripts/create_bartending_demo.py').read_text(encoding='utf-8'))

def lit(m,rough=.15,metal=0):
    m.set_editor_property('shading_model',unreal.MaterialShadingModel.MSM_DEFAULT_LIT)
    m.set_editor_property('translucency_lighting_mode',unreal.TranslucencyLightingMode.TLM_SURFACE_PER_PIXEL_LIGHTING)
    prop(scalar(m,'Roughness',rough),unreal.MaterialProperty.MP_ROUGHNESS)
    prop(scalar(m,'Metallic',metal),unreal.MaterialProperty.MP_METALLIC)
def mask(m,a,channels):
    n=node(m,unreal.MaterialExpressionComponentMask)
    for ch in ['r','g','b','a']: n.set_editor_property(ch,ch in channels)
    connect(a,n); return n
def normal_map(m,name):
    n=node(m,unreal.MaterialExpressionTextureSample); n.set_editor_property('texture',unreal.load_asset(ROOT+'/Textures/'+name)); n.set_editor_property('sampler_type',unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL); prop(n,unreal.MaterialProperty.MP_NORMAL)

# One visible liquid shell avoids double blending of front/back surfaces.
liquid=unreal.load_asset(ROOT+'/Materials/M_LayeredLiquid'); liquid.set_editor_property('two_sided',False); save(liquid)

glass=mat('M_Glass',True); glass.set_editor_property('two_sided',False)
inp=[('N',node(glass,unreal.MaterialExpressionPixelNormalWS)),('V',node(glass,unreal.MaterialExpressionCameraVectorWS)),('Env',tex(glass,'Environment',ROOT+'/Textures/BarEnv_00'))]
rgba(glass,custom(glass,'float f=pow(1-saturate(abs(dot(normalize(N),normalize(V)))),3); float3 e=TextureCubeSampleLevel(Env,EnvSampler,reflect(-normalize(V),normalize(N)),1).rgb; return float4(min(e,4)*.12+float3(.4,.5,.55)*f,.008+f*.28);',inp)); save(glass)

ice=mat('M_Ice',True); ice.set_editor_property('two_sided',False); lit(ice,.08); normal_map(ice,'Ice_normal')
inp=[('N',node(ice,unreal.MaterialExpressionPixelNormalWS)),('V',node(ice,unreal.MaterialExpressionCameraVectorWS)),('World',node(ice,unreal.MaterialExpressionWorldPosition)),('LiquidSurface',scalar(ice,'LiquidSurface',320)),('Env',tex(ice,'Environment',ROOT+'/Textures/BarEnv_00')),('Matcap',tex(ice,'Matcap',ROOT+'/Textures/Ice_matcap'))]
c=custom(ice,'''float3 n=normalize(N),v=normalize(V); float f=pow(1-saturate(abs(dot(n,v))),3);
float3 vn=mul(n,(float3x3)ResolvedView.TranslatedWorldToView);
float3 cap=Texture2DSample(Matcap,MatcapSampler,vn.xy*.5+.5).rgb;
float3 env=TextureCubeSampleLevel(Env,EnvSampler,reflect(-v,n),1).rgb;
float above=smoothstep(LiquidSurface-3,LiquidSurface+2,World.z);
return float4((cap*.45+env*.22)*lerp(float3(.36,.7,.66),float3(.9,1,1),above),lerp(.12,.64+f*.25,above));''',inp)
rgba(ice,c); prop(vector(ice,'IceTint',(.65,.85,.88,1)),unreal.MaterialProperty.MP_BASE_COLOR); save(ice)

base=unreal.load_asset(ROOT+'/Materials/M_CupBase')
# Rebuild a neutral champagne metal using the original Unity tint.
LIB.delete_all_material_expressions(base); base.set_editor_property('shading_model',unreal.MaterialShadingModel.MSM_DEFAULT_LIT)
prop(vector(base,'Gold',(1,.844,.701,1)),unreal.MaterialProperty.MP_BASE_COLOR); prop(scalar(base,'Metallic',1),unreal.MaterialProperty.MP_METALLIC); prop(scalar(base,'Roughness',.19),unreal.MaterialProperty.MP_ROUGHNESS); save(base)

table=mat('M_BarCounter'); lit(table,.24,.3)
inp=[('World',node(table,unreal.MaterialExpressionWorldPosition)),('Noise',tex(table,'MicroSurface',ROOT+'/Textures/Noise_cloud'))]
c=custom(table,'''float noise=Texture2DSample(Noise,NoiseSampler,World.xy*.035).r;
float contact=smoothstep(65,145,length(World.xy));
return float3(.23,.16,.095)*lerp(.35,1,contact)*lerp(.9,1.08,noise);''',inp,unreal.CustomMaterialOutputType.CMOT_FLOAT3)
prop(c,unreal.MaterialProperty.MP_BASE_COLOR); save(table)

bar=mat('M_BarBackdrop'); uv=node(bar,unreal.MaterialExpressionTextureCoordinate); t=tex(bar,'Bar',ROOT+'/Textures/Bar')
prop(custom(bar,'return Texture2DSample(Bar,BarSampler,float2(UV.x,clamp(1-UV.y+.20,0,.99))).rgb*2.0;', [('UV',uv),('Bar',t)],unreal.CustomMaterialOutputType.CMOT_FLOAT3),unreal.MaterialProperty.MP_EMISSIVE_COLOR); save(bar)

levels.load_level(ROOT+'/Maps/L_Bartending')
demo=next(a for a in actors.get_all_level_actors() if isinstance(a,unreal.BartendingDemo))
# Reference presentation: pale citrus layers transitioning into a cyan top.
colors=[(.40,.54,.09,.96),(.44,.57,.12,.96),(.45,.59,.17,.95),(.25,.55,.47,.94),(.28,.64,.68,.94)]
presets=list(demo.get_editor_property('presets'))
for p,col in zip(presets,colors):
    p.set_editor_property('color',unreal.LinearColor(*col)); p.set_editor_property('blend_range',.48); p.set_editor_property('bubbles',1.54); p.set_editor_property('warp',.1); p.set_editor_property('mask',None)
demo.set_editor_property('presets',presets); demo.preview_full()
for actor in list(actors.get_all_level_actors()):
    if isinstance(actor,(unreal.Light,unreal.SkyLight)) or actor.get_actor_label()=='Bar counter': actors.destroy_actor(actor)
    elif actor.get_actor_label()=='Unity original bar backdrop':
        actor.set_actor_location(unreal.Vector(0,550,580),False,False)
        actor.set_actor_rotation(unreal.Rotator(pitch=0,yaw=0,roll=90),False)
        actor.set_actor_scale3d(unreal.Vector(24,24,1))
    elif isinstance(actor,unreal.PostProcessVolume):
        s=actor.get_editor_property('settings')
        for n,v in [('override_auto_exposure_method',True),('auto_exposure_method',unreal.AutoExposureMethod.AEM_MANUAL),('override_auto_exposure_bias',True),('auto_exposure_bias',0),('override_auto_exposure_apply_physical_camera_exposure',True),('auto_exposure_apply_physical_camera_exposure',False),('override_bloom_intensity',True),('bloom_intensity',.15)]: s.set_editor_property(n,v)
        actor.set_editor_property('settings',s)
counter=actors.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(0,-100,-5)); counter.set_actor_label('Bar counter'); counter.static_mesh_component.set_static_mesh(unreal.load_asset('/Engine/BasicShapes/Cube')); counter.static_mesh_component.set_material(0,table); counter.set_actor_scale3d(unreal.Vector(30,16,.06))
sky=actors.spawn_actor_from_class(unreal.SkyLight,unreal.Vector(0,0,600)); sky.set_actor_label('Bar environment'); sky.light_component.set_editor_property('source_type',unreal.SkyLightSourceType.SLS_SPECIFIED_CUBEMAP); sky.light_component.set_editor_property('cubemap',unreal.load_asset(ROOT+'/Textures/BarEnv_00')); sky.light_component.set_mobility(unreal.ComponentMobility.MOVABLE); sky.light_component.set_intensity(.35)
for label,pos,color,power in [('Warm key',(-350,-350,650),(1,.95,.86),400),('Cool rim',(300,150,500),(.56,.78,1),350)]:
    pos=unreal.Vector(*pos); light=actors.spawn_actor_from_class(unreal.RectLight,pos,unreal.MathLibrary.find_look_at_rotation(pos,unreal.Vector(0,0,200))); light.set_actor_label(label); light.light_component.set_mobility(unreal.ComponentMobility.MOVABLE); light.light_component.set_editor_property('intensity_units',unreal.LightUnits.LUMENS); light.light_component.set_intensity(power); light.light_component.set_light_color(unreal.LinearColor(*color)); light.light_component.set_editor_property('source_width',180); light.light_component.set_editor_property('source_height',450); light.light_component.set_editor_property('attenuation_radius',1600)
levels.save_current_level()


unreal.log("COCKTAIL_REFINEMENT_SAVED")

exec((root/'Plugins/Bartending/Scripts/configure_transparency.py').read_text(encoding='utf-8'))
