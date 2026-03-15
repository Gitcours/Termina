// Deferred.hlsl — fullscreen PBR lighting pass (compute).
// Reads GBuffer textures, evaluates all submitted lights, outputs HDR color.

#include "Common/Bindless.hlsli"
#include "Common/Lighting.hlsli"

#pragma compute CSMain

struct PushConstants
{
    int             AlbedoIndex;
    int             NormalsIndex;
    int             ORMIndex;
    int             EmissiveIndex;
    int             DepthIndex;
    int             OutputIndex;        // RWTexture2D HDR UAV
    int             LightBufferIndex;
    int             LightCount;
    column_major float4x4 InvViewProj;
    float3          CameraPos;
    int             Width;
    int             Height;
    int             ShadowMaskIndex; // -1 if no RT shadow pass
};
PUSH_CONSTANTS(PushConstants);

[numthreads(8, 8, 1)]
void CSMain(uint3 id : SV_DispatchThreadID)
{
    if (id.x >= (uint)PUSH.Width || id.y >= (uint)PUSH.Height)
        return;

    Texture2D<float4>   albedoTex   = ResourceDescriptorHeap[PUSH.AlbedoIndex];
    Texture2D<float4>   normalsTex  = ResourceDescriptorHeap[PUSH.NormalsIndex];
    Texture2D<float4>   ormTex      = ResourceDescriptorHeap[PUSH.ORMIndex];
    Texture2D<float4>   emissiveTex = ResourceDescriptorHeap[PUSH.EmissiveIndex];
    Texture2D<float>    depthTex    = ResourceDescriptorHeap[PUSH.DepthIndex];
    RWTexture2D<float4> hdr         = ResourceDescriptorHeap[PUSH.OutputIndex];

    float depth = depthTex[id.xy];

    // Sky pixels — no geometry, output black (tonemap/sky pass can replace later)
    if (depth >= 1.0f)
    {
        hdr[id.xy] = float4(0.0f, 0.0f, 0.0f, 1.0f);
        return;
    }

    // --- Reconstruct world-space position from depth ---
    float2 uv     = (float2(id.xy) + 0.5f) / float2(PUSH.Width, PUSH.Height);
    uv.y = 1.0f - uv.y;
    float4 ndcPos = float4(uv * 2.0f - 1.0f, depth, 1.0f);
    float4 ws4    = mul(PUSH.InvViewProj, ndcPos);
    float3 worldPos = ws4.xyz / ws4.w;

    // --- GBuffer decode ---
    float3 albedo    = albedoTex[id.xy].rgb;
    float3 normalEnc = normalsTex[id.xy].rgb;
    float3 N         = normalize(normalEnc * 2.0f - 1.0f);
    float3 orm       = ormTex[id.xy].rgb;
    float  occlusion = orm.r;
    float  roughness = max(orm.g, 0.04f);
    float  metallic  = orm.b;
    float3 emissive  = emissiveTex[id.xy].rgb;

    // View direction (towards camera)
    float3 V = normalize(PUSH.CameraPos - worldPos);

    // Flat ambient (will be replaced by IBL / sky later)
    float3 ambient = float3(0.03f, 0.03f, 0.03f) * albedo * occlusion;

    // --- Accumulate light contributions ---
    float3 Lo = float3(0.0f, 0.0f, 0.0f);

    float shadowFactor = 1.0f;
    if (PUSH.ShadowMaskIndex >= 0)
    {
        Texture2D<float> shadowMask = ResourceDescriptorHeap[PUSH.ShadowMaskIndex];
        shadowFactor = shadowMask[id.xy];
    }

    if (PUSH.LightCount > 0) {
        StructuredBuffer<GPULight> lights = ResourceDescriptorHeap[PUSH.LightBufferIndex];
        for (int i = 0; i < PUSH.LightCount; ++i)
        {
            float sf = (lights[i].Type == 0) ? shadowFactor : 1.0f;
            Lo += EvaluateLight(lights[i], worldPos, N, V, albedo, roughness, metallic, sf);
        }
    }

    float3 color = ambient + Lo + emissive;
    hdr[id.xy] = float4(color, 1.0f);
}
