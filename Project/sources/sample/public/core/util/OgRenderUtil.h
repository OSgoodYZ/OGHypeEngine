#pragma once
#ifndef _OG_RENDER_UTIL_H__
#define _OG_RENDER_UTIL_H__
#include "OgPrecompile.h"
#include "glm/glm.hpp"
/* #include "render/OgRenderDefinitions.h" */ namespace Render { struct OgSwapChain; }
/**
* @file Render에 필요한 유틸리티 클래스를 모아 놓습니다.
*/
using namespace std;

OG_NAMESPACE_SAMPLE_BEGIN

#pragma region OgRect
struct OG_API OgRect
{
	float x{};
	float y{};
	float width{};
	float height{};

	OgRect() {}

	OgRect(float x, float y, float width, float height)
		: x(x), y(y), width(width), height(height)
	{

	}

	inline float Left() const
	{
		return x;
	}

	inline float Right() const
	{
		return x + width;
	}

	inline float Top() const
	{
		return y;
	}

	inline float Bottom() const
	{
		return y + height;
	}

	inline glm::vec2 Min() const
	{
		return glm::vec2(x, y);
	}

	inline glm::vec2 Max() const
	{
		return glm::vec2(x + width, y + height);
	}

	inline glm::vec2 Center() const
	{
		return (Max() - Min() / 2.0f);
	}

	inline bool Contain(glm::vec2 position) const
	{

		return Left() <= position.x && Right() >= position.x && Top() <= position.y && Bottom() >= position.y;
	}

	inline bool operator==(const OgRect& o)
	{
		return (x == o.x) && (y == o.y) && (width == o.width) && (height == o.height);
	}

	inline bool operator!=(const OgRect& o)
	{
		return !operator==(o);
	}
};
#pragma endregion

#pragma region OgSurface

struct OG_API OgSurface
{
	uint32 handle;

	OgRect rect;

	Render::OgSwapChain* swapchain;

	OgSurface(float width, float height);
};
#pragma endregion

OG_NAMESPACE_SAMPLE_END


#endif // _OG_RENDER_UTIL_H__