#pragma once

//define


#define OBJECTSHADER_COMPILE_PS
#define OBJECTSHADER_LAYOUT_PREPASS
#define PREPASS
#define DISABLE_ALPHATEST
 


#ifdef TRANSPARENT
#define DISABLE_TRANSPARENT_SHADOWMAP
#endif

#ifdef PLANARREFLECTION
#define DISABLE_ENVMAPS
#define DISABLE_VOXELGI
#endif

#ifdef WATER
#define DISABLE_VOXELGI
#endif

#define LIGHTMAP_QUALITY_BICUBICOBJECTSHADER_USE_COLOR


#include "../globals.hlsli"
#include "../brdf.hlsli"
#include "../lightingHF.hlsli"
#include "../ShaderInterop_SurfelGI.h"

// DEFINITIONS
//////////////////

PUSHCONSTANT(push, ObjectPushConstants);


CBUFFER(MaterialParams, CBSLOT_MATERIALPARAMS)
{

int texture10003;
int texture10033;
int texture10055;


};


inline uint GetSubsetIndex()
{
    return push.GetSubsetIndex();
}
inline ShaderMesh GetMesh()
{
    return load_mesh(push.GetMeshIndex());
}
inline ShaderMaterial GetMaterial()
{
    return load_material(push.GetMaterialIndex());
}
inline ShaderMaterial GetMaterial1()
{
    return load_material(GetMesh().blendmaterial1);
}
inline ShaderMaterial GetMaterial2()
{
    return load_material(GetMesh().blendmaterial2);
}
inline ShaderMaterial GetMaterial3()
{
    return load_material(GetMesh().blendmaterial3);
}

#define sampler_objectshader			bindless_samplers[GetFrame().sampler_objectshader_index]

#define texture_basecolormap			bindless_textures[GetMaterial().texture_basecolormap_index]
#define texture_normalmap				bindless_textures[GetMaterial().texture_normalmap_index]
#define texture_surfacemap				bindless_textures[GetMaterial().texture_surfacemap_index]
#define texture_emissivemap				bindless_textures[GetMaterial().texture_emissivemap_index]
#define texture_displacementmap			bindless_textures[GetMaterial().texture_displacementmap_index]
#define texture_occlusionmap			bindless_textures[GetMaterial().texture_occlusionmap_index]
#define texture_transmissionmap			bindless_textures[GetMaterial().texture_transmissionmap_index]
#define texture_sheencolormap			bindless_textures[GetMaterial().texture_sheencolormap_index]
#define texture_sheenroughnessmap		bindless_textures[GetMaterial().texture_sheenroughnessmap_index]
#define texture_clearcoatmap			bindless_textures[GetMaterial().texture_clearcoatmap_index]
#define texture_clearcoatroughnessmap	bindless_textures[GetMaterial().texture_clearcoatroughnessmap_index]
#define texture_clearcoatnormalmap		bindless_textures[GetMaterial().texture_clearcoatnormalmap_index]
#define texture_specularmap				bindless_textures[GetMaterial().texture_specularmap_index]

#define texture_blend1_basecolormap		bindless_textures[GetMaterial1().texture_basecolormap_index]
#define texture_blend1_normalmap		bindless_textures[GetMaterial1().texture_normalmap_index]
#define texture_blend1_surfacemap		bindless_textures[GetMaterial1().texture_surfacemap_index]
#define texture_blend1_emissivemap		bindless_textures[GetMaterial1().texture_emissivemap_index]

#define texture_blend2_basecolormap		bindless_textures[GetMaterial2().texture_basecolormap_index]
#define texture_blend2_normalmap		bindless_textures[GetMaterial2().texture_normalmap_index]
#define texture_blend2_surfacemap		bindless_textures[GetMaterial2().texture_surfacemap_index]
#define texture_blend2_emissivemap		bindless_textures[GetMaterial2().texture_emissivemap_index]

#define texture_blend3_basecolormap		bindless_textures[GetMaterial3().texture_basecolormap_index]
#define texture_blend3_normalmap		bindless_textures[GetMaterial3().texture_normalmap_index]
#define texture_blend3_surfacemap		bindless_textures[GetMaterial3().texture_surfacemap_index]
#define texture_blend3_emissivemap		bindless_textures[GetMaterial3().texture_emissivemap_index]

uint load_entitytile(uint tileIndex)
{
#ifdef TRANSPARENT
	return bindless_buffers[GetCamera().buffer_entitytiles_transparent_index].Load(tileIndex * sizeof(uint));
#else
    return bindless_buffers[GetCamera().buffer_entitytiles_opaque_index].Load(tileIndex * sizeof(uint));
#endif // TRANSPARENT
}

#ifdef OBJECTSHADER_LAYOUT_SHADOW
#define OBJECTSHADER_USE_WIND
#endif // OBJECTSHADER_LAYOUT_SHADOW

#ifdef OBJECTSHADER_LAYOUT_SHADOW_TEX
#define OBJECTSHADER_USE_WIND
#define OBJECTSHADER_USE_UVSETS
#endif // OBJECTSHADER_LAYOUT_SHADOW_TEX

#ifdef OBJECTSHADER_LAYOUT_PREPASS
#define OBJECTSHADER_USE_CLIPPLANE
#define OBJECTSHADER_USE_WIND
#define OBJECTSHADER_USE_INSTANCEID
#endif // OBJECTSHADER_LAYOUT_SHADOW

