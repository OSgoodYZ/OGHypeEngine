#include "OgPlayWindow.h"
#include "sample/public/core/OgTriangleSample.h"
#include "sample/public/core/OgFBXSample.h"
#include "system/OgSystemContext.h"
#include "system/OgInput.h"
#include <algorithm>
OG_NAMESPACE_SAMPLE_BEGIN

OgPlayWindow::OgPlayWindow(Render::OgRenderContext* renderContext, const Config& config)
	: OgEditorWindow(renderContext, config)
{
}

OgPlayWindow::~OgPlayWindow()
{
	// 자동으로 정리됨
}

void OgPlayWindow::SetSample(std::unique_ptr<OgSampleBase> sample)
{
	if (_currentSample)
	{
		_currentSample->OnDestroy();
	}
	
	_currentSample = std::move(sample);
	
	if (_currentSample && _swapchain)
	{
		_currentSample->OnInit(_swapchain);
	}
}

void OgPlayWindow::onInit()
{
	// ImGui 렌더러 초기화
	_imguiRenderer = std::make_unique<OgImguiRenderer>(_renderContext);
	
	// ImGui Win32 백엔드 초기화
	initImGui();
	
	// 샘플 초기화
	if (_currentSample && _swapchain)
	{
		_currentSample->OnInit(_swapchain);
	}
}

void OgPlayWindow::onDestroy()
{
	// 샘플 정리
	if (_currentSample)
	{
		_currentSample->OnDestroy();
		_currentSample.reset();
	}
	
	// ImGui 정리
	shutdownImGui();
	
	// ImGui 렌더러 정리
	_imguiRenderer.reset();
}

void OgPlayWindow::onUpdate(float deltaTime)
{
	// 샘플 업데이트
	if (_currentSample)
	{
		_currentSample->OnUpdate(deltaTime);
	}
}

void OgPlayWindow::onRender(Render::OgCommandEncoderHandle* encoder)
{
	// 1. 샘플 렌더링 (렌더 타겟에)
	if (_currentSample)
	{
		_currentSample->OnRender(encoder, _swapchain);
	}
	
	// 2. ImGui 프레임 시작
	beginImGuiFrame();
	
	// 3. UI 렌더링
	onRenderUI();
	
	// 4. ImGui 프레임 종료
	endImGuiFrame();
	
	// 5. ImGui 렌더링 (스왑체인에)
	renderImGui(encoder);
}

void OgPlayWindow::onResize(uint32 width, uint32 height)
{
	_renderContext->Suspend(_swapchain);
	_imguiRenderer->RemoveSurface(_swapchain);

	// ImGui 디스플레이 크기 업데이트
	ImGuiContext* context = static_cast<ImGuiContext*>(OgImGuiContextManager::GetMainImGuiContext());
	if (context) 
	{
		ImGuiIO& io = context->IO;
		io.DisplaySize.x = static_cast<float>(width);
		io.DisplaySize.y = static_cast<float>(height);
	}
	
	// 샘플 리사이즈
	if (_currentSample)
	{
		_currentSample->OnResize(width, height);
	}
	_renderContext->Restore(_swapchain);
}

