#ifndef AP_OCEAN2_H
#define AP_OCEAN2_H

#include "globals.hlsli"

struct Ocean2Constants
{
    uint displacementTextureArrayWindWaves;
    uint displacementTextureLocalWaves;
    uint gradientsTextureArrayWindWaves;
    uint momentsTextureArrayWindWaves;
         
    uint gradientsTextureLocalWaves;
    uint textureFoam;
    uint textureBubbles;
    uint textureWindGusts;
  
};

PUSHCONSTANT(push, Ocean2Constants);



#define g_displacementTextureArrayWindWaves bindless_textures2DArray[push.displacementTextureArrayWindWaves]
#define g_displacementTextureLocalWaves bindless_textures[push.displacementTextureLocalWaves]
#define g_gradientsTextureArrayWindWaves bindless_textures2DArray[push.gradientsTextureArrayWindWaves]
#define g_momentsTextureArrayWindWaves bindless_textures2DArray[push.momentsTextureArrayWindWaves]
#define g_gradientsTextureLocalWaves bindless_textures[push.gradientsTextureLocalWaves]

#define g_textureFoam bindless_textures[push.textureFoam]
#define g_textureBubbles bindless_textures[push.textureBubbles]
#define g_textureWindGusts bindless_textures[push.textureWindGusts]


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

struct PerInstanceElement
{
    float2 patchWorldspaceOrigin;
    float patchWorldspaceScale;
    float patchMorphConstantAndSign;
};



cbuffer OCEAN_VS_CBUFFER_PERINSTANCE : register(b0)
{
    PerInstanceElement g_perInstanceData[4096];
};

cbuffer OCEAN_VS_HS_DS_CBUFFER : register(b1)
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
};

cbuffer OCEAN_PS_CBUFFER : register(b2)
{
	// Defined by wind waves simulation
    float g_cascadeToCascadeScale;
    float g_windWavesTextureSizeInTexels;
    float g_UVWarpingAmplitude2;
    float g_UVWarpingFrequency2;
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
    float3 g_eyePos2;
    float g_useMicrofacetFresnel;
											// x16 bytes boundary
    float g_useMicrofacetSpecular;
    float g_useMicrofacetReflection;

	// CBs must be multiple of 16 bytes large
    float pad0;
    float pad1;
}

#endif // AP_OCEAN2_H



