// Signed data -> displayable colors. Raw signed RTs otherwise look black.
float4 state = Texture2DSampleLevel(StateTexture, StateTextureSampler, UV, 0);
float2 value = lerp(state.rg / 60.0, state.ba / 180.0, ShowVelocity);
return saturate(float3(max(value.x, 0) + max(value.y, 0) * 0.35,
                       max(value.y, 0),
                       max(-value.x, 0) + max(-value.y, 0) * 0.4) + 0.025);
