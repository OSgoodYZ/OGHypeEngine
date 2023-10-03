#include "OgSample.h"

#include "render/private/vulkan/OgRenderContext_Vulkan.h"
OG_NAMESPACE_SAMPLE_BEGIN

void OgSample::Run(OgSystemContext* systemContext) {
	initWindow();
	initVulkan(systemContext);
	mainLoop();
	cleanup();
}

void OgSample::initWindow() 
{
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	_window = glfwCreateWindow(_width, _height, "Vulkan", nullptr, nullptr);
}

void OgSample::initVulkan(OgSystemContext* systemContext)
{
	_renderContext = new Render::OgRenderContextVulkan(systemContext);
	_renderContext->Load();
	_renderContext->Init();
	
	Render::OgSwapChainInfo scInfo;
	scInfo.useDepthBuffer = true;
	scInfo.useStencilBuffer = false;
	scInfo.depthBufferFormat = Render::OgRenderTextureFormat::DEFAULT_DEPTH;
	_swapchain = _renderContext->CreateSwapchain(systemContext->headWindow, scInfo);

}

void OgSample::mainLoop() 
{
	while (!glfwWindowShouldClose(_window)) 
	{
		glfwPollEvents();
	}
}

void OgSample::cleanup() 
{
	glfwDestroyWindow(_window);
	glfwTerminate();

	_renderContext->Shutdown();
	delete _renderContext;
		


}


OG_NAMESPACE_SAMPLE_END