#pragma once
#ifndef _OG_PLAY_WINDOW_H__
#define _OG_PLAY_WINDOW_H__
#include "OgPrecompile.h"

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <vector>
#include <cstring>

#include "system/OgSystemContext.h"
#include "system/OgNativeWindow.h"
#include "system/OgNativeEvent.h"
#include "system/OgImGUIManager.h"
#include "system/thirdparty/imgui/imgui.h"
#include "system/thirdparty/imgui/backends/imgui_impl_win32.h"

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

#include "render/OgRenderContext.h"
#include "sample/public/editor/OgImguiRenderer.h"
#include "sample/public/core/OgTriangle.h"


const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

OG_NAMESPACE_SAMPLE_BEGIN

class OG_API OgPlayWindow
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
		, _triangleSample(renderContext)
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


		onUpdate();

		if (_updatable && _presentable)
		{
			prepare(_delta);
			render(_delta);
		}
		

		

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
		Render::OgSwapChainInfo scInfo;
		scInfo.useDepthBuffer = true;
		scInfo.useStencilBuffer = false;
		scInfo.depthBufferFormat = Render::OgRenderTextureFormat::DEFAULT_DEPTH;

		_swapchain = _renderContext->CreateSwapchain(_handle, scInfo);
		_triangleSample.OnInit(_swapchain);
		//_renderContext->AcquireNextImageIndex(_swapchain);
		_submitIndex = 0;
		_encoders.resize(_renderContext->maxSubmitCount);
		for (int i = 0; i < _renderContext->maxSubmitCount; ++i)
		{
			_encoders[i]= _renderContext->CreateCommandEncoder();
			
		}

		// ImGui 렌더러 초기화
		_imguiRenderer = new OgImguiRenderer(_renderContext);
		
		// ImGui Win32 백엔드 초기화
		// 기존 ImGui 컨텍스트 가져오기
		ImGuiContext* context = static_cast<ImGuiContext*>(OgImGuiContextManager::GetMainImGuiContext());
		if (context)
		{
			// 현재 컨텍스트로 설정
			ImGui::SetCurrentContext(context);
			
			// Win32 백엔드 초기화
			if (ImGui_ImplWin32_Init(_handle->win32.handle))
			{
				_imguiWin32Initialized = true;
			}
			else
			{
				LOGE(OG_ID, "Failed to initialize ImGui Win32 backend");
			}
		}
		else
		{
			LOGE(OG_ID, "ImGui context is not initialized. Please call OgImGuiContextManager::Initialize() first.");
		}
	}

	void onDestroy()
	{
		// ImGui Win32 백엔드 정리
		if (_imguiWin32Initialized)
		{
			ImGuiContext* context = static_cast<ImGuiContext*>(OgImGuiContextManager::GetMainImGuiContext());
			if (context)
			{
				ImGui::SetCurrentContext(context);
				ImGui_ImplWin32_Shutdown();
			}
			_imguiWin32Initialized = false;
		}
		
		for (int i = 0; i < _renderContext->maxSubmitCount; ++i)
		{
			_renderContext->DestroyCommandEncoder(_encoders[i]);
		}

		_encoders.clear();

		if (_imguiRenderer)
		{
			delete _imguiRenderer;
			_triangleSample.OnDestroy();
			_renderContext->DestroySwapchain(_swapchain);
		}

		_triangleSample.OnDestroy();
		_renderContext->DestroySwapchain(_swapchain);

	}

	void onGUI()
	{
		// TODO
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
		Position position = GetPosition();
		//_surface->rect.x = static_cast<float>(position.x);
		//_surface->rect.y = static_cast<float>(position.y);
		//_surface->rect.width = static_cast<float>(GetWidth());
		//_surface->rect.height = static_cast<float>(GetHeight());

		//s_inputHandler->UpdateArea(_surface->rect);
		
		// 윈도우 크기 변경 시 ImGui DisplaySize도 업데이트
		ImGuiContext* context = static_cast<ImGuiContext*>(OgImGuiContextManager::GetMainImGuiContext());
		if (context) 
		{
			ImGuiIO& io = context->IO;
			io.DisplaySize.x = static_cast<float>(GetWidth());
			io.DisplaySize.y = static_cast<float>(GetHeight());
		}

		restore();
	}

	void onPrepare(float deltaTime)
	{
	
	ImGuiContext* context = static_cast<ImGuiContext*>(OgImGuiContextManager::GetMainImGuiContext());
	if (!context)
	{
		LOGE(OG_ID, "ImGui context is null in onPrepare");
		return;
	}
	
	ImGui::SetCurrentContext(context);
	
	ImGuiIO& io = context->IO;
	io.DisplaySize.x = static_cast<float>(GetWidth());
	io.DisplaySize.y = static_cast<float>(GetHeight());

	// ImGui Win32 프레임 시작 - 초기화가 되어 있는 경우에만
	if (_imguiWin32Initialized)
	{
		ImGui_ImplWin32_NewFrame();
	}
	
	// TODO: 단지 테스트 코드!!
	ImGui::NewFrame();
	
	// ImGui 윈도우 크기를 화면의 절반 정도로 설정
	ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x * 0.6f, io.DisplaySize.y * 0.6f), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.2f, io.DisplaySize.y * 0.2f), ImGuiCond_FirstUseEver);
		
	// ImGui 윈도우 시작
	ImGui::Begin("Triangle Render Target", nullptr, ImGuiWindowFlags_None);

	// 렌더 타겟의 실제 크기
	float renderTargetWidth = static_cast<float>(_triangleSample.GetRenderTargetWidth());
	float renderTargetHeight = static_cast<float>(_triangleSample.GetRenderTargetHeight());
	
	// 사용 가능한 영역 계산
	ImVec2 contentRegionAvail = ImGui::GetContentRegionAvail();
	ImVec2 imageSize;
	
	if (renderTargetWidth > 0 && renderTargetHeight > 0 && contentRegionAvail.x > 0 && contentRegionAvail.y > 0) {
	 // 비율을 유지하면서 창에 맞게 크기 조정
	 float scaleX = contentRegionAvail.x / renderTargetWidth;
	 float scaleY = contentRegionAvail.y / renderTargetHeight;
	 float scale = std::min(scaleX, scaleY);
	 
	 // 크기 제한 (너무 작거나 크지 않도록)
	 scale = std::max(scale, 0.1f);  // 최소 10%
	 scale = std::min(scale, 2.0f);  // 최대 200%
	 
	 imageSize.x = renderTargetWidth * scale;
	 imageSize.y = renderTargetHeight * scale;
	 
	 // 디버깅 정보
	 ImGui::Text("Render Target: %.0f x %.0f", renderTargetWidth, renderTargetHeight);
	 ImGui::Text("Available Space: %.0f x %.0f", contentRegionAvail.x, contentRegionAvail.y);
	 ImGui::Text("Scale: %.2f%% (%.0f x %.0f)", scale * 100.0f, imageSize.x, imageSize.y);
	 ImGui::Separator();
	} else {
	// 기본 크기 사용
	imageSize.x = 400.0f;
	imageSize.y = 300.0f;
	ImGui::Text("Using default size");
	}

	// 이미지를 중앙에 배치
	float centerX = (contentRegionAvail.x - imageSize.x) * 0.5f;
	if (centerX > 0) {
	ImGui::SetCursorPosX(ImGui::GetCursorPosX() + centerX);
	}
	
	// ImGui에 이미지로 텍스쳐 표시 - 외부 텍스처를 ImTextureID로 사용
	ImGui::Image((ImTextureID)_triangleSample.GetRenderTargetTexture(), imageSize);
	ImGui::End();

	// 변경된 ImGui 커맨드 리스트로 DrawData 업데이트
	ImGui::Render();
	 
	// ImGui 업데이트
	_imguiRenderer->UpdateGPUContext(context);
	_imguiRenderer->UpdateSurface(_swapchain);
	
	// 삼각형 렌더 타겟 텍스쳐를 ImGui 렌더러에 외부 텍스쳐로 설정
		// 이제 삼각형이 스왑 체인이 아닌 렌더 타겟에 그려지고, 렌더 타겟은 ImGui에 텍스쳐로 표시됩니다
	_imguiRenderer->SetExternalTexture(_triangleSample.GetRenderTargetTexture());
	}

	void onRender(float deltaTime)
	{
		ImGuiContext* context = static_cast<ImGuiContext*>(OgImGuiContextManager::GetMainImGuiContext());
		_encoders[_submitIndex]->Begin();
		
		// 먼저 삼각형을 렌더 타겟에 렌더링
		_triangleSample.OnRender(_encoders[_submitIndex], _swapchain);
		
		// 그 다음 ImGui를 스왑체인에 렌더링
		ImDrawData* drawData = ImGui::GetDrawData();
		if (drawData)
		{
			OgRenderParam param;
			param.drawList = drawData;
			param.surface = _swapchain;
			std::hash<ImGuiContext*> hash_fn;
			size_t hash_value = hash_fn(context);
			param.guiContextKey = hash_value;  // 메인 컨텍스트는 0
			_imguiRenderer->RenderGUI(_encoders[_submitIndex], param);
		}

		_encoders[_submitIndex]->End();
		
		// 모든 렌더링 커맨드를 한 번에 Submit
		_renderContext->Submit(_swapchain, _encoders[_submitIndex]);
	}

	void onPresent()
	{
		//_renderContext->AcquireNextImageIndex(_swapchain);
		_triangleSample.OnPresent(_swapchain,_presentable);
		
	}

	void onNextFrame()
	{
		_triangleSample.OnNextFrame(_presentable);
		_imguiRenderer->NextFrame(_encoders[_submitIndex], _swapchain);
		_submitIndex = (_submitIndex + 1) % _encoders.size();

	}

	void onRestore()
	{
		_triangleSample.OnSuspend(_swapchain);
		_triangleSample.OnRestore(_swapchain);
		
	}

	virtual void onOpen()
	{
		
	}

	virtual void onClose()
	{
		
	}

	void onDropFiles(const std::vector<std::string>& absolutePaths) {}
		

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
	
	OgImguiRenderer* _imguiRenderer;

	OgTriangle _triangleSample;

	

	std::string _title;

	OgPlayWindow* _parent;

	OgNativeWindow* _handle;
	
	

	uint32 _tick;

	float _delta;

	bool _updatable;
	bool _presentable;
	bool _shouldCloseNextFrame;
	bool _imguiWin32Initialized = false;
	// Command encoders for triple buffering
	std::vector<Render::OgCommandEncoderHandle*> _encoders;
	uint32 _submitIndex;
	// TODO
	//LvAssetBundle::Finder _finder;
};



OG_NAMESPACE_SAMPLE_END

#endif // _OG_PLAY_WINDOW_H__