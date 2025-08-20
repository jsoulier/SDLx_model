cbuffer UniformViewProj : register(b0, space1)
{
    float4x4 ViewProj : packoffset(c0);
};

struct Input
{
    float3 Position : TEXCOORD0;
    float3 Instance : TEXCOORD1;
    uint Color : TEXCOORD2;
};

struct Output
{
    float4 Position : SV_POSITION;
    float4 Color : TEXCOORD0;
};

Output main(Input input)
{
    Output output;
    output.Color.r = ((input.Color >> 24) & 0xFFu) / 255.0f;
    output.Color.g = ((input.Color >> 16) & 0xFFu) / 255.0f;
    output.Color.b = ((input.Color >> 8) & 0xFFu) / 255.0f;
    output.Color.a = ((input.Color >> 0) & 0xFFu) / 255.0f;
    output.Position = mul(ViewProj, float4(input.Instance + input.Position, 1.0f));
    return output;
}