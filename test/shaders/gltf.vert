cbuffer UniformViewProj : register(b0, space1)
{
    float4x4 ViewProj : packoffset(c0);
};

struct Input
{
    float3 Position : POSITION;
    float3 Normal : NORMAL;
    float2 Texcoord : TEXCOORD0;
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
    output.Texcoord = input.Texcoord;
    output.Position = mul(ViewProj, float4(input.Position, 1.0f));
    return output;
}