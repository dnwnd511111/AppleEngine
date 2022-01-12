/*
 * Copyright (c) 2019, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */

#ifndef _GFSDK_WAVEWORKS_H
#define _GFSDK_WAVEWORKS_H

#include "GFSDK_WaveWorks_GUID.h"
#include "GFSDK_WaveWorks_Common.h"
#include "GFSDK_WaveWorks_FloatTypes.h"


/*===========================================================================
  Preamble
  ===========================================================================*/
#ifndef GFSDK_WAVEWORKS_LINKAGE
	#if WAVEWORKS_DYNAMIC_BUILD // if we're building or using DLL
		#ifndef GFSDK_WAVEWORKS_BUILDING_DLL
		#define GFSDK_WAVEWORKS_BUILDING_DLL 0
		#endif
		#if GFSDK_WAVEWORKS_BUILDING_DLL // If we're building DLL
			#define GFSDK_WAVEWORKS_LINKAGE __GFSDK_EXPORT__
		#else // otherwise we're using the DLL
			#define GFSDK_WAVEWORKS_LINKAGE __GFSDK_IMPORT__
		#endif
	#else // if we're building or using static library
		#define GFSDK_WAVEWORKS_LINKAGE
	#endif
#endif

#define GFSDK_WAVEWORKS_DECL(ret_type) GFSDK_WAVEWORKS_LINKAGE ret_type GFSDK_WAVEWORKS_CALL_CONV

		/*===========================================================================
		  Memory management definitions
		  ===========================================================================*/

#ifndef GFSDK_WAVEWORKS_MALLOC_HOOKS_DEFINED
#define GFSDK_WAVEWORKS_MALLOC_HOOKS_DEFINED

	typedef void* (GFSDK_WAVEWORKS_CALL_CONV *GFSDK_WAVEWORKS_MALLOC) (size_t size);
	typedef void (GFSDK_WAVEWORKS_CALL_CONV *GFSDK_WAVEWORKS_FREE) (void *p);
	typedef void* (GFSDK_WAVEWORKS_CALL_CONV *GFSDK_WAVEWORKS_ALIGNED_MALLOC) (size_t size, size_t alignment);
	typedef void (GFSDK_WAVEWORKS_CALL_CONV *GFSDK_WAVEWORKS_ALIGNED_FREE) (void *p);

	struct GFSDK_WaveWorks_Malloc_Hooks
	{
		GFSDK_WAVEWORKS_MALLOC pMalloc;
		GFSDK_WAVEWORKS_FREE pFree;
		GFSDK_WAVEWORKS_ALIGNED_MALLOC pAlignedMalloc;
		GFSDK_WAVEWORKS_ALIGNED_FREE pAlignedFree;
	};

#endif


/*===========================================================================
	Globals/init
	===========================================================================*/

GFSDK_WAVEWORKS_DECL(const char*) GFSDK_WaveWorks_GetBuildString();

// Use these calls to globally initialize/release on D3D device create/destroy.
GFSDK_WAVEWORKS_DECL(gfsdk_wwresult) GFSDK_WaveWorks_Init(const GFSDK_WaveWorks_Malloc_Hooks* pOptionalMallocHooks, const GFSDK_WaveWorks_API_GUID& apiGUID);
GFSDK_WAVEWORKS_DECL(gfsdk_wwresult) GFSDK_WaveWorks_Release();

/*===========================================================================
	Wind Waves Simulation
	===========================================================================*/

struct GFSDK_WaveWorks_Wind_Waves_Simulation_Parameters
{
	// The direction of the wind inducing the local waves
	gfsdk_float2 base_wind_direction;
	// The speed of the wind inducing the waves. If GFSDK_WaveWorks_Simulation_Settings.use_Beaufort_scale is set, this is
	// interpreted as a Beaufort scale value. Otherwise, it is interpreted as meters per second
	float base_wind_speed;
	// The distance over which the wind was blowing at speed defined by parameter above to develop waves, or "fetch" parameter in JONSWAP spectrum, in km
	float base_wind_distance;
	// The degree to which waves appear to move in the wind direction (vs. randomly moving waves), in the [0, 1] range
	float base_wind_dependency;
	// The factor that defines amplification of dominant wavelengths peak in JONSWAP spectrum, normally in the [3.3, 7] range
	float base_spectrum_peaking;
	// The simulation spectrum can be low-pass filtered to eliminate wavelengths that could end up under-sampled, this controls
	// upper limit of wavelengths (in meters) that are considered small
	float base_small_waves_cutoff_length;
	// The simulation spectrum can be low-pass filtered to eliminate wavelengths that could end up under-sampled, this controls
	// how much the small waves are damped, in [0, 1] range
	float base_small_waves_cutoff_power;
	// Amplitude scale factor for simulated wave amplitude
	float base_amplitude_multiplier;

	// The direction of the distant wind inducing the swell
	gfsdk_float2 swell_wind_direction;
	// The speed of the wind inducing the swell
	float swell_wind_speed;
	// The distance over which the wind was blowing at speed defined by parameter above to develop swell, or "fetch" parameter in JONSWAP spectrum, in km
	float swell_wind_distance;
	// The degree to which swell appears to move in the wind direction
	float swell_wind_dependency;
	// The factor that defines amplification of dominant swell wavelengths peak in JONSWAP spectrum, normally in the [3.3, 7] range
	float swell_spectrum_peaking;
	// The simulation spectrum can be low-pass filtered to eliminate wavelengths that could end up under-sampled, this controls
	// upper limit of wavelengths (in meters) that are considered small
	float swell_small_waves_cutoff_length;
	// The simulation spectrum can be low-pass filtered to eliminate wavelengths that could end up under-sampled, this controls
	// how much the small waves are damped, in [0, 1] range
	float swell_small_waves_cutoff_power;
	// Amplitude scale factor for simulated wave amplitude
	float swell_amplitude_multiplier;

