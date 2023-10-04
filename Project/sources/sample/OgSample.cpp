#include "OgSample.h"

#include "render/private/vulkan/OgRenderContext_Vulkan.h"

using namespace std;
OG_NAMESPACE_SAMPLE_BEGIN

void OgSample::Run(OgSystemContext* systemContext) {
	initVulkan(systemContext);
	initWindow();
	mainLoop();
	cleanup();
}

void OgSample::initWindow() 
{
	//glfwInit();
	//glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	//glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	//_window = glfwCreateWindow(_width, _height, "Vulkan", nullptr, nullptr);
	_handle = new OgPlayWindow(_renderContext, "Vulkan", _width, _height);
	_handle->Open();
}

void OgSample::initVulkan(OgSystemContext* systemContext)
{
	_renderContext = new Render::OgRenderContextVulkan(systemContext);
	_renderContext->Load();
	_renderContext->Init();
	
	//Render::OgSwapChainInfo scInfo;
	//scInfo.useDepthBuffer = true;
	//scInfo.useStencilBuffer = false;
	//scInfo.depthBufferFormat = Render::OgRenderTextureFormat::DEFAULT_DEPTH;
	//_swapchain = _renderContext->CreateSwapchain(systemContext->headWindow, scInfo);

	//OgPlayWindow(Render::OgRenderContext* renderContext, const char* name, uint32_t width, uint32 height, OgPlayWindow* parent = nullptr)


}

void OgSample::mainLoop() 
{
	//while (!glfwWindowShouldClose(_window)) 
	//{
	//	glfwPollEvents();
	//}

	while (true)
	{

		// TODO
		og_system_poll_events();

		::vector<OgPlayWindow*> deleteWindows;

		//for (int32 i = static_cast<int32>(windows.Count()) - 1; i >= 0; i += -1)
		{
			OgPlayWindow& window = *_handle;

			if (window.ShouldClose() == false)
			{
				window.PeekEvent();
			}
			else
			{
				//dispatchQueueHandle->Execute();
				window.Close();
				deleteWindows.push_back(&window);
			}
		}


		if (nullptr != _handle)
		{
			_renderContext->Collect();

//			for (size_t i = 0; i < windows.Count(); ++i)
			{
				OgPlayWindow& window = *_handle;
				if (!window.ShouldClose())
				{
					window.NextFrame();
					window.Update();
					window.Present();
				}
			}

		}
		else
		{
			break;
		}

	}

}

void OgSample::cleanup() 
{
	//glfwDestroyWindow(_window);
	//glfwTerminate();

	_renderContext->Shutdown();
	delete _renderContext;
		


}


OG_NAMESPACE_SAMPLE_END