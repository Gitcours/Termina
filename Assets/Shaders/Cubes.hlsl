// Cube.hlsl

#include "Common/Bindless.hlsli"

#pragma vertex VSMain
#pragma pixel FSMain

// 36 vertices
static const float3 POSITIONS[] = {
    // Front face (Z+)
    float3(-0.5f, -0.5f,  0.5f),
    float3( 0.5f, -0.5f,  0.5f),
    float3( 0.5f,  0.5f,  0.5f),
    float3( 0.5f,  0.5f,  0.5f),
    float3(-0.5f,  0.5f,  0.5f),
    float3(-0.5f, -0.5f,  0.5f),

    // Back face (Z-)
    float3( 0.5f, -0.5f, -0.5f),
    float3(-0.5f, -0.5f, -0.5f),
    float3(-0.5f,  0.5f, -0.5f),
    float3(-0.5f,  0.5f, -0.5f),
    float3( 0.5f,  0.5f, -0.5f),
    float3( 0.5f, -0.5f, -0.5f),

    // Right face (X+)
    float3( 0.5f, -0.5f,  0.5f),
    float3( 0.5f, -0.5f, -0.5f),
    float3( 0.5f,  0.5f, -0.5f),
    float3( 0.5f,  0.5f, -0.5f),
    float3( 0.5f,  0.5f,  0.5f),
    float3( 0.5f, -0.5f,  0.5f),

    // Left face (X-)
    float3(-0.5f, -0.5f, -0.5f),
    float3(-0.5f, -0.5f,  0.5f),
    float3(-0.5f,  0.5f,  0.5f),
    float3(-0.5f,  0.5f,  0.5f),
    float3(-0.5f,  0.5f, -0.5f),
    float3(-0.5f, -0.5f, -0.5f),

    // Top face (Y+)
    float3(-0.5f,  0.5f,  0.5f),
    float3( 0.5f,  0.5f,  0.5f),
    float3( 0.5f,  0.5f, -0.5f),
    float3( 0.5f,  0.5f, -0.5f),
    float3(-0.5f,  0.5f, -0.5f),
    float3(-0.5f,  0.5f,  0.5f),

    // Bottom face (Y-)
    float3(-0.5f, -0.5f, -0.5f),
    float3( 0.5f, -0.5f, -0.5f),
    float3( 0.5f, -0.5f,  0.5f),
    float3( 0.5f, -0.5f,  0.5f),
    float3(-0.5f, -0.5f,  0.5f),
    float3(-0.5f, -0.5f, -0.5f)
};