	// In addition to height displacements, the simulation also applies lateral displacements. This controls the non-linearity
	// and therefore 'choppiness' in the resulting wave shapes. Should normally be set to 1.0, can be in the [0, 1] range.
	float lateral_multiplier;
	// The global time multiplier
	float time_scale;

	// These parameters define sinusoidal UV warping for individual cascades to hide repeats in certain cases
	// Unitless amplitude of sinusoidal distortion (as a fraction of cascade's UV units), should normally be in [0, 0.05] range
	float uv_warping_amplitude; 
	// Frequency for sinusoidal distortion, should normally be in  [1,3] range
	float uv_warping_frequency; 

	// The crests of the waves become covered with foam if
	// Jacobian of wave curvature gets higher than this threshold. The range is [0, 1].
	float foam_whitecaps_threshold;

	// The turbulent energy representing foam and bubbles spread in water starts generating on the crests of the waves if
	// Jacobian of wave curvature gets higher than this threshold. The range is [0, 1], the typical values are in [0.2, 0.4] range.
	float foam_generation_threshold;
	// The amount of turbulent energy injected in areas defined by foam_generation_threshold parameter on each simulation step.
	// The range is [0, 1], the typical values are [0, 0.1] range.
	float foam_generation_amount;
	// The speed of spatial dissipation of turbulent energy. The range is [0, 1], the typical values are in [0.5, 1] range.
	float foam_dissipation_speed;
	// In addition to spatial dissipation, the turbulent energy dissolves over time. This parameter sets the speed of
	// dissolving over time. The range is [0, 1], the typical values are in [0.9, 0.99] range.
	float foam_falloff_speed;
};

struct GFSDK_WaveWorks_Wind_Waves_Simulation_Settings
{
	// The detail level of the simulation: this drives the resolution of the FFT 
	GFSDK_WaveWorks_Simulation_DetailLevel detail_level;

	// This parameter determines whether the simulation is done on the GPU or CPU
	GFSDK_WaveWorks_Simulation_API simulation_api;
			
	// The repeat interval for the fft simulation in worldspace, in meters
	float simulation_period;

	// True if base_wind_speed in GFSDK_WaveWorks_Simulation_Params should accept Beaufort scale value
	// False if base_wind_speed in GFSDK_WaveWorks_Simulation_Params should accept meters/second
	bool use_Beaufort_scale;

	// Should the CPU memory for displacements and displacement archive be preallocated and 
	// the displacement data be read back to the CPU so GFSDK_WaveWorks_Simulation_GetDisplacements(...) would have data to process on CPU side?
	bool enable_CPU_driven_displacement_calculation;

	// Should the GPU memory for sample points, displacement results and displacement archive be preallocated and
	// it would be possible the displacement data in GFSDK_WaveWorks_Simulation_GetDisplacements(...) to be calculated on GPU?
	bool enable_GPU_driven_displacement_calculation;

	// If readback is enabled, displacement data can be kept alive in a FIFO for historical lookups
	// e.g. in order to implement predict/correct for a networked application
	uint32_t num_readback_FIFO_entries;

	// The threading model to use when the CPU simulation path is active
	// Can be set to none (meaning: simulation is performed on the calling thread, synchronously), automatic, or even
	// an explicitly specified thread count
	GFSDK_WaveWorks_Simulation_CPU_Threading_Model CPU_simulation_threading_model;

	// Number of GPUs used (for multi-GPU setups) 
	uint32_t num_GPUs;
	// Controls the use of graphics pipeline timers
	bool enable_GPU_timers;
	// Controls the use of CPU timers to gather profiling data
	bool enable_CPU_timers;
};

struct GFSDK_WaveWorks_Wind_Waves_Simulation_Stats
{
	// The times spent on particular simulation tasks, measured in milliseconds (1e-3 sec)
	// CPU time spent by main app thread waiting for CPU FFT simulation results using CPU 
	float CPU_main_thread_wait_time;			 
	// CPU time spent on CPU FFT simulation: sum time spent in threads that perform simulation work
	float CPU_threads_total_time;				 
	// GPU time spent on GPU FFT simulation
	float GPU_simulation_time;					 
	// GPU time spent on non-simulation e.g. updating gradient maps, foam simulation, mips generation, etc
	float GPU_update_time;						 
	// Total GPU time spent on all workloads
	float GPU_total_time;						 
};

struct GFSDK_WaveWorks_Wind_Waves_Rendering_Data
{
	// Mapping from world space to texture arrays: 
	// UV for each cascade on rendering should be = worldpace coordinates * scale + offset
	float cascade0_UV_scale;
	float cascade1_UV_scale;
	float cascade2_UV_scale;
	float cascade3_UV_scale;
	float cascade0_UV_offset;
	float cascade1_UV_offset;
	float cascade2_UV_offset;
	float cascade3_UV_offset;
	// Simulation settings can define UV distortion to help hiding repeats. 
	// To maintain 
	// UVs for cascades in this case should be offset by the following rules in the shader:
	// U += uv_warping_amplitude * cos(V * uv_warping_frequency)
	// V += uv_warping_amplitude * sin(U * uv_warping_frequency)
	float uv_warping_amplitude;
	float uv_warping_frequency;
	// How much each cascade is bigger than the previous one in world space
	// This parameter is useful for calculating surface foam by combining foam from all the cascades
	float cascade_to_cascade_scale;
	// The crests of the waves become covered with foam if Jacobian of wave curvature gets higher than this threshold. 
	// This parameter is set by WaveWorks internally if Beaufort scale is used, or equals the corresponding member of GFSDK_WaveWorks_Wind_Waves_Simulation_Parameters otherwise.
	float foam_whitecaps_threshold;

