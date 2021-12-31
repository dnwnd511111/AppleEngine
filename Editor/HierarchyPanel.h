#pragma once
#include "BasePanel.h"



namespace Panel
{

	class HierarchyPanel : public BasePanel
	{
	public:
		HierarchyPanel(Editor* editor);
		virtual ~HierarchyPanel() = default;

		void ImGuiRender(float dt) override; 
		void ImGuiRenderProperties(float dt);

	private:
		void DrawEntityNode(ap::ecs::Entity entity) const;
		void DrawComponents(ap::ecs::Entity entity, int& subsetIdx);
		

	};

}

