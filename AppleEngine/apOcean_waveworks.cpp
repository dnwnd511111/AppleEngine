#include "CommonInclude.h"
#include "apOcean_waveworks.h"
#include <vector>
#include "apGraphicsDevice_DX12.h"
#include "apResourceManager.h"
#include "apScene.h"
#include "apEventHandler.h"
#include "apRenderer.h"

//#pragma comment(lib,"NVWaveWorks_static.d3d12.lib")
//#pragma comment(lib,"d3d12.lib")
//#pragma comment(lib,"nvrhi_d3d12.lib")




static uint32_t getNumMipLevels(uint32_t const squareTextureSize)
{
	uint32_t mipLevels = 1;
	uint32_t texSize = squareTextureSize;
	while (texSize > 1)
	{
		mipLevels++;
		texSize /= 2;
	}
	return mipLevels;
}

struct OCEAN_VS_HS_DS_CBUFFER
{
	// Data used in vertex shader
	gfsdk_float4x4 g_matViewProj;
	gfsdk_float3   g_eyePos;
	float          g_meanOceanLevel;
	// x16 bytes boundary
	float          g_useDiamondPattern;

	// Data used in hull shader
	float          g_dynamicTesselationAmount;
	float          g_staticTesselationOffset;

	// Data used in domain shader
	// Wind waves related data
	float          g_cascade0UVScale;
	// x16 bytes boundary
	float          g_cascade1UVScale;
	float          g_cascade2UVScale;
	float          g_cascade3UVScale;
	float          g_cascade0UVOffset;
	// x16 bytes boundary
	float          g_cascade1UVOffset;
	float          g_cascade2UVOffset;
	float          g_cascade3UVOffset;
	float          g_UVWarpingAmplitude;
	// x16 bytes boundary
	float          g_UVWarpingFrequency;

	// Local waves related data
	float          g_localWavesSimulationDomainWorldspaceSize;
	gfsdk_float2   g_localWavesSimulationDomainWorldspaceCenter;
	float pad15;

	uint displacementTextureArrayWindWaves;
	uint displacementTextureLocalWaves;
	uint gradientsTextureArrayWindWaves;
	uint momentsTextureArrayWindWaves;

	uint gradientsTextureLocalWaves;
	uint textureFoam;
	uint textureBubbles;
	uint textureWindGusts;

	uint skydomeTexture;
};

struct OCEAN_PS_CBUFFER
{
	// Defined by wind waves simulation
	float g_cascadeToCascadeScale;
	float g_windWavesTextureSizeInTexels;
	float g_UVWarpingAmplitude;
	float g_UVWarpingFrequency;
	// x16 bytes boundary
	float g_windWavesFoamWhitecapsThreshold;

	// Defined by local waves simulation
	float g_localWavesTextureSizeInTexels;
	float g_localWavesFoamWhitecapsThreshold;
	float g_SimulationDomainSize;
	// x16 bytes boundary
	gfsdk_float2 g_SimulationDomainCenter;

	// Defined by application
	float g_beckmannRoughness;
	float g_showCascades;
	// x16 bytes boundary
	gfsdk_float3 g_sunDirection;
	float g_sunIntensity;
	// x16 bytes boundary
	gfsdk_float3 g_eyePos;
	float g_useMicrofacetFresnel;
	// x16 bytes boundary
	float g_useMicrofacetSpecular;
	float g_useMicrofacetReflection;

	// CBs must be multiple of 16 bytes large
	float pad0;
	float pad1;

	//
	DirectX::XMFLOAT4 g_WaterColor;
	DirectX::XMFLOAT4 g_WaterColorIntensity;

	DirectX::XMFLOAT3 g_FoamColor;
	float pad4;

	DirectX::XMFLOAT3 g_FoamUnderwaterColor;
	float pad5;
};

struct OCEAN_VS_CBUFFER_PERINSTANCE_ENTRY
{
	gfsdk_float2 g_patchWorldspaceOrigin;
	float g_patchWorldspaceScale;
	float g_patchMorphConstantAndSign;
};

struct MARKER_VS_CBUFFER
{
	gfsdk_float4x4 g_matViewProj;
};


namespace ap
{
	using namespace graphics;
	using namespace scene;

	namespace ocean2_internal
	{

		

		ap::graphics::Texture foamIntensityTexture;
		ap::graphics::Texture foamBubblesTexture;
		ap::graphics::Texture windGustsTexture;

		ap::graphics::Texture skydome;


		Shader		ocean2VS;
		Shader		ocean2HS;
		Shader		ocean2DS;
		Shader		ocean2PS;
		Shader		wireframePS;

		PipelineState PSO, PSO_wire;

		InputLayout			inputLayout;
		RasterizerState		rasterizerState;
		RasterizerState		wireRS;
		DepthStencilState	depthStencilState;
		BlendState			blendState;


		void LoadShaders()
		{
			ap::renderer::LoadShader(ShaderStage::VS, ocean2VS, "ocean2VS.cso");
			ap::renderer::LoadShader(ShaderStage::HS, ocean2HS, "ocean2HS.cso");
			ap::renderer::LoadShader(ShaderStage::DS, ocean2DS, "ocean2DS.cso");
			ap::renderer::LoadShader(ShaderStage::PS, ocean2PS, "ocean2PS.cso");

			ap::renderer::LoadShader(ShaderStage::PS, wireframePS, "oceanSurfaceSimplePS.cso");


			GraphicsDevice* device = ap::graphics::GetDevice();
			{
				PipelineStateDesc desc;
				desc.vs = &ocean2VS;
				desc.hs = &ocean2HS;
				desc.ds = &ocean2DS;
				desc.ps = &ocean2PS;
				desc.bs = &blendState;
				desc.rs = &rasterizerState;
				desc.dss = &depthStencilState;
				desc.pt = PrimitiveTopology::PATCHLIST;
				desc.il = &inputLayout;
				device->CreatePipelineState(&desc, &PSO);

				desc.ps = &wireframePS;
				desc.rs = &wireRS;
				device->CreatePipelineState(&desc, &PSO_wire);
			}

		}

	}
	
	using namespace ocean2_internal;


