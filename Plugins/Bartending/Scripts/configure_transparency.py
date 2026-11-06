"""Separate glass hemispheres and ice above/below the liquid into depth-ordered layers."""
from pathlib import Path
import unreal
root=Path(unreal.Paths.project_dir())
builder=(root/'Plugins/Bartending/Scripts/create_bartending_demo.py').read_text(encoding='utf-8')
# Rebuild only the liquid graph; do not reconstruct scene actors or lighting.
exec(builder[:builder.index("for name,is_ice")])
liquid.set_editor_property('two_sided',False); save(liquid)
for name,side,strength in [('M_Glass',1,.32),('M_GlassBack',-1,.08)]:
    m=mat(name,True); m.set_editor_property('two_sided',False)
    inp=[('N',node(m,unreal.MaterialExpressionPixelNormalWS)),('V',node(m,unreal.MaterialExpressionCameraVectorWS)),('World',node(m,unreal.MaterialExpressionWorldPosition)),('Center',node(m,unreal.MaterialExpressionObjectPositionWS)),('Env',tex(m,'Environment',ROOT+'/Textures/BarEnv_00')),('Side',scalar(m,'SurfaceSide',side)),('Strength',scalar(m,'ReflectionStrength',strength))]
    rgba(m,custom(m,'''float3 n=normalize(N),v=normalize(V);
float3 radial=normalize(float3(World.xy-Center.xy,0));
float shell=smoothstep(-.015,.015,dot(radial,v)*Side);
n=normalize(radial+float3(0,0,-.035));
float fresnel=.02+.98*pow(1-saturate(abs(dot(n,v))),5);
float3 e=TextureCubeSampleLevel(Env,EnvSampler,reflect(-v,n),3).rgb;
e=e/(1+e*.25);
return float4(e,shell*fresnel*Strength);''',inp)); save(m)
invisible=mat('M_Invisible',True); prop(scalar(invisible,'Opacity',0),unreal.MaterialProperty.MP_OPACITY); save(invisible)
for name,side in [('M_Ice',1),('M_IceSubmerged',-1)]:
    m=mat(name,True); m.set_editor_property('two_sided',False); m.set_editor_property('shading_model',unreal.MaterialShadingModel.MSM_DEFAULT_LIT); m.set_editor_property('translucency_lighting_mode',unreal.TranslucencyLightingMode.TLM_SURFACE_PER_PIXEL_LIGHTING)
    normal=node(m,unreal.MaterialExpressionTextureSample); normal.set_editor_property('texture',unreal.load_asset(ROOT+'/Textures/Ice_normal')); normal.set_editor_property('sampler_type',unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL); prop(normal,unreal.MaterialProperty.MP_NORMAL)
    prop(scalar(m,'Roughness',.10),unreal.MaterialProperty.MP_ROUGHNESS); prop(vector(m,'IceTint',(.65,.85,.88,1)),unreal.MaterialProperty.MP_BASE_COLOR)
    inp=[('N',node(m,unreal.MaterialExpressionPixelNormalWS)),('V',node(m,unreal.MaterialExpressionCameraVectorWS)),('World',node(m,unreal.MaterialExpressionWorldPosition)),('LiquidSurface',scalar(m,'LiquidSurface',320)),('Side',scalar(m,'SurfaceSide',side)),('Env',tex(m,'Environment',ROOT+'/Textures/BarEnv_00')),('Matcap',tex(m,'Matcap',ROOT+'/Textures/Ice_matcap'))]
    rgba(m,custom(m,'''float3 n=normalize(N),v=normalize(V);
float f=pow(1-saturate(abs(dot(n,v))),3);
float3 vn=mul(n,(float3x3)ResolvedView.TranslatedWorldToView);
float3 cap=Texture2DSample(Matcap,MatcapSampler,vn.xy*.5+.5).rgb;
float3 env=TextureCubeSampleLevel(Env,EnvSampler,reflect(-v,n),2).rgb;
float region=Side>0 ? step(LiquidSurface,World.z) : 1-step(LiquidSurface,World.z);
return float4(cap*.5+env*.08,region*(.65+f*.25));''',inp)); save(m)
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem); actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
assert levels.load_level(ROOT+'/Maps/L_Bartending')
demo=next(a for a in actors.get_all_level_actors() if isinstance(a,unreal.BartendingDemo))
for key,name in [('glass_material','M_Glass'),('back_glass_material','M_GlassBack'),('invisible_material','M_Invisible'),('ice_material','M_Ice'),('submerged_ice_material','M_IceSubmerged')]: demo.set_editor_property(key,unreal.load_asset(ROOT+'/Materials/'+name))
demo.preview_full(); assert levels.save_current_level()
unreal.log('TRANSPARENCY_LAYERS_CONFIGURED')
