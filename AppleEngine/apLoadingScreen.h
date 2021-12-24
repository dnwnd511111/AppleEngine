#pragma once
#include "apRenderPath2D.h"
#include "apColor.h"
#include "apJobSystem.h"
#include "apVector.h"

#include <functional>

namespace ap
{

	class Application;

	class LoadingScreen :
		public RenderPath2D
	{
	private:
		ap::jobsystem::context ctx;
		ap::vector<std::function<void(ap::jobsystem::JobArgs)>> tasks;
		std::function<void()> finish;
	public:

		//Add a loading task which should be executed
		//use std::bind( YourFunctionPointer )
		void addLoadingFunction(std::function<void(ap::jobsystem::JobArgs)> loadingFunction);
		//Helper for loading a whole renderable component
		void addLoadingComponent(RenderPath* component, Application* main, float fadeSeconds = 0, ap::Color fadeColor = ap::Color(0, 0, 0, 255));
		//Set a function that should be called when the loading finishes
		//use std::bind( YourFunctionPointer )
		void onFinished(std::function<void()> finishFunction);
		//See if the loading is currently running
		bool isActive();

		//Start Executing the tasks and mark the loading as active
		virtual void Start() override;
		//Clear all tasks
		virtual void Stop() override;
	};

}
