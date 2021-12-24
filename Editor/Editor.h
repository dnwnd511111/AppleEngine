#pragma once

#include "resource.h"


class EditorLoadingScreen : public ap::LoadingScreen
{
private:
	ap::Sprite sprite;
	ap::SpriteFont font;
public:
	void Load() override;
	void Update(float dt) override;
};

class Editor;
class EditorComponent : public ap::RenderPath2D
{
public:

	Editor* main = nullptr;
	std::unique_ptr<ap::RenderPath3D> renderPath;

};

class Editor : public ap::Application
{
public:
	EditorComponent renderComponent;
	EditorLoadingScreen loader;

	void Initialize() override;
};