void OgPlayWindow::onEvent(const OgNativeEvent& evt)
{
	// ImGui가 입력을 처리하는 경우 샘플로 전달하지 않음
	ImGuiContext* context = static_cast<ImGuiContext*>(OgImGuiContextManager::GetMainImGuiContext());
	if (!context)
		return;
		
	ImGui::SetCurrentContext(context);
	ImGuiIO& io = ImGui::GetIO();
	
	switch (evt.type)
	{
	case OG_KEY_PRESS:
	case OG_KEY_RELEASE:
		{
			// ImGui가 키보드 입력을 사용하지 않는 경우에만 전달
			if (!io.WantCaptureKeyboard && _currentSample)
			{
				// OgFBXSample로 다운캐스트 시도
				OgFBXSample* fbxSample = dynamic_cast<OgFBXSample*>(_currentSample.get());
				if (fbxSample)
				{
					int action = (evt.type == OG_KEY_PRESS) ? OG_PRESS : OG_RELEASE;
					int mods = 0;
					if (evt.key.shift) mods |= OG_MOD_SHIFT;
					if (evt.key.control) mods |= OG_MOD_CONTROL;
					if (evt.key.alt) mods |= OG_MOD_ALT;
					if (evt.key.system) mods |= OG_MOD_SUPER;
					
					fbxSample->OnKeyPress(evt.key.keyCode, action, mods);
				}
			}
		}
		break;
		
	case OG_MOUSE_PRESS:
	case OG_MOUSE_RELEASE:
		{
			// ImGui가 마우스 입력을 사용하지 않는 경우에만 전달
			if (_currentSample)// !io.WantCaptureMouse && 
			{
				OgFBXSample* fbxSample = dynamic_cast<OgFBXSample*>(_currentSample.get());
				if (fbxSample)
				{
					int action = (evt.type == OG_MOUSE_PRESS) ? OG_PRESS : OG_RELEASE;
					fbxSample->OnMouseButton(evt.mouse.button, action, evt.mouse.mods);
				}
			}
		}
		break;
		
	case OG_MOUSE_MOVE:
		{
			if (_currentSample) // !io.WantCaptureMouse && 
			{
				OgFBXSample* fbxSample = dynamic_cast<OgFBXSample*>(_currentSample.get());
				if (fbxSample)
				{
					fbxSample->OnMouseMove(evt.mouse.pos.x, evt.mouse.pos.y);
				}
			}
		}
		break;
		
	case OG_MOUSE_WHEEL_CHANGE:
		{
			if (_currentSample) // !io.WantCaptureMouse && 
			{
				OgFBXSample* fbxSample = dynamic_cast<OgFBXSample*>(_currentSample.get());
				if (fbxSample)
				{
					// wheelDelta를 yoffset으로 사용 (수직 스크롤)
					fbxSample->OnMouseScroll(0.0, static_cast<double>(evt.mouse.wheelDelta));
				}
			}
		}
		break;
	}
}

