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
base=unreal.load_asset(ROOT+'/Materials/M_CupBase')
# Rebuild a neutral champagne metal using the original Unity tint.
LIB.delete_all_material_expressions(base); base.set_editor_property('shading_model',unreal.MaterialShadingModel.MSM_DEFAULT_LIT)
prop(vector(base,'Gold',(1,.844,.701,1)),unreal.MaterialProperty.MP_BASE_COLOR); prop(scalar(base,'Metallic',1),unreal.MaterialProperty.MP_METALLIC); prop(scalar(base,'Roughness',.19),unreal.MaterialProperty.MP_ROUGHNESS); save(base)

table=mat('M_BarCounter'); lit(table,.39,.08)
prop(scalar(table,'Specular',.22),unreal.MaterialProperty.MP_SPECULAR)
inp=[('World',node(table,unreal.MaterialExpressionWorldPosition)),('Noise',tex(table,'MicroSurface',ROOT+'/Textures/Noise_cloud'))]
c=custom(table,'''float noise=Texture2DSample(Noise,NoiseSampler,World.xy*.035).r;
float contact=smoothstep(65,145,length(World.xy));
return float3(.19,.129,.084)*lerp(.35,1,contact)*lerp(.9,1.08,noise);''',inp,unreal.CustomMaterialOutputType.CMOT_FLOAT3)
prop(c,unreal.MaterialProperty.MP_BASE_COLOR); save(table)

bar=mat('M_BarBackdrop')
# Frame the shelves over the sofa in the original photograph, top to bottom.
world=node(bar,unreal.MaterialExpressionWorldPosition)
bar_photo=tex(bar,'Bar',ROOT+'/Textures/Bar')
background=custom(bar,"""float2 uv=float2(.56-World.x*.00032,.625-World.z*.00056);
float3 photo=Texture2DSample(Bar,BarSampler,clamp(uv,.001,.999)).rgb;
return photo*1.45+float3(.026,.035,.055);""",[('World',world),('Bar',bar_photo)],unreal.CustomMaterialOutputType.CMOT_FLOAT3)
prop(background,unreal.MaterialProperty.MP_EMISSIVE_COLOR)
save(bar)

levels.load_level(ROOT+'/Maps/L_Bartending')
demo=next(a for a in actors.get_all_level_actors() if isinstance(a,unreal.BartendingDemo))
# Reference presentation: pale citrus layers transitioning into a cyan top.
colors=[(.012,.02,.40,.60),(.018,.035,.60,.60),(.025,.05,.72,.58),(.15,.04,.43,.55),(.78,.14,.30,.50)]
presets=list(demo.get_editor_property('presets'))
for p,col in zip(presets,colors):
    p.set_editor_property('color',unreal.LinearColor(*col)); p.set_editor_property('blend_range',.48); p.set_editor_property('bubbles',1.54); p.set_editor_property('warp',.1); p.set_editor_property('mask',None)
demo.set_editor_property('presets',presets); demo.preview_full()
# Slightly elevated hero angle opens the ellipse of the free surface.
camera=demo.get_component_by_class(unreal.CameraComponent)
camera_pos=unreal.Vector(0,-1750,590)
camera.set_relative_location(camera_pos,False,False)
camera.set_relative_rotation(unreal.MathLibrary.find_look_at_rotation(camera_pos,unreal.Vector(0,0,220)),False,False)

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
sky=actors.spawn_actor_from_class(unreal.SkyLight,unreal.Vector(0,0,600)); sky.set_actor_label('Bar environment'); sky.light_component.set_editor_property('source_type',unreal.SkyLightSourceType.SLS_SPECIFIED_CUBEMAP); sky.light_component.set_editor_property('cubemap',unreal.load_asset(ROOT+'/Textures/BarEnv_00')); sky.light_component.set_mobility(unreal.ComponentMobility.MOVABLE); sky.light_component.set_intensity(.85)
for label,pos,color,power in [('Warm key',(-350,-350,230),(1,.91,.78),750),('Cool rim',(300,-105,300),(.74,.87,1),500)]:
    pos=unreal.Vector(*pos); light=actors.spawn_actor_from_class(unreal.RectLight,pos,unreal.MathLibrary.find_look_at_rotation(pos,unreal.Vector(0,0,200))); light.set_actor_label(label); light.light_component.set_mobility(unreal.ComponentMobility.MOVABLE); light.light_component.set_editor_property('intensity_units',unreal.LightUnits.LUMENS); light.light_component.set_intensity(power); light.light_component.set_light_color(unreal.LinearColor(*color)); light.light_component.set_editor_property('source_width',100); light.light_component.set_editor_property('source_height',280); light.light_component.set_editor_property('attenuation_radius',1600)
levels.save_current_level()


unreal.log("COCKTAIL_REFINEMENT_SAVED")

exec((root/'Plugins/Bartending/Scripts/configure_transparency.py').read_text(encoding='utf-8'))
