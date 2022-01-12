/*
 * Copyright (c) 2019, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */

#ifndef __GFSDK_FLOAT_TYPES
#define __GFSDK_FLOAT_TYPES

#include <math.h>
#include <stdint.h>

typedef struct
{
	float x;
	float y;
} gfsdk_float2;

typedef struct
{
	float x;
	float y;
	float z;
} gfsdk_float3;

typedef struct
{
	float x;
	float y;
	float z;
	float w;
} gfsdk_float4;

// Implicit row major
typedef struct
{
	float _11, _12, _13, _14;
	float _21, _22, _23, _24;
	float _31, _32, _33, _34;
	float _41, _42, _43, _44;
} gfsdk_float4x4;

// Convenience one-liners
// Float vectors
inline gfsdk_float2 gfsdk_make_float2(float x, float y) { return{ x, y }; }
inline gfsdk_float3 gfsdk_make_float3(float x, float y, float z) { return{ x, y, z }; }
inline gfsdk_float3 gfsdk_make_float3(const gfsdk_float4& a) { return{ a.x, a.y, a.z }; }
inline gfsdk_float4 gfsdk_make_float4(float x, float y, float z, float w) { return{ x, y, z, w }; }
inline gfsdk_float4 gfsdk_make_float4(const gfsdk_float3& a, float w) { return{ a.x, a.y, a.z, w }; }

inline gfsdk_float2 operator+(const gfsdk_float2& a, const gfsdk_float2& b) { return{ a.x + b.x, a.y + b.y }; }
inline gfsdk_float3 operator+(const gfsdk_float3& a, const gfsdk_float3& b) { return{ a.x + b.x, a.y + b.y, a.z + b.z }; }
inline gfsdk_float4 operator+(const gfsdk_float4& a, const gfsdk_float4& b) { return{ a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w }; }

inline gfsdk_float2 operator-(const gfsdk_float2& a, const gfsdk_float2& b) { return{ a.x - b.x, a.y - b.y }; }
inline gfsdk_float3 operator-(const gfsdk_float3& a, const gfsdk_float3& b) { return{ a.x - b.x, a.y - b.y, a.z - b.z }; }
inline gfsdk_float4 operator-(const gfsdk_float4& a, const gfsdk_float4& b) { return{ a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w }; }

inline gfsdk_float2 operator*(const gfsdk_float2& b, const float s) { return{ s*b.x, s*b.y }; }
inline gfsdk_float3 operator*(const float s, const gfsdk_float3& b) { return{ s*b.x, s*b.y, s*b.z }; }
inline gfsdk_float4 operator*(const float s, const gfsdk_float4& b) { return{ s*b.x, s*b.y, s*b.z, s*b.w }; }

inline gfsdk_float2& operator+=(gfsdk_float2& a, const gfsdk_float2& b) { a.x += b.x; a.y += b.y; return a; }
inline gfsdk_float3& operator+=(gfsdk_float3& a, const gfsdk_float3& b) { a.x += b.x; a.y += b.y; a.z += b.z; return a; }
inline gfsdk_float4& operator+=(gfsdk_float4& a, const gfsdk_float4& b) { a.x += b.x; a.y += b.y; a.z += b.z; a.w += b.w; return a; }

inline gfsdk_float2& operator*=(gfsdk_float2& a, const float b) { a.x *= b; a.y *= b; return a; }
inline gfsdk_float3& operator*=(gfsdk_float3& a, const float b) { a.x *= b; a.y *= b; a.z *= b; return a; }
inline gfsdk_float4& operator*=(gfsdk_float4& a, const float b) { a.x *= b; a.y *= b; a.z *= b; a.w *= b; return a; }

inline float length(const gfsdk_float2& a) { return sqrtf(a.x*a.x + a.y*a.y); }
inline float length(const gfsdk_float3& a) { return sqrtf(a.x*a.x + a.y*a.y + a.z*a.z); }
inline float length(const gfsdk_float4& a) { return sqrtf(a.x*a.x + a.y*a.y + a.z*a.z + a.w*a.w); }

inline gfsdk_float2 normalize(const gfsdk_float2& a) { float f = length(a); if (f == 0) f = 1.0f; return{ a.x / f, a.y / f }; }
inline gfsdk_float3 normalize(const gfsdk_float3& a) { float f = length(a); if (f == 0) f = 1.0f; return{ a.x / f, a.y / f, a.z / f }; }
inline gfsdk_float4 normalize(const gfsdk_float4& a) { float f = length(a); if (f == 0) f = 1.0f; return{ a.x / f, a.y / f, a.z / f, a.w / f }; }
inline gfsdk_float3 cross(const gfsdk_float3& a, const gfsdk_float3& b) { return{ (a.y * b.z) - (a.z * b.y), (a.z * b.x) - (a.x * b.z), (a.x * b.y) - (a.y * b.x) }; }
inline float dot(const gfsdk_float3& a, const gfsdk_float3& b) { return{ a.x * b.x + a.y * b.y + a.z * b.z }; }

// Float 4x4 matrices
inline gfsdk_float4x4 identity() { return{ /*row1*/1.0f, 0, 0, 0, /*row2*/0, 1.0f, 0, 0, /*row3*/0, 0, 1.0f, 0, /*row4*/0, 0, 0, 1.0f }; }

inline gfsdk_float4x4 operator*(const gfsdk_float4x4& a, const gfsdk_float4x4& b)
{
	gfsdk_float4x4 result;
	for (uint32_t i = 0; i < 4; i++)
	{
		for (uint32_t j = 0; j < 4; j++)
		{
			float temp = 0;
			for (uint32_t k = 0; k < 4; k++)
			{
				temp += (&a._11)[k * 4 + j] * (&b._11)[i * 4 + k];
			}
			(&result._11)[i * 4 + j] = temp;
		}
	}
	return result;
}

inline gfsdk_float4 operator*(const gfsdk_float4x4& b, const gfsdk_float4& a)
{
	gfsdk_float4 result;
	uint32_t i, k;
	float temp;
	for (i = 0; i < 4; ++i)
	{
		temp = 0;
		for (k = 0; k < 4; ++k)
		{
			temp += (&b._11)[k * 4 + i] * (&a.x)[k];
		}
		(&result.x)[i] = temp;
	}
	return result;
}

#endif // __GFSDK_COMMON_TYPES