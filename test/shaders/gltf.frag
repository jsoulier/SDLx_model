cbuffer UniformLight : register(b0, space3)
{
    float4 Light : packoffset(c0);
};

Texture2D ColorTexture : register(t0, space2);
SamplerState ColorSampler : register(s0, space2);
Texture2D NormalTexture : register(t1, space2);
SamplerState NormalSampler : register(s1, space2);

struct Input
{
    float2 Texcoord : TEXCOORD0;
    float3 Normal   : TEXCOORD1;
};

float4 main(Input input) : SV_Target
{
    float3 normal = NormalTexture.Sample(NormalSampler, input.Texcoord).xyz;
    float light = saturate(dot(normal, -Light.xyz)) + Light.w; 
    float4 color = ColorTexture.Sample(ColorSampler, input.Texcoord);
    return color * light;
}