#ifdef OBJECTSHADER_LAYOUT_PREPASS_TEX
#define OBJECTSHADER_USE_CLIPPLANE
#define OBJECTSHADER_USE_WIND
#define OBJECTSHADER_USE_UVSETS
#define OBJECTSHADER_USE_DITHERING
#define OBJECTSHADER_USE_INSTANCEID
#endif // OBJECTSHADER_LAYOUT_SHADOW_TEX

#ifdef OBJECTSHADER_LAYOUT_COMMON
#define OBJECTSHADER_USE_CLIPPLANE
#define OBJECTSHADER_USE_WIND
#define OBJECTSHADER_USE_UVSETS
#define OBJECTSHADER_USE_ATLAS
#define OBJECTSHADER_USE_COLOR
#define OBJECTSHADER_USE_NORMAL
#define OBJECTSHADER_USE_TANGENT
#define OBJECTSHADER_USE_POSITION3D
#define OBJECTSHADER_USE_EMISSIVE
#define OBJECTSHADER_USE_INSTANCEID
#endif // OBJECTSHADER_LAYOUT_COMMON


struct PixelInput
{
    precise float4 pos : SV_POSITION;

#ifdef OBJECTSHADER_USE_INSTANCEID
	uint instanceID : INSTANCEID;
#endif // OBJECTSHADER_USE_INSTANCEID

#ifdef OBJECTSHADER_USE_CLIPPLANE
	float  clip : SV_ClipDistance0;
#endif // OBJECTSHADER_USE_CLIPPLANE

#ifdef OBJECTSHADER_USE_DITHERING
	nointerpolation float dither : DITHER;
#endif // OBJECTSHADER_USE_DITHERING

#ifdef OBJECTSHADER_USE_EMISSIVE
	uint emissiveColor : EMISSIVECOLOR;
#endif // OBJECTSHADER_USE_EMISSIVE

#ifdef OBJECTSHADER_USE_COLOR
	float4 color : COLOR;
#endif // OBJECTSHADER_USE_COLOR

#ifdef OBJECTSHADER_USE_UVSETS
	float4 uvsets : UVSETS;
#endif // OBJECTSHADER_USE_UVSETS

#ifdef OBJECTSHADER_USE_ATLAS
	float2 atl : ATLAS;
#endif // OBJECTSHADER_USE_ATLAS

#ifdef OBJECTSHADER_USE_NORMAL
	float3 nor : NORMAL;
#endif // OBJECTSHADER_USE_NORMAL

#ifdef OBJECTSHADER_USE_TANGENT
	float4 tan : TANGENT;
#endif // OBJECTSHADER_USE_TANGENT

#ifdef OBJECTSHADER_USE_POSITION3D
	float3 pos3D : WORLDPOSITION;
#endif // OBJECTSHADER_USE_POSITION3D

#ifdef OBJECTSHADER_USE_RENDERTARGETARRAYINDEX
#ifdef VPRT_EMULATION
	uint RTIndex : RTINDEX;
#else
	uint RTIndex : SV_RenderTargetArrayIndex;
#endif // VPRT_EMULATION
#endif // OBJECTSHADER_USE_RENDERTARGETARRAYINDEX
};


// METHODS
////////////

inline void ApplyEmissive(in Surface surface, inout Lighting lighting)
{
    lighting.direct.specular += surface.emissiveColor;
}

inline void LightMapping(in int lightmap, in float2 ATLAS, inout Lighting lighting)
{
	[branch]
    if (lightmap >= 0 && any(ATLAS))
    {
        Texture2D<float4> texture_lightmap = bindless_textures[NonUniformResourceIndex(lightmap)];
#ifdef LIGHTMAP_QUALITY_BICUBIC
		lighting.indirect.diffuse = SampleTextureCatmullRom(texture_lightmap, sampler_linear_clamp, ATLAS).rgb;
#else
        lighting.indirect.diffuse = texture_lightmap.SampleLevel(sampler_linear_clamp, ATLAS, 0).rgb;
#endif // LIGHTMAP_QUALITY_BICUBIC
    }
}

inline void NormalMapping(in float4 uvsets, inout float3 N, in float3x3 TBN, out float3 bumpColor)
{
	[branch]
    if (GetMaterial().normalMapStrength > 0 && GetMaterial().uvset_normalMap >= 0)
    {
        const float2 UV_normalMap = GetMaterial().uvset_normalMap == 0 ? uvsets.xy : uvsets.zw;
        float3 normalMap = float3(texture_normalmap.Sample(sampler_objectshader, UV_normalMap).rg, 1);
        bumpColor = normalMap.rgb * 2 - 1;
        N = normalize(lerp(N, mul(bumpColor, TBN), GetMaterial().normalMapStrength));
        bumpColor *= GetMaterial().normalMapStrength;
    }
    else
    {
        bumpColor = 0;
    }
}

inline float3 PlanarReflection(in Surface surface, in float2 bumpColor)
{
	[branch]
    if (GetCamera().texture_reflection_index >= 0)
    {
        float4 reflectionUV = mul(GetCamera().reflection_view_projection, float4(surface.P, 1));
        reflectionUV.xy /= reflectionUV.w;
        reflectionUV.xy = reflectionUV.xy * float2(0.5, -0.5) + 0.5;
        return bindless_textures[GetCamera().texture_reflection_index].SampleLevel(sampler_linear_clamp, reflectionUV.xy + bumpColor * GetMaterial().normalMapStrength, 0).rgb;
    }
    return 0;
}

