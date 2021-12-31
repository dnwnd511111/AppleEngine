#include "apApplication.h"
#include "apRenderPath.h"
#include "apRenderer.h"
#include "apHelper.h"
#include "apTimer.h"
#include "apInput.h"
#include "apBacklog.h"
#include "apVersion.h"
#include "apEnums.h"
#include "apTextureHelper.h"
#include "apProfiler.h"
#include "apInitializer.h"
#include "apArguments.h"
#include "apFont.h"
#include "apImage.h"
#include "apEventHandler.h"

#include "apGraphicsDevice_DX12.h"

#include <string>
#include <algorithm>
#include <new>
#include <cstdlib>
#include <atomic>

static std::atomic<uint32_t> number_of_heap_allocations{ 0 };
static std::atomic<size_t> size_of_heap_allocations{ 0 };

using namespace ap::graphics;

namespace ap
{

	void Application::Initialize()
	{
		if (initialized)
		{
			return;
		}
		initialized = true;

		ap::initializer::InitializeComponentsAsync();
	}

	void Application::ActivatePath(RenderPath* component, float fadeSeconds, ap::Color fadeColor)
	{
		if (component != nullptr)
		{
			component->init(canvas);
		}

		// Fade manager will activate on fadeout
		fadeManager.Clear();
		fadeManager.Start(fadeSeconds, fadeColor, [this, component]() {

			if (GetActivePath() != nullptr)
			{
				GetActivePath()->Stop();
			}

			if (component != nullptr)
			{
				component->Start();
			}
			activePath = component;
			});

		fadeManager.Update(0); // If user calls ActivatePath without fadeout, it will be instant
	}

