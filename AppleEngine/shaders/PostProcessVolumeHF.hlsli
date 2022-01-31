#ifndef AP_POSTPROCESSVOLUME_HF
#define AP_POSTPROCESSVOLUME_HF

#include "../globals.hlsli"

//define
%s

PUSHCONSTANT(push, PostprocessVolumePush);

#define sampler_objectshader			bindless_samplers[GetFrame().sampler_objectshader_index]


#define sceneTexture bindless_textures[push.sceneTexture_index]


CBUFFER(MaterialParams, CBSLOT_MATERIALPARAMS)
{

%s

};



float4 main(float4 pos : SV_Position, float2 uv : TEXCOORD) : SV_Target
{
    
    
    
    float4 color = 1;
    float3 EmissiveColor = 0;
    
    %s
    
    
   
    color.rgb = EmissiveColor.rgb;

    
    return color;
    
}


#endif //AP_POSTPROCESSVOLUME_HF