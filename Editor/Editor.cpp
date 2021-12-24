
#include "pch.h"
#include "Editor.h"
#include "apRenderer.h"

//#include "ModelImporter.h"
//#include "Translator.h"

#include <string>
#include <cassert>
#include <cmath>
#include <filesystem>
#include <limits>

using namespace ap::graphics;
using namespace ap::primitive;
using namespace ap::rectpacker;
using namespace ap::scene;
using namespace ap::ecs;

void Editor::Initialize()
{
	Application::Initialize();


	// With this mode, file data for resources will be kept around. This allows serializing embedded resource data inside scenes
	ap::resourcemanager::SetMode(ap::resourcemanager::Mode::ALLOW_RETAIN_FILEDATA);

	infoDisplay.active = true;
	infoDisplay.watermark = true;
	infoDisplay.fpsinfo = true;
	infoDisplay.resolution = true;
	//infoDisplay.logical_size = true;
	infoDisplay.colorspace = true;
	infoDisplay.heap_allocation_counter = true;

	ap::renderer::SetOcclusionCullingEnabled(true);

	loader.Load();

	renderComponent.main = this;

	loader.addLoadingComponent(&renderComponent, this, 0.2f);

	ActivatePath(&loader, 0.2f);
}

void EditorLoadingScreen::Load()
{

	LoadingScreen::Load();
}

void EditorLoadingScreen::Update(float dt)
{

	LoadingScreen::Update(dt);

}