	void Ocean2::Create()
	{
		auto device = static_cast<ap::graphics::GraphicsDevice_DX12*>(ap::graphics::GetDevice());
		
		HRESULT hr = device->device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
		assert(SUCCEEDED(hr));


		// Creating wind waves simulation
		GFSDK_WaveWorks_Wind_Waves_Simulation_CreateDirectX12(
			device->device.Get(),
			device->queues[QUEUE_GRAPHICS].queue.Get(),
			true,
			parameters.OceanWindSimulationSettings,
			parameters.OceanWindSimulationParameters,
			&(hOceanWindSimulation));

		// Creating local waves simulation
		GFSDK_WaveWorks_Local_Waves_Simulation_CreateDirectX12(
			device->device.Get(),
			device->queues[QUEUE_GRAPHICS].queue.Get(),
			true, parameters.OceanLocalSimulationSettings,
			parameters.OceanLocalSimulationParameters,
			&(hOceanLocalSimulation));

		// Creating quadtree
		GFSDK_WaveWorks_Quadtree_Create(parameters.OceanQuadtreeParameters, &hOceanQuadtree);

		
		ReCreateResource();

	}
	Ocean2::Ocean2()
	{
		// Initializing the wind simulation parameters & settings
		parameters.OceanWindSimulationParameters.time_scale = 1.0f;
		parameters.OceanWindSimulationParameters.lateral_multiplier = 1.f;
		parameters.OceanWindSimulationParameters.uv_warping_amplitude = 0.03f;
		parameters.OceanWindSimulationParameters.uv_warping_frequency = 2.f;
		
		parameters.OceanWindSimulationParameters.base_wind_direction = { 1.0f, 0.0f };
		parameters.OceanWindSimulationParameters.base_wind_speed = 4.7f;
		parameters.OceanWindSimulationParameters.base_wind_distance = 0.1f;
		parameters.OceanWindSimulationParameters.base_wind_dependency = 1.0f;
		parameters.OceanWindSimulationParameters.base_spectrum_peaking = 3.3f;
		parameters.OceanWindSimulationParameters.base_small_waves_cutoff_length = 0.0f;
		parameters.OceanWindSimulationParameters.base_small_waves_cutoff_power = 0.0f;
		parameters.OceanWindSimulationParameters.base_amplitude_multiplier = 1.0f;
		
		parameters.OceanWindSimulationParameters.swell_wind_direction = { 0.0f, 1.0f };
		parameters.OceanWindSimulationParameters.swell_wind_speed = 1.5f;
		parameters.OceanWindSimulationParameters.swell_wind_distance = 520.0f;
		parameters.OceanWindSimulationParameters.swell_wind_dependency = 1.0f;
		parameters.OceanWindSimulationParameters.swell_spectrum_peaking = 10.0f;
		parameters.OceanWindSimulationParameters.swell_small_waves_cutoff_length = 60.0f;
		parameters.OceanWindSimulationParameters.swell_small_waves_cutoff_power = 1.0f;
		parameters.OceanWindSimulationParameters.swell_amplitude_multiplier = 1.0f;
		
		parameters.OceanWindSimulationParameters.foam_whitecaps_threshold = 0.5f;
		parameters.OceanWindSimulationParameters.foam_dissipation_speed = 0.6f;
		parameters.OceanWindSimulationParameters.foam_falloff_speed = 0.985f;
		parameters.OceanWindSimulationParameters.foam_generation_amount = 0.12f;
		parameters.OceanWindSimulationParameters.foam_generation_threshold = 0.37f;

		parameters.OceanWindSimulationSettings.simulation_period = 1000.0f;
		parameters.OceanWindSimulationSettings.simulation_api = GFSDK_WaveWorks_Simulation_API_Compute;
		parameters.OceanWindSimulationSettings.detail_level = GFSDK_WaveWorks_Simulation_DetailLevel_Extreme;
		parameters.OceanWindSimulationSettings.enable_CPU_driven_displacement_calculation = true;
		parameters.OceanWindSimulationSettings.enable_GPU_driven_displacement_calculation = true;
		parameters.OceanWindSimulationSettings.num_readback_FIFO_entries = parameters.ReadbackArchiveSize;
		parameters.OceanWindSimulationSettings.CPU_simulation_threading_model = GFSDK_WaveWorks_Simulation_CPU_Threading_Model_Automatic;
		parameters.OceanWindSimulationSettings.use_Beaufort_scale = true;
		parameters.OceanWindSimulationSettings.num_GPUs = 1;
		parameters.OceanWindSimulationSettings.enable_GPU_timers = true;
		parameters.OceanWindSimulationSettings.enable_CPU_timers = true;

		// Initializing the local simulation parameters & settings
		parameters.OceanLocalSimulationParameters.amplitude_multiplier = 1.0f;
		parameters.OceanLocalSimulationParameters.lateral_multiplier = 1.0f;

		parameters.OceanLocalSimulationParameters.foam_whitecaps_threshold = 0.3f;
		parameters.OceanLocalSimulationParameters.foam_dissipation_speed = 0.3f;
		parameters.OceanLocalSimulationParameters.foam_falloff_speed = 0.6f;
		parameters.OceanLocalSimulationParameters.foam_generation_amount = 0.2f;
		parameters.OceanLocalSimulationParameters.foam_generation_threshold = 0.6f;

		parameters.OceanLocalSimulationSettings.simulation_domain_center.x = 0;
		parameters.OceanLocalSimulationSettings.simulation_domain_center.y = 0;
		parameters.OceanLocalSimulationSettings.simulation_api = GFSDK_WaveWorks_Simulation_API_Compute;
		parameters.OceanLocalSimulationSettings.simulation_domain_worldspace_size = 200.0f;
		parameters.OceanLocalSimulationSettings.simulation_domain_grid_size = 512;
		parameters.OceanLocalSimulationSettings.enable_CPU_driven_displacement_calculation = true;
		parameters.OceanLocalSimulationSettings.enable_GPU_driven_displacement_calculation = true;
		parameters.OceanLocalSimulationSettings.CPU_simulation_threading_model = GFSDK_WaveWorks_Simulation_CPU_Threading_Model_Automatic;
		parameters.OceanLocalSimulationSettings.num_GPUs = 1;
		parameters.OceanLocalSimulationSettings.enable_GPU_timers = true;
		parameters.OceanLocalSimulationSettings.enable_CPU_timers = true;

		// Resetting the stats
		memset(&OceanWindSimulationStats, 0, sizeof(OceanWindSimulationStats));
		memset(&OceanWindSimulationStatsFiltered, 0, sizeof(OceanWindSimulationStatsFiltered));

		memset(&OceanLocalSimulationStats, 0, sizeof(OceanLocalSimulationStats));
		memset(&OceanLocalSimulationStatsFiltered, 0, sizeof(OceanLocalSimulationStatsFiltered));

		bForceKick = true;

		// Initializing quadtree parameters
		parameters.OceanQuadtreeParameters.min_patch_length = 5.0f;
		parameters.OceanQuadtreeParameters.max_edge_length = 10.0f;
		parameters.OceanQuadtreeParameters.cell_count = 64;
		parameters.OceanQuadtreeParameters.mean_sea_level = 0.0f;
		parameters.OceanQuadtreeParameters.max_LOD_number = 15;
		parameters.OceanQuadtreeParameters.geomorphing_degree = 1.0f;
		parameters.OceanQuadtreeParameters.generate_diamond_pattern = false;
	}
	void Ocean2::ResourceUpdate()
	{

		ap::graphics::GraphicsDevice_DX12* device = static_cast<ap::graphics::GraphicsDevice_DX12*>(ap::graphics::GetDevice());
		Scene& scene = GetScene();
		CameraComponent& camera = GetCamera();
		float delta = scene.dt;

	
		//device->queues[QUEUE_GRAPHICS].queue->Wait(fence.Get(), 1);
		//device->queues[QUEUE_GRAPHICS].queue->Signal(fence.Get(), 0);

		// Adding rain
		if ((parameters.bRain) && (!bNeedToUpdateLocalWavesSimulationProperties))
		{
			gfsdk_float4 dd[200];
			for (int i = 0; i < 200; i++)
			{
				uint32_t x = rand() % parameters.OceanLocalSimulationSettings.simulation_domain_grid_size;
				uint32_t y = rand() % parameters.OceanLocalSimulationSettings.simulation_domain_grid_size;
				dd[i].x = (float)x;
				dd[i].y = (float)y;
				dd[i].z = parameters.fRainDropSize * (rand() % 100) * 0.01f;
				dd[i].w = 0.0f;
			}
			GFSDK_WaveWorks_Local_Waves_Simulation_AddDisturbances(hOceanLocalSimulation, dd, 200);
		}

		// Adding "boat"
		if ((parameters.bBoat) && (!bNeedToUpdateLocalWavesSimulationProperties))
		{
			float time = (float)dSimulationTime;
			uint32_t N = parameters.OceanLocalSimulationSettings.simulation_domain_grid_size;
			fBoatX = (float)(100.0f * sinf(time * 0.1f));
			fBoatY = (float)(100.0f * cosf(time * 0.13f));
			float dX = 0.1f * 100.0f * cosf((float)time * 0.1f);
			float dY = -0.13f * 100.0f * sinf((float)time * 0.13f);
			float speed = sqrtf(dX * dX + dY * dY);
			fBoatX = (float)(100.0f * sinf(time * 0.1f));
			fBoatY = (float)(100.0f * cosf(time * 0.13f));
			gfsdk_float4 dd[100];
			for (int32_t i = 0; i < 10; i++)
			{
				for (int32_t j = 0; j < 10; j++)
				{
					float x = fBoatX + i - (10 / 2) + (N / 2);
					float y = fBoatY + j - (10 / 2) + (N / 2);
					dd[i + 10 * j].x = x;
					dd[i + 10 * j].y = y;
					dd[i + 10 * j].z = 0.13f * speed * delta;
					dd[i + 10 * j].w = 0;
				}
			}
			GFSDK_WaveWorks_Local_Waves_Simulation_AddDisturbances(hOceanLocalSimulation, dd, 100);
			/*gfsdk_float2 cameraXY = { fBoatX, fBoatY };
			cameraXY *= OceanLocalSimulationSettings.simulation_domain_worldspace_size / OceanLocalSimulationSettings.simulation_domain_grid_size;
			gfsdk_float3 cameraPos = { cameraXY.x - 30.0f , 10, cameraXY.y - 3.0f };
			gfsdk_float3 atPos = { cameraXY.x , 5, cameraXY.y };
			Camera.LookAt(cameraPos, atPos);*/
		}


		// Simulating local waves
		if (bNeedToResetLocalWavesSimulation)
		{
			GFSDK_WaveWorks_Local_Waves_Simulation_Reset(hOceanLocalSimulation);
			bNeedToResetLocalWavesSimulation = false;
		}


		if (parameters.bSimulateWater || bForceKick || (gfsdk_wwresult_NONE == GFSDK_WaveWorks_Local_Waves_Simulation_GetStagingCursor(hOceanLocalSimulation, NULL)))
		{
			GFSDK_WaveWorks_Local_Waves_Simulation_SetDeltaTime(hOceanLocalSimulation, delta);
			GFSDK_WaveWorks_Local_Waves_Simulation_Kick(hOceanLocalSimulation, &iLastLocalSimulationKickID);
		}


		if (SyncMode >= SynchronizationMode_RenderOnly)
		{
			// Block until the just-submitted kick is ready to render
			uint64_t stagingCursorKickID = iLastLocalSimulationKickID - 1;	// Just ensure that the initial value is different from last kick,
																					// so that we continue waiting if the staging cursor is empty
			GFSDK_WaveWorks_Local_Waves_Simulation_GetStagingCursor(hOceanLocalSimulation, &stagingCursorKickID);
			while (stagingCursorKickID != iLastLocalSimulationKickID)
			{
				const bool doBlock = true;
				GFSDK_WaveWorks_Local_Waves_Simulation_AdvanceStagingCursor(hOceanLocalSimulation, doBlock);
				GFSDK_WaveWorks_Local_Waves_Simulation_GetStagingCursor(hOceanLocalSimulation, &stagingCursorKickID);
			}

			if ((SyncMode >= SynchronizationMode_Readback) && (parameters.iReadbackUsage > 0))
			{
				uint64_t readbackCursorKickID = iLastLocalSimulationKickID - 1;	// Just ensure that the initial value is different from last kick,
																							// so that we continue waiting if the staging cursor is empty
				while (readbackCursorKickID != iLastLocalSimulationKickID)
				{
					const bool doBlock = true;
					GFSDK_WaveWorks_Local_Waves_Simulation_AdvanceReadbackCursor(hOceanLocalSimulation, doBlock);
					GFSDK_WaveWorks_Local_Waves_Simulation_GetReadbackCursor(hOceanLocalSimulation, &readbackCursorKickID);
				}
			}
		}
		else
		{

			// Keep feeding the simulation pipeline until it is full - this loop should skip in all
			// cases except the first iteration, when the simulation pipeline is first 'primed'
			while (GFSDK_WaveWorks_Local_Waves_Simulation_GetStagingCursor(hOceanLocalSimulation, NULL) == gfsdk_wwresult_NONE)
			{
				GFSDK_WaveWorks_Local_Waves_Simulation_SetDeltaTime(hOceanLocalSimulation, delta);
				GFSDK_WaveWorks_Local_Waves_Simulation_Kick(hOceanLocalSimulation, &iLastLocalSimulationKickID);
			}

		}

		// Getting simulation timings
		GFSDK_WaveWorks_Local_Waves_Simulation_GetStats(hOceanLocalSimulation, OceanLocalSimulationStats);

		// Exponential filtering for stats
		OceanLocalSimulationStatsFiltered.CPU_main_thread_wait_time = OceanLocalSimulationStatsFiltered.CPU_main_thread_wait_time * 0.999f + 0.001f * OceanLocalSimulationStats.CPU_main_thread_wait_time;
		OceanLocalSimulationStatsFiltered.CPU_threads_total_time = OceanLocalSimulationStatsFiltered.CPU_threads_total_time * 0.999f + 0.001f * OceanLocalSimulationStats.CPU_threads_total_time;
		OceanLocalSimulationStatsFiltered.GPU_simulation_time = OceanLocalSimulationStatsFiltered.GPU_simulation_time * 0.999f + 0.001f * OceanLocalSimulationStats.GPU_simulation_time;
		OceanLocalSimulationStatsFiltered.GPU_update_time = OceanLocalSimulationStatsFiltered.GPU_update_time * 0.999f + 0.001f * OceanLocalSimulationStats.GPU_update_time;
		OceanLocalSimulationStatsFiltered.GPU_total_time = OceanLocalSimulationStatsFiltered.GPU_total_time * 0.999f + 0.001f * OceanLocalSimulationStats.GPU_total_time;


		// deduce the rendering latency of the WaveWorks local waves simulation pipeline
		{
			uint64_t staging_cursor_kickID = 0;
			GFSDK_WaveWorks_Local_Waves_Simulation_GetStagingCursor(hOceanLocalSimulation, &staging_cursor_kickID);
			iLocalSimulationRenderLatency = (uint32_t)(iLastLocalSimulationKickID - staging_cursor_kickID);
		}


		// likewise with the readback latency
		if (parameters.iReadbackUsage > 0)
		{
			uint64_t readback_cursor_kickID = 0;
			if (GFSDK_WaveWorks_Local_Waves_Simulation_GetReadbackCursor(hOceanLocalSimulation, &readback_cursor_kickID) == gfsdk_wwresult_OK)
			{
				iLocalSimulationReadbackLatency = (int32_t)(iLastLocalSimulationKickID - readback_cursor_kickID);
			}
			else
			{
				iLocalSimulationReadbackLatency = -1;
			}
		}
		else
		{
			iLocalSimulationReadbackLatency = -1;
		}

		// Simulating wind waves
		if (parameters.bSimulateWater || bForceKick || (gfsdk_wwresult_NONE == GFSDK_WaveWorks_Wind_Waves_Simulation_GetStagingCursor(hOceanWindSimulation, NULL)))
		{
			dSimulationTime += delta;
			GFSDK_WaveWorks_Wind_Waves_Simulation_SetTime(hOceanWindSimulation, dSimulationTime);
			GFSDK_WaveWorks_Wind_Waves_Simulation_Kick(hOceanWindSimulation, &iLastWindSimulationKickID);
		}


		if (SyncMode >= SynchronizationMode_RenderOnly)
		{
			// Block until the just-submitted kick is ready to render
			uint64_t stagingCursorKickID = iLastWindSimulationKickID - 1;	// Just ensure that the initial value is different from last kick,
																// so that we continue waiting if the staging cursor is empty
			GFSDK_WaveWorks_Wind_Waves_Simulation_GetStagingCursor(hOceanWindSimulation, &stagingCursorKickID);
			while (stagingCursorKickID != iLastWindSimulationKickID)
			{
				const bool doBlock = true;
				GFSDK_WaveWorks_Wind_Waves_Simulation_AdvanceStagingCursor(hOceanWindSimulation, doBlock);
				GFSDK_WaveWorks_Wind_Waves_Simulation_GetStagingCursor(hOceanWindSimulation, &stagingCursorKickID);
			}

			if ((SyncMode >= SynchronizationMode_Readback) && (parameters.iReadbackUsage > 0))
			{
				uint64_t readbackCursorKickID = iLastWindSimulationKickID - 1;	// Just ensure that the initial value is different from last kick,
																	// so that we continue waiting if the staging cursor is empty
				while (readbackCursorKickID != iLastWindSimulationKickID)
				{
					const bool doBlock = true;
					GFSDK_WaveWorks_Wind_Waves_Simulation_AdvanceReadbackCursor(hOceanWindSimulation, doBlock);
					GFSDK_WaveWorks_Wind_Waves_Simulation_GetReadbackCursor(hOceanWindSimulation, &readbackCursorKickID);
				}
			}
		}
		else
		{
			// Keep feeding the simulation pipeline until it is full - this loop should skip in all
			// cases except the first iteration, when the simulation pipeline is first 'primed'
			while (GFSDK_WaveWorks_Wind_Waves_Simulation_GetStagingCursor(hOceanWindSimulation, NULL) == gfsdk_wwresult_NONE)
			{
				GFSDK_WaveWorks_Wind_Waves_Simulation_SetTime(hOceanWindSimulation, dSimulationTime);
				GFSDK_WaveWorks_Wind_Waves_Simulation_Kick(hOceanWindSimulation, &iLastWindSimulationKickID);
			}
		}


		bForceKick = false;


		// Exercise the readback archiving API
		if (GFSDK_WaveWorks_Wind_Waves_Simulation_GetReadbackCursor(hOceanWindSimulation, &iLastWindSimulationReadbackKickID) == gfsdk_wwresult_OK)
		{
			if ((iLastWindSimulationReadbackKickID - iLastWindSimulationArchivedKickID) > parameters.ReadbackArchiveInterval)
			{
				GFSDK_WaveWorks_Wind_Waves_Simulation_ArchiveDisplacements(hOceanWindSimulation);
				iLastWindSimulationArchivedKickID = iLastWindSimulationReadbackKickID;
			}
		}

		// deduce the rendering latency of the WaveWorks wind waves simulation pipeline
		{
			uint64_t staging_cursor_kickID = 0;
			GFSDK_WaveWorks_Wind_Waves_Simulation_GetStagingCursor(hOceanWindSimulation, &staging_cursor_kickID);
			iWindSimulationRenderLatency = (uint32_t)(iLastWindSimulationKickID - staging_cursor_kickID);
		}

		// likewise with the readback latency
		if (parameters.iReadbackUsage > 0)
		{
			uint64_t readback_cursor_kickID = 0;
			if (GFSDK_WaveWorks_Wind_Waves_Simulation_GetReadbackCursor(hOceanWindSimulation, &readback_cursor_kickID) == gfsdk_wwresult_OK)
			{
				iWindSimulationReadbackLatency = (int32_t)(iLastWindSimulationKickID - readback_cursor_kickID);
			}
			else
			{
				iWindSimulationReadbackLatency = -1;
			}
		}
		else
		{
			iWindSimulationReadbackLatency = -1;
		}

		// getting simulation timings
		GFSDK_WaveWorks_Wind_Waves_Simulation_GetStats(hOceanWindSimulation, OceanWindSimulationStats);

		// exponential filtering for stats
		OceanWindSimulationStatsFiltered.CPU_main_thread_wait_time = OceanWindSimulationStatsFiltered.CPU_main_thread_wait_time * 0.999f + 0.001f * OceanWindSimulationStats.CPU_main_thread_wait_time;
		OceanWindSimulationStatsFiltered.CPU_threads_total_time = OceanWindSimulationStatsFiltered.CPU_threads_total_time * 0.999f + 0.001f * OceanWindSimulationStats.CPU_threads_total_time;
		OceanWindSimulationStatsFiltered.GPU_simulation_time = OceanWindSimulationStatsFiltered.GPU_simulation_time * 0.999f + 0.001f * OceanWindSimulationStats.GPU_simulation_time;
		OceanWindSimulationStatsFiltered.GPU_update_time = OceanWindSimulationStatsFiltered.GPU_update_time * 0.999f + 0.001f * OceanWindSimulationStats.GPU_update_time;
		OceanWindSimulationStatsFiltered.GPU_total_time = OceanWindSimulationStatsFiltered.GPU_total_time * 0.999f + 0.001f * OceanWindSimulationStats.GPU_total_time;


		

		// reading back marker coords
		if (parameters.iReadbackUsage > 0)
		{
			//UpdateMarkers();
		}

		//device->queues[QUEUE_GRAPHICS].queue->Signal(fence.Get(), 1);
		

	}