#define NUM_PARALLAX_OCCLUSION_STEPS 32
inline void ParallaxOcclusionMapping(inout float4 uvsets, in float3 V, in float3x3 TBN)
{
	[branch]
    if (GetMaterial().parallaxOcclusionMapping > 0 && GetMaterial().uvset_displacementMap >= 0)
    {
        V = mul(TBN, V);
        float layerHeight = 1.0 / NUM_PARALLAX_OCCLUSION_STEPS;
        float curLayerHeight = 0;
        float2 dtex = GetMaterial().parallaxOcclusionMapping * V.xy / NUM_PARALLAX_OCCLUSION_STEPS;
        float2 originalTextureCoords = GetMaterial().uvset_displacementMap == 0 ? uvsets.xy : uvsets.zw;
        float2 currentTextureCoords = originalTextureCoords;
        float2 derivX = ddx_coarse(currentTextureCoords);
        float2 derivY = ddy_coarse(currentTextureCoords);
        float heightFromTexture = 1 - texture_displacementmap.SampleGrad(sampler_linear_wrap, currentTextureCoords, derivX, derivY).r;
        uint iter = 0;
		[loop]
        while (heightFromTexture > curLayerHeight && iter < NUM_PARALLAX_OCCLUSION_STEPS)
        {
            curLayerHeight += layerHeight;
            currentTextureCoords -= dtex;
            heightFromTexture = 1 - texture_displacementmap.SampleGrad(sampler_linear_wrap, currentTextureCoords, derivX, derivY).r;
            iter++;
        }
        float2 prevTCoords = currentTextureCoords + dtex;
        float nextH = heightFromTexture - curLayerHeight;
        float prevH = 1 - texture_displacementmap.SampleGrad(sampler_linear_wrap, prevTCoords, derivX, derivY).r - curLayerHeight + layerHeight;
        float weight = nextH / (nextH - prevH);
        float2 finalTextureCoords = mad(prevTCoords, weight, currentTextureCoords * (1.0 - weight));
        float2 difference = finalTextureCoords - originalTextureCoords;
        uvsets += difference.xyxy;
    }
}

inline void ForwardLighting(inout Surface surface, inout Lighting lighting)
{
#ifndef DISABLE_DECALS
	[branch]
    if (xForwardDecalMask != 0)
    {
		// decals are enabled, loop through them first:
        float4 decalAccumulation = 0;
        const float3 P_dx = ddx_coarse(surface.P);
        const float3 P_dy = ddy_coarse(surface.P);

        uint bucket_bits = xForwardDecalMask;

		[loop]
        while (bucket_bits != 0)
        {
			// Retrieve global entity index from local bucket, then remove bit from local bucket:
            const uint bucket_bit_index = firstbitlow(bucket_bits);
            const uint entity_index = bucket_bit_index;
            bucket_bits ^= 1u << bucket_bit_index;

			[branch]
            if (decalAccumulation.a < 1)
            {
                ShaderEntity decal = load_entity(GetFrame().decalarray_offset + entity_index);
                if ((decal.layerMask & surface.layerMask) == 0)
                    continue;

                float4x4 decalProjection = load_entitymatrix(decal.GetMatrixIndex());
                int decalTexture = asint(decalProjection[3][0]);
                int decalNormal = asint(decalProjection[3][1]);
                decalProjection[3] = float4(0, 0, 0, 1);
                const float3 clipSpacePos = mul(decalProjection, float4(surface.P, 1)).xyz;
                const float3 uvw = clipSpacePos.xyz * float3(0.5, -0.5, 0.5) + 0.5;
				[branch]
                if (is_saturated(uvw))
                {
                    float4 decalColor = 1;
					[branch]
                    if (decalTexture >= 0)
                    {
						// mipmapping needs to be performed by hand:
                        const float2 decalDX = mul(P_dx, (float3x3) decalProjection).xy;
                        const float2 decalDY = mul(P_dy, (float3x3) decalProjection).xy;
                        decalColor = bindless_textures[NonUniformResourceIndex(decalTexture)].SampleGrad(sampler_objectshader, uvw.xy, decalDX, decalDY);
                    }
					// blend out if close to cube Z:
                    float edgeBlend = 1 - pow(saturate(abs(clipSpacePos.z)), 8);
                    decalColor.a *= edgeBlend;
                    decalColor *= decal.GetColor();
					// apply emissive:
                    lighting.direct.specular += max(0, decalColor.rgb * decal.GetEmissive() * edgeBlend);
					// perform manual blending of decals:
					//  NOTE: they are sorted top-to-bottom, but blending is performed bottom-to-top
                    decalAccumulation.rgb = mad(1 - decalAccumulation.a, decalColor.a * decalColor.rgb, decalAccumulation.rgb);
                    decalAccumulation.a = mad(1 - decalColor.a, decalAccumulation.a, decalColor.a);
					[branch]
                    if (decalAccumulation.a >= 1.0)
                    {
						// force exit:
                        break;
                    }
                }
            }
            else
            {
				// force exit:
                break;
            }

        }

        surface.albedo.rgb = lerp(surface.albedo.rgb, decalAccumulation.rgb, decalAccumulation.a);
    }
#endif // DISABLE_DECALS


#ifndef DISABLE_ENVMAPS
	// Apply environment maps:
    float4 envmapAccumulation = 0;

#ifndef ENVMAPRENDERING
#ifndef DISABLE_LOCALENVPMAPS
	[branch]
    if (xForwardEnvProbeMask != 0)
    {
        uint bucket_bits = xForwardEnvProbeMask;

		[loop]
        while (bucket_bits != 0)
        {
			// Retrieve global entity index from local bucket, then remove bit from local bucket:
            const uint bucket_bit_index = firstbitlow(bucket_bits);
            const uint entity_index = bucket_bit_index;
            bucket_bits ^= 1u << bucket_bit_index;

			[branch]
            if (envmapAccumulation.a < 1)
            {
                ShaderEntity probe = load_entity(GetFrame().envprobearray_offset + entity_index);
                if ((probe.layerMask & surface.layerMask) == 0)
                    continue;

                const float4x4 probeProjection = load_entitymatrix(probe.GetMatrixIndex());
                const float3 clipSpacePos = mul(probeProjection, float4(surface.P, 1)).xyz;
                const float3 uvw = clipSpacePos.xyz * float3(0.5, -0.5, 0.5) + 0.5;
				[branch]
                if (is_saturated(uvw))
                {
                    const float4 envmapColor = EnvironmentReflection_Local(surface, probe, probeProjection, clipSpacePos);
					// perform manual blending of probes:
					//  NOTE: they are sorted top-to-bottom, but blending is performed bottom-to-top
                    envmapAccumulation.rgb = mad(1 - envmapAccumulation.a, envmapColor.a * envmapColor.rgb, envmapAccumulation.rgb);
                    envmapAccumulation.a = mad(1 - envmapColor.a, envmapAccumulation.a, envmapColor.a);
					[branch]
                    if (envmapAccumulation.a >= 1.0)
                    {
						// force exit:
                        break;
                    }
                }
            }
            else
            {
				// force exit:
                break;
            }

        }
    }
#endif // DISABLE_LOCALENVPMAPS
#endif //ENVMAPRENDERING

	// Apply global envmap where there is no local envmap information:
	[branch]
    if (envmapAccumulation.a < 0.99)
    {
        envmapAccumulation.rgb = lerp(EnvironmentReflection_Global(surface), envmapAccumulation.rgb, envmapAccumulation.a);
    }
    lighting.indirect.specular += max(0, envmapAccumulation.rgb);
#endif // DISABLE_ENVMAPS

#ifndef DISABLE_VOXELGI
    VoxelGI(surface, lighting);
#endif //DISABLE_VOXELGI

	[branch]
    if (any(xForwardLightMask))
    {
		// Loop through light buckets for the draw call:
        const uint first_item = 0;
        const uint last_item = first_item + GetFrame().lightarray_count - 1;
        const uint first_bucket = first_item / 32;
        const uint last_bucket = min(last_item / 32, 1); // only 2 buckets max (uint2) for forward pass!
		[loop]
        for (uint bucket = first_bucket; bucket <= last_bucket; ++bucket)
        {
            uint bucket_bits = xForwardLightMask[bucket];

			[loop]
            while (bucket_bits != 0)
            {
				// Retrieve global entity index from local bucket, then remove bit from local bucket:
                const uint bucket_bit_index = firstbitlow(bucket_bits);
                const uint entity_index = bucket * 32 + bucket_bit_index;
                bucket_bits ^= 1u << bucket_bit_index;

                ShaderEntity light = load_entity(GetFrame().lightarray_offset + entity_index);
                if ((light.layerMask & surface.layerMask) == 0)
                    continue;

                if (light.GetFlags() & ENTITY_FLAG_LIGHT_STATIC)
                {
                    continue; // static lights will be skipped (they are used in lightmap baking)
                }

                switch (light.GetType())
                {
                    case ENTITY_TYPE_DIRECTIONALLIGHT:
				{
                            light_directional(light, surface, lighting);
                        }
                        break;
                    case ENTITY_TYPE_POINTLIGHT:
				{
                            light_point(light, surface, lighting);
                        }
                        break;
                    case ENTITY_TYPE_SPOTLIGHT:
				{
                            light_spot(light, surface, lighting);
                        }
                        break;
                }
            }
        }
    }

}

