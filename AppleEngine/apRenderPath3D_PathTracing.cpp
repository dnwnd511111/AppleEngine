#include "apRenderPath3D_PathTracing.h"
#include "apRenderer.h"
#include "apImage.h"
#include "apHelper.h"
#include "apTextureHelper.h"
#include "apSprite.h"
#include "apProfiler.h"
#include "apScene.h"
#include "apBacklog.h"


using namespace ap::graphics;
using namespace ap::scene;


namespace ap
{

#if __has_include("OpenImageDenoise/oidn.hpp")
#define OPEN_IMAGE_DENOISE
#include "OpenImageDenoise/oidn.hpp"
#pragma comment(lib,"OpenImageDenoise.lib")
#pragma comment(lib,"tbb.lib")
	// Also provide OpenImageDenoise.dll and tbb.dll near the exe!
	bool DenoiserCallback(void* userPtr, double n)
	{
		auto renderpath = (RenderPath3D_PathTracing*)userPtr;
		if (renderpath->getProgress() < 1)
		{
			renderpath->denoiserProgress = 0;
			return false;
		}
		renderpath->denoiserProgress = (float)n;
		return true;
	}
	bool RenderPath3D_PathTracing::isDenoiserAvailable() const { return true; }
#else
	bool RenderPath3D_PathTracing::isDenoiserAvailable() const { return false; }
#endif

	void RenderPath3D_PathTracing::ResizeBuffers()
	{
		RenderPath2D::ResizeBuffers(); // we don't need to use any buffers from RenderPath3D, so skip those

		GraphicsDevice* device = ap::graphics::GetDevice();

		XMUINT2 internalResolution = GetInternalResolution();

		{
			TextureDesc desc;
			desc.bind_flags = BindFlag::UNORDERED_ACCESS | BindFlag::SHADER_RESOURCE | BindFlag::RENDER_TARGET;
			desc.format = Format::R32G32B32A32_FLOAT;
			desc.width = internalResolution.x;
			desc.height = internalResolution.y;
			device->CreateTexture(&desc, nullptr, &traceResult);
			device->SetName(&traceResult, "traceResult");

#ifdef OPEN_IMAGE_DENOISE
			desc.bind_flags = BindFlag::UNORDERED_ACCESS;
			desc.layout = ResourceState::UNORDERED_ACCESS;
			device->CreateTexture(&desc, nullptr, &denoiserAlbedo);
			device->SetName(&denoiserAlbedo, "denoiserAlbedo");
			device->CreateTexture(&desc, nullptr, &denoiserNormal);
			device->SetName(&denoiserNormal, "denoiserNormal");
#endif // OPEN_IMAGE_DENOISE
		}
		{
			TextureDesc desc;
			desc.bind_flags = BindFlag::SHADER_RESOURCE | BindFlag::UNORDERED_ACCESS;
			desc.format = Format::R11G11B10_FLOAT;
			desc.width = internalResolution.x;
			desc.height = internalResolution.y;
			device->CreateTexture(&desc, nullptr, &rtPostprocess);
			device->SetName(&rtPostprocess, "rtPostprocess");


			desc.width /= 4;
			desc.height /= 4;
			desc.bind_flags = BindFlag::UNORDERED_ACCESS | BindFlag::SHADER_RESOURCE;
			device->CreateTexture(&desc, nullptr, &rtGUIBlurredBackground[0]);
			device->SetName(&rtGUIBlurredBackground[0], "rtGUIBlurredBackground[0]");

			desc.width /= 4;
			desc.height /= 4;
			device->CreateTexture(&desc, nullptr, &rtGUIBlurredBackground[1]);
			device->SetName(&rtGUIBlurredBackground[1], "rtGUIBlurredBackground[1]");
			device->CreateTexture(&desc, nullptr, &rtGUIBlurredBackground[2]);
			device->SetName(&rtGUIBlurredBackground[2], "rtGUIBlurredBackground[2]");
		}

		{
			RenderPassDesc desc;
			desc.attachments.push_back(RenderPassAttachment::RenderTarget(&traceResult, RenderPassAttachment::LoadOp::CLEAR));

			device->CreateRenderPass(&desc, &renderpass_debugbvh);
		}

		ap::renderer::CreateLuminanceResources(luminanceResources, internalResolution);
		ap::renderer::CreateBloomResources(bloomResources, internalResolution);

		// also reset accumulation buffer state:
		sam = -1;
	}