void OgPlayWindow::initImGui()
{
	// 기존 ImGui 컨텍스트 가져오기
	ImGuiContext* context = static_cast<ImGuiContext*>(OgImGuiContextManager::GetMainImGuiContext());
	if (context)
	{
		// 현재 컨텍스트로 설정
		ImGui::SetCurrentContext(context);
		
		// Win32 백엔드 초기화
		if (ImGui_ImplWin32_Init(_nativeWindow->win32.handle))
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

void OgPlayWindow::shutdownImGui()
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
}

void OgPlayWindow::beginImGuiFrame()
{
	ImGuiContext* context = static_cast<ImGuiContext*>(OgImGuiContextManager::GetMainImGuiContext());
	if (!context)
	{
		LOGE(OG_ID, "ImGui context is null");
		return;
	}
	
	ImGui::SetCurrentContext(context);
	
	ImGuiIO& io = context->IO;
	io.DisplaySize.x = static_cast<float>(GetWidth());
	io.DisplaySize.y = static_cast<float>(GetHeight());

	// ImGui Win32 프레임 시작
	if (_imguiWin32Initialized)
	{
		ImGui_ImplWin32_NewFrame();
	}
	
	ImGui::NewFrame();
}

void OgPlayWindow::endImGuiFrame()
{
	ImGui::Render();
}

void OgPlayWindow::renderImGui(Render::OgCommandEncoderHandle* encoder)
{
	ImGuiContext* context = static_cast<ImGuiContext*>(OgImGuiContextManager::GetMainImGuiContext());
	ImDrawData* drawData = ImGui::GetDrawData();
	
	if (drawData && _imguiRenderer)
	{
		// ImGui 업데이트
		_imguiRenderer->UpdateGPUContext(context);
		_imguiRenderer->UpdateSurface(_swapchain);
		
		// 샘플의 렌더 타겟 텍스처를 외부 텍스처로 설정
		if (_currentSample)
		{
			_imguiRenderer->SetExternalTexture(_currentSample->GetRenderTargetTexture());
		}
		
		// ImGui 렌더링
		OgRenderParam param;
		param.drawList = drawData;
		param.surface = _swapchain;
		std::hash<ImGuiContext*> hash_fn;
		size_t hash_value = hash_fn(context);
		param.guiContextKey = hash_value;
		
		_imguiRenderer->RenderGUI(encoder, param);
		_imguiRenderer->NextFrame(encoder, _swapchain);
	}
}

// OgSampleViewerWindow 구현
void OgSampleViewerWindow::onRenderUI()
{
	ImGuiIO& io = ImGui::GetIO();
	
	// 레이아웃 설정
	const float leftPanelWidth = 250.0f;
	const float topPanelHeight = 200.0f;
	const float padding = 5.0f;
	
	// 왼쪽 패널 - Sample Selector
	ImGui::SetNextWindowPos(ImVec2(padding, padding), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(leftPanelWidth, io.DisplaySize.y - 2 * padding), ImGuiCond_FirstUseEver);
	renderSampleSelector();
	
	// 오른쪽 상단 - Debug Info
	float rightPanelPosX = leftPanelWidth + 2 * padding;
	float rightPanelWidth = io.DisplaySize.x - rightPanelPosX - padding;
	
	ImGui::SetNextWindowPos(ImVec2(rightPanelPosX, padding), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(rightPanelWidth, topPanelHeight), ImGuiCond_FirstUseEver);
	renderDebugInfo();
	
	// 오른쪽 하단 - Sample Viewer
	float viewerHeight = io.DisplaySize.y - topPanelHeight - 3 * padding;
	
	ImGui::SetNextWindowPos(ImVec2(rightPanelPosX, topPanelHeight + 2 * padding), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(rightPanelWidth, viewerHeight), ImGuiCond_FirstUseEver);
	renderSampleViewer();
}

void OgSampleViewerWindow::renderSampleViewer()
{
	if (ImGui::Begin("Sample Viewer", nullptr))
	{
		OgSampleBase* sample = GetSample();
		if (sample && sample->GetRenderTargetTexture())
		{
			// 렌더 타겟 크기
			float rtWidth = static_cast<float>(sample->GetRenderTargetWidth());
			float rtHeight = static_cast<float>(sample->GetRenderTargetHeight());
			
			// 사용 가능한 영역
			ImVec2 availSize = ImGui::GetContentRegionAvail();
			
			// 비율 유지하면서 크기 계산
			float scale = 1.0f;
			if (rtWidth > 0 && rtHeight > 0 && availSize.x > 0 && availSize.y > 0)
			{
				float scaleX = availSize.x / rtWidth;
				float scaleY = availSize.y / rtHeight;
				scale = std::min(scaleX, scaleY);
				scale = std::clamp(scale, 0.1f, 2.0f);
			}
			
			ImVec2 imageSize(rtWidth * scale, rtHeight * scale);
			
			// 중앙 정렬
			float centerX = (availSize.x - imageSize.x) * 0.5f;
			float centerY = (availSize.y - imageSize.y) * 0.5f;
			
			if (centerX > 0) ImGui::SetCursorPosX(ImGui::GetCursorPosX() + centerX);
			if (centerY > 0) ImGui::SetCursorPosY(ImGui::GetCursorPosY() + centerY);
			
			// 이미지 렌더링
			ImGui::Image(
				reinterpret_cast<ImTextureID>(sample->GetRenderTargetTexture()),
				imageSize
			);
			
			// 이미지 위에 정보 오버레이
			ImDrawList* draw_list = ImGui::GetWindowDrawList();
			ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
			canvas_pos.y -= imageSize.y;
			
			// 배경 박스
			draw_list->AddRectFilled(
				canvas_pos,
				ImVec2(canvas_pos.x + 200, canvas_pos.y + 25),
				IM_COL32(0, 0, 0, 128)
			);
			
			// 텍스트
			char info[256];
			snprintf(info, sizeof(info), "Size: %ux%u Scale: %.1f%%", 
				sample->GetRenderTargetWidth(), 
				sample->GetRenderTargetHeight(),
				scale * 100.0f);
			draw_list->AddText(
				ImVec2(canvas_pos.x + 5, canvas_pos.y + 5),
				IM_COL32(255, 255, 255, 255),
				info
			);
		}
		else
		{
			// 중앙에 메시지 표시
			ImVec2 availSize = ImGui::GetContentRegionAvail();
			ImVec2 textSize = ImGui::CalcTextSize("No sample loaded");
			ImGui::SetCursorPosX((availSize.x - textSize.x) * 0.5f);
			ImGui::SetCursorPosY((availSize.y - textSize.y) * 0.5f);
			ImGui::Text("No sample loaded");
		}
	}
	ImGui::End();
}

void OgSampleViewerWindow::renderDebugInfo()
{
	if (ImGui::Begin("Debug Info", nullptr))
	{
		// 성능 정보
		ImGui::Text("Performance");
		ImGui::Separator();
		ImGui::Text("FPS: %.1f (%.3f ms/frame)", ImGui::GetIO().Framerate, 1000.0f / ImGui::GetIO().Framerate);
		ImGui::Text("Window Size: %u x %u", GetWidth(), GetHeight());
		
		ImGui::Spacing();
		
		// 샘플 정보
		OgSampleBase* sample = GetSample();
		if (sample)
		{
			ImGui::Text("Sample Information");
			ImGui::Separator();
			ImGui::Text("Type: %s", _currentSampleIndex == 0 ? "Triangle" : "FBX Model");
			ImGui::Text("Render Target: %u x %u", 
				sample->GetRenderTargetWidth(), 
				sample->GetRenderTargetHeight());
			ImGui::Text("Status: %s", sample->IsInitialized() ? "Running" : "Not Initialized");
			
			// FBX 샘플인 경우 추가 정보
			if (_currentSampleIndex == 1)
			{
				ImGui::Spacing();
				ImGui::Text("Controls");
				ImGui::Separator();
				ImGui::BulletText("Mouse: Rotate camera");
				ImGui::BulletText("Scroll: Zoom in/out");
				ImGui::BulletText("W/A/S/D: Move camera");
			}
		}
		
		// 시스템 정보
		ImGui::Spacing();
		ImGui::Text("System Information");
		ImGui::Separator();
		ImGui::Text("Renderer: %s", _renderContext ? "Active" : "Inactive");
		ImGui::Text("ImGui Backend: Win32");
	}
	ImGui::End();
}

void OgSampleViewerWindow::renderSampleSelector()
{
	if (ImGui::Begin("Sample Selector", nullptr))
	{
		const char* sampleNames[] = {
			"Triangle Sample",
			"FBX Model Sample"
		};
		
		ImGui::Text("Available Samples");
		ImGui::Separator();
		ImGui::Spacing();
		
		// 리스트박스 스타일로 변경
		if (ImGui::BeginListBox("##samples", ImVec2(-1, 100)))
		{
			for (int i = 0; i < IM_ARRAYSIZE(sampleNames); i++)
			{
				const bool isSelected = (_currentSampleIndex == i);
				if (ImGui::Selectable(sampleNames[i], isSelected))
				{
					if (_currentSampleIndex != i)
					{
						_currentSampleIndex = i;
						switchSample(i);
					}
				}
				
				if (isSelected)
					ImGui::SetItemDefaultFocus();
			}
			ImGui::EndListBox();
		}
		
		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();
		
		// 현재 샘플 설명
		ImGui::Text("Description");
		ImGui::Separator();
		ImGui::PushTextWrapPos();
		
		switch (_currentSampleIndex)
		{
		case 0:
			ImGui::TextWrapped("Triangle Sample");
			ImGui::BulletText("Basic triangle rendering");
			ImGui::BulletText("Vertex color interpolation");
			ImGui::BulletText("No user interaction");
			break;
		case 1:
			ImGui::TextWrapped("FBX Model Sample");
			ImGui::BulletText("3D cube rendering");
			ImGui::BulletText("Camera controls");
			ImGui::BulletText("Y-axis rotation animation");
			ImGui::BulletText("Mouse & keyboard input");
			break;
		}
		
		ImGui::PopTextWrapPos();
		
		// 샘플 제어 버튼
		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();
		
		ImGui::Text("Controls");
		ImGui::Separator();
		
		if (ImGui::Button("Reload Sample", ImVec2(-1, 0)))
		{
			switchSample(_currentSampleIndex);
		}
		
		if (GetSample() && GetSample()->IsInitialized())
		{
			if (ImGui::Button("Reset View", ImVec2(-1, 0)))
			{
				// 샘플별 리셋 기능 구현 가능
				OgFBXSample* fbxSample = dynamic_cast<OgFBXSample*>(GetSample());
				if (fbxSample)
				{
					// 카메라 리셋 등의 기능 추가 가능
				}
			}
		}
	}
	ImGui::End();
}

void OgSampleViewerWindow::switchSample(int index)
{
	std::unique_ptr<OgSampleBase> newSample;
	
	switch (index)
	{
	case 0:
		newSample = std::make_unique<OgTriangleSample>(_renderContext);
		break;
	case 1:
		newSample = std::make_unique<OgFBXSample>(_renderContext);
		break;
	default:
		return;
	}
	
	// 새 샘플로 교체
	SetSample(std::move(newSample));
}

OG_NAMESPACE_SAMPLE_END
