// cbuffer UniformLight : register(b0, space3)
// {
//     float4 Light : packoffset(c0);
// };

Texture2D<float4> PaletteTexture : register(t0, space2);
SamplerState PaletteSampler : register(s0, space2);

struct Input
{
    float2 Texcoord : TEXCOORD0;
    float3 Normal : TEXCOORD1;
};

float4 main(Input input) : SV_Target
{
    // float light = saturate(dot(input.Normal, -Light.xyz)) + Light.w;
    // float4 color = PaletteTexture.Sample(PaletteSampler, input.Texcoord);
    // return color * light;
    return PaletteTexture.Sample(PaletteSampler, input.Texcoord);
}