	void Application::Run()
	{
		if (!initialized)
		{
			// Initialize in a lazy way, so the user application doesn't have to call this explicitly
			Initialize();
			initialized = true;
		}
		if (!ap::initializer::IsInitializeFinished())
		{
			// Until engine is not loaded, present initialization screen...
			CommandList cmd = graphicsDevice->BeginCommandList();
			graphicsDevice->RenderPassBegin(&swapChain, cmd);
			ap::image::SetCanvas(canvas, cmd);
			Viewport viewport;
			viewport.width = (float)swapChain.desc.width;
			viewport.height = (float)swapChain.desc.height;
			graphicsDevice->BindViewports(1, &viewport, cmd);
			if (ap::initializer::IsInitializeFinished(ap::initializer::INITIALIZED_SYSTEM_FONT))
			{
				ap::font::SetCanvas(canvas, cmd);
				ap::font::Params params;
				params.posX = 5.f;
				params.posY = 5.f;
				std::string text = ap::backlog::getText();
				float textheight = ap::font::TextHeight(text, params);
				float screenheight = canvas.GetLogicalHeight();
				if (textheight > screenheight)
				{
					params.posY = screenheight - textheight;
				}
				ap::font::Draw(text, params, cmd);
			}
			graphicsDevice->RenderPassEnd(cmd);
			graphicsDevice->SubmitCommandLists();
			return;
		}


		if (!is_window_active && !ap::arguments::HasArgument("alwaysactive") && !isEditor)
		{
			// If the application is not active, disable Update loops:
			deltaTimeAccumulator = 0;
			return;
		}

		ap::profiler::BeginFrame();

		deltaTime = float(std::max(0.0, timer.elapsed() / 1000.0));
		timer.record();

		// Wake up the events that need to be executed on the main thread, in thread safe manner:
		ap::eventhandler::FireEvent(ap::eventhandler::EVENT_THREAD_SAFE_POINT, 0);

		const float dt = framerate_lock ? (1.0f / targetFrameRate) : deltaTime;

		fadeManager.Update(dt);

		ColorSpace colorspace = graphicsDevice->GetSwapChainColorSpace(&swapChain);

		if (GetActivePath() != nullptr)
		{
			GetActivePath()->colorspace = colorspace;
			GetActivePath()->init(canvas);
			GetActivePath()->PreUpdate();
		}

		// Fixed time update:
		auto range = ap::profiler::BeginRangeCPU("Fixed Update");
		{
			if (frameskip)
			{
				deltaTimeAccumulator += dt;
				if (deltaTimeAccumulator > 10)
				{
					// application probably lost control, fixed update would take too long
					deltaTimeAccumulator = 0;
				}

				const float targetFrameRateInv = 1.0f / targetFrameRate;
				while (deltaTimeAccumulator >= targetFrameRateInv)
				{
					FixedUpdate();
					deltaTimeAccumulator -= targetFrameRateInv;
				}
			}
			else
			{
				FixedUpdate();
			}
		}
		ap::profiler::EndRange(range); // Fixed Update

		// Variable-timed update:
		Update(dt);

		Render();

		ap::input::Update(window);

		// Begin final compositing:
		CommandList cmd = graphicsDevice->BeginCommandList();
		ap::image::SetCanvas(canvas, cmd);
		ap::font::SetCanvas(canvas, cmd);
		Viewport viewport;
		viewport.width = (float)swapChain.desc.width;
		viewport.height = (float)swapChain.desc.height;
		graphicsDevice->BindViewports(1, &viewport, cmd);

		bool colorspace_conversion_required = colorspace == ColorSpace::HDR10_ST2084;

		if (isEditor)
		{
			if (colorspace_conversion_required)
			{
				// In HDR10, we perform the compositing in a custom linear color space render target
				graphicsDevice->RenderPassBegin(&renderpass, cmd);
			}
			else
			{
				graphicsDevice->RenderPassBegin(&renderpass_imgui, cmd);
			}
			Compose(cmd);
			graphicsDevice->RenderPassEnd(cmd);


			if (colorspace_conversion_required)
			{
				// In HDR10, we perform a final mapping from linear to HDR10, into the swapchain
				graphicsDevice->RenderPassBegin(&renderpass_imgui, cmd);
				ap::image::Params fx;
				fx.enableFullScreen();
				fx.enableHDR10OutputMapping();
				ap::image::Draw(&rendertarget, fx, cmd);
				graphicsDevice->RenderPassEnd(cmd);
			}


			graphicsDevice->RenderPassBegin(&swapChain, cmd);
			graphicsDevice->BeginImGui();
			ImGuiRender();
			graphicsDevice->EndImGui(cmd);
			graphicsDevice->RenderPassEnd(cmd);

		}
		else
		{

			if (colorspace_conversion_required)
			{
				// In HDR10, we perform the compositing in a custom linear color space render target
				graphicsDevice->RenderPassBegin(&renderpass, cmd);
			}
			else
			{
				// If swapchain is SRGB or Linear HDR, it can be used for blending
				//	- If it is SRGB, the render path will ensure tonemapping to SDR
				//	- If it is Linear HDR, we can blend trivially in linear space
				graphicsDevice->RenderPassBegin(&swapChain, cmd);
			}
			Compose(cmd);
			graphicsDevice->RenderPassEnd(cmd);

			if (colorspace_conversion_required)
			{
				// In HDR10, we perform a final mapping from linear to HDR10, into the swapchain
				graphicsDevice->RenderPassBegin(&swapChain, cmd);
				ap::image::Params fx;
				fx.enableFullScreen();
				fx.enableHDR10OutputMapping();
				ap::image::Draw(&rendertarget, fx, cmd);
				graphicsDevice->RenderPassEnd(cmd);
			}
		}
		

		ap::profiler::EndFrame(cmd);
		graphicsDevice->SubmitCommandLists();
	}

	void Application::Update(float dt)
	{
		auto range = ap::profiler::BeginRangeCPU("Update");


		ap::backlog::Update(canvas, dt);

		if (GetActivePath() != nullptr)
		{
			GetActivePath()->Update(dt);
			GetActivePath()->PostUpdate();
		}

		ap::profiler::EndRange(range); // Update
	}

	void Application::FixedUpdate()
	{

		if (GetActivePath() != nullptr)
		{
			GetActivePath()->FixedUpdate();
		}
	}

