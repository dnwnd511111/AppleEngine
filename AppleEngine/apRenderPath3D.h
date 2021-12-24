#pragma once
#include "apRenderPath2D.h"
#include "apRenderer.h"
#include "apGraphicsDevice.h"
#include "apResourceManager.h"
#include "apScene.h"

#include <memory>

namespace ap
{

	class RenderPath3D :
		public RenderPath2D
	{
	public:
		enum AO
		{
			AO_DISABLED,	// no ambient occlusion
			AO_SSAO,		// simple brute force screen space ambient occlusion
			AO_HBAO,		// horizon based screen space ambient occlusion
			AO_MSAO,		// multi scale screen space ambient occlusion
			AO_RTAO,		// ray traced ambient occlusion
			// Don't alter order! (bound to lua manually)
		};
	private:
		float exposure = 1.0f;
		float bloomThreshold = 1.0f;
		float motionBlurStrength = 100.0f;
		float dofStrength = 10.0f;
		float sharpenFilterAmount = 0.28f;
		float outlineThreshold = 0.2f;
		float outlineThickness = 1.0f;
		XMFLOAT4 outlineColor = XMFLOAT4(0, 0, 0, 1);
		float aoRange = 1.0f;
		uint32_t aoSampleCount = 16;
		float aoPower = 1.0f;
		float chromaticAberrationAmount = 2.0f;
		uint32_t screenSpaceShadowSampleCount = 16;
		float screenSpaceShadowRange = 1;
		float eyeadaptionKey = 0.115f;
		float eyeadaptionRate = 1;
		float fsrSharpness = 1.0f;

		AO ao = AO_DISABLED;
		bool fxaaEnabled = false;
		bool ssrEnabled = false;
		bool raytracedReflectionsEnabled = false;
		bool reflectionsEnabled = true;
		bool shadowsEnabled = true;
		bool bloomEnabled = true;
		bool colorGradingEnabled = true;
		bool volumeLightsEnabled = true;
		bool lightShaftsEnabled = false;
		bool lensFlareEnabled = true;
		bool motionBlurEnabled = false;
		bool depthOfFieldEnabled = true;
		bool eyeAdaptionEnabled = false;
		bool sharpenFilterEnabled = false;
		bool outlineEnabled = false;
		bool chromaticAberrationEnabled = false;
		bool ditherEnabled = true;
		bool occlusionCullingEnabled = true;
		bool sceneUpdateEnabled = true;
		bool fsrEnabled = true;

		uint32_t msaaSampleCount = 1;

	public:
		ap::graphics::Texture rtMain;
		ap::graphics::Texture rtMain_render; // can be MSAA
		ap::graphics::Texture rtGbuffer[ap::renderer::GBUFFER_COUNT];
		ap::graphics::Texture rtPrimitiveID_render; // can be MSAA
		ap::graphics::Texture rtReflection; // contains the scene rendered for planar reflections
		ap::graphics::Texture rtSSR; // standard screen-space reflection results
		ap::graphics::Texture rtSceneCopy; // contains the rendered scene that can be fed into transparent pass for distortion effect
		ap::graphics::Texture rtSceneCopy_tmp; // temporary for gaussian mipchain
		ap::graphics::Texture rtWaterRipple; // water ripple sprite normal maps are rendered into this
		ap::graphics::Texture rtParticleDistortion; // contains distortive particles
		ap::graphics::Texture rtParticleDistortion_Resolved; // contains distortive particles
		ap::graphics::Texture rtVolumetricLights[2]; // contains the volumetric light results
		ap::graphics::Texture rtTemporalAA[2]; // temporal AA history buffer
		ap::graphics::Texture rtBloom; // contains the bright parts of the image + mipchain
		ap::graphics::Texture rtBloom_tmp; // temporary for bloom downsampling
		ap::graphics::Texture rtAO; // full res AO
		ap::graphics::Texture rtShadow; // raytraced shadows mask
		ap::graphics::Texture rtSun[2]; // 0: sun render target used for lightshafts (can be MSAA), 1: radial blurred lightshafts
		ap::graphics::Texture rtSun_resolved; // sun render target, but the resolved version if MSAA is enabled
		ap::graphics::Texture rtGUIBlurredBackground[3];	// downsampled, gaussian blurred scene for GUI
		ap::graphics::Texture rtShadingRate; // UINT8 shading rate per tile
		ap::graphics::Texture rtFSR[2]; // FSR upscaling result (full resolution LDR)

		ap::graphics::Texture rtPostprocess; // ping-pong with main scene RT in post-process chain

		ap::graphics::Texture depthBuffer_Main; // used for depth-testing, can be MSAA
		ap::graphics::Texture depthBuffer_Copy; // used for shader resource, single sample
		ap::graphics::Texture depthBuffer_Copy1; // used for disocclusion check
		ap::graphics::Texture depthBuffer_Reflection; // used for reflection, single sample
		ap::graphics::Texture rtLinearDepth; // linear depth result + mipchain (max filter)

		ap::graphics::RenderPass renderpass_depthprepass;
		ap::graphics::RenderPass renderpass_main;
		ap::graphics::RenderPass renderpass_transparent;
		ap::graphics::RenderPass renderpass_reflection_depthprepass;
		ap::graphics::RenderPass renderpass_reflection;
		ap::graphics::RenderPass renderpass_downsamplescene;
		ap::graphics::RenderPass renderpass_lightshafts;
		ap::graphics::RenderPass renderpass_volumetriclight;
		ap::graphics::RenderPass renderpass_particledistortion;
		ap::graphics::RenderPass renderpass_waterripples;

		ap::graphics::Texture debugUAV; // debug UAV can be used by some shaders...
		ap::renderer::TiledLightResources tiledLightResources;
		ap::renderer::TiledLightResources tiledLightResources_planarReflection;
		ap::renderer::LuminanceResources luminanceResources;
		ap::renderer::SSAOResources ssaoResources;
		ap::renderer::MSAOResources msaoResources;
		ap::renderer::RTAOResources rtaoResources;
		ap::renderer::RTReflectionResources rtreflectionResources;
		ap::renderer::SSRResources ssrResources;
		ap::renderer::RTShadowResources rtshadowResources;
		ap::renderer::ScreenSpaceShadowResources screenspaceshadowResources;
		ap::renderer::DepthOfFieldResources depthoffieldResources;
		ap::renderer::MotionBlurResources motionblurResources;
		ap::renderer::VolumetricCloudResources volumetriccloudResources;
		ap::renderer::VolumetricCloudResources volumetriccloudResources_reflection;
		ap::renderer::BloomResources bloomResources;
		ap::renderer::SurfelGIResources surfelGIResources;

		mutable const ap::graphics::Texture* lastPostprocessRT = &rtPostprocess;
		// Post-processes are ping-ponged, this function helps to obtain the last postprocess render target that was written
		const ap::graphics::Texture* GetLastPostprocessRT() const
		{
			return lastPostprocessRT;
		}

		virtual void RenderAO(ap::graphics::CommandList cmd) const;
		virtual void RenderSSR(ap::graphics::CommandList cmd) const;
		virtual void RenderOutline(ap::graphics::CommandList cmd) const;
		virtual void RenderLightShafts(ap::graphics::CommandList cmd) const;
		virtual void RenderVolumetrics(ap::graphics::CommandList cmd) const;
		virtual void RenderSceneMIPChain(ap::graphics::CommandList cmd) const;
		virtual void RenderTransparents(ap::graphics::CommandList cmd) const;
		virtual void RenderPostprocessChain(ap::graphics::CommandList cmd) const;

		void ResizeBuffers() override;

		ap::scene::CameraComponent* camera = &ap::scene::GetCamera();
		ap::scene::CameraComponent camera_previous;
		ap::scene::CameraComponent camera_reflection;
		ap::scene::CameraComponent camera_reflection_previous;

		ap::scene::Scene* scene = &ap::scene::GetScene();
		ap::renderer::Visibility visibility_main;
		ap::renderer::Visibility visibility_reflection;

		FrameCB frameCB = {};

		uint8_t instanceInclusionMask_RTAO = 0xFF;
		uint8_t instanceInclusionMask_RTShadow = 0xFF;
		uint8_t instanceInclusionMask_RTReflection = 0xFF;
		uint8_t instanceInclusionMask_SurfelGI = 0xFF;
		uint8_t instanceInclusionMask_Lightmap = 0xFF;

		const ap::graphics::Texture* GetDepthStencil() const override { return &depthBuffer_Main; }
		const ap::graphics::Texture* GetGUIBlurredBackground() const override { return &rtGUIBlurredBackground[2]; }

		constexpr float getExposure() const { return exposure; }
		constexpr float getBloomThreshold() const { return bloomThreshold; }
		constexpr float getMotionBlurStrength() const { return motionBlurStrength; }
		constexpr float getDepthOfFieldStrength() const { return dofStrength; }
		constexpr float getSharpenFilterAmount() const { return sharpenFilterAmount; }
		constexpr float getOutlineThreshold() const { return outlineThreshold; }
		constexpr float getOutlineThickness() const { return outlineThickness; }
		constexpr XMFLOAT4 getOutlineColor() const { return outlineColor; }
		constexpr float getAORange() const { return aoRange; }
		constexpr uint32_t getAOSampleCount() const { return aoSampleCount; }
		constexpr float getAOPower() const { return aoPower; }
		constexpr float getChromaticAberrationAmount() const { return chromaticAberrationAmount; }
		constexpr uint32_t getScreenSpaceShadowSampleCount() const { return screenSpaceShadowSampleCount; }
		constexpr float getScreenSpaceShadowRange() const { return screenSpaceShadowRange; }
		constexpr float getEyeAdaptionKey() const { return eyeadaptionKey; }
		constexpr float getEyeAdaptionRate() const { return eyeadaptionRate; }
		constexpr float getFSRSharpness() const { return fsrSharpness; }

		constexpr bool getAOEnabled() const { return ao != AO_DISABLED; }
		constexpr AO getAO() const { return ao; }
		constexpr bool getSSREnabled() const { return ssrEnabled; }
		constexpr bool getRaytracedReflectionEnabled() const { return raytracedReflectionsEnabled; }
		constexpr bool getShadowsEnabled() const { return shadowsEnabled; }
		constexpr bool getReflectionsEnabled() const { return reflectionsEnabled; }
		constexpr bool getFXAAEnabled() const { return fxaaEnabled; }
		constexpr bool getBloomEnabled() const { return bloomEnabled; }
		constexpr bool getColorGradingEnabled() const { return colorGradingEnabled; }
		constexpr bool getVolumeLightsEnabled() const { return volumeLightsEnabled; }
		constexpr bool getLightShaftsEnabled() const { return lightShaftsEnabled; }
		constexpr bool getLensFlareEnabled() const { return lensFlareEnabled; }
		constexpr bool getMotionBlurEnabled() const { return motionBlurEnabled; }
		constexpr bool getDepthOfFieldEnabled() const { return depthOfFieldEnabled; }
		constexpr bool getEyeAdaptionEnabled() const { return eyeAdaptionEnabled; }
		constexpr bool getSharpenFilterEnabled() const { return sharpenFilterEnabled && getSharpenFilterAmount() > 0; }
		constexpr bool getOutlineEnabled() const { return outlineEnabled; }
		constexpr bool getChromaticAberrationEnabled() const { return chromaticAberrationEnabled; }
		constexpr bool getDitherEnabled() const { return ditherEnabled; }
		constexpr bool getOcclusionCullingEnabled() const { return occlusionCullingEnabled; }
		constexpr bool getSceneUpdateEnabled() const { return sceneUpdateEnabled; }
		constexpr bool getFSREnabled() const { return fsrEnabled; }

		constexpr uint32_t getMSAASampleCount() const { return msaaSampleCount; }

		constexpr void setExposure(float value) { exposure = value; }
		constexpr void setBloomThreshold(float value) { bloomThreshold = value; }
		constexpr void setMotionBlurStrength(float value) { motionBlurStrength = value; }
		constexpr void setDepthOfFieldStrength(float value) { dofStrength = value; }
		constexpr void setSharpenFilterAmount(float value) { sharpenFilterAmount = value; }
		constexpr void setOutlineThreshold(float value) { outlineThreshold = value; }
		constexpr void setOutlineThickness(float value) { outlineThickness = value; }
		constexpr void setOutlineColor(const XMFLOAT4& value) { outlineColor = value; }
		constexpr void setAORange(float value) { aoRange = value; }
		constexpr void setAOSampleCount(uint32_t value) { aoSampleCount = value; }
		constexpr void setAOPower(float value) { aoPower = value; }
		constexpr void setChromaticAberrationAmount(float value) { chromaticAberrationAmount = value; }
		constexpr void setScreenSpaceShadowSampleCount(uint32_t value) { screenSpaceShadowSampleCount = value; }
		constexpr void setScreenSpaceShadowRange(float value) { screenSpaceShadowRange = value; }
		constexpr void setEyeAdaptionKey(float value) { eyeadaptionKey = value; }
		constexpr void setEyeAdaptionRate(float value) { eyeadaptionRate = value; }
		constexpr void setFSRSharpness(float value) { fsrSharpness = value; }

		void setAO(AO value);
		void setSSREnabled(bool value);
		void setRaytracedReflectionsEnabled(bool value);
		constexpr void setShadowsEnabled(bool value) { shadowsEnabled = value; }
		constexpr void setReflectionsEnabled(bool value) { reflectionsEnabled = value; }
		constexpr void setFXAAEnabled(bool value) { fxaaEnabled = value; }
		constexpr void setBloomEnabled(bool value) { bloomEnabled = value; }
		constexpr void setColorGradingEnabled(bool value) { colorGradingEnabled = value; }
		constexpr void setVolumeLightsEnabled(bool value) { volumeLightsEnabled = value; }
		constexpr void setLightShaftsEnabled(bool value) { lightShaftsEnabled = value; }
		constexpr void setLensFlareEnabled(bool value) { lensFlareEnabled = value; }
		constexpr void setMotionBlurEnabled(bool value) { motionBlurEnabled = value; }
		constexpr void setDepthOfFieldEnabled(bool value) { depthOfFieldEnabled = value; }
		constexpr void setEyeAdaptionEnabled(bool value) { eyeAdaptionEnabled = value; }
		constexpr void setSharpenFilterEnabled(bool value) { sharpenFilterEnabled = value; }
		constexpr void setOutlineEnabled(bool value) { outlineEnabled = value; }
		constexpr void setChromaticAberrationEnabled(bool value) { chromaticAberrationEnabled = value; }
		constexpr void setDitherEnabled(bool value) { ditherEnabled = value; }
		constexpr void setOcclusionCullingEnabled(bool value) { occlusionCullingEnabled = value; }
		constexpr void setSceneUpdateEnabled(bool value) { sceneUpdateEnabled = value; }
		void setFSREnabled(bool value);

		virtual void setMSAASampleCount(uint32_t value) { msaaSampleCount = value; }

		void PreUpdate() override;
		void Update(float dt) override;
		void Render() const override;
		void Compose(ap::graphics::CommandList cmd) const override;
	};

}