inline void TiledLighting(inout Surface surface, inout Lighting lighting)
{
    const uint2 tileIndex = uint2(floor(surface.pixel / TILED_CULLING_BLOCKSIZE));
    const uint flatTileIndex = flatten2D(tileIndex, GetCamera().entity_culling_tilecount.xy) * SHADER_ENTITY_TILE_BUCKET_COUNT;


#ifndef DISABLE_DECALS
	[branch]
    if (GetFrame().decalarray_count > 0)
    {
		// decals are enabled, loop through them first:
        float4 decalAccumulation = 0;
        const float3 P_dx = ddx_coarse(surface.P);
        const float3 P_dy = ddy_coarse(surface.P);

		// Loop through decal buckets in the tile:
        const uint first_item = GetFrame().decalarray_offset;
        const uint last_item = first_item + GetFrame().decalarray_count - 1;
        const uint first_bucket = first_item / 32;
        const uint last_bucket = min(last_item / 32, max(0, SHADER_ENTITY_TILE_BUCKET_COUNT - 1));
		[loop]
        for (uint bucket = first_bucket; bucket <= last_bucket; ++bucket)
        {
            uint bucket_bits = load_entitytile(flatTileIndex + bucket);

			// This is the wave scalarizer from Improved Culling - Siggraph 2017 [Drobot]:
            bucket_bits = WaveReadLaneFirst(WaveActiveBitOr(bucket_bits));

			[loop]
            while (bucket_bits != 0)
            {
				// Retrieve global entity index from local bucket, then remove bit from local bucket:
                const uint bucket_bit_index = firstbitlow(bucket_bits);
                const uint entity_index = bucket * 32 + bucket_bit_index;
                bucket_bits ^= 1u << bucket_bit_index;

				[branch]
                if (entity_index >= first_item && entity_index <= last_item && decalAccumulation.a < 1)
                {
                    ShaderEntity decal = load_entity(entity_index);
                    if ((decal.layerMask & surface.layerMask) == 0)
                        continue;

                    float4x4 decalProjection = load_entitymatrix(decal.GetMatrixIndex());
                    int decalTexture = asint(decalProjection[3][0]);
                    int decalNormal = asint(decalProjection[3][1]);
                    decalProjection[3] = float4(0, 0, 0, 1);
                    const float3 clipSpacePos = mul(decalProjection, float4(surface.P, 1)).xyz;
                    const float3 uvw = clipSpacePos.xyz * float3(0.5, -0.5, 0.5) + 0.5;
					[branch]
                    if (is_saturated(uvw))
                    {
						// mipmapping needs to be performed by hand:
                        float4 decalColor = 1;
						[branch]
                        if (decalTexture >= 0)
                        {
                            const float2 decalDX = mul(P_dx, (float3x3) decalProjection).xy;
                            const float2 decalDY = mul(P_dy, (float3x3) decalProjection).xy;
                            decalColor = bindless_textures[NonUniformResourceIndex(decalTexture)].SampleGrad(sampler_objectshader, uvw.xy, decalDX, decalDY);
                        }
						// blend out if close to cube Z:
                        float edgeBlend = 1 - pow(saturate(abs(clipSpacePos.z)), 8);
                        decalColor.a *= edgeBlend;
                        decalColor *= decal.GetColor();
						// apply emissive:
                        lighting.direct.specular += max(0, decalColor.rgb * decal.GetEmissive() * edgeBlend);
						// perform manual blending of decals:
						//  NOTE: they are sorted top-to-bottom, but blending is performed bottom-to-top
                        decalAccumulation.rgb = mad(1 - decalAccumulation.a, decalColor.a * decalColor.rgb, decalAccumulation.rgb);
                        decalAccumulation.a = mad(1 - decalColor.a, decalAccumulation.a, decalColor.a);
						[branch]
                        if (decalAccumulation.a >= 1.0)
                        {
							// force exit:
                            bucket = SHADER_ENTITY_TILE_BUCKET_COUNT;
                            break;
                        }
                    }
                }
                else if (entity_index > last_item)
                {
					// force exit:
                    bucket = SHADER_ENTITY_TILE_BUCKET_COUNT;
                    break;
                }

            }
        }

        surface.albedo.rgb = lerp(surface.albedo.rgb, decalAccumulation.rgb, decalAccumulation.a);
    }
#endif // DISABLE_DECALS


#ifndef DISABLE_ENVMAPS
	// Apply environment maps:
    float4 envmapAccumulation = 0;

#ifndef DISABLE_LOCALENVPMAPS
	[branch]
    if (GetFrame().envprobearray_count > 0)
    {
		// Loop through envmap buckets in the tile:
        const uint first_item = GetFrame().envprobearray_offset;
        const uint last_item = first_item + GetFrame().envprobearray_count - 1;
        const uint first_bucket = first_item / 32;
        const uint last_bucket = min(last_item / 32, max(0, SHADER_ENTITY_TILE_BUCKET_COUNT - 1));
		[loop]
        for (uint bucket = first_bucket; bucket <= last_bucket; ++bucket)
        {
            uint bucket_bits = load_entitytile(flatTileIndex + bucket);

			// Bucket scalarizer - Siggraph 2017 - Improved Culling [Michal Drobot]:
            bucket_bits = WaveReadLaneFirst(WaveActiveBitOr(bucket_bits));

			[loop]
            while (bucket_bits != 0)
            {
				// Retrieve global entity index from local bucket, then remove bit from local bucket:
                const uint bucket_bit_index = firstbitlow(bucket_bits);
                const uint entity_index = bucket * 32 + bucket_bit_index;
                bucket_bits ^= 1u << bucket_bit_index;

				[branch]
                if (entity_index >= first_item && entity_index <= last_item && envmapAccumulation.a < 1)
                {
                    ShaderEntity probe = load_entity(entity_index);
                    if ((probe.layerMask & surface.layerMask) == 0)
                        continue;

                    const float4x4 probeProjection = load_entitymatrix(probe.GetMatrixIndex());
                    const float3 clipSpacePos = mul(probeProjection, float4(surface.P, 1)).xyz;
                    const float3 uvw = clipSpacePos.xyz * float3(0.5, -0.5, 0.5) + 0.5;
					[branch]
                    if (is_saturated(uvw))
                    {
                        const float4 envmapColor = EnvironmentReflection_Local(surface, probe, probeProjection, clipSpacePos);
						// perform manual blending of probes:
						//  NOTE: they are sorted top-to-bottom, but blending is performed bottom-to-top
                        envmapAccumulation.rgb = mad(1 - envmapAccumulation.a, envmapColor.a * envmapColor.rgb, envmapAccumulation.rgb);
                        envmapAccumulation.a = mad(1 - envmapColor.a, envmapAccumulation.a, envmapColor.a);
						[branch]
                        if (envmapAccumulation.a >= 1.0)
                        {
							// force exit:
                            bucket = SHADER_ENTITY_TILE_BUCKET_COUNT;
                            break;
                        }
                    }
                }
                else if (entity_index > last_item)
                {
					// force exit:
                    bucket = SHADER_ENTITY_TILE_BUCKET_COUNT;
                    break;
                }

            }
        }
    }
#endif // DISABLE_LOCALENVPMAPS
	
	// Apply global envmap where there is no local envmap information:
	[branch]
    if (envmapAccumulation.a < 0.99)
    {
        envmapAccumulation.rgb = lerp(EnvironmentReflection_Global(surface), envmapAccumulation.rgb, envmapAccumulation.a);
    }
    lighting.indirect.specular += max(0, envmapAccumulation.rgb);
#endif // DISABLE_ENVMAPS

#ifndef DISABLE_VOXELGI
    VoxelGI(surface, lighting);
#endif //DISABLE_VOXELGI

	[branch]
    if (GetFrame().lightarray_count > 0)
    {
        uint4 shadow_mask_packed = 0;
#ifdef SHADOW_MASK_ENABLED
		[branch]
		if (GetFrame().options & OPTION_BIT_SHADOW_MASK && GetCamera().texture_rtshadow_index >= 0)
		{
			shadow_mask_packed = bindless_textures_uint4[GetCamera().texture_rtshadow_index][surface.pixel / 2];
		}
#endif // SHADOW_MASK_ENABLED

		// Loop through light buckets in the tile:
        const uint first_item = GetFrame().lightarray_offset;
        const uint last_item = first_item + GetFrame().lightarray_count - 1;
        const uint first_bucket = first_item / 32;
        const uint last_bucket = min(last_item / 32, max(0, SHADER_ENTITY_TILE_BUCKET_COUNT - 1));
		[loop]
        for (uint bucket = first_bucket; bucket <= last_bucket; ++bucket)
        {
            uint bucket_bits = load_entitytile(flatTileIndex + bucket);

			// Bucket scalarizer - Siggraph 2017 - Improved Culling [Michal Drobot]:
            bucket_bits = WaveReadLaneFirst(WaveActiveBitOr(bucket_bits));

			[loop]
            while (bucket_bits != 0)
            {
				// Retrieve global entity index from local bucket, then remove bit from local bucket:
                const uint bucket_bit_index = firstbitlow(bucket_bits);
                const uint entity_index = bucket * 32 + bucket_bit_index;
                bucket_bits ^= 1u << bucket_bit_index;

				// Check if it is a light and process:
				[branch]
                if (entity_index >= first_item && entity_index <= last_item)
                {
                    ShaderEntity light = load_entity(entity_index);
                    if ((light.layerMask & surface.layerMask) == 0)
                        continue;

                    if (light.GetFlags() & ENTITY_FLAG_LIGHT_STATIC)
                    {
                        continue; // static lights will be skipped (they are used in lightmap baking)
                    }

                    float shadow_mask = 1;
#ifdef SHADOW_MASK_ENABLED
					[branch]
					if (GetFrame().options & OPTION_BIT_SHADOW_MASK && light.IsCastingShadow())
					{
						uint shadow_index = entity_index - GetFrame().lightarray_offset;
						if (shadow_index < 16)
						{
							uint mask_shift = (shadow_index % 4) * 8;
							uint mask_bucket = shadow_index / 4;
							uint mask = (shadow_mask_packed[mask_bucket] >> mask_shift) & 0xFF;
							if (mask == 0)
							{
								continue;
							}
							shadow_mask = mask / 255.0;
						}
					}
#endif // SHADOW_MASK_ENABLED

                    switch (light.GetType())
                    {
                        case ENTITY_TYPE_DIRECTIONALLIGHT:
					{
                                light_directional(light, surface, lighting, shadow_mask);
                            }
                            break;
                        case ENTITY_TYPE_POINTLIGHT:
					{
                                light_point(light, surface, lighting, shadow_mask);
                            }
                            break;
                        case ENTITY_TYPE_SPOTLIGHT:
					{
                                light_spot(light, surface, lighting, shadow_mask);
                            }
                            break;
                    }
                }
                else if (entity_index > last_item)
                {
					// force exit:
                    bucket = SHADER_ENTITY_TILE_BUCKET_COUNT;
                    break;
                }

            }
        }
    }


#ifndef TRANSPARENT
	[branch]
    if (GetFrame().options & OPTION_BIT_SURFELGI_ENABLED && GetCamera().texture_surfelgi_index >= 0 && surfel_cellvalid(surfel_cell(surface.P)))
    {
        lighting.indirect.diffuse = bindless_textures[GetCamera().texture_surfelgi_index][surface.pixel].rgb;
    }
#endif // TRANSPARENT

}