	void Application::Render()
	{
		auto range = ap::profiler::BeginRangeCPU("Render");


		if (GetActivePath() != nullptr)
		{
			GetActivePath()->Render();
		}

		ap::profiler::EndRange(range); // Render
	}

	void Application::Compose(CommandList cmd)
	{
		auto range = ap::profiler::BeginRangeCPU("Compose");

		if (GetActivePath() != nullptr)
		{
			GetActivePath()->Compose(cmd);
		}

		if (fadeManager.IsActive())
		{
			// display fade rect
			static ap::image::Params fx;
			fx.siz.x = canvas.GetLogicalWidth();
			fx.siz.y = canvas.GetLogicalHeight();
			fx.opacity = fadeManager.opacity;
			ap::image::Draw(ap::texturehelper::getColor(fadeManager.color), fx, cmd);
		}

		// Draw the information display
		if (infoDisplay.active ) //юс╫ц
		{
			infodisplay_str.clear();
			if (infoDisplay.watermark)
			{
				infodisplay_str += "Apple Engine ";
				infodisplay_str += " ";

#if defined(_ARM)
				infodisplay_str += "[ARM]";
#elif defined(_WIN64)
				infodisplay_str += "[64-bit]";
#elif defined(_WIN32)
				infodisplay_str += "[32-bit]";
#endif

#ifdef PLATFORM_UWP
				infodisplay_str += "[UWP]";
#endif

#ifdef APPLEENGINE_BUILD_DX12
				if (dynamic_cast<GraphicsDevice_DX12*>(graphicsDevice.get()))
				{
					infodisplay_str += "[DX12]";
				}
#endif
#ifdef APPLEENGINE_BUILD_VULKAN
				if (dynamic_cast<GraphicsDevice_Vulkan*>(graphicsDevice.get()))
				{
					infodisplay_str += "[Vulkan]";
				}
#endif

#ifdef _DEBUG
				infodisplay_str += "[DEBUG]";
#endif
				if (graphicsDevice->IsDebugDevice())
				{
					infodisplay_str += "[debugdevice]";
				}
				infodisplay_str += "\n";
			}
			if (infoDisplay.resolution)
			{
				infodisplay_str += "Resolution: " + std::to_string(canvas.GetPhysicalWidth()) + " x " + std::to_string(canvas.GetPhysicalHeight()) + " (" + std::to_string(int(canvas.GetDPI())) + " dpi)\n";
			}
			if (infoDisplay.logical_size)
			{
				infodisplay_str += "Logical Size: " + std::to_string(int(canvas.GetLogicalWidth())) + " x " + std::to_string(int(canvas.GetLogicalHeight())) + "\n";
			}
			if (infoDisplay.colorspace)
			{
				infodisplay_str += "Color Space: ";
				ColorSpace colorSpace = graphicsDevice->GetSwapChainColorSpace(&swapChain);
				switch (colorSpace)
				{
				default:
				case ap::graphics::ColorSpace::SRGB:
					infodisplay_str += "sRGB";
					break;
				case ap::graphics::ColorSpace::HDR10_ST2084:
					infodisplay_str += "ST.2084 (HDR10)";
					break;
				case ap::graphics::ColorSpace::HDR_LINEAR:
					infodisplay_str += "Linear (HDR)";
					break;
				}
				infodisplay_str += "\n";
			}
			if (infoDisplay.fpsinfo)
			{
				deltatimes[fps_avg_counter++ % arraysize(deltatimes)] = deltaTime;
				float displaydeltatime = deltaTime;
				if (fps_avg_counter > arraysize(deltatimes))
				{
					float avg_time = 0;
					for (int i = 0; i < arraysize(deltatimes); ++i)
					{
						avg_time += deltatimes[i];
					}
					displaydeltatime = avg_time / arraysize(deltatimes);
				}

				infodisplay_str += std::to_string(int(std::round(1.0f / displaydeltatime))) + " FPS\n";
			}
			if (infoDisplay.heap_allocation_counter)
			{
				infodisplay_str += "Heap allocations per frame: " + std::to_string(number_of_heap_allocations.load()) + " (" + std::to_string(size_of_heap_allocations.load()) + " bytes)\n";
				number_of_heap_allocations.store(0);
				size_of_heap_allocations.store(0);
			}
			if (infoDisplay.pipeline_count)
			{
				infodisplay_str += "Graphics pipelines active: " + std::to_string(graphicsDevice->GetActivePipelineCount()) + "\n";
			}

#ifdef _DEBUG
			//infodisplay_str += "Warning: This is a [DEBUG] build, performance will be slow!\n";
#endif
			if (graphicsDevice->IsDebugDevice())
			{
				infodisplay_str += "Warning: Graphics is in [debugdevice] mode, performance will be slow!\n";
			}

			ap::font::Draw(infodisplay_str, ap::font::Params(4, 4, infoDisplay.size, ap::font::APFALIGN_LEFT, ap::font::APFALIGN_TOP, ap::Color(255, 255, 255, 255), ap::Color(0, 0, 0, 255)), cmd);

			if (infoDisplay.colorgrading_helper)
			{
				ap::image::Draw(ap::texturehelper::getColorGradeDefault(), ap::image::Params(0, 0, 256.0f / canvas.GetDPIScaling(), 16.0f / canvas.GetDPIScaling()), cmd);
			}
		}

		ap::profiler::DrawData(canvas, 4, 120, cmd);

		ap::backlog::Draw(canvas, cmd);

		ap::profiler::EndRange(range); // Compose
	}