	// Size of mip 0 of all the texture arrays in texels
	uint32_t size_of_texture_arrays;

	// Pointers to native API objects that can be used for rendering:
	// ID3D11Resource in case DX11 is used, ID3D12Resource in case DX12 is used, etc.
	// The resources are RGBA16 float texture arrays with 4 slices and all miplevels generated internally,
	// the size of texture arrays depend on GFSDK_WaveWorks_Wind_Waves_Simulation_Settings::detail_level
	// The textures can be recreated internally if GFSDK_WaveWorks_Wind_Waves_Simulation_Settings change on the call to GFSDK_WaveWorks_Wind_Waves_Simulation_UpdateProperties
	void* displacements_texture_array;
	void* gradients_texture_array;
	void* moments_texture_array;
};

// Simulation lifetime management
GFSDK_WAVEWORKS_DECL(gfsdk_wwresult) GFSDK_WaveWorks_Wind_Waves_Simulation_CreateDirectX11( void* pDeviceContext, bool enable_graphics, 
																							const GFSDK_WaveWorks_Wind_Waves_Simulation_Settings& settings, 
																							const GFSDK_WaveWorks_Wind_Waves_Simulation_Parameters& params, 
																							GFSDK_WaveWorks_Wind_Waves_SimulationHandle* pResult);
GFSDK_WAVEWORKS_DECL(gfsdk_wwresult) GFSDK_WaveWorks_Wind_Waves_Simulation_CreateDirectX12( void* pDevice, void* pQueue, bool enable_graphics, 
																							const GFSDK_WaveWorks_Wind_Waves_Simulation_Settings& settings, 
																							const GFSDK_WaveWorks_Wind_Waves_Simulation_Parameters& params, 
																							GFSDK_WaveWorks_Wind_Waves_SimulationHandle* pResult);
GFSDK_WAVEWORKS_DECL(gfsdk_wwresult) GFSDK_WaveWorks_Wind_Waves_Simulation_CreateVK(void* pVkInstance,
																					void* pVulkanPhysicalDevice,
																					void* pVulkanDevice,
																					void* pGraphicsQueue, int32_t graphicsQueueIndex, /*void* pGraphicsCommandPool */
																					void* pTransferQueue, int32_t transferQueueIndex, /*void* pTransferCommandPool */
																					void* pComputeQueue, int32_t computeQueueIndex, /*void* pComputeCommandPool*/
																					const char **instanceExtensions, int32_t numInstanceExtensions,
																					const char **layers, int32_t numLayers,
																					const char **deviceExtensions, int32_t numDeviceExtensions,
																					bool enable_graphics, 
																					const GFSDK_WaveWorks_Wind_Waves_Simulation_Settings& settings, 
																					const GFSDK_WaveWorks_Wind_Waves_Simulation_Parameters& params, 
																					GFSDK_WaveWorks_Wind_Waves_SimulationHandle* pResult);

GFSDK_WAVEWORKS_DECL(gfsdk_wwresult) GFSDK_WaveWorks_Wind_Waves_Simulation_Destroy(GFSDK_WaveWorks_Wind_Waves_SimulationHandle hSim);

// A simulation can be updated with new settings and properties - this is universally preferable to recreating
// a simulation from scratch, since WaveWorks will only do as much reinitialization work as is necessary to implement
// the changes in the setup. For instance, simple changes of wind speed require no reallocations and no interruptions
// to the simulation and rendering pipeline
GFSDK_WAVEWORKS_DECL(gfsdk_wwresult) GFSDK_WaveWorks_Wind_Waves_Simulation_UpdateProperties(GFSDK_WaveWorks_Wind_Waves_SimulationHandle hSim, 
																							const GFSDK_WaveWorks_Wind_Waves_Simulation_Settings& settings, 
																							const GFSDK_WaveWorks_Wind_Waves_Simulation_Parameters& params);

// Sets the absolute simulation time for the next kick. WaveWorks guarantees that the same displacements will be
// generated for the same settings and input times, even across different platforms (e.g. to enable network-
// synchronized implementations)
GFSDK_WAVEWORKS_DECL(gfsdk_wwresult) GFSDK_WaveWorks_Wind_Waves_Simulation_SetTime(GFSDK_WaveWorks_Wind_Waves_SimulationHandle hSim, double dAppTime);

// Retrieves the data thatt he application will need to render the water surface
GFSDK_WAVEWORKS_DECL(gfsdk_wwresult) GFSDK_WaveWorks_Wind_Waves_GetDataForRendering(GFSDK_WaveWorks_Wind_Waves_SimulationHandle hSim, GFSDK_WaveWorks_Wind_Waves_Rendering_Data& renderingData);

