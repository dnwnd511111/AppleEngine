#pragma once
#include "apRenderPath3D.h"
#include "apVector.h"

namespace ap
{

	class RenderPath3D_PathTracing :
		public RenderPath3D
	{
	protected:
		int sam = -1;
		int target = 1024;
		ap::graphics::Texture traceResult;

		ap::vector<uint8_t> texturedata_src;
		ap::vector<uint8_t> texturedata_dst;
		ap::vector<uint8_t> texturedata_albedo;
		ap::vector<uint8_t> texturedata_normal;
		ap::graphics::Texture denoiserAlbedo;
		ap::graphics::Texture denoiserNormal;
		ap::graphics::Texture denoiserResult;
		ap::jobsystem::context denoiserContext;

		ap::graphics::RenderPass renderpass_debugbvh;

		void ResizeBuffers() override;

	public:
		const ap::graphics::Texture* GetDepthStencil() const override { return nullptr; };

		void Update(float dt) override;
		void Render() const override;
		void Compose(ap::graphics::CommandList cmd) const override;

		int getCurrentSampleCount() const { return sam; }
		void setTargetSampleCount(int value) { target = value; }
		float getProgress() const { return (float)sam / (float)target; }

		float denoiserProgress = 0;
		float getDenoiserProgress() const { return denoiserProgress; }
		bool isDenoiserAvailable() const;

		void resetProgress() { sam = -1; denoiserProgress = 0; }

		uint8_t instanceInclusionMask_PathTrace = 0xFF;
	};

}
