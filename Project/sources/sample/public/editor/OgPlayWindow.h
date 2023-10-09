#pragma once
#ifndef _OG_PLAY_WINDOW_H__
#define _OG_PLAY_WINDOW_H__
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

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

OG_NAMESPACE_SAMPLE_BEGIN

class OgPlayWindow
{
public:

	struct Position
	{
		uint32 x;
		uint32 y;

		Position(uint32 x, uint32 y)
			: x(x)
			, y(y)
		{}
	};

	OgPlayWindow() = delete;

	OgPlayWindow(Render::OgRenderContext* renderContext, const char* name, uint32_t width, uint32 height, OgPlayWindow* parent = nullptr)
		: _title(name)
		, _parent(parent)
		, _tick(0)
		, _delta(0.f)
		, _updatable(false)
		, _presentable(false)
		, _shouldCloseNextFrame(false)
		, _renderContext(renderContext)
	{
		OgNativeWindow* nativeParent = nullptr;
		if (_parent != nullptr)
		{
			nativeParent = _parent->_handle;
		}

		OgFrameBufferConfig fbConfig;
		OgWindowConfig wConfig;
		memset(&fbConfig, 0, sizeof(OgFrameBufferConfig));
		memset(&wConfig, 0, sizeof(OgWindowConfig));

		fbConfig.redBits = 8;
		fbConfig.greenBits = 8;
		fbConfig.blueBits = 8;
		fbConfig.alphaBits = 8;

		wConfig.width = width;
		wConfig.height = height;
		wConfig.title = name;
		wConfig.resizable = true;
		wConfig.decorated = true;
		wConfig.focused = false;
		wConfig.autoIconify = true;
		wConfig.floating = false;
		wConfig.share = nativeParent;

		_handle = og_window_create(og_system_get_context(), &wConfig, &fbConfig);

		if (!_handle)
		{
			//OG_LOG(error, "Can't create Window");
		}

		// TODO: inputHandler 작업
		//s_inputHandler = OgInput::GetHandler();
		//s_inputHandler->Init(LvVec2f(0, 0), OgRect(0, 0, width, height), &_handle->input);
		//OgInputManager::Bind(_handle);
		//OgNativeEventHandler::Bind(&s_system);
		onInit();
	}

	~OgPlayWindow()
	{
		og_window_destroy(_handle);

		onDestroy();
	}

	// Editor Run Method
	void PeekEvent()
	{
		OgNativeEvent evt;
		while (og_window_event_poll(_handle, &evt))
		{
			switch (evt.type)
			{
			case OG_WINDOW_RESTORE:
			case OG_WINDOW_RESIZED:
			{
				_updatable = true;
				_presentable = true;
				onResize();
				break;
			}
			case OG_WINDOW_MAXMIZE:
			{
				_updatable = true;
				_presentable = true;
				onResize();
				break;
			}
			case OG_WINDOW_MINIMIZE:
			{
				_presentable = false;
				_updatable = false;

				//renderModule->GetRenderContext()->WaitDeviceIdle();
				break;
			}
			case OG_WINDOW_RESIZING:
			{
				_presentable = false;
				_updatable = false;
				break;
			}
			case OG_DROP_FILES:
			{
				std::vector<std::string>* paths = (std::vector<std::string>*)(evt.drop.paths);
				onDropFiles(*paths);
				break;
			}
			}
		}

		bool shouldUpdateDelta = false;

		if (_handle->minimized == false)
		{
			_renderContext->AcquireNextImageIndex(_swapchain);
			shouldUpdateDelta = true;
		}

		if (shouldUpdateDelta)
		{
			// TODO: @osgood make OgTime.h
			//uint32 current = lv_time_milli();
			//_delta = (current - _tick) / 1000.0f;
			//_tick = current;
		}
	}


	// Editor Run Method
	void Update()
	{
		Render::OgCommandEncoderHandle* encoder = getSubmitEncorder();
		encoder->Begin();

		onUpdate();

		if (_updatable && _presentable)
		{
			prepare(_delta);
			render(_delta);
		}
		encoder->End();

		_renderContext->Submit(_swapchain, encoder);

		// TODO: @osgood inputHandler작업
		//s_inputHandler->Update();
	}

	void NextFrame()
	{
		onNextFrame();
	}

	void Present()
	{
		present();
	}

