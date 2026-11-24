"""Build the liquid, forward-lit glass shells and depth-writing ice."""
from pathlib import Path
import unreal
root=Path(unreal.Paths.project_dir())
builder=(root/'Plugins/Bartending/Scripts/create_bartending_demo.py').read_text(encoding='utf-8')
# Rebuild only the liquid graph; do not reconstruct scene actors or lighting.
exec(builder[:builder.index("for name,is_ice")])
liquid.set_editor_property('two_sided',True); liquid.set_editor_property('blend_mode',unreal.BlendMode.BLEND_TRANSLUCENT); save(liquid)
# The metal trim remains a separate opaque slot on the original FBX.
def use_surface_lighting(material, roughness):
    material.set_editor_property('two_sided', False)
    material.set_editor_property('shading_model', unreal.MaterialShadingModel.MSM_DEFAULT_LIT)
    material.set_editor_property('translucency_lighting_mode', unreal.TranslucencyLightingMode.TLM_SURFACE_PER_PIXEL_LIGHTING)
    prop(scalar(material, 'Roughness', roughness), unreal.MaterialProperty.MP_ROUGHNESS)

def surface_normal(material, name):
    sample = node(material, unreal.MaterialExpressionTextureSample)
    sample.set_editor_property('texture', unreal.load_asset(ROOT + '/Textures/' + name))
    sample.set_editor_property('sampler_type', unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
    prop(sample, unreal.MaterialProperty.MP_NORMAL)

def reflection_inputs(material):
    return [('N', node(material, unreal.MaterialExpressionPixelNormalWS)),
            ('V', node(material, unreal.MaterialExpressionCameraVectorWS)),
            ('World', node(material, unreal.MaterialExpressionWorldPosition)),
            ('Env', tex(material, 'Environment', ROOT + '/Textures/BarEnv_00'))]

for name, side, strength in [('M_Glass', 1, 1), ('M_GlassBack', -1, .50)]:
    glass = mat(name, True)
    use_surface_lighting(glass, .085)
    surface_normal(glass, 'Goblet_normal')
    prop(vector(glass, 'GlassTint', (.07,.10,.12,1)), unreal.MaterialProperty.MP_BASE_COLOR)
    inputs = reflection_inputs(glass) + [
        ('Center', node(glass, unreal.MaterialExpressionObjectPositionWS)),
        ('Side', scalar(glass, 'SurfaceSide', side)),
        ('Strength', scalar(glass, 'ReflectionStrength', strength))]
    rgba(glass, custom(glass, """float3 v=normalize(V), n=normalize(N);
float3 radial=normalize(float3(World.xy-Center.xy,0));
float shell=smoothstep(-.015,.015,dot(radial,v)*Side);
float grazing=1-saturate(abs(dot(n,v)));
float3 reflection=TextureCubeSampleLevel(Env,EnvSampler,reflect(-v,n),1.6).rgb;
float fresnel=.045+.955*pow(grazing,4);
// Finite strip sources, seen across the curvature of the blown glass.
float3 ray=reflect(-v,n);
float3 key=normalize(float3(-350,-350,230)-World);
float3 rim=normalize(float3(300,-105,300)-World);
float k=exp(-400*pow(length(normalize(ray.xy)-normalize(key.xy)),2)-4*pow(ray.z-key.z,2));
float r=exp(-550*pow(length(normalize(ray.xy)-normalize(rim.xy)),2)-5*pow(ray.z-rim.z,2));
float bands=k*.40+r*.31;
float3 highlight=k*float3(1,.92,.78)+r*float3(.73,.87,1);
float3 film=reflection/(1+.15*reflection);
return float4(lerp(film,highlight, saturate(2*bands)), shell*Strength*saturate(.035+fresnel*.88+bands));""", inputs))
    save(glass)

invisible=mat('M_Invisible', True)
prop(scalar(invisible,'Opacity',0),unreal.MaterialProperty.MP_OPACITY)
save(invisible)

# As in Unity's geometry pass, one nearest ice fragment wins the depth test.
# The transparent liquid can then cover only the parts in front of that fragment.
ice = mat('M_Ice')
use_surface_lighting(ice,.12)
surface_normal(ice, 'Ice_normal')
prop(scalar(ice,'Specular',.12),unreal.MaterialProperty.MP_SPECULAR)
prop(vector(ice,'DiffuseScatter',(.004,.008,.010,1)),unreal.MaterialProperty.MP_BASE_COLOR)
inputs=reflection_inputs(ice) + [
    ('Bar',tex(ice,'Bar',ROOT+'/Textures/Bar')),
    ('Matcap',tex(ice,'Matcap',ROOT+'/Textures/Ice_matcap')),
    ('DrinkColor',vector(ice,'DrinkColor',(.78,.14,.30,1))),
    ('LiquidSurface',scalar(ice,'LiquidSurface',320))]
color=custom(ice,"""float3 n=normalize(N),v=normalize(V);
float3 vn=mul(n,(float3x3)ResolvedView.TranslatedWorldToView);
float3 cap=Texture2DSample(Matcap,MatcapSampler,vn.xy*float2(.5,-.5)+.5).rgb;
float3 travel=refract(-v,faceforward(n,-v,n),1.0/1.31);
float3 hit=World+travel*max(0,(550-World.y)/max(travel.y,.10));
float2 uv=float2(.56-hit.x*.00032,.625-hit.z*.00056);
float3 through=Texture2DSampleLevel(Bar,BarSampler,clamp(uv,.001,.999),3).rgb;
float wet=smoothstep(-2,10,LiquidSurface-World.z);
through=lerp(through/(1+through),DrinkColor.rgb,wet*.75);
return lerp(through,cap/(1+cap),.45)*float3(.74,.88,.94);""",inputs,unreal.CustomMaterialOutputType.CMOT_FLOAT3)
prop(color,unreal.MaterialProperty.MP_EMISSIVE_COLOR)
save(ice)
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem); actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
assert levels.load_level(ROOT+'/Maps/L_Bartending')
demo=next(a for a in actors.get_all_level_actors() if isinstance(a,unreal.BartendingDemo))
for key,name in [('glass_material','M_Glass'),('back_glass_material','M_GlassBack'),('invisible_material','M_Invisible'),('ice_material','M_Ice')]: demo.set_editor_property(key,unreal.load_asset(ROOT+'/Materials/'+name))
demo.preview_full(); assert levels.save_current_level()
unreal.log('TRANSPARENCY_LAYERS_CONFIGURED')