	void Application::SetWindow(ap::platform::window_type window, bool fullscreen)
	{
		this->window = window;

		// User can also create a graphics device if custom logic is desired, but they must do before this function!
		if (graphicsDevice == nullptr)
		{
			bool debugdevice = ap::arguments::HasArgument("debugdevice");
			bool gpuvalidation = ap::arguments::HasArgument("gpuvalidation");

			bool use_dx12 = ap::arguments::HasArgument("dx12");
			bool use_vulkan = ap::arguments::HasArgument("vulkan");

#ifndef APPLEENGINE_BUILD_DX12
			if (use_dx12) {
				ap::helper::messageBox("The engine was built without DX12 support!", "Error");
				use_dx12 = false;
			}
#endif
#ifndef APPLEENGINE_BUILD_VULKAN
			if (use_vulkan) {
				ap::helper::messageBox("The engine was built without Vulkan support!", "Error");
				use_vulkan = false;
			}
#endif

			if (!use_dx12 && !use_vulkan)
			{
#if defined(APPLEENGINE_BUILD_DX12)
				use_dx12 = true;
#elif defined(APPLEENGINE_BUILD_VULKAN)
				use_vulkan = true;
#else
				ap::backlog::post("No rendering backend is enabled! Please enable at least one so we can use it as default", ap::backlog::LogLevel::Error);
				assert(false);
#endif
			}
			assert(use_dx12 || use_vulkan);

			if (use_vulkan)
			{
#ifdef APPLEENGINE_BUILD_VULKAN
				ap::renderer::SetShaderPath(ap::renderer::GetShaderPath() + "spirv/");
				graphicsDevice = std::make_unique<GraphicsDevice_Vulkan>(window, debugdevice);
#endif
			}
			else if (use_dx12)
			{
#ifdef APPLEENGINE_BUILD_DX12
				ap::renderer::SetShaderPath(ap::renderer::GetShaderPath() + "hlsl6/");
				graphicsDevice = std::make_unique<GraphicsDevice_DX12>(debugdevice, gpuvalidation);
#endif
			}
		}
		ap::graphics::GetDevice() = graphicsDevice.get();

		canvas.init(window);

		SwapChainDesc desc;
		if (swapChain.IsValid())
		{
			// it will only resize, but keep format and other settings
			desc = swapChain.desc;
		}
		else
		{
			// initialize for the first time
			desc.buffer_count = 3;
			desc.format = Format::R10G10B10A2_UNORM;
		}
		desc.width = canvas.GetPhysicalWidth();
		desc.height = canvas.GetPhysicalHeight();
		desc.allow_hdr = allow_hdr;
		bool success = graphicsDevice->CreateSwapChain(&desc, window, &swapChain);
		assert(success);

		swapChainVsyncChangeEvent = ap::eventhandler::Subscribe(ap::eventhandler::EVENT_SET_VSYNC, [this](uint64_t userdata) {
			SwapChainDesc desc = swapChain.desc;
			desc.vsync = userdata != 0;
			bool success = graphicsDevice->CreateSwapChain(&desc, nullptr, &swapChain);
			assert(success);
			});


		if (graphicsDevice->GetSwapChainColorSpace(&swapChain) == ColorSpace::HDR10_ST2084)
		{
			TextureDesc desc;
			desc.width = swapChain.desc.width;
			desc.height = swapChain.desc.height;
			desc.format = Format::R11G11B10_FLOAT;
			desc.bind_flags = BindFlag::RENDER_TARGET | BindFlag::SHADER_RESOURCE;
			bool success = graphicsDevice->CreateTexture(&desc, nullptr, &rendertarget);
			assert(success);
			graphicsDevice->SetName(&rendertarget, "Application::rendertarget");

			RenderPassDesc renderpassdesc;
			renderpassdesc.attachments.push_back(RenderPassAttachment::RenderTarget(&rendertarget, RenderPassAttachment::LoadOp::CLEAR));
			success = graphicsDevice->CreateRenderPass(&renderpassdesc, &renderpass);
			assert(success);
		}

		if (isEditor)
		{
			TextureDesc desc;
			desc.width = swapChain.desc.width;
			desc.height = swapChain.desc.height;
			desc.format = Format::R10G10B10A2_UNORM;
			desc.bind_flags = BindFlag::RENDER_TARGET | BindFlag::SHADER_RESOURCE;
			bool success = graphicsDevice->CreateTexture(&desc, nullptr, &rendertarget_imgui);
			assert(success);
			graphicsDevice->SetName(&rendertarget_imgui, "Application::rendertarget_imgui");

			RenderPassDesc renderpassdesc;
			renderpassdesc.attachments.push_back(RenderPassAttachment::RenderTarget(&rendertarget_imgui, RenderPassAttachment::LoadOp::CLEAR));
			success = graphicsDevice->CreateRenderPass(&renderpassdesc, &renderpass_imgui);
			assert(success);
		}
	}