	void Ocean2::Initialize()
	{
		
		auto device = static_cast<ap::graphics::GraphicsDevice_DX12*>(ap::graphics::GetDevice());

		GFSDK_WaveWorks_Init(nullptr, GFSDK_WAVEWORKS_API_GUID);

		inputLayout.elements =
		{
			{ "POSITION",	0, Format::R32G32_FLOAT , 0, InputLayout::APPEND_ALIGNED_ELEMENT, InputClassification::PER_VERTEX_DATA },
		};

		RasterizerState ras_desc;
		ras_desc.fill_mode = FillMode::SOLID;
		ras_desc.cull_mode = CullMode::NONE;
		ras_desc.front_counter_clockwise = false;
		ras_desc.depth_bias = 0;
		ras_desc.slope_scaled_depth_bias = 0.0f;
		ras_desc.depth_bias_clamp = 0.0f;
		ras_desc.depth_clip_enable = true;
		ras_desc.multisample_enable = true;
		ras_desc.antialiased_line_enable = false;
		rasterizerState = ras_desc;

		ras_desc.fill_mode = FillMode::WIREFRAME;
		wireRS = ras_desc;

		DepthStencilState depth_desc;
		depth_desc.depth_enable = true;
		depth_desc.depth_write_mask = DepthWriteMask::ALL;
		depth_desc.depth_func = ComparisonFunc::GREATER;
		depth_desc.stencil_enable = false;
		depthStencilState = depth_desc;

		BlendState blend_desc;
		blend_desc.alpha_to_coverage_enable = false;
		blend_desc.independent_blend_enable = false;
		blend_desc.render_target[0].blend_enable = false;
		blend_desc.render_target[0].src_blend = Blend::SRC_ALPHA;
		blend_desc.render_target[0].dest_blend = Blend::INV_SRC_ALPHA;
		blend_desc.render_target[0].blend_op = BlendOp::ADD;
		blend_desc.render_target[0].src_blend_alpha = Blend::ONE;
		blend_desc.render_target[0].dest_blend_alpha = Blend::ZERO;
		blend_desc.render_target[0].blend_op_alpha = BlendOp::ADD;
		blend_desc.render_target[0].render_target_write_mask = ColorWrite::ENABLE_ALL;
		blendState = blend_desc;

		foamIntensityTexture = ap::resourcemanager::Load("Resources/images/foam_intensity.dds").GetTexture();
		foamBubblesTexture = ap::resourcemanager::Load("Resources/images/foam_bubbles.dds").GetTexture();
		windGustsTexture = ap::resourcemanager::Load("Resources/images/wind_gusts.dds").GetTexture();
		skydome = ap::resourcemanager::Load("Resources/images/sky1.dds").GetTexture();

		device->SetName(&foamIntensityTexture, "foam_intensity");
		device->SetName(&foamBubblesTexture, "foam_bubbles");
		device->SetName(&windGustsTexture, "wind_gusts");
		device->SetName(&skydome, "skydome");


		static ap::eventhandler::Handle handle = ap::eventhandler::Subscribe(ap::eventhandler::EVENT_RELOAD_SHADERS, [](uint64_t userdata) { LoadShaders(); });

		LoadShaders();
	}