	void RenderPath3D_PathTracing::Update(float dt)
	{
		setOcclusionCullingEnabled(false);

		if (camera->IsDirty())
		{
			camera->SetDirty(false);
			sam = -1;
		}
		else
		{
			for (size_t i = 0; i < scene->transforms.GetCount(); ++i)
			{
				const TransformComponent& transform = scene->transforms[i];

				if (transform.IsDirty())
				{
					sam = -1;
					break;
				}
			}

			if (sam >= 0)
			{
				for (size_t i = 0; i < scene->materials.GetCount(); ++i)
				{
					const MaterialComponent& material = scene->materials[i];

					if (material.IsDirty())
					{
						sam = -1;
						break;
					}
				}
			}
		}
		sam++;

		if (sam > target)
		{
			sam = target;
		}
		if (target < sam)
		{
			resetProgress();
		}

		scene->SetAccelerationStructureUpdateRequested(sam == 0);
		setSceneUpdateEnabled(sam == 0);

		RenderPath3D::Update(dt);


#ifdef OPEN_IMAGE_DENOISE
		if (sam == target)
		{
			if (!denoiserResult.IsValid() && !ap::jobsystem::IsBusy(denoiserContext))
			{
				
				texturedata_src.clear();
				texturedata_dst.clear();
				texturedata_albedo.clear();
				texturedata_normal.clear();

				if (ap::helper::saveTextureToMemory(traceResult, texturedata_src))
				{
					ap::helper::saveTextureToMemory(denoiserAlbedo, texturedata_albedo);
					ap::helper::saveTextureToMemory(denoiserNormal, texturedata_normal);

					texturedata_dst.resize(texturedata_src.size());

					ap::jobsystem::Execute(denoiserContext, [&](ap::jobsystem::JobArgs args) {

						size_t width = (size_t)traceResult.desc.width;
						size_t height = (size_t)traceResult.desc.height;
						{
							// https://github.com/OpenImageDenoise/oidn#c11-api-example

							// Create an Intel Open Image Denoise device
							static oidn::DeviceRef device = oidn::newDevice();
							static bool init = false;
							if (!init)
							{
								device.commit();
								init = true;
							}

							// Create a denoising filter
							oidn::FilterRef filter = device.newFilter("RT"); // generic ray tracing filter
							filter.setImage("color", texturedata_src.data(), oidn::Format::Float3, width, height, 0, sizeof(XMFLOAT4));
							if (!texturedata_albedo.empty())
							{
								filter.setImage("albedo", texturedata_albedo.data(), oidn::Format::Float3, width, height, 0, sizeof(XMFLOAT4)); // optional
							}
							if (!texturedata_normal.empty())
							{
								filter.setImage("normal", texturedata_normal.data(), oidn::Format::Float3, width, height, 0, sizeof(XMFLOAT4)); // optional
							}
							filter.setImage("output", texturedata_dst.data(), oidn::Format::Float3, width, height, 0, sizeof(XMFLOAT4));
							filter.set("hdr", true); // image is HDR
							//filter.set("cleanAux", true);
							filter.commit();

							denoiserProgress = 0;
							filter.setProgressMonitorFunction(DenoiserCallback, this);

							// Filter the image
							filter.execute();

							// Check for errors
							const char* errorMessage;
							auto error = device.getError(errorMessage);
							if (error != oidn::Error::None && error != oidn::Error::Cancelled)
							{
								ap::backlog::post(std::string("[OpenImageDenoise error] ") + errorMessage);
							}
						}

						GraphicsDevice* device = ap::graphics::GetDevice();

						TextureDesc desc;
						desc.width = (uint32_t)width;
						desc.height = (uint32_t)height;
						desc.bind_flags = BindFlag::SHADER_RESOURCE;
						desc.format = Format::R32G32B32A32_FLOAT;

						SubresourceData initdata;
						initdata.data_ptr = texturedata_dst.data();
						initdata.row_pitch = uint32_t(sizeof(XMFLOAT4) * width);
						device->CreateTexture(&desc, &initdata, &denoiserResult);

						});
				}
			}
		}
		else
		{
			denoiserResult = Texture();
			denoiserProgress = 0;
		}
#endif // OPEN_IMAGE_DENOISE
	}