// Retrieve an array of simulated displacements for some given array of x-y locations - use GetReadbackCursor() to identify the
// kick which produced the simulation results that are about to be retrieved
GFSDK_WAVEWORKS_DECL(gfsdk_wwresult) GFSDK_WaveWorks_Wind_Waves_Simulation_GetDisplacements(GFSDK_WaveWorks_Wind_Waves_SimulationHandle hSim, 
																							const gfsdk_float2* pInSamplePoints, 
																							gfsdk_float4* pOutDisplacements, 
																							uint32_t numSamples, 
																							bool calculateOnGPU);

// Get the most recent simulation statistics
GFSDK_WAVEWORKS_DECL(gfsdk_wwresult) GFSDK_WaveWorks_Wind_Waves_Simulation_GetStats(GFSDK_WaveWorks_Wind_Waves_SimulationHandle hSim, GFSDK_WaveWorks_Wind_Waves_Simulation_Stats& stats);

// For the current simulation settings and params, calculate an estimate of the maximum displacement that can be generated by the simulation.
// This can be used to conservatively inflate camera frusta for culling purposes (e.g. as a suitable value for Quadtree_SetFrustumCullMargin)
GFSDK_WAVEWORKS_DECL(float) GFSDK_WaveWorks_Wind_Waves_Simulation_GetConservativeMaxDisplacementEstimate(GFSDK_WaveWorks_Wind_Waves_SimulationHandle hSim);

// Kicks off the work to update the simulation to the most recent time specified by SetTime
// The top of the simulation pipeline is always run on the CPU, whereas the bottom may be run on either the CPU or GPU, depending on whether the simulation
// is using the CPU or GPU path internally, and whether graphics interop is required for rendering.
// If necessary, this call will block until the CPU part of the pipeline is able to accept further in-flight work. If the CPU part of the pipeline
// is already completely full, this means waiting for an in-flight kick to exit the CPU pipeline (kicks are processed in FIFO order)
GFSDK_WAVEWORKS_DECL(gfsdk_wwresult) GFSDK_WaveWorks_Wind_Waves_Simulation_Kick(GFSDK_WaveWorks_Wind_Waves_SimulationHandle hSim, uint64_t* pOutKickID);

// The staging cursor points to the most recent kick to exit the CPU part of the simulation pipeline (and therefore the kick whose state would be set by a
// subsequent call to SetRenderState)
// Returns gfsdk_wwresult_NONE if no simulation results are staged
// The staging cursor will only ever change during an API call, and is guaranteed to advance by a maximum of one kick in any one call
GFSDK_WAVEWORKS_DECL(gfsdk_wwresult) GFSDK_WaveWorks_Wind_Waves_Simulation_GetStagingCursor(GFSDK_WaveWorks_Wind_Waves_SimulationHandle hSim, uint64_t* pOutKickID);

// Advances the staging cursor
// Use block to specify behaviour in the case where there is an in-flight kick in the CPU part of the simulation pipeline
// Returns gfsdk_wwresult_NONE if there are no in-flight kicks in the CPU part of the simulation pipeline
// Returns gfsdk_wwresult_WOULD_BLOCK if there are in-flight kicks in the CPU part of the pipeline, but they're not ready for staging
GFSDK_WAVEWORKS_DECL(gfsdk_wwresult) GFSDK_WaveWorks_Wind_Waves_Simulation_AdvanceStagingCursor(GFSDK_WaveWorks_Wind_Waves_SimulationHandle hSim, bool block);

// Waits until the staging cursor is ready to advance (i.e. waits until a non-blocking call to AdvanceStagingCursor would succeed)
// Returns gfsdk_wwresult_NONE if there are no in-flight kicks in the CPU part of the simulation pipeline
GFSDK_WAVEWORKS_DECL(gfsdk_wwresult) GFSDK_WaveWorks_Wind_Waves_Simulation_WaitStagingCursor(GFSDK_WaveWorks_Wind_Waves_SimulationHandle hSim);

// The readback cursor points to the kick whose results would be fetched by a call to GetDisplacements
// Returns gfsdk_wwresult_NONE if no results are available for readback
// The readback cursor will only ever change during an API call, and is guaranteed to advance by a maximum of one kick in any one call
GFSDK_WAVEWORKS_DECL(gfsdk_wwresult) GFSDK_WaveWorks_Wind_Waves_Simulation_GetReadbackCursor(GFSDK_WaveWorks_Wind_Waves_SimulationHandle hSim, uint64_t* pOutKickID);

// Advances the readback cursor
// Use block to specify behaviour in the case where there is an in-flight readback
// Returns gfsdk_wwresult_NONE if there are no readbacks in-flight beyond staging
// Returns gfsdk_wwresult_WOULD_BLOCK if there are readbacks in-flight  beyond staging, but they're not yet ready
GFSDK_WAVEWORKS_DECL(gfsdk_wwresult) GFSDK_WaveWorks_Wind_Waves_Simulation_AdvanceReadbackCursor(GFSDK_WaveWorks_Wind_Waves_SimulationHandle hSim, bool block);

// Archives the current readback results in the readback FIFO, evicting the oldest FIFO entry if necessary
// Returns gfsdk_wwresult_FAIL if no results are available for readback
GFSDK_WAVEWORKS_DECL(gfsdk_wwresult) GFSDK_WaveWorks_Wind_Waves_Simulation_ArchiveDisplacements(GFSDK_WaveWorks_Wind_Waves_SimulationHandle hSim);