inline void ApplyLighting(in Surface surface, in Lighting lighting, inout float4 color)
{
    LightingPart combined_lighting = CombineLighting(surface, lighting);
    color.rgb = lerp(surface.albedo * combined_lighting.diffuse, surface.refraction.rgb, surface.refraction.a) + combined_lighting.specular;
}

inline void ApplyFog(in float distance, float3 P, float3 V, inout float4 color)
{
    const float fogAmount = GetFogAmount(distance, P, V);
	
    if (GetFrame().options & OPTION_BIT_REALISTIC_SKY)
    {
        const float3 skyLuminance = texture_skyluminancelut.SampleLevel(sampler_point_clamp, float2(0.5, 0.5), 0).rgb;
        color.rgb = lerp(color.rgb, skyLuminance, fogAmount);
    }
    else
    {
        const float3 V = float3(0.0, -1.0, 0.0);
        color.rgb = lerp(color.rgb, GetDynamicSkyColor(V, false, false, false, true), fogAmount);
    }
}

inline uint AlphaToCoverage(float alpha, float alphaTest, float4 svposition)
{
    if (alphaTest == 0)
    {
		// No alpha test, force full coverage:
        return ~0u;
    }

    if (GetFrame().options & OPTION_BIT_TEMPORALAA_ENABLED)
    {
		// When Temporal AA is enabled, dither the alpha mask with animated blue noise:
        alpha -= blue_noise(svposition.xy, svposition.w).x / GetCamera().sample_count;
    }
    else if (GetCamera().sample_count > 1)
    {
		// Without Temporal AA, use static dithering:
        alpha -= dither(svposition.xy) / GetCamera().sample_count;
    }
    else
    {
		// Without Temporal AA and MSAA, regular alpha test behaviour will be used:
        alpha -= alphaTest;
    }

    if (alpha > 0)
    {
        return ~0u >> (31u - uint(alpha * GetCamera().sample_count));
    }
    return 0;
}




