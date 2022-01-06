#pragma once

#include <GFSDK_WaveWorks.h>
#include "apGraphicsDevice.h"

namespace ap
{ 

	class Ocean2
	{
		enum SynchronizationMode
		{
			SynchronizationMode_None = 0,
			SynchronizationMode_RenderOnly,
			SynchronizationMode_Readback,
			Num_SynchronizationModes
		};

		enum { NumMarkersXY = 1000, NumMarkers = NumMarkersXY * NumMarkersXY };

	public:
		bool isInitialized = false;

		// Simulation state
		GFSDK_WaveWorks_Wind_Waves_SimulationHandle			hOceanWindSimulation = nullptr;
		GFSDK_WaveWorks_Wind_Waves_Simulation_Parameters	OceanWindSimulationParameters;
		GFSDK_WaveWorks_Wind_Waves_Simulation_Settings		OceanWindSimulationSettings;
		GFSDK_WaveWorks_Wind_Waves_Simulation_Stats			OceanWindSimulationStats;
		GFSDK_WaveWorks_Wind_Waves_Simulation_Stats			OceanWindSimulationStatsFiltered;

		GFSDK_WaveWorks_Local_Waves_SimulationHandle		hOceanLocalSimulation = nullptr;
		GFSDK_WaveWorks_Local_Waves_Simulation_Parameters	OceanLocalSimulationParameters;
		GFSDK_WaveWorks_Local_Waves_Simulation_Settings		OceanLocalSimulationSettings;
		GFSDK_WaveWorks_Local_Waves_Simulation_Stats		OceanLocalSimulationStats;
		GFSDK_WaveWorks_Local_Waves_Simulation_Stats		OceanLocalSimulationStatsFiltered;

		GFSDK_WaveWorks_QuadtreeHandle                      hOceanQuadtree = nullptr;
		GFSDK_WaveWorks_Quadtree_Params                     OceanQuadtreeParameters;
		GFSDK_WaveWorks_Quadtree_Stats                      OceanQuadtreeStats;

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

		enum { ReadbackArchiveSize = 30 };
		enum { ReadbackArchiveInterval = 10 };
		float fReadbackCoord = 0.f;

		SynchronizationMode SyncMode = SynchronizationMode_None;

		bool bForceKick = false;
		float fMinDisplacement = 0;
		float fMaxDisplacement = 0;

		float fMinTotalDisplacement = 0;
		float fMaxTotalDisplacement = 0;
		float fSWH = 0;

		// Animation & rendering state
		float fCameraClipNear = 0.1f;
		float fCameraClipFar = 100000.0f;
		double dSimulationTime = 0.0;
		bool bNeedToUpdateWindWavesSimulationProperties = false;
		bool bNeedToUpdateLocalWavesSimulationProperties = false;
		bool bNeedToResetLocalWavesSimulation = false;
		bool bRain = false;
		bool bBoat = true;
		float fRainDropSize = 0.05f;
		bool bNeedToUpdateSky = true;
		float fBaseWindDirection = 45.0f;
		float fSwellWindDirection = -18.0f;
		float fTessellationAmount = 1.0f;
		float fTessellationOffset = 0.0f;

		float fBoatX = 0;
		float fBoatY = 0;


		// Rendering variables
		bool bRenderWireframe = false;
		bool bRenderWater = true;
		bool bRenderMarkers = false;
		//bool bRenderSky = true;
		//bool bUseDynamicSky = true;
		bool bSimulateWater = true;
		bool bShowWindWavesSimulationStatistics = false;
		bool bShowLocalWavesSimulationStatistics = false;
		float fSunAngle = 20.0f;
		float fBeckmannRoughness = 0.00001f;
		float fSunIntensity = 1.0f;
		bool bUseMicrofacetFresnel = true;
		bool bUseMicrofacetSpecular = true;
		bool bUseMicrofacetReflection = true;
		int iRefinementSteps = 1;
		int iReadbackUsage = 0;

		bool bShowCascades = false;
		bool bNeedToUpdateQuadtreeProperties = false;

	private:

		ap::graphics::GPUBuffer oceanSurfaceVB;
		ap::graphics::GPUBuffer oceanSurfaceIB;

		ap::graphics::Texture foamIntensityTexture;
		ap::graphics::Texture foamBubblesTexture;
		ap::graphics::Texture windGustsTexture;

		ap::graphics::Texture windWavesDisplacementsTextureArray;
		ap::graphics::Texture windWavesGradientsTextureArray;
		ap::graphics::Texture windWavesMomentsTextureArray;

		ap::graphics::Texture localWavesDisplacementsTexture;
		ap::graphics::Texture localWavesGradientsTexture;

	public:
		void Init();
		void ResourceUpdate();
		void Render(ap::graphics::CommandList cmd);
		void CreateResource();

	};
}