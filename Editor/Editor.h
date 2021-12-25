#pragma once

#include "AppleEngine.h"
#include "Translator.h"


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

	enum RENDERPATH
	{
		RENDERPATH_DEFAULT,
		RENDERPATH_PATHTRACING,
	};
	void ChangeRenderPath(RENDERPATH path);
	const ap::graphics::Texture* GetGUIBlurredBackground() const override { return renderPath->GetGUIBlurredBackground(); }

	void ResizeBuffers() override;
	void ResizeLayout() override;
	void Load() override;
	void Start() override;
	void PreUpdate() override;
	void FixedUpdate() override;
	void Update(float dt) override;
	void PostUpdate() override;
	void Render() const override;
	void Compose(ap::graphics::CommandList cmd) const override;


	enum EDITORSTENCILREF
	{
		EDITORSTENCILREF_CLEAR = 0x00,
		EDITORSTENCILREF_HIGHLIGHT_OBJECT = 0x01,
		EDITORSTENCILREF_HIGHLIGHT_MATERIAL = 0x02,
		EDITORSTENCILREF_LAST = 0x0F,
	};
	ap::graphics::Texture rt_selectionOutline_MSAA;
	ap::graphics::Texture rt_selectionOutline[2];
	ap::graphics::RenderPass renderpass_selectionOutline[2];
	float selectionOutlineTimer = 0;
	const XMFLOAT4 selectionColor = XMFLOAT4(1, 0.6f, 0, 1);
	const XMFLOAT4 selectionColor2 = XMFLOAT4(0, 1, 0.6f, 0.35f);

	Translator translator;
	ap::scene::PickResult hovered;

	void ClearSelected();
	void AddSelected(ap::ecs::Entity entity);
	void AddSelected(const ap::scene::PickResult& picked);
	bool IsSelected(ap::ecs::Entity entity) const;

	ap::Archive clipboard;

	ap::vector<ap::Archive> history;
	int historyPos = -1;
	enum HistoryOperationType
	{
		HISTORYOP_TRANSLATOR,
		HISTORYOP_DELETE,
		HISTORYOP_SELECTION,
		HISTORYOP_PAINTTOOL,
		HISTORYOP_NONE
	};

	void ResetHistory();
	ap::Archive& AdvanceHistory();
	void ConsumeHistoryOperation(bool undo);


};

class Editor : public ap::Application
{
public:
	EditorComponent renderComponent;
	EditorLoadingScreen loader;

	void Initialize() override;
};