// Identical to GFSDK_WaveWorks_Simulation_GetDisplacements, except values are retrieved from the readback FIFO
// The readback entries to use are specified using the 'coord' parameter, as follows:
//    - specify 0.f to read from the most recent entry in the FIFO
//    - specify (num_readback_FIFO_entries-1) to read from the oldest entry in the FIFO
//    - intervening entries may be accessed the same way, using a zero-based index
//    - if 'coord' is fractional, the nearest pair of entries will be lerp'd accordingly (fractional lookups are therefore more CPU-intensive)
//
GFSDK_WAVEWORKS_DECL(gfsdk_wwresult) GFSDK_WaveWorks_Wind_Waves_Simulation_GetArchivedDisplacements(GFSDK_WaveWorks_Wind_Waves_SimulationHandle hSim, 
																									float coord, 
																									const gfsdk_float2* pInSamplePoints, 
																									gfsdk_float4* pOutDisplacements, 
																									uint32_t numSamples);


/*===========================================================================
Local Waves Simulation
===========================================================================*/

struct GFSDK_WaveWorks_Local_Waves_Simulation_Parameters
{
	// Amplitude scale factor for simulated wave amplitude
	float amplitude_multiplier;

	// In addition to height displacements, the simulation also applies lateral displacements. This controls the non-linearity
	// and therefore 'choppiness' in the resulting wave shapes. Should normally be set to 1.0, can be in the [0, 1] range.
	float lateral_multiplier;

	// The crests of the waves become covered with foam if
	// Jacobian of wave curvature gets higher than this threshold. The range is [0, 1].
	float foam_whitecaps_threshold;
	// The turbulent energy representing foam and bubbles spread in water starts generating on the crests of the waves if
	// Jacobian of wave curvature gets higher than this threshold. The range is [0, 1], the typical values are in [0.2, 0.4] range.
	float foam_generation_threshold;
	// The amount of turbulent energy injected in areas defined by foam_generation_threshold parameter on each simulation step.
	// The range is [0, 1], the typical values are [0, 0.1] range.
	float foam_generation_amount;
	// The speed of spatial dissipation of turbulent energy. The range is [0, 1], the typical values are in [0.5, 1] range.
	float foam_dissipation_speed;
	// In addition to spatial dissipation, the turbulent energy dissolves over time. This parameter sets the speed of
	// dissolving over time. The range is [0, 1], the typical values are in [0.9, 0.99] range.
	float foam_falloff_speed;
};

struct GFSDK_WaveWorks_Local_Waves_Simulation_Settings
{
	// The size of the simulation grid in cells
	// Adjusted internally to be closest power of two from above in the range from 64 to 2048
	uint32_t simulation_domain_grid_size;

	// The coordinates of the center of the simulation domain on horizontal plane, in meters
	gfsdk_float2 simulation_domain_center;

	// The size of the simulation domain, in meters
	// The simulation covers (simulation_domain_center - simulation_domain_worldspace_size/2.0) .. (simulation_domain_center + simulation_domain_worldspace_size/2.0) area in world space
	float simulation_domain_worldspace_size;

	// This parameter determines whether the simulation is done on the GPU or CPU
	GFSDK_WaveWorks_Simulation_API simulation_api;

	// Should the CPU memory for displacements and displacement archive be preallocated and 
	// the displacement data be read back to the CPU so GFSDK_WaveWorks_Simulation_GetDisplacements(...) would have data to process on CPU side?
	bool enable_CPU_driven_displacement_calculation;

	// Should the GPU memory for sample points, displacement results and displacement archive be preallocated and
	// it would be possible the displacement data in GFSDK_WaveWorks_Simulation_GetDisplacements(...) to be calculated on GPU?
	bool enable_GPU_driven_displacement_calculation;

	// The threading model to use when the CPU simulation path is active
	// Can be set to none (meaning: simulation is performed on the calling thread, synchronously), automatic, or even
	// an explicitly specified thread count
	GFSDK_WaveWorks_Simulation_CPU_Threading_Model CPU_simulation_threading_model;

	// Number of GPUs used (for multi-GPU setups) 
	int32_t num_GPUs;

	// Controls the use of graphics pipeline timers
	bool enable_GPU_timers;

	// Controls the use of CPU timers to gather profiling data
	bool enable_CPU_timers;
};

struct GFSDK_WaveWorks_Local_Waves_Simulation_Stats
{
	// the times spent on particular simulation tasks, measured in milliseconds (1e-3 sec)
	float CPU_main_thread_wait_time;			 // CPU time spent by main app thread waiting for CPU FFT simulation results using CPU 
	float CPU_threads_total_time;				 // CPU time spent on CPU FFT simulation: sum time spent in threads that perform simulation work
	float GPU_simulation_time;					 // GPU time spent on GPU FFT simulation
	float GPU_update_time;						 // GPU time spent on non-simulation e.g. updating gradient maps, foam simulation, mips generation, etc
	float GPU_total_time;						 // Total GPU time spent on all workloads
};

struct GFSDK_WaveWorks_Local_Waves_Rendering_Data
{
	// Mapping from world space to textures: 
	// UV for each cascade on rendering should be = float2(0.5, 0.5) + (worldspace position - simulation_domain_worldspace_center) / simulation_domain_worldspace_size
	gfsdk_float2 simulation_domain_worldspace_center;
	float simulation_domain_worldspace_size;

	// Size of mip 0 of all the textures in texels
	uint32_t size_of_texture;

