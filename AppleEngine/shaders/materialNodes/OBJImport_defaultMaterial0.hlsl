
#pragma once
#define OBJECTSHADER_LAYOUT_COMMON
#include "objectHF.hlsli"
//#include "globals.hlsli"

//test

struct MaterialParams
{

int texture10003;


};


CONSTANTBUFFER(g_xMaterialParams, MaterialParams, CBSLOT_MATERIALPARAMS);

float4 main(PixelInput input) : SV_TARGET
{

float4 materialExpression10003 = bindless_textures[g_xMaterialParams.texture10003].Sample(sampler_objectshader,  input.uvsets.xy); 

float3 BaseColor = materialExpression10003.rgb;
float Opacity = 1;


return float4(BaseColor,Opacity);


}      

