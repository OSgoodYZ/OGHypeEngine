#pragma once
#ifndef _OG_TRIANGLE_H__
#define _OG_TRIANGLE_H__
#include "OgPrecompile.h"
#include "render/OgRenderContext.h"


OG_NAMESPACE_SAMPLE_BEGIN

/**
* @brief Render backend를 테스트 하기위한 sample 코드의 main Loop입니다.
*/
class OG_API OgTriangle
{
public:
	OgTriangle(Render::OgRenderContext* renderContext)
		:_renderContext(renderContext)
	{

	};

	~OgTriangle() {};

	void OnRender();

private:
	


	Render::OgRenderContext* _renderContext = nullptr;
};

OG_NAMESPACE_SAMPLE_END


#endif // _OG_TRIANGLE_H__