	// Pointers to native API objects that can be used for rendering:
	// ID3D11Resource in case DX11 is used, ID3D12Resource in case DX12 is used, etc.
	// The resources are RGBA16 float texture with all miplevels generated internally,
	// the size of texture depends on GFSDK_WaveWorks_Local_Waves_Simulation_Settings::simulation_domain_grid_size
	// The textures can be recreated internally if GFSDK_WaveWorks_Local_Waves_Simulation_Settings change on the call to GFSDK_WaveWorks_Local_Waves_Simulation_UpdateProperties
	void* displacements_texture;
	void* gradients_texture;
};

// Simulation lifetime management
GFSDK_WAVEWORKS_DECL(gfsdk_wwresult) GFSDK_WaveWorks_Local_Waves_Simulation_CreateDirectX11(void* pDeviceContext, 
																							bool enable_graphics, 
																							const GFSDK_WaveWorks_Local_Waves_Simulation_Settings& settings, 
																							const GFSDK_WaveWorks_Local_Waves_Simulation_Parameters& params, 
																							GFSDK_WaveWorks_Local_Waves_SimulationHandle* pResult);
GFSDK_WAVEWORKS_DECL(gfsdk_wwresult) GFSDK_WaveWorks_Local_Waves_Simulation_CreateDirectX12(void* pDevice, void* pQueue, 
																							bool enable_graphics, 
																							const GFSDK_WaveWorks_Local_Waves_Simulation_Settings& settings, 
																							const GFSDK_WaveWorks_Local_Waves_Simulation_Parameters& params, 
																							GFSDK_WaveWorks_Local_Waves_SimulationHandle* pResult);
GFSDK_WAVEWORKS_DECL(gfsdk_wwresult) GFSDK_WaveWorks_Local_Waves_Simulation_CreateVK(	void* pVkInstance,
																						void* pVulkanPhysicalDevice,
																						void* pVulkanDevice,
																						void* pGraphicsQueue, int32_t graphicsQueueIndex, /*void* pGraphicsCommandPool */
																						void* pTransferQueue, int32_t transferQueueIndex, /*void* pTransferCommandPool */
																						void* pComputeQueue, int32_t computeQueueIndex, /*void* pComputeCommandPool*/
																						const char **instanceExtensions, int32_t numInstanceExtensions,
																						const char **layers, int32_t numLayers,
																						const char **deviceExtensions, int32_t numDeviceExtensions,
																						bool enable_graphics,
																						const GFSDK_WaveWorks_Local_Waves_Simulation_Settings& settings,
																						const GFSDK_WaveWorks_Local_Waves_Simulation_Parameters& params,
																						GFSDK_WaveWorks_Local_Waves_SimulationHandle* pResult);

GFSDK_WAVEWORKS_DECL(gfsdk_wwresult) GFSDK_WaveWorks_Local_Waves_Simulation_Destroy(GFSDK_WaveWorks_Local_Waves_SimulationHandle hSim);

// A simulation can be 'updated' with new settings and properties - this is universally preferable to recreating
// a simulation from scratch, since WaveWorks will only do as much reinitialization work as is necessary to implement
// the changes in the setup. For instance, simple changes of foam generation parameters require no reallocations and no interruptions
// to the simulation and rendering pipeline
GFSDK_WAVEWORKS_DECL(gfsdk_wwresult) GFSDK_WaveWorks_Local_Waves_Simulation_UpdateProperties(	GFSDK_WaveWorks_Local_Waves_SimulationHandle hSim, 
																								const GFSDK_WaveWorks_Local_Waves_Simulation_Settings& settings, 
																								const GFSDK_WaveWorks_Local_Waves_Simulation_Parameters& params);

// Sets the time delta for the next kick. 
GFSDK_WAVEWORKS_DECL(gfsdk_wwresult) GFSDK_WaveWorks_Local_Waves_Simulation_SetDeltaTime(GFSDK_WaveWorks_Local_Waves_SimulationHandle hSim, float fDeltaTime);

// Retrieves the data and texture arrays needed to render the water surface
GFSDK_WAVEWORKS_DECL(gfsdk_wwresult) GFSDK_WaveWorks_Local_Waves_GetDataForRendering(GFSDK_WaveWorks_Local_Waves_SimulationHandle hSim, GFSDK_WaveWorks_Local_Waves_Rendering_Data& renderingData);

// Retrieve an array of simulated displacements for some given array of x-y locations - use GetReadbackCursor() to identify the
// kick which produced the simulation results that are about to be retrieved
GFSDK_WAVEWORKS_DECL(gfsdk_wwresult) GFSDK_WaveWorks_Local_Waves_Simulation_GetDisplacements(	GFSDK_WaveWorks_Local_Waves_SimulationHandle hSim, 
																								const gfsdk_float2* pInSamplePoints, 
																								gfsdk_float4* pOutDisplacements, 
																								uint32_t numSamples, 
																								bool calculateOnGPU);

// Get the most recent simulation statistics
GFSDK_WAVEWORKS_DECL(gfsdk_wwresult) GFSDK_WaveWorks_Local_Waves_Simulation_GetStats(GFSDK_WaveWorks_Local_Waves_SimulationHandle hSim, GFSDK_WaveWorks_Local_Waves_Simulation_Stats& stats);

