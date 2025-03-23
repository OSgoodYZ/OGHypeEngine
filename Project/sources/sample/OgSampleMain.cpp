#include "OgSampleMain.h"

#include "system/OgImGUIManager.h"
#include "render/private/vulkan/OgRenderContext_Vulkan.h"
#include "sample/public/editor/OgPlayWindow.h"  
using namespace std;
OG_NAMESPACE_SAMPLE_BEGIN

void OgSampleMain::Run(OgSystemContext* systemContext) {
	initRenderContext(systemContext);
	initImGUIContext();
	initWindow();
	mainLoop();
	finalWindow();
	finalImGUIContext();
	finalRenderContext();
}

void OgSampleMain::initRenderContext(OgSystemContext* systemContext)
{
	_renderContext = new Render::OgRenderContextVulkan(systemContext);
	_renderContext->Load();
	_renderContext->Init();

}

void OgSampleMain::initImGUIContext()
{
	OgImGuiContextManager::Initialize();

	ImGuiContext* ctx = OgImGuiContextManager::CreateContext(true);

	OgImGuiContextManager::SetMainImGuiContext(ctx);
}

void OgSampleMain::initWindow() 
{
	_handle = new OgPlayWindow(_renderContext, "Vulkan", _width, _height);
	_handle->Open();
}

void OgSampleMain::finalWindow()
{
	_handle->Close();
	delete _handle;
	_handle = nullptr;
	
}



void OgSampleMain::mainLoop() 
{

	while (true)
	{
		
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
			

//			for (size_t i = 0; i < windows.Count(); ++i)
			{
				OgPlayWindow& window = *_handle;
				if (!window.ShouldClose())
				{
					window.NextFrame();
					window.Update();
					window.Present();
					_renderContext->Collect();
				}
			}

		}
		else
		{
			break;
		}

	}

}

void OgSampleMain::finalImGUIContext()
{
	OgImGuiContextManager::Finalize();
}

void OgSampleMain::finalRenderContext()
{

	_renderContext->Shutdown();
	delete _renderContext;
		


}


OG_NAMESPACE_SAMPLE_END