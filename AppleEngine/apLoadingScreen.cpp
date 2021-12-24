#include "apLoadingScreen.h"
#include "apApplication.h"

#include <thread>

namespace ap
{

	bool LoadingScreen::isActive()
	{
		return ap::jobsystem::IsBusy(ctx);
	}

	void LoadingScreen::addLoadingFunction(std::function<void(ap::jobsystem::JobArgs)> loadingFunction)
	{
		if (loadingFunction != nullptr)
		{
			tasks.push_back(loadingFunction);
		}
	}

	void LoadingScreen::addLoadingComponent(RenderPath* component, Application* main, float fadeSeconds, ap::Color fadeColor)
	{
		addLoadingFunction([=](ap::jobsystem::JobArgs args) {
			component->Load();
			});
		onFinished([=] {
			main->ActivatePath(component, fadeSeconds, fadeColor);
			});
	}

	void LoadingScreen::onFinished(std::function<void()> finishFunction)
	{
		if (finishFunction != nullptr)
			finish = finishFunction;
	}

	void LoadingScreen::Start()
	{
		for (auto& x : tasks)
		{
			ap::jobsystem::Execute(ctx, x);
		}
		std::thread([this]() {
			ap::jobsystem::Wait(ctx);
			finish();
			}).detach();

			RenderPath2D::Start();
	}

	void LoadingScreen::Stop()
	{
		tasks.clear();
		finish = nullptr;

		RenderPath2D::Stop();
	}

}
