
#pragma once
#define OBJECTSHADER_LAYOUT_COMMON
#include "../objectHF.hlsli"
//#include "../globals.hlsli"

//test




CBUFFER(MaterialParams, CBSLOT_MATERIALPARAMS)
{

float3 materialExpression10013;
float pad10013;


};

float4 main(PixelInput input) : SV_TARGET
{


float3 BaseColor = materialExpression10013.rgb;
float Opacity = 1;


return float4(BaseColor,Opacity);


}      

