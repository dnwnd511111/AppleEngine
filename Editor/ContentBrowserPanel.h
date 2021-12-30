#pragma once

#include "BasePanel.h"
#include <filesystem>

namespace Panel
{
	class ContentBrowserPanel :public BasePanel
	{

		enum ICON_TYPE
		{
			ICON_FOLDER,
			ICON_PNG,
			ICON_BTN_BACK,
			ICON_BTN_FORWRD,
			ICON_BTN_REFRESH,
			ICON_FILE,
			ICON_COUNT
		};

	public:
		ContentBrowserPanel(Editor* editor);
		virtual ~ContentBrowserPanel() = default;

		void ImGuiRender(float dt) override;
			
	private:
		void RenderTopBar();

	private:
		std::filesystem::path currentDirectory;
		std::filesystem::path selectedPath;
		
		bool isContentBrowserHovered;

		ap::Resource contentIcons[ICON_COUNT];

	};

}


