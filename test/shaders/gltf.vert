cbuffer UniformViewProj : register(b0, space1)
{
    float4x4 ViewProj : packoffset(c0);
};

cbuffer UniformModel : register(b1, space1)
{
    float4x4 Model : packoffset(c0);
};

struct Input
{
    float3 Position : TEXCOORD0;
    float2 Texcoord : TEXCOORD1;
    float3 Normal : TEXCOORD2;
};

struct Output
{
    float4 Position : SV_POSITION;
    float2 Texcoord : TEXCOORD0;
    float3 Normal : TEXCOORD1;
};

Output main(Input input)
{
    Output output;
    output.Normal = input.Normal;
    output.Texcoord = float2(input.Texcoord.x, 1.0f - input.Texcoord.y);
    output.Position = mul(ViewProj, mul(Model, float4(input.Position, 1.0f)));
    return output;
}