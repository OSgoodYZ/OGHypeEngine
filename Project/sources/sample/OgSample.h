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

#include "system/OgSystemContext.h"
#include "render/OgRenderContext.h"

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

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

	// initVulkan
	//void createInstance();
	//setupDebugMessenger();
	//createSurface();
	//pickPhysicalDevice();
	//createLogicalDevice();
	//createSwapChain();
	//createImageViews();
	//createRenderPass();
	//createGraphicsPipeline();
	//createFramebuffers();
	//createCommandPool();
	//createCommandBuffers();
	//createSyncObjects();

	GLFWwindow* _window;
	const uint32 _width = 800;
	const uint32 _height = 600;

	Render::OgRenderContext* _renderContext = nullptr;
};

OG_NAMESPACE_SAMPLE_END

#endif // _OG_SAMPLE_H__