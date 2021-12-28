#pragma once

class Editor;

namespace Panel
{

	class BasePanel
	{
	public:
		BasePanel() = delete;
		BasePanel(Editor* editor);
		virtual ~BasePanel() = default;

		virtual void ImGuiRender(float dt) {};

		Editor* editor;

	};

}