#pragma once
#include "CommonInclude.h"
#include "apGraphicsDevice.h"
#include "apColor.h"

namespace ap::texturehelper
{
	void Initialize();

	const ap::graphics::Texture* getRandom64x64();
	const ap::graphics::Texture* getColorGradeDefault();
	const ap::graphics::Texture* getNormalMapDefault();
	const ap::graphics::Texture* getBlackCubeMap();
	const ap::graphics::Texture* getUINT4();
	const ap::graphics::Texture* getBlueNoise();

	const ap::graphics::Texture* getWhite();
	const ap::graphics::Texture* getBlack();
	const ap::graphics::Texture* getTransparent();
	const ap::graphics::Texture* getColor(ap::Color color);

	bool CreateTexture(ap::graphics::Texture& texture, const uint8_t* data, uint32_t width, uint32_t height, ap::graphics::Format format = ap::graphics::Format::R8G8B8A8_UNORM);
};

