
#include "ocean2HF.hlsli"


// ---------------------------------------------------------------------------
// Helper functions
// ---------------------------------------------------------------------------

float3 GFSDK_WaveWorks_GetUndisplacedVertexWorldPosition(GFSDK_WAVEWORKS_VERTEX_INPUT In)
{
    float2 vertexPos2D = In.vertexPos2D;
    PerInstanceElement instanceData = g_perInstanceData[In.instanceID];

	// Use geo-morphing in 2D on the plane to smooth away LOD boundaries.
    float geomorphConstant = abs(instanceData.patchMorphConstantAndSign);
    float geomorphSign = -sign(instanceData.patchMorphConstantAndSign);
    if (geomorphSign == 0.0)
        geomorphSign = 1.0;
    float2 geomorphOffset = geomorphSign.xx;

	// Calculating origin and target positions for geomorphing iteratively, 
	// as the geomorphing can morph vertices multiple (up to 4) levels down
    float geomorphScale = 1.0;
    float2 vertexPos2DSrc = vertexPos2D;
    float2 vertexPos2DTarget = vertexPos2D;
    float geomorphAmount = 0.0;

    for (int geomorphCurrentLevel = 0; geomorphCurrentLevel < 4; geomorphCurrentLevel++)
    {
        if (g_useDiamondPattern == 0)
        {
            float2 intpart;
            float2 rempart = modf(0.5 * geomorphScale * vertexPos2DSrc, intpart);

            if (rempart.x == 0.5)
                vertexPos2DTarget.x = vertexPos2DSrc.x + geomorphOffset.x;
            if (rempart.y == 0.5)
                vertexPos2DTarget.y = vertexPos2DSrc.y + geomorphOffset.y;
        }
        else
        {
			// Calculating target vertex positions is more complicated for diamond pattern
            float2 intpart;
            float2 rempart = modf(0.25 * geomorphScale * vertexPos2DSrc, intpart);
            float2 mirror = float2(1.0, 1.0);

            if (rempart.x > 0.5)
            {
                rempart.x = 1.0 - rempart.x;
                mirror.x = -mirror.x;
            }
            if (rempart.y > 0.5)
            {
                rempart.y = 1.0 - rempart.y;
                mirror.y = -mirror.y;
            }

            if (rempart.x == 0.25 && rempart.y == 0.25)
                vertexPos2DTarget.xy = vertexPos2DSrc.xy - geomorphOffset * mirror;
            else if (rempart.x == 0.25)
                vertexPos2DTarget.x = vertexPos2DSrc.x + geomorphOffset.x * mirror.x;
            else if (rempart.y == 0.25)
                vertexPos2DTarget.y = vertexPos2DSrc.y + geomorphOffset.y * mirror.y;
        }

		// Calculating target geomorphing LOD
        float3 worldspacePos = float3(vertexPos2DTarget * instanceData.patchWorldspaceScale + instanceData.patchWorldspaceOrigin, g_meanOceanLevel);
        float3 eyeVec = worldspacePos - g_eyePos;
        float d = length(eyeVec);
        float geomorphTargetLevel = log2(d * geomorphConstant) + 1.0;

        geomorphAmount = saturate(2.0 * (geomorphTargetLevel - float(geomorphCurrentLevel)));

        if (geomorphAmount < 1.0)
        {
            break;
        }
        else
        {
            vertexPos2DSrc = vertexPos2DTarget;
            geomorphScale *= 0.5;

            if (g_useDiamondPattern > 0)
            {
                geomorphOffset *= -2.0;
            }
            else
            {
                geomorphOffset *= 2.0;
            }
        }
    }

	// Lerping 
    vertexPos2D = lerp(vertexPos2DSrc, vertexPos2DTarget, geomorphAmount);

	// Transforming the patch vertices to worldspace
    return float3(vertexPos2D.xy * instanceData.patchWorldspaceScale + instanceData.patchWorldspaceOrigin, g_meanOceanLevel);
}

// ---------------------------------------------------------------------------
// Vertex shader
// ---------------------------------------------------------------------------
VS_OUTPUT main(GFSDK_WAVEWORKS_VERTEX_INPUT In)
{
    VS_OUTPUT Output;
    Output.worldspacePosition = float4(GFSDK_WaveWorks_GetUndisplacedVertexWorldPosition(In), 1.0);
    return Output;
}
