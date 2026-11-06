// Material Custom inputs: StateTexture (TextureObject), UV (float2), Height (float).
// SampleLevel is required because this code runs in the vertex shader.
float2 bend = Texture2DSampleLevel(StateTexture, StateTextureSampler, UV, 0).rg;
float weight = Height * Height;
return float3(bend * weight, -min(length(bend) * 0.25, 18.0) * weight);
