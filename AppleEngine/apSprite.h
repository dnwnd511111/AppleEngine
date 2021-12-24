#pragma once
#include "apImage.h"
#include "apGraphicsDevice.h"
#include "apResourceManager.h"
#include "apRandom.h"

#include <memory>
#include <string>

namespace ap
{
	class Sprite
	{
	private:
		enum FLAGS
		{
			EMPTY = 0,
			HIDDEN = 1 << 0,
			DISABLE_UPDATE = 1 << 1,
		};
		uint32_t _flags = EMPTY;

		std::string textureName, maskName;
	public:
		Sprite(const std::string& newTexture = "", const std::string& newMask = "");

		virtual void FixedUpdate();
		virtual void Update(float dt);
		virtual void Draw(ap::graphics::CommandList cmd) const;

		constexpr void SetHidden(bool value = true) { if (value) { _flags |= HIDDEN; } else { _flags &= ~HIDDEN; } }
		constexpr bool IsHidden() const { return _flags & HIDDEN; }
		constexpr void SetDisableUpdate(bool value = true) { if (value) { _flags |= DISABLE_UPDATE; } else { _flags &= ~DISABLE_UPDATE; } }
		constexpr bool IsDisableUpdate() const { return _flags & DISABLE_UPDATE; }

		ap::image::Params params;
		ap::Resource textureResource;
		ap::Resource maskResource;

		struct Anim
		{
			struct MovingTexAnim
			{
				float speedX = 0; // the speed of texture scrolling animation in horizontal direction
				float speedY = 0; // the speed of texture scrolling animation in vertical direction
			};
			struct DrawRectAnim
			{
				float frameRate = 30; // target frame rate of the spritesheet animation (eg. 30, 60, etc.)
				int frameCount = 1; // how many frames are in the animation in total
				int horizontalFrameCount = 0; // how many horizontal frames there are (optional, use if the spritesheet contains multiple rows)

				float _elapsedTime = 0; // internal use; you don't need to initialize
				int _currentFrame = 0; // internal use; you don't need to initialize
			};
			struct WobbleAnim
			{
				XMFLOAT2 amount = XMFLOAT2(0, 0);	// how much the sprite wobbles in X and Y direction
				float speed = 1; // how fast the sprite wobbles

				float corner_angles[4]; // internal use; you don't need to initialize
				float corner_speeds[4]; // internal use; you don't need to initialize
				float corner_angles2[4]; // internal use; you don't need to initialize
				float corner_speeds2[4]; // internal use; you don't need to initialize
				WobbleAnim()
				{
					for (int i = 0; i < 4; ++i)
					{
						corner_angles[i] = ap::random::GetRandom(0, 1000) / 1000.0f * XM_2PI;
						corner_speeds[i] = ap::random::GetRandom(500, 1000) / 1000.0f;
						if (ap::random::GetRandom(0, 1) == 0)
						{
							corner_speeds[i] *= -1;
						}
						corner_angles2[i] = ap::random::GetRandom(0, 1000) / 1000.0f * XM_2PI;
						corner_speeds2[i] = ap::random::GetRandom(500, 1000) / 1000.0f;
						if (ap::random::GetRandom(0, 1) == 0)
						{
							corner_speeds2[i] *= -1;
						}
					}
				}
			};

			bool repeatable = false;
			XMFLOAT3 vel = XMFLOAT3(0, 0, 0);
			float rot = 0;
			float scaleX = 0;
			float scaleY = 0;
			float opa = 0;
			float fad = 0;
			MovingTexAnim movingTexAnim;
			DrawRectAnim drawRectAnim;
			WobbleAnim wobbleAnim;
		};
		Anim anim;

		const ap::graphics::Texture* getTexture() const
		{
			if (textureResource.IsValid())
			{
				return &textureResource.GetTexture();
			}
			return nullptr;
		}
	};
}
