#include "OgEditorWindow.h"
#include "system/OgSystemContext.h"
#include "system/OgInput.h"
#include <unordered_map>

OG_NAMESPACE_SAMPLE_BEGIN

// 윈도우포인터로 EditorWindow 객체 찾기 위한 맵
static std::unordered_map<OgNativeWindow*, OgEditorWindow*> s_windowMap;

OgEditorWindow::OgEditorWindow(Render::OgRenderContext* renderContext, const Config& config)
	: _renderContext(renderContext)
	, _nativeWindow(nullptr)
	, _swapchain(nullptr)
	, _currentEncoderIndex(0)
	, _isOpen(false)
	, _shouldClose(false)
	, _canRender(false)
	, _config(config)
{
}

OgEditorWindow::~OgEditorWindow()
{
	if (_isOpen)
	{
		Close();
	}
}

void OgEditorWindow::Open()
{
	if (_isOpen)
		return;

	// 네이티브 윈도우 생성
	OgFrameBufferConfig fbConfig;
	OgWindowConfig wConfig;
	memset(&fbConfig, 0, sizeof(OgFrameBufferConfig));
	memset(&wConfig, 0, sizeof(OgWindowConfig));

	fbConfig.redBits = 8;
	fbConfig.greenBits = 8;
	fbConfig.blueBits = 8;
	fbConfig.alphaBits = 8;

	wConfig.width = _config.width;
	wConfig.height = _config.height;
	wConfig.title = _config.title.c_str();
	wConfig.resizable = _config.resizable;
	wConfig.decorated = _config.decorated;
	wConfig.focused = false;
	wConfig.autoIconify = true;
	wConfig.floating = false;
	wConfig.share = _config.parent ? _config.parent->_nativeWindow : nullptr;

	_nativeWindow = og_window_create(og_system_get_context(), &wConfig, &fbConfig);
	if (!_nativeWindow)
	{
		LOGE(OG_ID, "Failed to create native window");
		return;
	}
	
	// Input 콜백 설정
	setupInputCallbacks();

	// 스왑체인 생성
	createSwapchain();
	
	// 커맨드 인코더 생성
	createCommandEncoders();
	
	// 서브클래스 초기화
	onInit();
	
	// 윈도우 표시
	og_window_show(_nativeWindow);
	og_window_focus_in(_nativeWindow);
	
	_isOpen = true;
	_canRender = true;
	
	onOpen();
}

void OgEditorWindow::Close()
{
	if (!_isOpen)
		return;
		
	onClose();
	
	_canRender = false;
	_isOpen = false;
	
	// 서브클래스 정리
	onDestroy();
	
	// 커맨드 인코더 정리
	destroyCommandEncoders();
	
	// 스왑체인 정리
	destroySwapchain();
	
	// 네이티브 윈도우 정리
	if (_nativeWindow)
	{
		// 윈도우 맵에서 제거
		s_windowMap.erase(_nativeWindow);
		
		og_window_destroy(_nativeWindow);
		_nativeWindow = nullptr;
	}
}

bool OgEditorWindow::ShouldClose() const
{
	return _shouldClose || (_nativeWindow && _nativeWindow->shouldClose);
}

void OgEditorWindow::PeekEvents()
{
	if (!_nativeWindow)
		return;
		
	OgNativeEvent evt;
	while (og_window_event_poll(_nativeWindow, &evt))
	{
		processEvent(evt);
	}
	
	// 최소화되지 않았을 때만 다음 이미지 획득
	if (!_nativeWindow->minimized)
	{
		_renderContext->AcquireNextImageIndex(_swapchain);
	}
}

void OgEditorWindow::Update(float deltaTime)
{
	if (!_canRender)
		return;
		
	onUpdate(deltaTime);
}

void OgEditorWindow::Render()
{
	if (!_canRender || !_swapchain)
		return;
		
	auto encoder = _encoders[_currentEncoderIndex];
	encoder->Begin();
	
	onRender(encoder);
	
	encoder->End();
	
	_renderContext->Submit(_swapchain, encoder);
}

void OgEditorWindow::Present()
{
	if (!_canRender || !_swapchain)
		return;
		
	_renderContext->Present(_swapchain);
	_currentEncoderIndex = (_currentEncoderIndex + 1) % _encoders.size();
}

void OgEditorWindow::processEvent(const OgNativeEvent& evt)
{
	switch (evt.type)
	{
	case OG_WINDOW_RESTORE:
	case OG_WINDOW_RESIZED:
		_canRender = true;
		onResize(GetWidth(), GetHeight());
		break;
		
	case OG_WINDOW_MAXMIZE:
		_canRender = true;
		onResize(GetWidth(), GetHeight());
		break;
		
	case OG_WINDOW_MINIMIZE:
		_canRender = false;
		break;
		
	case OG_WINDOW_RESIZING:
		_canRender = false;
		break;
		
	case OG_DROP_FILES:
		// 드롭 파일 처리
		break;
	}
	
	// 서브클래스에 이벤트 전달
	onEvent(evt);
}

void OgEditorWindow::createSwapchain()
{
	Render::OgSwapChainInfo scInfo;
	scInfo.useDepthBuffer = true;
	scInfo.useStencilBuffer = false;
	scInfo.depthBufferFormat = Render::OgRenderTextureFormat::DEFAULT_DEPTH;

	_swapchain = _renderContext->CreateSwapchain(_nativeWindow, scInfo);
}