#ifdef DISABLE_ALPHATEST
[earlydepthstencil]
#endif // DISABLE_ALPHATEST

// entry point:
#ifdef PREPASS
uint2 main(PixelInput input, in uint primitiveID : SV_PrimitiveID, out uint coverage : SV_Coverage) : SV_Target
#else
float4 main(PixelInput input, in bool is_frontface : SV_IsFrontFace) : SV_Target
#endif // PREPASS


// Pixel shader base:
{
	const float depth = input.pos.z;
	const float lineardepth = input.pos.w;
	const float2 pixel = input.pos.xy;
	const float2 ScreenCoord = pixel * GetCamera().internal_resolution_rcp;
	float3 bumpColor = 0;

#ifndef DISABLE_ALPHATEST
#ifndef TRANSPARENT
#ifndef ENVMAPRENDERING
#ifdef OBJECTSHADER_USE_DITHERING
	// apply dithering:
	clip(dither(pixel + GetTemporalAASampleRotation()) - (1 - input.dither));
#endif // OBJECTSHADER_USE_DITHERING
#endif // DISABLE_ALPHATEST
#endif // TRANSPARENT
#endif // ENVMAPRENDERING


	Surface surface;
	surface.init();


#ifdef OBJECTSHADER_USE_NORMAL
	if (is_frontface == false)
	{
		input.nor = -input.nor;
	}
	surface.N = normalize(input.nor);
#endif // OBJECTSHADER_USE_NORMAL

#ifdef OBJECTSHADER_USE_POSITION3D
	surface.P = input.pos3D;
	surface.V = GetCamera().position - surface.P;
	float dist = length(surface.V);
	surface.V /= dist;
#endif // OBJECTSHADER_USE_POSITION3D

#ifdef OBJECTSHADER_USE_TANGENT
#if 0
	float3x3 TBN = compute_tangent_frame(surface.N, surface.P, input.uvsets.xy);
#else
	float4 tangent = input.tan;
	tangent.xyz = normalize(tangent.xyz);
	float3 binormal = normalize(cross(tangent.xyz, surface.N) * tangent.w);
	float3x3 TBN = float3x3(tangent.xyz, binormal, surface.N);
#endif

#ifdef POM
	ParallaxOcclusionMapping(input.uvsets, surface.V, TBN);
#endif // POM

#endif // OBJECTSHADER_USE_TANGENT

    
    float4 color = 1;
    
#ifdef OBJECTSHADER_USE_UVSETS
    
float4 materialExpression10003 = bindless_textures[texture10003].Sample(sampler_objectshader,  input.uvsets.xy); 
float4 materialExpression10033 = bindless_textures[texture10033].Sample(sampler_objectshader,  input.uvsets.xy); 
float3 materialExpression10041 = materialExpression10003.rgb + materialExpression10033.rgb;
float4 materialExpression10055 = bindless_textures[texture10055].Sample(sampler_objectshader,  input.uvsets.xy); 
float3 materialExpression10050 = materialExpression10041.rgb + materialExpression10055.rgb;

float3 BaseColor = materialExpression10050.rgb;
float Opacity = 1;


    color.rgb = BaseColor.rgb;
    color.a = Opacity;
#endif   // OBJECTSHADER_USE_UVSETS
    
    


#ifdef OBJECTSHADER_USE_COLOR
	color *= input.color;
#endif // OBJECTSHADER_USE_COLOR


#ifdef TRANSPARENT
#ifndef DISABLE_ALPHATEST
	// Alpha test is only done for transparents
	//	- Prepass will write alpha coverage mask
	//	- Opaque will 
	clip(color.a - GetMaterial().alphaTest);
#endif // DISABLE_ALPHATEST
#endif // TRANSPARENT



#ifndef WATER
#ifdef OBJECTSHADER_USE_TANGENT
	NormalMapping(input.uvsets, surface.N, TBN, bumpColor);
#endif // OBJECTSHADER_USE_TANGENT
#endif // WATER


	float4 surfaceMap = 1;

#ifdef OBJECTSHADER_USE_UVSETS
	[branch]
	if (GetMaterial().uvset_surfaceMap >= 0)
	{
		const float2 UV_surfaceMap = GetMaterial().uvset_surfaceMap == 0 ? input.uvsets.xy : input.uvsets.zw;
		surfaceMap = texture_surfacemap.Sample(sampler_objectshader, UV_surfaceMap);
	}
#endif // OBJECTSHADER_USE_UVSETS


	float4 specularMap = 1;

#ifdef OBJECTSHADER_USE_UVSETS
	[branch]
	if (GetMaterial().uvset_specularMap >= 0)
	{
		const float2 UV_specularMap = GetMaterial().uvset_specularMap == 0 ? input.uvsets.xy : input.uvsets.zw;
		specularMap = texture_specularmap.Sample(sampler_objectshader, UV_specularMap);
		specularMap.rgb = DEGAMMA(specularMap.rgb);
	}
#endif // OBJECTSHADER_USE_UVSETS



	surface.create(GetMaterial(), color, surfaceMap, specularMap);


	// Emissive map:
	surface.emissiveColor = GetMaterial().GetEmissive();

#ifdef OBJECTSHADER_USE_UVSETS
	[branch]
	if (any(surface.emissiveColor) && GetMaterial().uvset_emissiveMap >= 0)
	{
		const float2 UV_emissiveMap = GetMaterial().uvset_emissiveMap == 0 ? input.uvsets.xy : input.uvsets.zw;
		float4 emissiveMap = texture_emissivemap.Sample(sampler_objectshader, UV_emissiveMap);
		emissiveMap.rgb = DEGAMMA(emissiveMap.rgb);
		surface.emissiveColor *= emissiveMap.rgb * emissiveMap.a;
	}
#endif // OBJECTSHADER_USE_UVSETS




#ifdef OBJECTSHADER_USE_EMISSIVE
	surface.emissiveColor *= Unpack_R11G11B10_FLOAT(input.emissiveColor);
#endif // OBJECTSHADER_USE_EMISSIVE


#ifdef OBJECTSHADER_USE_UVSETS
	// Secondary occlusion map:
	[branch]
	if (GetMaterial().IsOcclusionEnabled_Secondary() && GetMaterial().uvset_occlusionMap >= 0)
	{
		const float2 UV_occlusionMap = GetMaterial().uvset_occlusionMap == 0 ? input.uvsets.xy : input.uvsets.zw;
		surface.occlusion *= texture_occlusionmap.Sample(sampler_objectshader, UV_occlusionMap).r;
	}
#endif // OBJECTSHADER_USE_UVSETS


#ifndef PREPASS
#ifndef ENVMAPRENDERING
#ifndef TRANSPARENT
	[branch]
	if (GetCamera().texture_ao_index >= 0)
	{
		surface.occlusion *= bindless_textures_float[GetCamera().texture_ao_index].SampleLevel(sampler_linear_clamp, ScreenCoord, 0).r;
	}
#endif // TRANSPARENT
#endif // ENVMAPRENDERING
#endif // PREPASS



	surface.sss = GetMaterial().subsurfaceScattering;
	surface.sss_inv = GetMaterial().subsurfaceScattering_inv;

	surface.pixel = pixel;
	surface.screenUV = ScreenCoord;

	surface.layerMask = GetMaterial().layerMask;

	surface.update();

	float3 ambient = GetAmbient(surface.N);
	ambient = lerp(ambient, ambient * surface.sss.rgb, saturate(surface.sss.a));

	Lighting lighting;
	lighting.create(0, 0, ambient, 0);




#ifdef TRANSPARENT
	[branch]
	if (GetMaterial().transmission > 0)
	{
		surface.transmission = GetMaterial().transmission;

#ifdef OBJECTSHADER_USE_UVSETS
		[branch]
		if (GetMaterial().uvset_transmissionMap >= 0)
		{
			const float2 UV_transmissionMap = GetMaterial().uvset_transmissionMap == 0 ? input.uvsets.xy : input.uvsets.zw;
			float transmissionMap = texture_transmissionmap.Sample(sampler_objectshader, UV_transmissionMap).r;
			surface.transmission *= transmissionMap;
		}
#endif // OBJECTSHADER_USE_UVSETS

		[branch]
		if (GetCamera().texture_refraction_index >= 0)
		{
			Texture2D texture_refraction = bindless_textures[GetCamera().texture_refraction_index];
			float2 size;
			float mipLevels;
			texture_refraction.GetDimensions(0, size.x, size.y, mipLevels);
			const float2 normal2D = mul((float3x3)GetCamera().view, surface.N.xyz).xy;
			float2 perturbatedRefrTexCoords = ScreenCoord.xy + normal2D * GetMaterial().refraction;
			float4 refractiveColor = texture_refraction.SampleLevel(sampler_linear_clamp, perturbatedRefrTexCoords, surface.roughness * mipLevels);
			surface.refraction.rgb = surface.albedo * refractiveColor.rgb;
			surface.refraction.a = surface.transmission;
		}
	}
#endif // TRANSPARENT


#ifdef OBJECTSHADER_USE_ATLAS
	LightMapping(load_instance(input.instanceID).lightmap, input.atl, lighting);
#endif // OBJECTSHADER_USE_ATLAS


#ifdef OBJECTSHADER_USE_EMISSIVE
	ApplyEmissive(surface, lighting);
#endif // OBJECTSHADER_USE_EMISSIVE


#ifdef FORWARD
	ForwardLighting(surface, lighting);
#endif // FORWARD


#ifdef TILEDFORWARD
	TiledLighting(surface, lighting);
#endif // TILEDFORWARD


#ifndef WATER
#ifndef ENVMAPRENDERING
#ifndef TRANSPARENT
	[branch]
	if (GetCamera().texture_ssr_index >= 0)
	{
		float4 ssr = bindless_textures[GetCamera().texture_ssr_index].SampleLevel(sampler_linear_clamp, ScreenCoord, 0);
		lighting.indirect.specular = lerp(lighting.indirect.specular, ssr.rgb * surface.F, ssr.a);
	}
#endif // TRANSPARENT
#endif // ENVMAPRENDERING
#endif // WATER



	ApplyLighting(surface, lighting, color);


#ifdef OBJECTSHADER_USE_POSITION3D
	ApplyFog(dist, GetCamera().position, surface.V, color);
#endif // OBJECTSHADER_USE_POSITION3D


	color = max(0, color);


	// end point:
#ifdef PREPASS
	coverage = AlphaToCoverage(color.a, GetMaterial().alphaTest, input.pos);

	PrimitiveID prim;
	prim.primitiveIndex = primitiveID;
	prim.instanceIndex = input.instanceID;
	prim.subsetIndex = GetSubsetIndex();
	return prim.pack();
#else
	return color;
#endif // PREPASS
}





       