	void Application::ImGuiRender()
	{
		
	}

}


// Heap alloc replacements are used to count heap allocations:
//	It is good practice to reduce the amount of heap allocations that happen during the frame,
//	so keep an eye on the info display of the engine while Application::InfoDisplayer::heap_allocation_counter is enabled

void* operator new(std::size_t size) {
	number_of_heap_allocations.fetch_add(1);
	size_of_heap_allocations.fetch_add(size);
	void* p = malloc(size);
	if (!p) throw std::bad_alloc();
	return p;
}
void* operator new[](std::size_t size) {
	number_of_heap_allocations.fetch_add(1);
	size_of_heap_allocations.fetch_add(size);
	void* p = malloc(size);
	if (!p) throw std::bad_alloc();
	return p;
}
void* operator new[](std::size_t size, const std::nothrow_t&) throw() {
	number_of_heap_allocations.fetch_add(1);
	size_of_heap_allocations.fetch_add(size);
	return malloc(size);
}
void* operator new(std::size_t size, const std::nothrow_t&) throw() {
	number_of_heap_allocations.fetch_add(1);
	size_of_heap_allocations.fetch_add(size);
	return malloc(size);
}
void operator delete(void* ptr) throw() { free(ptr); }
void operator delete (void* ptr, const std::nothrow_t&) throw() { free(ptr); }
void operator delete[](void* ptr) throw() { free(ptr); }
void operator delete[](void* ptr, const std::nothrow_t&) throw() { free(ptr); }
