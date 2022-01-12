#ifndef AP_OCEAN2_H
#define AP_OCEAN2_H

#include "globals.hlsli"
#include "Shaderinterop_Ocean2.h"


#define g_displacementTextureArrayWindWaves bindless_textures2DArray[displacementTextureArrayWindWaves]
#define g_displacementTextureLocalWaves bindless_textures[displacementTextureLocalWaves]
#define g_gradientsTextureArrayWindWaves bindless_textures2DArray[gradientsTextureArrayWindWaves]
#define g_momentsTextureArrayWindWaves bindless_textures2DArray[momentsTextureArrayWindWaves]
#define g_gradientsTextureLocalWaves bindless_textures[gradientsTextureLocalWaves]

#define g_textureFoam bindless_textures[textureFoam]
#define g_textureBubbles bindless_textures[textureBubbles]
#define g_textureWindGusts bindless_textures[textureWindGusts]

#define g_textureDynamicSkyDome bindless_textures[textureDynamicSkyDome]


#define g_samplerBilinear           sampler_aniso_wrap
#define g_samplerAnisotropic        sampler_aniso_wrap
#define g_samplerBilinearClamp      sampler_aniso_clamp

struct GFSDK_WAVEWORKS_VERTEX_INPUT
{
    float2 vertexPos2D : POSITION;
    uint instanceID : SV_InstanceID;
};

struct VS_OUTPUT
{
    float4 worldspacePosition : VSO;
};

struct HS_CONSTANTOUTPUT
{
    float edgeTessFactors[3] : SV_TessFactor;
    float insideTessFactor : SV_InsideTessFactor;
};

struct HS_OUTPUT
{
    float4 worldspacePosition : HSO;
};

struct DS_OUTPUT
{
    float4 clipSpacePosition : SV_Position;
    centroid float4 UVForCascade01 : TEXCOORD0;
    centroid float4 UVForCascade23 : TEXCOORD1;
    centroid float2 UVForLocalWaves : TEXCOORD2;
    centroid float4 cascadeBlendingFactors : TEXCOORD3;
    float3 worldspacePositionDisplaced : TEXCOORD4;
    float3 worldspacePositionUndisplaced : TEXCOORD5;
};


struct SURFACE_PARAMETERS
{
    float3 normal;
    float foamSurfaceFolding;
    float foamEnergy;
    float foamWaveHats;
    float2 firstOrderMoments;
    float2 secondOrderMomentsLowestLOD;
    float3 secondOrderMoments;
};



#endif // AP_OCEAN2_H



