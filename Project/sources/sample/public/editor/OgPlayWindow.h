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
#include "system/OgNativeWindow.h"
#include "system/OgNativeEvent.h"
#include "system/OgImGUIManager.h"
#include "system/thirdparty/imgui/imgui.h"

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

		// ImGui 렌더러 초기화
		//_imguiRenderer = new OgImguiRenderer(renderContext);
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
	}

	void onDestroy()
	{
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
		ImGuiIO& io = context->IO;
		io.DisplaySize.x = static_cast<float>(GetWidth());
		io.DisplaySize.y = static_cast<float>(GetHeight());

		// TODO: 단지 테스트 코드!!
		ImGui::NewFrame();
		//ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x * 0.8f, io.DisplaySize.y * 0.8f), ImGuiCond_FirstUseEver);
		//ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.1f, io.DisplaySize.y * 0.1f), ImGuiCond_FirstUseEver);
		// ImGui 윈도우 시작
		ImGui::Begin("Triangle Render Target", nullptr, ImGuiWindowFlags_None);

		// 디버깅: ImGui 레이아웃 정보 수집 및 확인
		ImVec2 windowSize = ImGui::GetWindowSize();
		ImVec2 cursorPos = ImGui::GetCursorPos();
		ImVec2 contentRegionAvail = ImGui::GetContentRegionAvail();
		ImVec2 contentRegionMax = ImGui::GetWindowContentRegionMax();
		ImVec2 contentRegionMin = ImGui::GetWindowContentRegionMin();
		ImGuiStyle& style = ImGui::GetStyle();
		ImVec2 windowPadding = style.WindowPadding;
		float titleBarHeight = ImGui::GetTextLineHeightWithSpacing();
		
		// 디버깅 정보 출력
		ImGui::Text("Debug Info:");
		ImGui::Text("Window Size: %.1f x %.1f", windowSize.x, windowSize.y);
		ImGui::Text("Cursor Pos: %.1f, %.1f", cursorPos.x, cursorPos.y);
		ImGui::Text("Content Avail: %.1f x %.1f", contentRegionAvail.x, contentRegionAvail.y);
		ImGui::Text("Content Max: %.1f x %.1f", contentRegionMax.x, contentRegionMax.y);
		ImGui::Text("Content Min: %.1f x %.1f", contentRegionMin.x, contentRegionMin.y);
		ImGui::Text("Window Padding: %.1f x %.1f", windowPadding.x, windowPadding.y);
		ImGui::Text("Title Bar Height: %.1f", titleBarHeight);
		ImGui::Separator();
		
		// 렌더 타겟의 실제 크기
		float renderTargetWidth = static_cast<float>(_triangleSample.GetRenderTargetWidth());
		float renderTargetHeight = static_cast<float>(_triangleSample.GetRenderTargetHeight());
		ImGui::Text("Render Target: %.0f x %.0f", renderTargetWidth, renderTargetHeight);
		
		// 사용 가능한 영역 계산 (간단한 방법 사용)
		ImVec2 imageSize;
		if (renderTargetWidth > 0 && renderTargetHeight > 0 && contentRegionAvail.x > 0 && contentRegionAvail.y > 0) {
			// GetContentRegionAvail()로 간단하게 계산
			float availWidth = contentRegionAvail.x - 20.0f; // 약간의 여백
			float availHeight = contentRegionAvail.y - 100.0f; // 디버깅 텍스트를 위한 공간
			
			if (availWidth > 0 && availHeight > 0) {
				float scaleX = availWidth / renderTargetWidth;
				float scaleY = availHeight / renderTargetHeight;
				float scale = std::min(scaleX, scaleY);
				scale = std::min(scale, 1.0f); // 확대 방지
				
				imageSize.x = renderTargetWidth * scale;
				imageSize.y = renderTargetHeight * scale;
				
				ImGui::Text("Scale: %.3f (X:%.3f, Y:%.3f)", scale, scaleX, scaleY);
				ImGui::Text("Final Image Size: %.1f x %.1f", imageSize.x, imageSize.y);
			} else {
				imageSize.x = 200.0f;
				imageSize.y = 150.0f;
				ImGui::Text("Using fallback size (insufficient space)");
			}
		} else {
			imageSize.x = 200.0f;
			imageSize.y = 150.0f;
			ImGui::Text("Using fallback size (not ready)");
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
		//ImGui::Render();
		_triangleSample.OnRender(_encoders[_submitIndex], _swapchain);
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
	// Command encoders for triple buffering
	std::vector<Render::OgCommandEncoderHandle*> _encoders;
	uint32 _submitIndex;
	// TODO
	//LvAssetBundle::Finder _finder;
};



OG_NAMESPACE_SAMPLE_END

#endif // _OG_PLAY_WINDOW_H__