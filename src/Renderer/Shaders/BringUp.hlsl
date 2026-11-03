// Code Modification - Rendering: Compile a camera-transformed, textured mesh pass for device bring-up.
// Scope: baseline vertex and pixel stages. Performance: one point-sampled texture lookup per pixel. Toggle: --no-render.
cbuffer Camera
{
    column_major float4x4 Mvp;
};

Texture2D<float4> Checker;
SamplerState PointSampler;

struct VertexOutput
{
    float4 position : SV_Position;
    float2 uv : TEXCOORD0;
};

VertexOutput VSMain(float3 position : ATTRIB0, float2 uv : ATTRIB1)
{
    VertexOutput output;
    output.position = mul(Mvp, float4(position, 1.0));
    output.uv = uv;
    return output;
}

float4 PSMain(VertexOutput input) : SV_Target0
{
    return Checker.Sample(PointSampler, input.uv);
}
// End of Code Modification - Rendering
