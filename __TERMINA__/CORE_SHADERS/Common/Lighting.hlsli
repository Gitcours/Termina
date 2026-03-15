#ifndef LIGHTING_HLSLI
#define LIGHTING_HLSLI

static const float PI = 3.14159265359f;

// -------------------------------------------------------------------
// GPU light struct — must match GPULight in GPULight.hpp (64 bytes)
// -------------------------------------------------------------------
struct GPULight
{
    float3 Position;   // World-space position (unused for directional)
    int    Type;       // 0=Directional, 1=Point, 2=Spot
    float3 Color;      // Linear RGB × intensity
    float  Range;      // Effective falloff range
    float3 Direction;  // Normalized world-space direction (directional/spot)
    float  InnerCos;   // Spot: cos(inner half-angle)
    float  OuterCos;   // Spot: cos(outer half-angle)
    float3 _pad;
};

// -------------------------------------------------------------------
// Cook-Torrance BRDF helpers
// -------------------------------------------------------------------

/// GGX normal distribution function
float D_GGX(float NdotH, float roughness)
{
    float a  = roughness * roughness;
    float a2 = a * a;
    float d  = NdotH * NdotH * (a2 - 1.0f) + 1.0f;
    return a2 / (PI * d * d);
}

/// Smith-Schlick geometry function (single term)
float G_SchlickGGX(float NdotV, float roughness)
{
    float r = roughness + 1.0f;
    float k = (r * r) / 8.0f;
    return NdotV / (NdotV * (1.0f - k) + k);
}

/// Smith combined geometry function
float G_Smith(float NdotV, float NdotL, float roughness)
{
    return G_SchlickGGX(NdotV, roughness) * G_SchlickGGX(NdotL, roughness);
}

/// Fresnel-Schlick approximation
float3 F_Schlick(float cosTheta, float3 F0)
{
    return F0 + (1.0f - F0) * pow(clamp(1.0f - cosTheta, 0.0f, 1.0f), 5.0f);
}

// -------------------------------------------------------------------
// Attenuation
// -------------------------------------------------------------------

/// Physically-based inverse-square falloff, smoothly clamped at range
float PointAttenuation(float dist, float range)
{
    float ratio = dist / range;
    float atten = 1.0f / (dist * dist + 1e-4f);
    float fade  = saturate(1.0f - ratio * ratio * ratio * ratio);
    return atten * fade * fade;
}

// -------------------------------------------------------------------
// Evaluate a single light contribution (Cook-Torrance specular + Lambertian diffuse)
// -------------------------------------------------------------------
float3 EvaluateLight(GPULight light,
                     float3   worldPos,
                     float3   N,
                     float3   V,
                     float3   albedo,
                     float    roughness,
                     float    metallic,
                     float    shadowFactor)
{
    float3 L       = float3(0.0f, 0.0f, 0.0f);
    float  radiance = 1.0f;

    if (light.Type == 0) // Directional
    {
        L       = normalize(-light.Direction);
        radiance = 1.0f;
    }
    else if (light.Type == 1) // Point
    {
        float3 toLight = light.Position - worldPos;
        float  dist    = length(toLight);
        L       = toLight / (dist + 1e-6f);
        radiance = PointAttenuation(dist, light.Range);
    }
    else if (light.Type == 2) // Spot
    {
        float3 toLight  = light.Position - worldPos;
        float  dist     = length(toLight);
        L        = toLight / (dist + 1e-6f);
        radiance = PointAttenuation(dist, light.Range);

        // Cone attenuation
        float  cosTheta = dot(-L, normalize(light.Direction));
        float  spotFade = saturate(
            (cosTheta - light.OuterCos) / (light.InnerCos - light.OuterCos + 1e-6f));
        radiance *= spotFade * spotFade;
    }

    float NdotL = max(dot(N, L), 0.0f);
    if (NdotL <= 0.0f || radiance <= 0.0f)
        return float3(0.0f, 0.0f, 0.0f);

    float3 H     = normalize(V + L);
    float  NdotV = max(dot(N, V), 0.0f);
    float  NdotH = max(dot(N, H), 0.0f);
    float  VdotH = max(dot(V, H), 0.0f);

    // Fresnel base reflectance — dielectrics ~0.04, metals use albedo
    float3 F0 = lerp(float3(0.04f, 0.04f, 0.04f), albedo, metallic);

    float3 F  = F_Schlick(VdotH, F0);
    float  D  = D_GGX(NdotH, roughness);
    float  G  = G_Smith(NdotV, NdotL, roughness);

    // Specular (Cook-Torrance)
    float3 specular = (D * G * F) / max(4.0f * NdotV * NdotL, 1e-4f);

    // Diffuse (Lambertian) — metals have no diffuse
    float3 kD    = (1.0f - F) * (1.0f - metallic);
    float3 diffuse = kD * albedo / PI;

    return (diffuse + specular) * light.Color * radiance * NdotL * shadowFactor;
}

#endif // LIGHTING_HLSLI