void OgEditorWindow::destroySwapchain()
{
	if (_swapchain)
	{
		_renderContext->DestroySwapchain(_swapchain);
		_swapchain = nullptr;
	}
}

void OgEditorWindow::createCommandEncoders()
{
	_encoders.resize(_renderContext->maxSubmitCount);
	for (int i = 0; i < _renderContext->maxSubmitCount; ++i)
	{
		_encoders[i] = _renderContext->CreateCommandEncoder();
	}
	_currentEncoderIndex = 0;
}

void OgEditorWindow::destroyCommandEncoders()
{
	for (auto encoder : _encoders)
	{
		_renderContext->DestroyCommandEncoder(encoder);
	}
	_encoders.clear();
}



void OgEditorWindow::setupInputCallbacks()
{
	if (!_nativeWindow)
		return;
		
	// 윈도우 맵에 등록
	s_windowMap[_nativeWindow] = this;
	
	// Input 콜백 설정
	_nativeWindow->input.onKey = &OgEditorWindow::onKeyCallback;
	_nativeWindow->input.onCharacterInput = &OgEditorWindow::onCharacterCallback;
	_nativeWindow->input.onCharacterModsInput = &OgEditorWindow::onCharacterModsCallback;
	_nativeWindow->input.onMouseButton = &OgEditorWindow::onMouseButtonCallback;
	_nativeWindow->input.onCursorPos = &OgEditorWindow::onCursorPosCallback;
}

void OgEditorWindow::onKeyCallback(OgNativeWindow* window, int keyCode, int scanCode, int action, int modifierKeys)
{
	// EditorWindow 객체 찾기
	auto it = s_windowMap.find(window);
	if (it == s_windowMap.end())
		return;
		
	OgEditorWindow* editorWindow = it->second;
	
	// 이벤트 생성
	OgNativeEvent evt;
	evt.type = (action == OG_PRESS || action == OG_REPEAT) ? OG_KEY_PRESS : OG_KEY_RELEASE;
	evt.key.keyCode = keyCode;
	evt.key.alt = (modifierKeys & OG_MOD_ALT) != 0;
	evt.key.control = (modifierKeys & OG_MOD_CONTROL) != 0;
	evt.key.shift = (modifierKeys & OG_MOD_SHIFT) != 0;
	evt.key.system = (modifierKeys & OG_MOD_SUPER) != 0;
	
	// onEvent 호출
	editorWindow->onEvent(evt);
}

void OgEditorWindow::onCharacterCallback(OgNativeWindow* window, unsigned int codepoint)
{
	// 필요한 경우 구현
}

void OgEditorWindow::onCharacterModsCallback(OgNativeWindow* window, unsigned int codepoint, int modifierKeys)
{
	// 필요한 경우 구현
}

void OgEditorWindow::onMouseButtonCallback(OgNativeWindow* window, int button, int action, int modifierKeys)
{
	// EditorWindow 객체 찾기
	auto it = s_windowMap.find(window);
	if (it == s_windowMap.end())
		return;
		
	OgEditorWindow* editorWindow = it->second;
	
	// 이벤트 생성
	OgNativeEvent evt;
	evt.type = (action == OG_PRESS) ? OG_MOUSE_PRESS : OG_MOUSE_RELEASE;
	evt.mouse.button = button;
	evt.mouse.mods = modifierKeys;
	evt.mouse.pos.x = static_cast<int>(window->input.virtualCursorPosX);
	evt.mouse.pos.y = static_cast<int>(window->input.virtualCursorPosY);
	evt.mouse.delta.x = 0;
	evt.mouse.delta.y = 0;
	evt.mouse.wheelDelta = 0;
	
	// onEvent 호출
	editorWindow->onEvent(evt);
}

void OgEditorWindow::onCursorPosCallback(OgNativeWindow* window, double xpos, double ypos)
{
	// EditorWindow 객체 찾기
	auto it = s_windowMap.find(window);
	if (it == s_windowMap.end())
		return;
		
	OgEditorWindow* editorWindow = it->second;
	
	// 첫 번째 마우스 이동 처리
	if (editorWindow->_firstMouse)
	{
		editorWindow->_lastMouseX = xpos;
		editorWindow->_lastMouseY = ypos;
		editorWindow->_firstMouse = false;
		return;
	}
	
	// 이벤트 생성
	OgNativeEvent evt;
	evt.type = OG_MOUSE_MOVE;
	evt.mouse.button = -1;
	evt.mouse.mods = 0;
	evt.mouse.pos.x = static_cast<int>(xpos);
	evt.mouse.pos.y = static_cast<int>(ypos);
	evt.mouse.delta.x = static_cast<int>(xpos - editorWindow->_lastMouseX);
	evt.mouse.delta.y = static_cast<int>(ypos - editorWindow->_lastMouseY);
	evt.mouse.wheelDelta = 0;
	
	editorWindow->_lastMouseX = xpos;
	editorWindow->_lastMouseY = ypos;
	
	// onEvent 호출
	editorWindow->onEvent(evt);
}

OG_NAMESPACE_SAMPLE_END