// Kicks off the work to update the simulation to the most recent time specified by SetDeltaTime
// The top of the simulation pipeline is always run on the CPU, whereas the bottom may be run on either the CPU or GPU, depending on whether the simulation
// is using the CPU or GPU path internally, and whether graphics interop is required for rendering.
// If necessary, this call will block until the CPU part of the pipeline is able to accept further in-flight work. If the CPU part of the pipeline
// is already completely full, this means waiting for an in-flight kick to exit the CPU pipeline (kicks are processed in FIFO order)
GFSDK_WAVEWORKS_DECL(gfsdk_wwresult) GFSDK_WaveWorks_Local_Waves_Simulation_Kick(GFSDK_WaveWorks_Local_Waves_SimulationHandle hSim, uint64_t* pOutKickID);

// The staging cursor points to the most recent kick to exit the CPU part of the simulation pipeline (and therefore the kick whose state would be set by a
// subsequent call to SetRenderState)
// Returns gfsdk_wwresult_NONE if no simulation results are staged
// The staging cursor will only ever change during an API call, and is guaranteed to advance by a maximum of one kick in any one call
GFSDK_WAVEWORKS_DECL(gfsdk_wwresult) GFSDK_WaveWorks_Local_Waves_Simulation_GetStagingCursor(GFSDK_WaveWorks_Local_Waves_SimulationHandle hSim, uint64_t* pOutKickID);

// Advances the staging cursor
// Use block to specify behaviour in the case where there is an in-flight kick in the CPU part of the simulation pipeline
// Returns gfsdk_wwresult_NONE if there are no in-flight kicks in the CPU part of the simulation pipeline
// Returns gfsdk_wwresult_WOULD_BLOCK if there are in-flight kicks in the CPU part of the pipeline, but they're not ready for staging
GFSDK_WAVEWORKS_DECL(gfsdk_wwresult) GFSDK_WaveWorks_Local_Waves_Simulation_AdvanceStagingCursor(GFSDK_WaveWorks_Local_Waves_SimulationHandle hSim, bool block);

// Waits until the staging cursor is ready to advance (i.e. waits until a non-blocking call to AdvanceStagingCursor would succeed)
// Returns gfsdk_wwresult_NONE if there are no in-flight kicks in the CPU part of the simulation pipeline
GFSDK_WAVEWORKS_DECL(gfsdk_wwresult) GFSDK_WaveWorks_Local_Waves_Simulation_WaitStagingCursor(GFSDK_WaveWorks_Local_Waves_SimulationHandle hSim);

// The readback cursor points to the kick whose results would be fetched by a call to GetDisplacements
// Returns gfsdk_wwresult_NONE if no results are available for readback
// The readback cursor will only ever change during an API call, and is guaranteed to advance by a maximum of one kick in any one call
GFSDK_WAVEWORKS_DECL(gfsdk_wwresult) GFSDK_WaveWorks_Local_Waves_Simulation_GetReadbackCursor(GFSDK_WaveWorks_Local_Waves_SimulationHandle hSim, uint64_t* pOutKickID);

// Advances the readback cursor
// Use block to specify behaviour in the case where there is an in-flight readback
// Returns gfsdk_wwresult_NONE if there are no readbacks in-flight beyond staging
// Returns gfsdk_wwresult_WOULD_BLOCK if there are readbacks in-flight  beyond staging, but they're not yet ready
GFSDK_WAVEWORKS_DECL(gfsdk_wwresult) GFSDK_WaveWorks_Local_Waves_Simulation_AdvanceReadbackCursor(GFSDK_WaveWorks_Local_Waves_SimulationHandle hSim, bool block);

// Resets the simulation and clears the simulated displacements/foam
// Reset does not affect the in-flight kicks, it takes place during the very first kick following this call 
GFSDK_WAVEWORKS_DECL(gfsdk_wwresult) GFSDK_WaveWorks_Local_Waves_Simulation_Reset(GFSDK_WaveWorks_Local_Waves_SimulationHandle hSim);

// Adds displacements and velocity potential to simulation.
// The function accepts the pointer to gfsdk_float4 array of data.
// - X component is X coordinate of disturbance point, Y component is Y coordinate of disturbance point,
// - Z component of the data is added to displacements, W component is added to velocity potential
// Scaling and shifting data from worldspace to simulation domain is performed if bMapFromWorldSpace is set to true,
// scaling and shifting uses bilinear filtering
// Adding disturbances does not affect the in-flight kicks, it takes place during the very first kick following this call. 
// If multiple AddDisturbances calls are made before the kick, then the all the disturbances from all the calls are accumulated.
GFSDK_WAVEWORKS_DECL(gfsdk_wwresult) GFSDK_WaveWorks_Local_Waves_Simulation_AddDisturbances(GFSDK_WaveWorks_Local_Waves_SimulationHandle hSim, gfsdk_float4* pInDisturbanceData, uint32_t numDisturbanceSamples);

/*===========================================================================
	Quadtree geometry generator
===========================================================================*/

struct GFSDK_WaveWorks_Quadtree_Params
{
	// The number of cells in the geometry patches along X and Y axes. 
	// The amount of generated vertices for patches representing the quadtree nodes is (cell_count + 1)*(cell_count + 1).
	// Adjusted to the closest multiple of 16 from above internally in the range of 16..128 so geomorphing can generate watertight geometry.
	uint32_t cell_count;
			
	// The size of the smallest geometry patch representing quadtree node in world space, in meters (i.e. the size of a finest LOD patch)
	float min_patch_length;
			
	// The LOD of the root node patches used to build quadtree. This determines the furthest
	// coverage of the water surface from the eye point as min_patch_length*(2^max_LOD_number) in meters
	uint32_t max_LOD_number;
			
