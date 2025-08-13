Texture2D<float4> PaletteTexture : register(t0, space2);
SamplerState PaletteSampler : register(s0, space2);

struct Input
{
    float2 Texcoord : TEXCOORD0;
    float3 Normal : TEXCOORD1;
};

float4 main(Input input) : SV_Target
{
    return PaletteTexture.Sample(PaletteSampler, input.Texcoord);
}