	void RenderPath3D_PathTracing::Render() const
	{
		GraphicsDevice* device = ap::graphics::GetDevice();
		ap::jobsystem::context ctx;

		if (sam < target)
		{
			// Setup:
			CommandList cmd = device->BeginCommandList();
			ap::jobsystem::Execute(ctx, [this, cmd](ap::jobsystem::JobArgs args) {

				ap::renderer::BindCameraCB(
					*camera,
					camera_previous,
					camera_reflection,
					cmd
				);
				ap::renderer::UpdateRenderData(visibility_main, frameCB, cmd);
				ap::renderer::UpdateRenderDataAsync(visibility_main, frameCB, cmd);

				if (scene->IsAccelerationStructureUpdateRequested())
				{
					ap::renderer::UpdateRaytracingAccelerationStructures(*scene, cmd);
				}
				});

			// Main scene:
			cmd = device->BeginCommandList();
			ap::jobsystem::Execute(ctx, [this, cmd](ap::jobsystem::JobArgs args) {

				GraphicsDevice* device = ap::graphics::GetDevice();

				ap::renderer::BindCameraCB(
					*camera,
					*camera,
					*camera,
					cmd
				);
				ap::renderer::BindCommonResources(cmd);

				if (ap::renderer::GetRaytraceDebugBVHVisualizerEnabled())
				{
					device->RenderPassBegin(&renderpass_debugbvh, cmd);

					Viewport vp;
					vp.width = (float)traceResult.GetDesc().width;
					vp.height = (float)traceResult.GetDesc().height;
					device->BindViewports(1, &vp, cmd);

					ap::renderer::RayTraceSceneBVH(*scene, cmd);

					device->RenderPassEnd(cmd);
				}
				else
				{
					auto range = ap::profiler::BeginRangeGPU("Traced Scene", cmd);

					ap::renderer::RayTraceScene(
						*scene,
						traceResult,
						sam,
						cmd,
						instanceInclusionMask_PathTrace,
						denoiserAlbedo.IsValid() ? &denoiserAlbedo : nullptr,
						denoiserNormal.IsValid() ? &denoiserNormal : nullptr
					);


					ap::profiler::EndRange(range); // Traced Scene
				}

				});
		}

		// Tonemap etc:
		CommandList cmd = device->BeginCommandList();
		ap::jobsystem::Execute(ctx, [this, cmd](ap::jobsystem::JobArgs args) {

			GraphicsDevice* device = ap::graphics::GetDevice();

			ap::renderer::BindCameraCB(
				*camera,
				*camera,
				*camera,
				cmd
			);
			ap::renderer::BindCommonResources(cmd);

			Texture srcTex = denoiserResult.IsValid() && !ap::jobsystem::IsBusy(denoiserContext) ? denoiserResult : traceResult;

			if (getEyeAdaptionEnabled())
			{
				ap::renderer::ComputeLuminance(
					luminanceResources,
					srcTex,
					cmd,
					getEyeAdaptionRate(),
					getEyeAdaptionKey()
				);
			}
			if (getBloomEnabled())
			{
				ap::renderer::ComputeBloom(
					bloomResources,
					srcTex,
					cmd,
					getBloomThreshold(),
					getExposure(),
					getEyeAdaptionEnabled() ? &luminanceResources.luminance : nullptr
				);
			}

			ap::renderer::Postprocess_Tonemap(
				srcTex,
				rtPostprocess,
				cmd,
				getExposure(),
				getDitherEnabled(),
				getColorGradingEnabled() ? (scene->weather.colorGradingMap.IsValid() ? &scene->weather.colorGradingMap.GetTexture() : nullptr) : nullptr,
				nullptr,
				getEyeAdaptionEnabled() ? &luminanceResources.luminance : nullptr,
				getBloomEnabled() ? &bloomResources.texture_bloom : nullptr,
				colorspace
			);
			lastPostprocessRT = &rtPostprocess;

			// GUI Background blurring:
			{
				auto range = ap::profiler::BeginRangeGPU("GUI Background Blur", cmd);
				device->EventBegin("GUI Background Blur", cmd);
				ap::renderer::Postprocess_Downsample4x(rtPostprocess, rtGUIBlurredBackground[0], cmd);
				ap::renderer::Postprocess_Downsample4x(rtGUIBlurredBackground[0], rtGUIBlurredBackground[2], cmd);
				ap::renderer::Postprocess_Blur_Gaussian(rtGUIBlurredBackground[2], rtGUIBlurredBackground[1], rtGUIBlurredBackground[2], cmd, -1, -1, true);
				device->EventEnd(cmd);
				ap::profiler::EndRange(range);
			}
			});

		RenderPath2D::Render();

		ap::jobsystem::Wait(ctx);
	}

	void RenderPath3D_PathTracing::Compose(CommandList cmd) const
	{
		GraphicsDevice* device = ap::graphics::GetDevice();

		device->EventBegin("RenderPath3D_PathTracing::Compose", cmd);

		ap::renderer::BindCommonResources(cmd);

		ap::image::Params fx;
		fx.enableFullScreen();
		fx.blendFlag = ap::enums::BLENDMODE_OPAQUE;
		fx.quality = ap::image::QUALITY_LINEAR;
		ap::image::Draw(&rtPostprocess, fx, cmd);

		device->EventEnd(cmd);

		RenderPath2D::Compose(cmd);
	}

}
