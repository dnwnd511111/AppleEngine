#ifndef AP_SHADERINTEROP_OCEAN2_H
#define AP_SHADERINTEROP_OCEAN2_H

#include "ShaderInterop.h"

CBUFFER(OCEAN_VS_HS_DS_CBUFFER, 3)
{
    // Data used in vertex shader
    float4x4 g_matViewProj;
    float3 g_eyePos;
    float g_meanOceanLevel;
    // x16 bytes boundary
    float g_useDiamondPattern;

    // Data used in hull shader
    float g_dynamicTesselationAmount;
    float g_staticTesselationOffset;

    // Data used in domain shader
    // Wind waves related data
    float g_cascade0UVScale;
    // x16 bytes boundary
    float g_cascade1UVScale;
    float g_cascade2UVScale;
    float g_cascade3UVScale;
    float g_cascade0UVOffset;
    // x16 bytes boundary
    float g_cascade1UVOffset;
    float g_cascade2UVOffset;
    float g_cascade3UVOffset;
    float g_UVWarpingAmplitude;
    // x16 bytes boundary
    float g_UVWarpingFrequency;

    // Local waves related data
    float g_localWavesSimulationDomainWorldspaceSize;
    float2 g_localWavesSimulationDomainWorldspaceCenter;

    uint displacementTextureArrayWindWaves;
    uint displacementTextureLocalWaves;
    uint gradientsTextureArrayWindWaves;
    uint momentsTextureArrayWindWaves;

    uint gradientsTextureLocalWaves;
    uint textureFoam;
    uint textureBubbles;
    uint textureWindGusts;

    uint textureDynamicSkyDome;
    float3 pad1;

    // Defined by wind waves simulation
    float g_cascadeToCascadeScale;
    float g_windWavesTextureSizeInTexels;
    float2 pad2;

    // x16 bytes boundary
    float g_windWavesFoamWhitecapsThreshold;

    // Defined by local waves simulation
    float g_localWavesTextureSizeInTexels;
    float g_localWavesFoamWhitecapsThreshold;
    float g_SimulationDomainSize;

    // x16 bytes boundary
    float2 g_SimulationDomainCenter;
    // Defined by application
    float g_beckmannRoughness;
    float g_showCascades;
    // x16 bytes boundary
    float3 g_sunDirection;
    float g_sunIntensity;
    // x16 bytes boundary
    float3 pad3;
    float g_useMicrofacetFresnel;
    // x16 bytes boundary
    float g_useMicrofacetSpecular;
    float g_useMicrofacetReflection;
    float2 pad4;


    //
    float4 g_WaterColor;
    float4 g_WaterDeepColor;
    float4 g_WaterColorIntensity;




};


struct OCEAN_VS_CBUFFER_PERINSTANCE_ENTRY
{
    float2 patchWorldspaceOrigin;
    float patchWorldspaceScale;
    float patchMorphConstantAndSign;
};


CBUFFER(OCEAN_VS_CBUFFER_PERINSTANCE, 2)
{
    OCEAN_VS_CBUFFER_PERINSTANCE_ENTRY g_perInstanceData[4096];
};





#endif //AP_SHADERINTEROP_OCEAN2_H