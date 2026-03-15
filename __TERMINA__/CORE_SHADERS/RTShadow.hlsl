// RTShadow.hlsl — ray-traced hard shadow mask (compute).
// Traces one shadow ray per pixel toward the primary directional light.
// Outputs R8_UNORM: 1.0 = lit, 0.0 = shadowed.

#define RAYTRACING 1
#include "Common/Bindless.hlsli"

#pragma compute CSMain

struct PushConstants
{
    int                       DepthIndex;
    int                       NormalsIndex;
    int                       OutputIndex;       // RWTexture2D<float> UAV
    int                       TLASIndex;
    int                       LightBufferIndex;  // unused (reserved)
    int                       LightCount;        // number of directional lights
    int                       Width;
    int                       Height;
    column_major float4x4     InvViewProj;
    float3                    CameraPos;
    int                       _pad;
    float3                    LightDir;          // normalized direction *toward* light
    int                       _pad2;
};
PUSH_CONSTANTS(PushConstants);

[numthreads(8, 8, 1)]
void CSMain(uint3 id : SV_DispatchThreadID)
{
    if (id.x >= (uint)PUSH.Width || id.y >= (uint)PUSH.Height)
        return;

    RWTexture2D<float> shadowOut = ResourceDescriptorHeap[PUSH.OutputIndex];

    // No directional lights — everything is lit
    if (PUSH.LightCount == 0) {
        shadowOut[id.xy] = 1.0f;
        return;
    }

    Texture2D<float>  depthTex   = ResourceDescriptorHeap[PUSH.DepthIndex];
    Texture2D<float4> normalsTex = ResourceDescriptorHeap[PUSH.NormalsIndex];

    float depth = depthTex[id.xy];

    // Sky pixels — fully lit
    if (depth >= 1.0f) {
        shadowOut[id.xy] = 1.0f;
        return;
    }

    // --- Reconstruct world-space position ---
    float2 uv     = (float2(id.xy) + 0.5f) / float2(PUSH.Width, PUSH.Height);
    uv.y          = 1.0f - uv.y;
    float4 ndcPos = float4(uv * 2.0f - 1.0f, depth, 1.0f);
    float4 ws4    = mul(PUSH.InvViewProj, ndcPos);
    float3 worldPos = ws4.xyz / ws4.w;

    // --- Decode GBuffer normal ---
    float3 normalEnc = normalsTex[id.xy].rgb;
    float3 N = normalize(normalEnc * 2.0f - 1.0f);

    float3 L = PUSH.LightDir;

    // N·L <= 0 → surface faces away from light, in shadow
    if (dot(N, L) <= 0.0f) {
        shadowOut[id.xy] = 0.0f;
        return;
    }

    // --- Trace shadow ray ---
    RaytracingAccelerationStructure tlas = GetAccelerationStructure(PUSH.TLASIndex);

    RayDesc ray;
    ray.Origin    = worldPos + N * 0.01f;   // normal bias to avoid self-intersection
    ray.Direction = L;
    ray.TMin      = 0.001f;
    ray.TMax      = 1000.0f;

    RayQuery<RAY_FLAG_FORCE_OPAQUE | RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH> rq;
    rq.TraceRayInline(tlas, 0, 1, ray);
    rq.Proceed();

    float shadow = (rq.CommittedStatus() == COMMITTED_TRIANGLE_HIT) ? 0.009f : 1.0f;
    shadowOut[id.xy] = shadow;
}
