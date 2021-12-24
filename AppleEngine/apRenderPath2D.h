#pragma once
#include "apRenderPath.h"
#include "apGUI.h"
#include "apVector.h"

#include <string>

namespace ap
{
	class Sprite;
	class SpriteFont;

	struct RenderItem2D
	{
		enum TYPE
		{
			SPRITE,
			FONT,
		} type = SPRITE;
		union
		{
			ap::Sprite* sprite = nullptr;
			ap::SpriteFont* font;
		};
		int order = 0;
	};
	struct RenderLayer2D
	{
		ap::vector<RenderItem2D> items;
		std::string name;
		int order = 0;
	};

	class RenderPath2D :
		public RenderPath
	{
	private:
		ap::graphics::Texture rtStenciled;
		ap::graphics::Texture rtStenciled_resolved;
		ap::graphics::Texture rtFinal;

		ap::graphics::RenderPass renderpass_stenciled;
		ap::graphics::RenderPass renderpass_final;

		ap::graphics::Texture rtLinearColorSpace;
		ap::graphics::RenderPass renderpass_linearize;

		ap::gui::GUI GUI;

		XMUINT2 current_buffersize{};
		float current_layoutscale{};

		mutable ap::graphics::Texture render_result = rtFinal;

		float hdr_scaling = 9.0f;

	public:
		// create resolution dependent resources, such as render targets
		virtual void ResizeBuffers();
		// update DPI dependent elements, such as GUI elements, sprites
		virtual void ResizeLayout();

		void Update(float dt) override;
		void FixedUpdate() override;
		void Render() const override;
		void Compose(ap::graphics::CommandList cmd) const override;

		const ap::graphics::Texture& GetRenderResult() const { return render_result; }
		virtual const ap::graphics::Texture* GetDepthStencil() const { return nullptr; }
		virtual const ap::graphics::Texture* GetGUIBlurredBackground() const { return nullptr; }

		void AddSprite(ap::Sprite* sprite, const std::string& layer = "");
		void RemoveSprite(ap::Sprite* sprite);
		void ClearSprites();
		int GetSpriteOrder(ap::Sprite* sprite);

		void AddFont(ap::SpriteFont* font, const std::string& layer = "");
		void RemoveFont(ap::SpriteFont* font);
		void ClearFonts();
		int GetFontOrder(ap::SpriteFont* font);

		ap::vector<RenderLayer2D> layers{ 1 };
		void AddLayer(const std::string& name);
		void SetLayerOrder(const std::string& name, int order);
		void SetSpriteOrder(ap::Sprite* sprite, int order);
		void SetFontOrder(ap::SpriteFont* font, int order);
		void SortLayers();
		void CleanLayers();

		const ap::gui::GUI& GetGUI() const { return GUI; }
		ap::gui::GUI& GetGUI() { return GUI; }

		float resolutionScale = 1.0f;
		XMUINT2 GetInternalResolution() const
		{
			return XMUINT2(
				uint32_t((float)GetPhysicalWidth() * resolutionScale),
				uint32_t((float)GetPhysicalHeight() * resolutionScale)
			);
		}

		float GetHDRScaling() const { return hdr_scaling; }
		void SetHDRScaling(float value) { hdr_scaling = value; }
	};

}
