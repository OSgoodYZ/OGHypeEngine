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
#include "public/editor/OgPlayWindow.h"  // 경로 수정


OG_NAMESPACE_SAMPLE_BEGIN

/**
* @brief Render backend를 테스트 하기위한 sample 코드의 main Loop입니다.
*/
class OG_API OgSampleMain
{
public:
	OgSampleMain() = default;
	~OgSampleMain() {};

	void Run(OgSystemContext* systemContext);

private:
	

	void initRenderContext(OgSystemContext* systemContext);

	void initImGUIContext();
	

	void initWindow();

	void mainLoop();

	void finalWindow();
	
	void finalImGUIContext();
	

	void finalRenderContext();

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