	void Ocean2::Render(ap::graphics::CommandList cmd)
	{

		cmdCount = cmd;

		gfsdk_float2 viewportSize;
		viewportSize.x = viewportWidth;
		viewportSize.y = viewportHeight;

		GraphicsDevice* device = ap::graphics::GetDevice();
		CameraComponent& camera = GetCamera();
		Scene& scene = GetScene();

		device->EventBegin("Ocean2 Rendering", cmd);


		// recreate resources if needed
		if (bNeedToUpdateQuadtreeProperties)
		{
			
			GFSDK_WaveWorks_Quadtree_UpdateParams(hOceanQuadtree, parameters.OceanQuadtreeParameters);
			ReCreateResource();
			bNeedToUpdateQuadtreeProperties = false;
		}

		if (bNeedToUpdateLocalWavesSimulationProperties)
		{
			
			GFSDK_WaveWorks_Local_Waves_Simulation_UpdateProperties(hOceanLocalSimulation, parameters.OceanLocalSimulationSettings, parameters.OceanLocalSimulationParameters);
			ReCreateResource();
			bNeedToUpdateLocalWavesSimulationProperties = false;
		}

		if (bNeedToUpdateWindWavesSimulationProperties)
		{
			
			GFSDK_WaveWorks_Wind_Waves_Simulation_UpdateProperties(hOceanWindSimulation, parameters.OceanWindSimulationSettings, parameters.OceanWindSimulationParameters);
			ReCreateResource();
			bNeedToUpdateWindWavesSimulationProperties = false;

		}



		bool wire = ap::renderer::IsWireRender();
		if (wire)
		{
			device->BindPipelineState(&PSO_wire, cmd);
		}
		else
		{
			device->BindPipelineState(&PSO, cmd);
		}



		XMMATRIX V = camera.GetView();
		XMMATRIX P = XMMatrixPerspectiveFovLH(camera.fov, camera.width / camera.height, camera.zNearP, camera.zFarP);
		XMVECTOR E = camera.GetEye();

		XMMATRIX P2 = camera.GetProjection();

		// Getting view and projection matrices
		gfsdk_float4x4 viewMatrix = *((gfsdk_float4x4*)(&V));
		gfsdk_float4x4 projMatrix = *((gfsdk_float4x4*)(&P));
		gfsdk_float4x4 projMatrix2 = *((gfsdk_float4x4*)(&P2));
		gfsdk_float3 eyePoint = *((gfsdk_float3*)(&E));



		//Internal quadtree math is left handed, z up, so flippings rows 2 and 3 here
		gfsdk_float4x4  vm = { viewMatrix._11, viewMatrix._12, viewMatrix._13,viewMatrix._14,
					   viewMatrix._31, viewMatrix._32, viewMatrix._33,viewMatrix._34,
					   viewMatrix._21, viewMatrix._22, viewMatrix._23,viewMatrix._24,
					   viewMatrix._41, viewMatrix._42, viewMatrix._43,viewMatrix._44 };



		// Getting data for rendering from WaveWorks 
		GFSDK_WaveWorks_Wind_Waves_Rendering_Data windWavesRenderingData;
		GFSDK_WaveWorks_Wind_Waves_GetDataForRendering(hOceanWindSimulation, windWavesRenderingData);

		GFSDK_WaveWorks_Local_Waves_Rendering_Data localWavesRenderingData;
		GFSDK_WaveWorks_Local_Waves_GetDataForRendering(hOceanLocalSimulation, localWavesRenderingData);

		OCEAN_VS_HS_DS_CBUFFER VSHSDSCB;
		VSHSDSCB.g_matViewProj = projMatrix2 * vm;
		VSHSDSCB.g_eyePos = { eyePoint.x, eyePoint.z, eyePoint.y };
		VSHSDSCB.g_meanOceanLevel = parameters.OceanQuadtreeParameters.mean_sea_level;
		VSHSDSCB.g_dynamicTesselationAmount = parameters.fTessellationAmount;
		VSHSDSCB.g_staticTesselationOffset = parameters.fTessellationOffset;
		VSHSDSCB.g_cascade0UVOffset = windWavesRenderingData.cascade0_UV_offset;
		VSHSDSCB.g_cascade0UVScale = windWavesRenderingData.cascade0_UV_scale;
		VSHSDSCB.g_cascade1UVOffset = windWavesRenderingData.cascade1_UV_offset;
		VSHSDSCB.g_cascade1UVScale = windWavesRenderingData.cascade1_UV_scale;
		VSHSDSCB.g_cascade2UVOffset = windWavesRenderingData.cascade2_UV_offset;
		VSHSDSCB.g_cascade2UVScale = windWavesRenderingData.cascade2_UV_scale;
		VSHSDSCB.g_cascade3UVOffset = windWavesRenderingData.cascade3_UV_offset;
		VSHSDSCB.g_cascade3UVScale = windWavesRenderingData.cascade3_UV_scale;
		VSHSDSCB.g_localWavesSimulationDomainWorldspaceCenter = localWavesRenderingData.simulation_domain_worldspace_center;
		VSHSDSCB.g_localWavesSimulationDomainWorldspaceSize = localWavesRenderingData.simulation_domain_worldspace_size;
		VSHSDSCB.g_useDiamondPattern = parameters.OceanQuadtreeParameters.generate_diamond_pattern ? 1.0f : 0.0f;
		VSHSDSCB.g_UVWarpingAmplitude = windWavesRenderingData.uv_warping_amplitude;
		VSHSDSCB.g_UVWarpingFrequency = windWavesRenderingData.uv_warping_frequency;

		VSHSDSCB.displacementTextureArrayWindWaves = device->GetDescriptorIndex(&windWavesDisplacementsTextureArray, SubresourceType::SRV);
		VSHSDSCB.displacementTextureLocalWaves = device->GetDescriptorIndex(&localWavesDisplacementsTexture, SubresourceType::SRV);
		VSHSDSCB.gradientsTextureArrayWindWaves = device->GetDescriptorIndex(&windWavesGradientsTextureArray, SubresourceType::SRV);
		VSHSDSCB.momentsTextureArrayWindWaves = device->GetDescriptorIndex(&windWavesMomentsTextureArray, SubresourceType::SRV);
		VSHSDSCB.gradientsTextureLocalWaves = device->GetDescriptorIndex(&localWavesGradientsTexture, SubresourceType::SRV);
		
		VSHSDSCB.textureFoam = device->GetDescriptorIndex(&foamIntensityTexture, SubresourceType::SRV);
		VSHSDSCB.textureBubbles = device->GetDescriptorIndex(&foamBubblesTexture, SubresourceType::SRV);
		VSHSDSCB.textureWindGusts = device->GetDescriptorIndex(&windGustsTexture, SubresourceType::SRV);
		VSHSDSCB.skydomeTexture= device->GetDescriptorIndex(&skydome, SubresourceType::SRV);


		device->BindDynamicConstantBuffer(VSHSDSCB, 2, cmd);

		OCEAN_PS_CBUFFER PSCB;
		PSCB.g_cascadeToCascadeScale = windWavesRenderingData.cascade_to_cascade_scale;
		PSCB.g_windWavesTextureSizeInTexels = (float)windWavesRenderingData.size_of_texture_arrays;
		PSCB.g_UVWarpingAmplitude = windWavesRenderingData.uv_warping_amplitude;
		PSCB.g_UVWarpingFrequency = windWavesRenderingData.uv_warping_frequency;
		PSCB.g_windWavesFoamWhitecapsThreshold = windWavesRenderingData.foam_whitecaps_threshold;
		PSCB.g_SimulationDomainCenter = localWavesRenderingData.simulation_domain_worldspace_center;
		PSCB.g_SimulationDomainSize = localWavesRenderingData.simulation_domain_worldspace_size;
		PSCB.g_localWavesTextureSizeInTexels = (float)localWavesRenderingData.size_of_texture;
		PSCB.g_localWavesFoamWhitecapsThreshold = parameters.OceanLocalSimulationParameters.foam_whitecaps_threshold;
		PSCB.g_beckmannRoughness = 0.00001f;
		PSCB.g_sunIntensity = 1.1f;
		PSCB.g_sunDirection = { cosf(45.0 * 0.0174533f), 0, sinf(45.0 * 0.0174533f) };
		PSCB.g_useMicrofacetFresnel = 1.0f ;
		PSCB.g_useMicrofacetSpecular = 1.0f;
		PSCB.g_useMicrofacetReflection =  1.0f ;
		PSCB.g_showCascades = parameters.bShowCascades ? 1.0f : 0.0f;
		PSCB.g_eyePos = { eyePoint.x, eyePoint.z, eyePoint.y };

		PSCB.g_WaterColor = parameters.waterDeepColor;
		PSCB.g_WaterColorIntensity = parameters.waterColorIntensity;
		PSCB.g_FoamColor = parameters.foamColor;
		PSCB.g_FoamUnderwaterColor = parameters.foamUnderwaterColor;


		device->BindDynamicConstantBuffer(PSCB, 3, cmd);
		
		uint32_t numNodes;
		GFSDK_WaveWorks_Quadtree_NodeRenderingProperties* pNodes;
		std::vector<GFSDK_WaveWorks_Quadtree_NodeRenderingProperties> nodes;
		float cullMargin = GFSDK_WaveWorks_Wind_Waves_Simulation_GetConservativeMaxDisplacementEstimate(hOceanWindSimulation);

		GFSDK_WaveWorks_Quadtree_GetNodesForRendering(hOceanQuadtree, vm, projMatrix, viewportSize, cullMargin, &numNodes, &pNodes);
		nodes.assign(pNodes, pNodes + numNodes);

		// Getting the quadtree stats
		GFSDK_WaveWorks_Quadtree_GetStats(hOceanQuadtree,OceanQuadtreeStats);


		std::vector<uint32_t> startIndices(16);
		std::vector<uint32_t> indexCounts(16);

		// Per-instance data for patch types
		std::vector <std::vector< OCEAN_VS_CBUFFER_PERINSTANCE_ENTRY > > perInstanceBuffers(16);

		// Reserving space for the entire constant buffer size (64kb or 4096 entries) in perInstanceBuffers
		// as current NVRHI implementation updates the entire contents of the constant buffers.
		for (uint32_t i = 0; i < 16; i++)
		{
			perInstanceBuffers[i].reserve(4096);
		}

		// Assembling the arrays
		for (uint32_t i = 0; i < nodes.size(); i++)
		{
			uint32_t patchType = nodes[i].patch_type;

			startIndices[patchType] = nodes[i].start_index;
			indexCounts[patchType] = nodes[i].num_indices;

			OCEAN_VS_CBUFFER_PERINSTANCE_ENTRY cbEntry;
			cbEntry.g_patchWorldspaceOrigin = nodes[i].patch_worldspace_origin;
			cbEntry.g_patchWorldspaceScale = nodes[i].patch_worldspace_scale;
			cbEntry.g_patchMorphConstantAndSign = nodes[i].geomorphing_distance_constant * nodes[i].geomorphing_sign;
			perInstanceBuffers[patchType].push_back(cbEntry);
		}


		
		const GPUBuffer* vbs[] = {
			&oceanSurfaceVB,
		};
		const uint32_t strides[] = {
			sizeof(XMFLOAT2),
		};
		device->BindVertexBuffers(vbs, 0, arraysize(vbs), strides, nullptr, cmd);
		device->BindIndexBuffer(&oceanSurfaceIB, IndexBufferFormat::UINT32, 0, cmd);



		

		/*{
			GPUBarrier barriers[] = {
				GPUBarrier::Image(&windWavesDisplacementsTextureArray, windWavesDisplacementsTextureArray.desc.layout, ResourceState::SHADER_RESOURCE),
				GPUBarrier::Image(&windWavesGradientsTextureArray, windWavesGradientsTextureArray.desc.layout, ResourceState::SHADER_RESOURCE),
				GPUBarrier::Image(&windWavesMomentsTextureArray, windWavesMomentsTextureArray.desc.layout, ResourceState::SHADER_RESOURCE),
				GPUBarrier::Image(&localWavesDisplacementsTexture,localWavesDisplacementsTexture.desc.layout, ResourceState::SHADER_RESOURCE),
				GPUBarrier::Image(&localWavesGradientsTexture,localWavesGradientsTexture.desc.layout, ResourceState::SHADER_RESOURCE),

			};
			device->Barrier(barriers, arraysize(barriers), cmd);
		}*/

		// Rendering the quadtree patches using instancing
		for (uint32_t i = 0; i < 16; i++)
		{

			if (perInstanceBuffers[i].size() > 0)
			{
				// Writing the entire constant buffer
				device->BindDynamicConstantBuffer_Array((void*)perInstanceBuffers[i].data(), 4096 * sizeof(OCEAN_VS_CBUFFER_PERINSTANCE_ENTRY), 1, cmd);
				
				// Setting up draw arguments to render up to 4096 instances.
				// Ocean quadtree usually generates <100 instances of patches of each type, 
				// but if there is a need to render more than 4096 instances, then structured / vertex buffer can be used instead of constant buffer,
				// or multiple draw calls with the constant buffer updates between them.
				uint32_t intstanceCount = std::min(4096, (int32_t)(perInstanceBuffers[i].size()));
				device->DrawIndexedInstanced(indexCounts[i], intstanceCount, startIndices[i], 0, 0, cmd);
				
			}
		}

		

		
		
		/*if (bRenderMarkers)
		{
			RenderMarkers(FB);
		}*/


		device->EventEnd(cmd);
	}


