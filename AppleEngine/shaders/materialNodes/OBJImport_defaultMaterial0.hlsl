
#pragma once
#define OBJECTSHADER_LAYOUT_COMMON
#include "../objectHF.hlsli"
//#include "../globals.hlsli"

//test




CBUFFER(MaterialParams, CBSLOT_MATERIALPARAMS)
{

int texture10003;


};

float4 main(PixelInput input) : SV_TARGET
{

float4 materialExpression10003 = bindless_textures[texture10003].Sample(sampler_objectshader,  input.uvsets.xy); 

float3 BaseColor = materialExpression10003.rgb;
float Opacity = 1;


return float4(BaseColor,Opacity);


}      