static const float3 COLORS[] = {
    // Front face - Red to Orange gradient
    float3(1.0f, 0.0f, 0.0f),  // Red
    float3(1.0f, 0.5f, 0.0f),  // Orange
    float3(1.0f, 0.3f, 0.0f),  // Red-Orange
    float3(1.0f, 0.3f, 0.0f),  // Red-Orange
    float3(1.0f, 0.2f, 0.0f),  // Red-Orange
    float3(1.0f, 0.0f, 0.0f),  // Red

    // Back face - Cyan to Blue gradient
    float3(0.0f, 1.0f, 1.0f),  // Cyan
    float3(0.0f, 0.5f, 1.0f),  // Sky Blue
    float3(0.0f, 0.7f, 1.0f),  // Light Blue
    float3(0.0f, 0.7f, 1.0f),  // Light Blue
    float3(0.0f, 0.6f, 1.0f),  // Blue
    float3(0.0f, 1.0f, 1.0f),  // Cyan

    // Right face - Green to Lime gradient
    float3(0.0f, 1.0f, 0.0f),  // Green
    float3(0.5f, 1.0f, 0.0f),  // Lime
    float3(0.3f, 1.0f, 0.2f),  // Light Green
    float3(0.3f, 1.0f, 0.2f),  // Light Green
    float3(0.2f, 1.0f, 0.1f),  // Green-Lime
    float3(0.0f, 1.0f, 0.0f),  // Green

    // Left face - Magenta to Purple gradient
    float3(1.0f, 0.0f, 1.0f),  // Magenta
    float3(0.7f, 0.0f, 1.0f),  // Purple
    float3(0.8f, 0.0f, 1.0f),  // Light Purple
    float3(0.8f, 0.0f, 1.0f),  // Light Purple
    float3(0.75f, 0.0f, 1.0f), // Purple-Magenta
    float3(1.0f, 0.0f, 1.0f),  // Magenta

    // Top face - Blue to Indigo gradient
    float3(0.0f, 0.0f, 1.0f),  // Blue
    float3(0.3f, 0.0f, 1.0f),  // Blue-Indigo
    float3(0.5f, 0.0f, 1.0f),  // Indigo
    float3(0.5f, 0.0f, 1.0f),  // Indigo
    float3(0.4f, 0.0f, 1.0f),  // Blue-Indigo
    float3(0.0f, 0.0f, 1.0f),  // Blue

    // Bottom face - Yellow to Green gradient
    float3(1.0f, 1.0f, 0.0f),  // Yellow
    float3(0.7f, 1.0f, 0.0f),  // Yellow-Green
    float3(0.8f, 1.0f, 0.0f),  // Light Yellow-Green
    float3(0.8f, 1.0f, 0.0f),  // Light Yellow-Green
    float3(0.85f, 1.0f, 0.0f), // Yellow-Green
    float3(1.0f, 1.0f, 0.0f)   // Yellow
};

static const float2 UVS[] = {
    // Front face
    float2(0.0f, 1.0f), float2(1.0f, 1.0f), float2(1.0f, 0.0f),
    float2(1.0f, 0.0f), float2(0.0f, 0.0f), float2(0.0f, 1.0f),
    // Back face
    float2(0.0f, 1.0f), float2(1.0f, 1.0f), float2(1.0f, 0.0f),
    float2(1.0f, 0.0f), float2(0.0f, 0.0f), float2(0.0f, 1.0f),
    // Right face
    float2(0.0f, 1.0f), float2(1.0f, 1.0f), float2(1.0f, 0.0f),
    float2(1.0f, 0.0f), float2(0.0f, 0.0f), float2(0.0f, 1.0f),
    // Left face
    float2(0.0f, 1.0f), float2(1.0f, 1.0f), float2(1.0f, 0.0f),
    float2(1.0f, 0.0f), float2(0.0f, 0.0f), float2(0.0f, 1.0f),
    // Top face
    float2(0.0f, 1.0f), float2(1.0f, 1.0f), float2(1.0f, 0.0f),
    float2(1.0f, 0.0f), float2(0.0f, 0.0f), float2(0.0f, 1.0f),
    // Bottom face
    float2(0.0f, 1.0f), float2(1.0f, 1.0f), float2(1.0f, 0.0f),
    float2(1.0f, 0.0f), float2(0.0f, 0.0f), float2(0.0f, 1.0f),
};

struct VertexOutput
{
    float4 Position : SV_Position;
    float3 Color : COLOR;
    float2 UV : TEXCOORD0;
};

struct PushConstants
{
    column_major float4x4 ModelViewProjection;

    int TextureIndex;
    int SamplerIndex;
};
PUSH_CONSTANTS(PushConstants);

VertexOutput VSMain(uint VertexID : SV_VertexID)
{
    VertexOutput output = (VertexOutput)0;

    output.Position = mul(PUSH.ModelViewProjection, float4(POSITIONS[VertexID], 1.0f));
    output.Color = COLORS[VertexID];
    output.UV = UVS[VertexID];

    return output;
}

float4 FSMain(VertexOutput input) : SV_Target
{
    Texture2D<float4> texture = ResourceDescriptorHeap[PUSH.TextureIndex];
    SamplerState sampler = SamplerDescriptorHeap[PUSH.SamplerIndex];
    float4 color = texture.Sample(sampler, input.UV);

    return float4(color.rgb * input.Color, 1.0f);
}
