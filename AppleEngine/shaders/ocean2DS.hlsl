
#include "ocean2HF.hlsli"


// ---------------------------------------------------------------------------
// Helper functions
// ---------------------------------------------------------------------------
DS_OUTPUT GFSDK_WaveWorks_GetDisplacedVertexAfterTessellation(float4 In0, float4 In1, float4 In2, float3 BarycentricCoords)
{
	// Get starting position
    precise float3 tessellatedWorldspacePosition = In0.xyz * BarycentricCoords.x + In1.xyz * BarycentricCoords.y + In2.xyz * BarycentricCoords.z;
    float3 worldspacePositionUndisplaced = tessellatedWorldspacePosition;
	
	// Blending factors for cascades	
    float distance = length(g_eyePos - worldspacePositionUndisplaced);
    float4 cascadeWordldspaceSizes = float4(1.0 / g_cascade0UVScale, 1.0 / g_cascade1UVScale, 1.0 / g_cascade2UVScale, 1.0 / g_cascade3UVScale);
    float4 cascadeBlendingFactors = float4(1.0, cascadeBlendingFactors.yzw = saturate(0.033 * (cascadeWordldspaceSizes.yzw * 30.0 - distance) / cascadeWordldspaceSizes.yzw));

	// UVs
    float2 UVForCascade0 = worldspacePositionUndisplaced.xy * g_cascade0UVScale + g_cascade0UVOffset.xx;
    float2 UVForCascade1 = worldspacePositionUndisplaced.xy * g_cascade1UVScale + g_cascade1UVOffset.xx;
    float2 UVForCascade2 = worldspacePositionUndisplaced.xy * g_cascade2UVScale + g_cascade2UVOffset.xx;
    float2 UVForCascade3 = worldspacePositionUndisplaced.xy * g_cascade3UVScale + g_cascade3UVOffset.xx;
    float2 UVForLocalWaves = float2(0.5, 0.5) + (worldspacePositionUndisplaced.xy - g_localWavesSimulationDomainWorldspaceCenter) / g_localWavesSimulationDomainWorldspaceSize;

    UVForCascade0 += float2(g_UVWarpingAmplitude * cos(UVForCascade0.y * g_UVWarpingFrequency), g_UVWarpingAmplitude * sin(UVForCascade0.x * g_UVWarpingFrequency));
    UVForCascade1 += float2(g_UVWarpingAmplitude * cos(UVForCascade1.y * g_UVWarpingFrequency), g_UVWarpingAmplitude * sin(UVForCascade1.x * g_UVWarpingFrequency));
    UVForCascade2 += float2(g_UVWarpingAmplitude * cos(UVForCascade2.y * g_UVWarpingFrequency), g_UVWarpingAmplitude * sin(UVForCascade2.x * g_UVWarpingFrequency));
    UVForCascade3 += float2(g_UVWarpingAmplitude * cos(UVForCascade3.y * g_UVWarpingFrequency), g_UVWarpingAmplitude * sin(UVForCascade3.x * g_UVWarpingFrequency));

	// displacements
    float3 displacement;
    displacement = cascadeBlendingFactors.x * g_displacementTextureArrayWindWaves.SampleLevel(g_samplerBilinear, float3(UVForCascade0, 0.0), 0.0).xyz;
    displacement += cascadeBlendingFactors.y == 0 ? float3(0, 0, 0) : cascadeBlendingFactors.y * g_displacementTextureArrayWindWaves.SampleLevel(g_samplerBilinear, float3(UVForCascade1, 1.0), 0.0).xyz;
    displacement += cascadeBlendingFactors.z == 0 ? float3(0, 0, 0) : cascadeBlendingFactors.z * g_displacementTextureArrayWindWaves.SampleLevel(g_samplerBilinear, float3(UVForCascade2, 2.0), 0.0).xyz;
    displacement += cascadeBlendingFactors.w == 0 ? float3(0, 0, 0) : cascadeBlendingFactors.w * g_displacementTextureArrayWindWaves.SampleLevel(g_samplerBilinear, float3(UVForCascade3, 3.0), 0.0).xyz;
    displacement += g_displacementTextureLocalWaves.SampleLevel(g_samplerBilinearClamp, UVForLocalWaves, 0.0).xyz;
    float3 worldspacePositionDisplaced = worldspacePositionUndisplaced + displacement;
	
	// Output
    DS_OUTPUT Output;
    Output.UVForCascade01.xy = UVForCascade0;
    Output.UVForCascade01.zw = UVForCascade1;
    Output.UVForCascade23.xy = UVForCascade2;
    Output.UVForCascade23.zw = UVForCascade3;
    Output.UVForLocalWaves = UVForLocalWaves;
    Output.cascadeBlendingFactors = cascadeBlendingFactors;
    Output.worldspacePositionDisplaced = worldspacePositionDisplaced;
    Output.worldspacePositionUndisplaced = worldspacePositionUndisplaced;
	
	// Output.clipSpacePosition is not initialized here
    return Output;
}


// ---------------------------------------------------------------------------
// Domain shader
// ---------------------------------------------------------------------------
[domain("tri")]
DS_OUTPUT main(HS_CONSTANTOUTPUT HSConstantData, const OutputPatch<HS_OUTPUT, 3> I, float3 f3BarycentricCoords : SV_DomainLocation)
{
    DS_OUTPUT Output = GFSDK_WaveWorks_GetDisplacedVertexAfterTessellation(I[0].worldspacePosition, I[1].worldspacePosition, I[2].worldspacePosition, f3BarycentricCoords);
    Output.clipSpacePosition = mul(g_matViewProj, float4(Output.worldspacePositionDisplaced, 1.0));
    return Output;
}