	// The upper limit of the length in pixels that an edge of triangle of the generated geometry can cover in screen space. 
	// Quadtree generator will try to generate geometry that respects this parameter. 
	// Clamped to 2..100 range internally.
	float max_edge_length;
			
	// The mean ocean level in meters, it is used internally for culling the quadtree nodes against view frustum.
	float mean_sea_level;
			
	// The flag that defines whether cell diagonals should be uniform or alternating (forming diamond pattern)
	bool generate_diamond_pattern;
			
	// The degree of geomorphing to use, in the range [0,1]
	float geomorphing_degree;
};

struct GFSDK_WaveWorks_Quadtree_Stats
{
	// The total amount of quadtree nodes created internally during visibility and LOD based quadtree subdivision.
	uint32_t num_quadtree_nodes_created;

	// The total amount of quadtree nodes to be rendered.
	uint32_t num_quadtree_nodes_for_rendering;

	// The total number of nodes added by subdividing existing nodes during quadtree refinement (balancing) to avoid LOD discontinuities between adjacent quadtree nodes.
	// If this value is not zero, then some of the quadtree nodes are excessively subdivided, 
	// and quadtree parameters should be adjusted to use quadtree structure more efficiently.
	uint32_t num_refinement_nodes_added;

	// The total number of quadtree refinement iterations that quadtree generator had to run 
	// to rebuild quadtree to avoid LOD discontinuities between adjacent quadtree nodes.
	// If this value is not zero, then the quadtree parameters should be adjusted to use quadtree structure more efficiently.
	uint32_t num_refinement_iterations;

	// CPU time spent on quadtree generation in update, measured in milliseconds (1e-3 sec)
	float CPU_quadtree_update_time;
};

struct GFSDK_WaveWorks_Quadtree_NodeRenderingProperties
{
	uint32_t start_index;                        // Argument to draw call
	uint32_t num_indices;                        // Argument to draw call
	uint32_t patch_type;                         // Can be used to sort patch topologies by type and draw multiple patches with same topology in a single draw call using instancing
	gfsdk_float2 patch_worldspace_origin;        // This and the following parameters can be used either in constant buffer or in instance buffer to move, scale and geomorph patch representing quadtree node
	float patch_worldspace_scale;
	float geomorphing_distance_constant;
	float geomorphing_sign;
};

// Quadtree lifetime management
GFSDK_WAVEWORKS_DECL(gfsdk_wwresult) GFSDK_WaveWorks_Quadtree_Create(const GFSDK_WaveWorks_Quadtree_Params& params, GFSDK_WaveWorks_QuadtreeHandle* pResult);
GFSDK_WAVEWORKS_DECL(gfsdk_wwresult) GFSDK_WaveWorks_Quadtree_Destroy(GFSDK_WaveWorks_QuadtreeHandle hQuadtree);

// Updates quadtree generation parameters with new GFSDK_WaveWorks_Quadtree_Params
GFSDK_WAVEWORKS_DECL(gfsdk_wwresult) GFSDK_WaveWorks_Quadtree_UpdateParams(GFSDK_WaveWorks_QuadtreeHandle hQuadtree, const GFSDK_WaveWorks_Quadtree_Params& params);

// Exposes pointers to internally generated data that can be used to create R32G32_FLOAT vertex and R32_UINT index buffers for rendering the quadtree.
// ppVertexPositions contains pointer to single set of vertex coordinates used to render all the patches representing quadtree nodes.
// ppIndices contains pointer to combination of the sets of indices defining all the possible topologies of patches representing the quadtree nodes.
// The data at the pointers is invalidated and recreated during GFSDK_WaveWorks_Quadtree_UpdateParams calls if 
// cell_count or generate_diamond_pattern changes in GFSDK_WaveWorks_Quadtree_UpdateParams,
// so the vertex and index buffers need to be recreated / updated on the application side.
GFSDK_WAVEWORKS_DECL(gfsdk_wwresult) GFSDK_WaveWorks_Quadtree_GetGeometryData(	GFSDK_WaveWorks_QuadtreeHandle hQuadtree, 
																				uint32_t* pNumVertices, 
																				gfsdk_float2** ppVertexPositions, 
																				uint32_t* pNumIndices, 
																				uint32_t** ppIndices);

// Builds quadtree based on view/projection matrices and viewport size.
// Exposes pointer to the internally generated and maintained array of GFSDK_WaveWorks_Quadtree_NodeRenderingProperties structures that can be used to render the quadtree.
// The data at the pointers is valid until the next call to the function or until next GFSDK_WaveWorks_Quadtree_UpdateParams call
GFSDK_WAVEWORKS_DECL(gfsdk_wwresult) GFSDK_WaveWorks_Quadtree_GetNodesForRendering(GFSDK_WaveWorks_QuadtreeHandle hQuadtree, 
																					const gfsdk_float4x4& matView, 
																					const gfsdk_float4x4& matProj, 
																					const gfsdk_float2& viewportSize, 
																					float cullMargin, 
																					uint32_t* pNumNodes, 
																					GFSDK_WaveWorks_Quadtree_NodeRenderingProperties** ppNodes);
		
// Get the most recent quadtree generation statistics
GFSDK_WAVEWORKS_DECL(gfsdk_wwresult) GFSDK_WaveWorks_Quadtree_GetStats(GFSDK_WaveWorks_QuadtreeHandle hQuadtree, GFSDK_WaveWorks_Quadtree_Stats& stats);

	
#endif	// _GFSDK_WAVEWORKS_H