	void Ocean2::ReCreateResource()
	{

		auto device = static_cast<ap::graphics::GraphicsDevice_DX12*>(ap::graphics::GetDevice());

		
		device->WaitForGPU();
	
	
		// Creating ocean surface vertex and index buffers
		uint32_t numVertices = 0;
		uint32_t numIndices = 0;
		gfsdk_float2* pVertexData = nullptr;
		uint32_t* pIndexData = nullptr;
		std::vector<gfsdk_float2> oceanVertices;
		std::vector<uint32_t> oceanIndices;

		GFSDK_WaveWorks_Quadtree_GetGeometryData(hOceanQuadtree, &numVertices, &pVertexData, &numIndices, &pIndexData);

		oceanVertices.assign(pVertexData, pVertexData + numVertices);
		oceanIndices.assign(pIndexData, pIndexData + numIndices);

		{
			GPUBufferDesc buf_desc;
			buf_desc.usage = Usage::DEFAULT;
			buf_desc.bind_flags = BindFlag::SHADER_RESOURCE;
			buf_desc.misc_flags = ResourceMiscFlag::BUFFER_RAW;
			buf_desc.stride = sizeof(gfsdk_float2);
			buf_desc.size = buf_desc.stride * numVertices;
			device->CreateBuffer(&buf_desc, oceanVertices.data(), &oceanSurfaceVB);

		}

		{
			GPUBufferDesc buf_desc;
			buf_desc.usage = Usage::DEFAULT;
			buf_desc.bind_flags = BindFlag::INDEX_BUFFER | BindFlag::SHADER_RESOURCE;
			buf_desc.format = Format::R32_UINT;
			buf_desc.stride = sizeof(uint32_t);
			buf_desc.size = buf_desc.stride * numIndices;
			device->CreateBuffer(&buf_desc, oceanIndices.data(), &oceanSurfaceIB);
		}

		//ap::graphics::GPUBuffer* a = static_cast<ap::graphics::GPUBuffer*>(oceanSurfaceIB.internal_state.get());

		// Creating NVRHI textures from native objects exposed from WaveWorks
		GFSDK_WaveWorks_Wind_Waves_Rendering_Data windWavesRenderingData;
		GFSDK_WaveWorks_Local_Waves_Rendering_Data localWavesRenderingData;
		GFSDK_WaveWorks_Wind_Waves_GetDataForRendering(hOceanWindSimulation, windWavesRenderingData);
		GFSDK_WaveWorks_Local_Waves_GetDataForRendering(hOceanLocalSimulation, localWavesRenderingData);



		{
			TextureDesc desc;
			desc.width = windWavesRenderingData.size_of_texture_arrays;
			desc.height = windWavesRenderingData.size_of_texture_arrays;
			desc.array_size = 4;
			desc.usage = Usage::DEFAULT;
			desc.bind_flags = BindFlag::SHADER_RESOURCE | BindFlag::UNORDERED_ACCESS;
			desc.format = Format::R16G16B16A16_FLOAT;
			desc.mip_levels = getNumMipLevels(windWavesRenderingData.size_of_texture_arrays);
			device->RecreateTextureFromNativeTexture(&desc, &windWavesDisplacementsTextureArray, windWavesRenderingData.displacements_texture_array);
			device->SetName(&windWavesDisplacementsTextureArray, "windWavesDisplacementsTextureArray");
			device->RecreateTextureFromNativeTexture(&desc, &windWavesGradientsTextureArray, windWavesRenderingData.gradients_texture_array);
			device->SetName(&windWavesGradientsTextureArray, "windWavesGradientsTextureArray");
			device->RecreateTextureFromNativeTexture(&desc, &windWavesMomentsTextureArray, windWavesRenderingData.moments_texture_array);
			device->SetName(&windWavesMomentsTextureArray, "windWavesMomentsTextureArray");

		}

		{
			TextureDesc desc;
			desc.width = localWavesRenderingData.size_of_texture;
			desc.height = localWavesRenderingData.size_of_texture;
			desc.array_size = 1;
			desc.usage = Usage::DEFAULT;
			desc.bind_flags = BindFlag::SHADER_RESOURCE | BindFlag::UNORDERED_ACCESS;
			desc.format = Format::R16G16B16A16_FLOAT;
			desc.mip_levels = getNumMipLevels(localWavesRenderingData.size_of_texture);
			device->RecreateTextureFromNativeTexture(&desc, &localWavesDisplacementsTexture, localWavesRenderingData.displacements_texture);
			device->SetName(&localWavesDisplacementsTexture, "localWavesDisplacementsTexture");
			device->RecreateTextureFromNativeTexture(&desc, &localWavesGradientsTexture, localWavesRenderingData.gradients_texture);
			device->SetName(&localWavesGradientsTexture, "localWavesGradientsTexture");

		}
		
		device->WaitForGPU();
	}
}