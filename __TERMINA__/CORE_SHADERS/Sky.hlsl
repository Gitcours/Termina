// Sky.hlsl — Procedural Sky Pass

#include "Common/Bindless.hlsli"

#pragma compute CSMain

struct PushConstants
{
    int OutputIndex;
    int DepthIndex;
    int Width;
    int Height;
    column_major float4x4 InvViewProj;
    float3 CameraPos;
    int _pad;
    float3 SunDir;
    int _pad2;
};
PUSH_CONSTANTS(PushConstants);

[numthreads(8, 8, 1)]
void CSMain(uint3 id : SV_DispatchThreadID)
{
    if (id.x >= (uint)PUSH.Width || id.y >= (uint)PUSH.Height)
        return;

    Texture2D<float> depthTex = ResourceDescriptorHeap[PUSH.DepthIndex];
    RWTexture2D<float4> outputTex = ResourceDescriptorHeap[PUSH.OutputIndex];

    float depth = depthTex[id.xy];

    // Only render sky on the far plane.
    if (depth < 1.0f)
        return;

    // Calculate normalized device coordinates
    float2 uv = (float2(id.xy) + 0.5f) / float2(PUSH.Width, PUSH.Height);
    uv.y = 1.0f - uv.y;
    float2 clipSpace = uv * 2.0f - 1.0f;

    // Reconstruct world position for the far plane to determine the view ray direction
    float4 ws4 = mul(PUSH.InvViewProj, float4(clipSpace, depth, 1.0f));
    float3 worldPos = ws4.xyz / ws4.w;

    float3 rayDir = normalize(worldPos.xyz - PUSH.CameraPos);

    // Simple sky gradient based on view direction's Y component
    float t = 0.5f * (rayDir.y + 1.0f);
    float3 skyColor = lerp(float3(0.5f, 0.7f, 0.9f), float3(0.05f, 0.15f, 0.4f), t);

    // Procedural Sun
    float3 sunDir = PUSH.SunDir;
    float sunIntensity = max(0.0f, dot(rayDir, sunDir));
    float sunScattering = pow(sunIntensity, 16.0f) * 0.5f;
    float sunDisc = pow(sunIntensity, 2048.0f) * 20.0f;

    skyColor += (sunScattering + sunDisc) * float3(1.0f, 0.9f, 0.7f);

    // Optional ground fading to hide artifacts below the horizon
    if (rayDir.y < 0.0f)
    {
        skyColor = lerp(skyColor, float3(0.1f, 0.1f, 0.15f), -rayDir.y);
    }

    outputTex[id.xy] = float4(skyColor, 1.0f);
}