	void Open()
	{
		//set Bundle Object Finder
		// TODO
		//Engine::LvObjectAddress::SetFinder(&_finder);

		og_window_show(_handle);
		og_window_focus_in(_handle);

		// TODO: @osgood make time.h
		//_tick = og_time_milli();

		_updatable = true;
		_presentable = true;

		onOpen();
	}

	void Close()
	{
		onClose();

		_presentable = false;
		_updatable = false;

		_tick = 0;

		//Todo: @osgood
		//og_window_focus_out(_handle);

		//set Bundle Object Finder
		// TODO: @osgood
		//Engine::OgObjectAddress::SetFinder(nullptr);
	}

	void CloseNextFrame()
	{
		_shouldCloseNextFrame = true;
	}

	OG_FORCEINLINE OgNativeWindow* GetNativeWindow() { return _handle; }

	bool ShouldClose() const { return ((_shouldCloseNextFrame == true) || _handle->shouldClose); }

	Position GetPosition() const { return Position(_handle->x, _handle->y); }

	uint32 GetWidth() const { return _handle->width; }

	uint32 GetHeight() const { return _handle->height; }


protected:

	void onInit()
	{
		for (size_t i = 0; i < _renderContext->maxSubmitCount; ++i)
		{
			_encoders.push_back(_renderContext->CreateCommandEncoder());
		}

		Render::OgSwapChainInfo scInfo;
		scInfo.useDepthBuffer = true;
		scInfo.useStencilBuffer = false;
		scInfo.depthBufferFormat = Render::OgRenderTextureFormat::DEFAULT_DEPTH;

		_swapchain = _renderContext->CreateSwapchain(_handle, scInfo);

	}

	void onDestroy()
	{
		_renderContext->DestroySwapchain(_swapchain);
		for (int i = 0; i < _renderContext->maxSubmitCount; ++i)
		{
			_renderContext->DestroyCommandEncoder(_encoders[i]);
		}

		_encoders.clear();

	}

	void onGUI()
	{

	}

	void onUpdate()
	{
		// TODO: @osgood time.h
		//double totalStart = lv_time_milli();
		float delta = _delta;


		onGUI();


	}

	void onResize()
	{
		_renderContext->Suspend(_swapchain);
		_renderContext->Restore(_swapchain);

		Position position = GetPosition();
		//_surface->rect.x = static_cast<float>(position.x);
		//_surface->rect.y = static_cast<float>(position.y);
		//_surface->rect.width = static_cast<float>(GetWidth());
		//_surface->rect.height = static_cast<float>(GetHeight());

		//s_inputHandler->UpdateArea(_surface->rect);

		restore();
	}

	void onPrepare(float deltaTime)
	{
		//app_prepare(deltaTime);
	}

	void onRender(float deltaTime)
	{
		//app_render(deltaTime);
	}

	void onPresent()
	{
		if (_presentable)
		{
			//app_present();
			_renderContext->Present(_swapchain);
		}
	}

	void onNextFrame()
	{
		if (_presentable)
		{
			_submitIndex = (_submitIndex + 1) % _renderContext->maxSubmitCount;
		}
	}

	void onRestore()
	{
		//app_restore();
	}

	virtual void onOpen()
	{
		//app_init();
	}

	virtual void onClose()
	{
		//app_destroy();
	}

	void onDropFiles(const std::vector<std::string>& absolutePaths) {}

	Render::OgCommandEncoderHandle* getSubmitEncorder() { return _encoders[_submitIndex]; }

	void prepare(float deltaTime)
	{
		onPrepare(deltaTime);
	}

	void render(float deltaTime)
	{
		onRender(deltaTime);
	}

	void present()
	{
		onPresent();
	}

	void restore()
	{
		onRestore();
	}

	using LvGUIContext = void*;

	Render::OgSwapChain* _swapchain;

	Render::OgRenderContext* _renderContext;

	uint32 _submitIndex = 0;

	std::string _title;

	OgPlayWindow* _parent;

	OgNativeWindow* _handle;

	std::vector<Render::OgCommandEncoderHandle*> _encoders;

	uint32 _tick;

	float _delta;

	bool _updatable;
	bool _presentable;
	bool _shouldCloseNextFrame;

	// TODO
	//LvAssetBundle::Finder _finder;
};



OG_NAMESPACE_SAMPLE_END

#endif // _OG_PLAY_WINDOW_H__