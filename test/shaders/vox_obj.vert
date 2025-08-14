cbuffer UniformViewProj : register(b0, space1)
{
    float4x4 ViewProj : packoffset(c0);
};

struct Input
{
    uint2 Vertex : TEXCOORD0;
};

struct Output
{
    float4 Position : SV_POSITION;
    float2 Texcoord : TEXCOORD0;
    float3 Normal : TEXCOORD1;
};

float3 GetPosition(uint2 vertex)
{
    float3 magnitude;
    float3 direction;
    magnitude.x = float((vertex.x >> 0) & 0xFFu);
    direction.x = float((vertex.x >> 8) & 0x1u);
    magnitude.y = float((vertex.x >> 9) & 0xFFu);
    direction.y = float((vertex.x >> 17) & 0x1u);
    magnitude.z = float((vertex.x >> 18) & 0xFFu);
    direction.z = float((vertex.x >> 26) & 0x1u);
    return (1.0f - 2.0f * direction) * magnitude;
}

static const float3 Normals[6] =
{
    float3(-1.0f, 0.0f, 0.0f),
    float3( 1.0f, 0.0f, 0.0f),
    float3( 0.0f,-1.0f, 0.0f),
    float3( 0.0f, 1.0f, 0.0f),
    float3( 0.0f, 0.0f,-1.0f),
    float3( 0.0f, 0.0f, 1.0f)
};

float3 GetNormal(uint2 vertex)
{
    return Normals[(vertex.y >> 0) & 0x7u];
}

float2 GetTexcoord(uint2 vertex)
{
    return float2(float((vertex.y >> 3) & 0xFFu) / 255.0f, 0.5f);
}

Output main(Input input)
{
    Output output;
    output.Normal = GetNormal(input.Vertex);
    output.Texcoord = GetTexcoord(input.Vertex);
    output.Position = mul(ViewProj, float4(GetPosition(input.Vertex), 1.0f));
    return output;
}