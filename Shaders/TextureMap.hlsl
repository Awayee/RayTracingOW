
//vs
struct VSInput {};
struct VSOutput {
    float4 SV_Pos : SV_POSITION;
    float2 UV: TEXCOORD0;
};

struct PSOutput {
    half4 outColor: SV_Target;
};

static float2 g_Vertices[6] = {
	float2(-1.0, -1.0), float2(1.0, -1.0), float2(-1.0,  1.0),
	float2(-1.0,  1.0), float2(1.0, -1.0), float2(1.0,  1.0), 
};

static float g_Depth = 0.5;

[[vk::binding(0, 0)]] SamplerState g_Sampler;
[[vk::binding(1, 0)]] Texture2D g_Texture;

VSOutput MainVS(VSInput vIn, uint vID: SV_VertexID) {
	float2 v = g_Vertices[vID];
    VSOutput vOut = (VSOutput)0;
    vOut.SV_Pos = float4(v.xy, g_Depth, 1.0);
	vOut.UV = v * 0.5 + 0.5;
    vOut.UV.y = 1.0 - vOut.UV.y;
    return vOut;    
}

PSOutput MainPS(VSOutput pIn) {
    float3 color = g_Texture.Sample(g_Sampler, pIn.UV.xy).rgb;
    color = pow(color, 1.0 / 2.0);// gama correction
    PSOutput pOut = (PSOutput)0;
    pOut.outColor = float4(color, 1.0);
    return pOut;
}
