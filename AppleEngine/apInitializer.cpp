#include "apInitializer.h"
#include "AppleEngine.h"

#include <string>
#include <thread>
#include <atomic>

namespace ap::initializer
{
	bool initializationStarted = false;
	ap::jobsystem::context ctx;
	ap::Timer timer;
	static std::atomic_bool systems[INITIALIZED_SYSTEM_COUNT]{};

	void InitializeComponentsImmediate()
	{
		InitializeComponentsAsync();
		ap::jobsystem::Wait(ctx);
	}
	void InitializeComponentsAsync()
	{
		timer.record();

		initializationStarted = true;

		std::string ss;
		ss += "\n[ap::initializer] Initializing Apple Engine, please wait...\n";
		ss += "Version: ";
		ss += ap::version::GetVersionString();
		ap::backlog::post(ss);

		size_t shaderdump_count = ap::renderer::GetShaderDumpCount();
		if (shaderdump_count > 0)
		{
			ap::backlog::post("Embedded shaders found: " + std::to_string(shaderdump_count));
		}

		ap::backlog::post("");
		ap::jobsystem::Initialize();

		ap::backlog::post("");
		ap::jobsystem::Execute(ctx, [](ap::jobsystem::JobArgs args) { ap::font::Initialize(); systems[INITIALIZED_SYSTEM_FONT].store(true); });
		ap::jobsystem::Execute(ctx, [](ap::jobsystem::JobArgs args) { ap::image::Initialize(); systems[INITIALIZED_SYSTEM_IMAGE].store(true); });
		ap::jobsystem::Execute(ctx, [](ap::jobsystem::JobArgs args) { ap::input::Initialize(); systems[INITIALIZED_SYSTEM_INPUT].store(true); });
		ap::jobsystem::Execute(ctx, [](ap::jobsystem::JobArgs args) { ap::renderer::Initialize(); systems[INITIALIZED_SYSTEM_RENDERER].store(true); });
		ap::jobsystem::Execute(ctx, [](ap::jobsystem::JobArgs args) { ap::texturehelper::Initialize(); systems[INITIALIZED_SYSTEM_TEXTUREHELPER].store(true); });
		ap::jobsystem::Execute(ctx, [](ap::jobsystem::JobArgs args) { ap::HairParticleSystem::Initialize(); systems[INITIALIZED_SYSTEM_HAIRPARTICLESYSTEM].store(true); });
		ap::jobsystem::Execute(ctx, [](ap::jobsystem::JobArgs args) { ap::EmittedParticleSystem::Initialize(); systems[INITIALIZED_SYSTEM_EMITTEDPARTICLESYSTEM].store(true); });
		ap::jobsystem::Execute(ctx, [](ap::jobsystem::JobArgs args) { ap::Ocean::Initialize(); systems[INITIALIZED_SYSTEM_OCEAN].store(true); });
		ap::jobsystem::Execute(ctx, [](ap::jobsystem::JobArgs args) { ap::gpusortlib::Initialize(); systems[INITIALIZED_SYSTEM_GPUSORTLIB].store(true); });
		ap::jobsystem::Execute(ctx, [](ap::jobsystem::JobArgs args) { ap::GPUBVH::Initialize(); systems[INITIALIZED_SYSTEM_GPUBVH].store(true); });
		ap::jobsystem::Execute(ctx, [](ap::jobsystem::JobArgs args) { ap::physics::Initialize(); systems[INITIALIZED_SYSTEM_PHYSICS].store(true); });
		ap::jobsystem::Execute(ctx, [](ap::jobsystem::JobArgs args) { ap::audio::Initialize(); systems[INITIALIZED_SYSTEM_AUDIO].store(true); });


		std::thread([] {
			ap::jobsystem::Wait(ctx);
			ap::backlog::post("\n[ap::initializer] Apple Engine Initialized (" + std::to_string((int)std::round(timer.elapsed())) + " ms)");
		}).detach();

	}

	bool IsInitializeFinished(INITIALIZED_SYSTEM system)
	{
		if (system == INITIALIZED_SYSTEM_COUNT)
		{
			return initializationStarted && !ap::jobsystem::IsBusy(ctx);
		}
		else
		{
			return systems[system].load();
		}
	}
}
