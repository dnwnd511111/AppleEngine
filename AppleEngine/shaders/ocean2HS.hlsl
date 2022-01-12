
#include "ocean2HF.hlsli"


// ---------------------------------------------------------------------------
// Helper functions
// ---------------------------------------------------------------------------
float GFSDK_WaveWorks_GetEdgeTessellationFactor(float4 vertex1, float4 vertex2)
{
    float3 edgeCenter = 0.5 * (vertex1.xyz + vertex2.xyz);
    float edgeLength = length(vertex1.xyz - vertex2.xyz);
    float distanceToEdge = length(g_eyePos - edgeCenter);
    return g_staticTesselationOffset + g_dynamicTesselationAmount * edgeLength / distanceToEdge;
}

// ---------------------------------------------------------------------------
// Hull constant shader
// ---------------------------------------------------------------------------

HS_CONSTANTOUTPUT HS_Constant(InputPatch<VS_OUTPUT, 3> I)
{
    HS_CONSTANTOUTPUT Output;
    Output.edgeTessFactors[0] = GFSDK_WaveWorks_GetEdgeTessellationFactor(I[1].worldspacePosition, I[2].worldspacePosition);
    Output.edgeTessFactors[1] = GFSDK_WaveWorks_GetEdgeTessellationFactor(I[2].worldspacePosition, I[0].worldspacePosition);
    Output.edgeTessFactors[2] = GFSDK_WaveWorks_GetEdgeTessellationFactor(I[0].worldspacePosition, I[1].worldspacePosition);
    Output.insideTessFactor = (Output.edgeTessFactors[0] + Output.edgeTessFactors[1] + Output.edgeTessFactors[2]) / 3.0;
    return Output;
}

// ---------------------------------------------------------------------------
// Hull shader
// ---------------------------------------------------------------------------
[domain("tri")]
[partitioning("fractional_odd")]
[outputtopology("triangle_cw")]
[patchconstantfunc("HS_Constant")]
[outputcontrolpoints(3)]
HS_OUTPUT main(InputPatch<VS_OUTPUT, 3> I, uint uCPID : SV_OutputControlPointID)
{
    HS_OUTPUT Output;
    Output.worldspacePosition = float4(I[uCPID].worldspacePosition);
    return Output;
}
