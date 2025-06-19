#include "OgPlayWindow.h"
#include "sample/public/core/OgTriangleSample.h"
#include "sample/public/core/OgFBXSample.h"
#include "system/OgSystemContext.h"
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
	renderSampleSelector();
	renderSampleViewer();
	renderDebugInfo();
}

void OgSampleViewerWindow::renderSampleViewer()
{
	ImGuiIO& io = ImGui::GetIO();
	
	// 메인 뷰어 윈도우
	ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x * 0.6f, io.DisplaySize.y * 0.6f), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.2f, io.DisplaySize.y * 0.2f), ImGuiCond_FirstUseEver);
	
	if (ImGui::Begin("Sample Viewer", nullptr, ImGuiWindowFlags_None))
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
			if (centerX > 0)
			{
				ImGui::SetCursorPosX(ImGui::GetCursorPosX() + centerX);
			}
			
			// 이미지 렌더링
			ImGui::Image(
				reinterpret_cast<ImTextureID>(sample->GetRenderTargetTexture()),
				imageSize
			);
		}
		else
		{
			ImGui::Text("No sample loaded");
		}
	}
	ImGui::End();
}

void OgSampleViewerWindow::renderDebugInfo()
{
	// 디버그 정보 윈도우
	if (ImGui::Begin("Debug Info", nullptr, ImGuiWindowFlags_None))
	{
		ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
		ImGui::Text("Window Size: %u x %u", GetWidth(), GetHeight());
		
		OgSampleBase* sample = GetSample();
		if (sample)
		{
			ImGui::Separator();
			ImGui::Text("Sample Info:");
			ImGui::Text("Render Target: %u x %u", 
				sample->GetRenderTargetWidth(), 
				sample->GetRenderTargetHeight());
			ImGui::Text("Initialized: %s", sample->IsInitialized() ? "Yes" : "No");
		}
	}
	ImGui::End();
}

void OgSampleViewerWindow::renderSampleSelector()
{
	// 샘플 선택 윈도우
	if (ImGui::Begin("Sample Selector", nullptr, ImGuiWindowFlags_None))
	{
		const char* sampleNames[] = {
			"Triangle Sample",
			"FBX Model Sample"
			// 나중에 더 잘 샘플들을 추가할 수 있습니다
		};
		
		if (ImGui::Combo("Select Sample", &_currentSampleIndex, sampleNames, IM_ARRAYSIZE(sampleNames)))
		{
			switchSample(_currentSampleIndex);
		}
		
		ImGui::Separator();
		
		// 현재 샘플 설명
		switch (_currentSampleIndex)
		{
		case 0:
			ImGui::TextWrapped("Triangle Sample: 기본적인 삼각형을 렌더링합니다.");
			break;
		case 1:
			ImGui::TextWrapped("FBX Model Sample: 3D 큐브를 렌더링하며 Y축을 기준으로 회전합니다. "
							 "실제 FBX 파일 로드 기능은 FBX SDK 또는 Assimp 라이브러리를 "
							 "통해 추가할 수 있습니다.");
			break;
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
