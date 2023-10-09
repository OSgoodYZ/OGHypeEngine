#pragma once
#ifndef _OG_SAMPLE_H__
#define _OG_SAMPLE_H__
#include "OgPrecompile.h"

//#include <vulkan/vulkan.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <vector>
#include <cstring>

#include "system/OgSystemContext.h"
#include "render/OgRenderContext.h"
#include "system/OgNativeWindow.h"
#include "system/OgNativeEvent.h"
#include "sample/public/editor/OgPlayWindow.h"


OG_NAMESPACE_SAMPLE_BEGIN

/**
* @brief 단수 vulkan code를 테스트 하기 위한 sample 코드이다. render쪽이 만들어진 후에 사라질 코드이다.
*/
class OG_API OgSample
{
public:
	OgSample() = default;
	~OgSample() {};

	void Run(OgSystemContext* systemContext);

private:
	void initWindow();
	void initVulkan(OgSystemContext* systemContext);
	
	void mainLoop();
	
	void cleanup();

	// TODO: @osgood window surface class 만들어서 따로 빼기

	OgPlayWindow* _handle;
	//GLFWwindow* _window;
	//Render::OgSwapChain* _swapchain;

	const uint32 _width = 800;
	const uint32 _height = 600;

	Render::OgRenderContext* _renderContext = nullptr;
};

OG_NAMESPACE_SAMPLE_END

#endif // _OG_SAMPLE_H__