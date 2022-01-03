#pragma once

#include "BasePanel.h"

namespace Panel
{

	class PaintToolPanel : public BasePanel
	{
	public:
		PaintToolPanel(Editor* editor);
		virtual ~PaintToolPanel() = default;

		void ImGuiRender(float dt) override;

	private:
		int selectedMode = 0 ;
		float brushRadius = 50;
		float brushAmount = 1;
		float brushFalloff = 0;
		int brushSpacing = 1;
		bool backfaces = false;
		bool wireframe = true;
		bool pressure = false;
		int selectedTextureIdx = 0;
		XMFLOAT4 textureColor = { 1, 1, 1, 1 };


		ap::ecs::Entity entity = ap::ecs::INVALID_ENTITY;
		int subset = -1;

		float rot = 0;
		float stroke_dist = 0;
		bool history_needs_recording_start = false;
		bool history_needs_recording_end = false;

		XMFLOAT2 pos = XMFLOAT2(0, 0);

	public:
		void Update(float dt);
		void DrawBrush() const;

		enum MODE
		{
			MODE_DISABLED,
			MODE_TEXTURE,
			MODE_VERTEXCOLOR,
			MODE_SCULPTING_ADD,
			MODE_SCULPTING_SUBTRACT,
			MODE_SOFTBODY_PINNING,
			MODE_SOFTBODY_PHYSICS,
			MODE_HAIRPARTICLE_ADD_TRIANGLE,
			MODE_HAIRPARTICLE_REMOVE_TRIANGLE,
			MODE_HAIRPARTICLE_LENGTH,
			MODE_WIND,
		};
		MODE GetMode() const;
		void SetEntity(ap::ecs::Entity value, int subsetindex = -1);
	};

}
