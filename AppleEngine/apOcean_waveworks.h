#pragma once

#include <GFSDK_WaveWorks.h>
#include "apGraphicsDevice.h"
#include "DirectXMath.h"
#include "Utility\dx12\d3d12.h"
#include <wrl/client.h> // ComPtr


namespace ap
{ 

	class Ocean2
	{
	public:
		Ocean2();

		enum SynchronizationMode
		{
			SynchronizationMode_None = 0,
			SynchronizationMode_RenderOnly,
			SynchronizationMode_Readback,
			Num_SynchronizationModes
		};

		enum { NumMarkersXY = 1000, NumMarkers = NumMarkersXY * NumMarkersXY };

		

		struct Ocean2Parameters
		{
			// Simulation state
			
			GFSDK_WaveWorks_Wind_Waves_Simulation_Parameters	OceanWindSimulationParameters;
			GFSDK_WaveWorks_Wind_Waves_Simulation_Settings		OceanWindSimulationSettings;
			
			GFSDK_WaveWorks_Local_Waves_Simulation_Parameters	OceanLocalSimulationParameters;
			GFSDK_WaveWorks_Local_Waves_Simulation_Settings		OceanLocalSimulationSettings;
	
			GFSDK_WaveWorks_Quadtree_Params                     OceanQuadtreeParameters;
			

			enum { ReadbackArchiveSize = 30 };
			enum { ReadbackArchiveInterval = 10 };
			float fReadbackCoord = 0.f;


			bool bRain = false;
			bool bBoat = false;
			float fRainDropSize = 0.05f;
			float fBaseWindDirection = 45.0f;
			float fSwellWindDirection = -18.0f;
			float fTessellationAmount = 1.0f;
			float fTessellationOffset = 0.0f;

		
			// Rendering variables
			bool bRenderMarkers = false;
			bool bSimulateWater = true;
			float fBeckmannRoughness = 0.00001f;
			
			int iRefinementSteps = 1;
			int iReadbackUsage = 0;

			bool bShowCascades = false;
			
			float fMinTotalDisplacement = 0;
			float fMaxTotalDisplacement = 0;
			float fSWH = 0;


		};

	public:

		float viewportWidth;
		float viewportHeight;

		GFSDK_WaveWorks_Wind_Waves_SimulationHandle			hOceanWindSimulation = nullptr;
		GFSDK_WaveWorks_Wind_Waves_Simulation_Stats			OceanWindSimulationStats;
		GFSDK_WaveWorks_Wind_Waves_Simulation_Stats			OceanWindSimulationStatsFiltered;

		GFSDK_WaveWorks_Local_Waves_SimulationHandle		hOceanLocalSimulation = nullptr;
		GFSDK_WaveWorks_Local_Waves_Simulation_Stats		OceanLocalSimulationStats;
		GFSDK_WaveWorks_Local_Waves_Simulation_Stats		OceanLocalSimulationStatsFiltered;

		GFSDK_WaveWorks_QuadtreeHandle                      hOceanQuadtree = nullptr;
		GFSDK_WaveWorks_Quadtree_Stats                      OceanQuadtreeStats;

		Ocean2Parameters parameters;

		uint64_t iLastWindSimulationKickID = 0;
		uint64_t iLastWindSimulationArchivedKickID = GFSDK_WaveWorks_InvalidKickID;
		uint64_t iLastWindSimulationReadbackKickID = GFSDK_WaveWorks_InvalidKickID;
		uint32_t iWindSimulationRenderLatency = 0;
		int32_t iWindSimulationReadbackLatency = 0;

		uint64_t iLastLocalSimulationKickID = 0;
		uint64_t iLastLocalSimulationArchivedKickID = GFSDK_WaveWorks_InvalidKickID;
		uint64_t iLastLocalSimulationReadbackKickID = GFSDK_WaveWorks_InvalidKickID;
		uint32_t iLocalSimulationRenderLatency = 0;
		int32_t iLocalSimulationReadbackLatency = 0;

		SynchronizationMode SyncMode = SynchronizationMode_None;

		// Animation & rendering state
		bool bNeedToUpdateWindWavesSimulationProperties = false;
		bool bNeedToUpdateLocalWavesSimulationProperties = false;
		bool bNeedToResetLocalWavesSimulation = false;
		bool bNeedToUpdateQuadtreeProperties = false;


		double dSimulationTime = 0.0;

		bool bForceKick = false;
		float fMinDisplacement = 0;
		float fMaxDisplacement = 0;

		

		float fBoatX = 0;
		float fBoatY = 0;


	private:

		Microsoft::WRL::ComPtr<ID3D12Fence> fence;

		ap::graphics::GPUBuffer oceanSurfaceVB;
		ap::graphics::GPUBuffer oceanSurfaceIB;

		

		ap::graphics::Texture windWavesDisplacementsTextureArray;
		ap::graphics::Texture windWavesGradientsTextureArray;
		ap::graphics::Texture windWavesMomentsTextureArray;

		ap::graphics::Texture localWavesDisplacementsTexture;
		ap::graphics::Texture localWavesGradientsTexture;

	public:
		static void Initialize();
		void Create();
		void ResourceUpdate();
		void Render( ap::graphics::CommandList cmd);
		void ReCreateResource();

	};
}