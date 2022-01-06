/*
 * Copyright (c) 2019, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */

#include <cstdlib>
#include <stdint.h>

#pragma pack(push,8) // Make sure we have consistent structure packings

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================
AUTO CONFIG
===========================================================================*/
#ifndef __GFSDK_COMMON_AUTOCONFIG
#define __GFSDK_COMMON_AUTOCONFIG

#if defined(__GNUC__) //|| defined (__ANDROID__)
#define __GFSDK_CC_GNU__ 1
#define __GFSDK_CC_MSVC__ 0
#else
#define __GFSDK_CC_GNU__ 0
#define __GFSDK_CC_MSVC__ 1
#endif

#endif

/*===========================================================================
MACROS
===========================================================================*/
#ifndef __GFSDK_COMMON_MACROS
#define __GFSDK_COMMON_MACROS

// GNU
#if __GFSDK_CC_GNU__
#define __GFSDK_EXPECT__(exp,tf) __builtin_expect(exp, tf)
#define __GFSDK_INLINE__ __attribute__((always_inline))
#define __GFSDK_NOLINE__ __attribute__((noinline))
#define __GFSDK_RESTRICT__ __restrict
#define __GFSDK_CDECL__
#define __GFSDK_EXPORT__ __declspec(dllexport)
#define __GFSDK_EXPORT__ __declspec(dllimport)
#endif


// MSVC
#if __GFSDK_CC_MSVC__
#define __GFSDK_EXPECT__(exp, tf) (exp)
#define __GFSDK_INLINE__ __forceinline
#define __GFSDK_NOINLINE__
#define __GFSDK_RESTRICT__ __restrict
#define __GFSDK_CDECL__ __cdecl
#define __GFSDK_EXPORT__ __declspec(dllexport)
#define __GFSDK_IMPORT__ __declspec(dllimport)
#endif

#endif // __GFSDK_COMMON_MACROS


#ifndef __GFSDK_COMMON
#define __GFSDK_COMMON

/*===========================================================================
Calling conventions
===========================================================================*/

#define GFSDK_WAVEWORKS_CALL_CONV __GFSDK_CDECL__

/*===========================================================================
Result codes
===========================================================================*/
enum gfsdk_wwresult 
{
	gfsdk_wwresult_INTERNAL_ERROR = -2,
	gfsdk_wwresult_FAIL = -1,
	gfsdk_wwresult_OK = 0,
	gfsdk_wwresult_NONE = 1,
	gfsdk_wwresult_WOULD_BLOCK = 2,
};


/*===========================================================================
Specifies the detail level of the simulation
===========================================================================*/
enum GFSDK_WaveWorks_Simulation_DetailLevel
{
	GFSDK_WaveWorks_Simulation_DetailLevel_Normal = 0,
	GFSDK_WaveWorks_Simulation_DetailLevel_High,
	GFSDK_WaveWorks_Simulation_DetailLevel_Extreme,
	Num_GFSDK_WaveWorks_Simulation_DetailLevels
};

/*===========================================================================
Specifies the underlying API for the simulation
===========================================================================*/
enum GFSDK_WaveWorks_Simulation_API
{
	GFSDK_WaveWorks_Simulation_API_CPU = 0,
	GFSDK_WaveWorks_Simulation_API_Compute = 1
};

/*===========================================================================
Controls the threading model when the CPU simulation path is used
===========================================================================*/
enum GFSDK_WaveWorks_Simulation_CPU_Threading_Model
{
	GFSDK_WaveWorks_Simulation_CPU_Threading_Model_Automatic = 0,	// Use an automatically-determined number of worker threads
	GFSDK_WaveWorks_Simulation_CPU_Threading_Model_1 = 1,			// Use 1 worker thread
	GFSDK_WaveWorks_Simulation_CPU_Threading_Model_2 = 2,			// Use 2 worker threads
	GFSDK_WaveWorks_Simulation_CPU_Threading_Model_3 = 3,			// Use 3 worker threads
	GFSDK_WaveWorks_Simulation_CPU_Threading_Model_4 = 4,			// Use 4 worker threads
																// etc...
																// i.e. it's safe to use higher values to represent even larger thread counts
};

/*===========================================================================
Handles
===========================================================================*/
struct Wind_Waves_Simulation;
typedef Wind_Waves_Simulation* GFSDK_WaveWorks_Wind_Waves_SimulationHandle;

struct Local_Waves_Simulation;
typedef Local_Waves_Simulation* GFSDK_WaveWorks_Local_Waves_SimulationHandle;

struct Quadtree;
typedef Quadtree* GFSDK_WaveWorks_QuadtreeHandle;

/*===========================================================================
API GUID
===========================================================================*/
struct GFSDK_WaveWorks_API_GUID
{
	GFSDK_WaveWorks_API_GUID(uint32_t c1, uint32_t c2, uint32_t c3, uint32_t c4) :
		Component1(c1), Component2(c2), Component3(c3), Component4(c4)
	{
	}

	uint32_t Component1;
	uint32_t Component2;
	uint32_t Component3;
	uint32_t Component4;
};

/*===========================================================================
Kick IDs
===========================================================================*/

#define GFSDK_WaveWorks_InvalidKickID uint64_t(-1)

#endif //__GFSDK_COMMON

#ifdef __cplusplus
}; //extern "C" {
#endif

#pragma pack(pop)
