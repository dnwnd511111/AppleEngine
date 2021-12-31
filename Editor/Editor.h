#pragma once

#include "AppleEngine.h"
#include "Translator.h"
#include "ImGuizmo.h"

#include "HierarchyPanel.h"
#include "ContentBrowserPanel.h"

enum PICKTYPE
{
	PICK_VOID = 0,
	PICK_OBJECT = ap::enums::RENDERTYPE_ALL,
	PICK_LIGHT = 8,
	PICK_DECAL = 16,
	PICK_ENVPROBE = 32,
	PICK_FORCEFIELD = 64,
	PICK_EMITTER = 128,
	PICK_HAIR = 256,
	PICK_CAMERA = 512,
	PICK_ARMATURE = 1024,
	PICK_SOUND = 2048,
};

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
private:
	ap::Resource pointLightTex, spotLightTex, dirLightTex, areaLightTex, decalTex, forceFieldTex, emitterTex, hairTex, cameraTex, armatureTex, soundTex;
public:

	ap::ecs::Entity mainCamera = ap::ecs::INVALID_ENTITY;

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

	void DeleteSelectedEntities();

	//
	uint32_t pickType = ~0;

	
	
};

class Editor : public ap::Application
{
public:
	EditorComponent renderComponent;
	EditorLoadingScreen loader;

	XMFLOAT2 viewportSize = { 0.0f, 0.0f };
	XMFLOAT2 viewportBounds[2];
	bool viewportFocused = false;
	bool viewportHovered = false;




	//panel
	Panel::HierarchyPanel hierarchyPanel{this};
	Panel::ContentBrowserPanel contentBrowserPanel{ this };

	void Initialize() override;
	void ImGuiRender() override;

private:
	void ImGuiRender_PlaceActors();
	void ImGuiRender_PostProcess();
	void ImGuiRender_ToolBar();
};
