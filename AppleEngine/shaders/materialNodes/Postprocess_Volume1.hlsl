#ifndef AP_POSTPROCESSVOLUME_HF
#define AP_POSTPROCESSVOLUME_HF

#include "../globals.hlsli"

//define


PUSHCONSTANT(push, PostprocessVolumePush);

#define sampler_objectshader			bindless_samplers[GetFrame().sampler_objectshader_index]


#define sceneTexture bindless_textures[push.sceneTexture_index]


CBUFFER(MaterialParams, CBSLOT_MATERIALPARAMS)
{

int texture10004;


};



float4 main(float4 pos : SV_Position, float2 uv : TEXCOORD) : SV_Target
{
    
    
    
    float4 color = 1;
    float3 EmissiveColor = 0;
    
    float4 materialExpression10004 = bindless_textures[texture10004].Sample(sampler_objectshader,uv.xy); 
float3 materialExpression10021 = sceneTexture.Sample(sampler_objectshader, uv).rgb.rgb + materialExpression10004.rgb;

EmissiveColor = materialExpression10021.rgb;

    
    
   
    color.rgb = EmissiveColor.rgb;

    
    return color;
    
}


#endif //AP_POSTPROCESSVOLUME_HF      