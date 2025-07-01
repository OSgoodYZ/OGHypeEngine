#include "OgPlayWindow.h"
#include "sample/public/core/OgTriangleSample.h"
#include "sample/public/core/OgModelSample.h"
#include "sample/public/core/OgComputeSample.h"
#include "sample/public/core/OgRayTracingSample.h"
#include "system/OgSystemContext.h"
#include "system/OgInput.h"
#include <algorithm>
#include <cmath>
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

		// 샘플별 초기화 처리
		handleSampleSpecificInit();
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

		// 샘플별 초기화 처리
		handleSampleSpecificInit();
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
	if (_currentSample && shouldRenderSample())
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

	// 샘플별 이벤트 처리
	if (_currentSample)
	{
		handleSampleSpecificEvent(evt, io);
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

// 샘플별 초기화 처리
void OgPlayWindow::handleSampleSpecificInit()
{
	if (!_currentSample)
		return;

	// OgModelSample 처리
	if (OgModelSample* modelSample = dynamic_cast<OgModelSample*>(_currentSample.get()))
	{
		if (modelSample->GetSelectedModelIndex() == -1)
		{
			_showModelBrowser = true;
		}
	}
	// OgComputeSample 처리
	else if (OgComputeSample* computeSample = dynamic_cast<OgComputeSample*>(_currentSample.get()))
	{
		// Compute 샘플별 초기화 처리 가능
		// 예: 컴퓨트 샘플 UI 설정 등
	}
	// OgTriangleSample 처리
	else if (OgTriangleSample* triangleSample = dynamic_cast<OgTriangleSample*>(_currentSample.get()))
	{
		// Triangle 샘플별 초기화 처리 가능
	}
}

// 샘플을 렌더링해야 하는지 확인
bool OgPlayWindow::shouldRenderSample() const
{
	if (!_currentSample)
		return false;

	// OgModelSample의 경우 모델이 선택되었는지 확인
	if (OgModelSample* modelSample = dynamic_cast<OgModelSample*>(_currentSample.get()))
	{
		return modelSample->GetSelectedModelIndex() != -1;
	}

	// 다른 샘플들은 항상 렌더링
	return true;
}

// 샘플별 이벤트 처리
void OgPlayWindow::handleSampleSpecificEvent(const OgNativeEvent& evt, const ImGuiIO& io)
{
	// OgModelSample 이벤트 처리
	if (OgModelSample* modelSample = dynamic_cast<OgModelSample*>(_currentSample.get()))
	{
		handleModelSampleEvent(modelSample, evt, io);
	}
	// OgComputeSample 이벤트 처리
	else if (OgComputeSample* computeSample = dynamic_cast<OgComputeSample*>(_currentSample.get()))
	{
		handleComputeSampleEvent(computeSample, evt, io);
	}
	// OgTriangleSample은 이벤트 처리가 필요 없음
}

// 모델 샘플 이벤트 처리
void OgPlayWindow::handleModelSampleEvent(OgModelSample* modelSample, const OgNativeEvent& evt, const ImGuiIO& io)
{
	switch (evt.type)
	{
	case OG_KEY_PRESS:
	case OG_KEY_RELEASE:
	{
		// ImGui가 키보드 입력을 사용하지 않는 경우에만 전달
		if (!io.WantCaptureKeyboard)
		{
			int action = (evt.type == OG_KEY_PRESS) ? OG_PRESS : OG_RELEASE;
			int mods = 0;
			if (evt.key.shift) mods |= OG_MOD_SHIFT;
			if (evt.key.control) mods |= OG_MOD_CONTROL;
			if (evt.key.alt) mods |= OG_MOD_ALT;
			if (evt.key.system) mods |= OG_MOD_SUPER;

			modelSample->OnKeyPress(evt.key.keyCode, action, mods);
		}
	}
	break;

	case OG_MOUSE_PRESS:
	case OG_MOUSE_RELEASE:
	{
		int action = (evt.type == OG_MOUSE_PRESS) ? OG_PRESS : OG_RELEASE;
		modelSample->OnMouseButton(evt.mouse.button, action, evt.mouse.mods);
	}
	break;

	case OG_MOUSE_MOVE:
	{
		modelSample->OnMouseMove(evt.mouse.pos.x, evt.mouse.pos.y);
	}
	break;

	case OG_MOUSE_WHEEL_CHANGE:
	{
		modelSample->OnMouseScroll(0.0, static_cast<double>(evt.mouse.wheelDelta));
	}
	break;
	}
}

// 컴퓨트 샘플 이벤트 처리
void OgPlayWindow::handleComputeSampleEvent(OgComputeSample* computeSample, const OgNativeEvent& evt, const ImGuiIO& io)
{
	// 컴퓨트 샘플에 필요한 이벤트 처리 추가
	// 예: 파라미터 조정을 위한 키보드 입력 등
	switch (evt.type)
	{
	case OG_KEY_PRESS:
	{
		if (!io.WantCaptureKeyboard)
		{
			// 컴퓨트 샘플 특정 키 처리
			// 예: R키로 리셋, 숫자 키로 파라미터 변경 등
		}
	}
	break;

	case OG_MOUSE_PRESS:
	case OG_MOUSE_MOVE:
	{
		// 필요시 마우스 이벤트 처리
	}
	break;
	}
}

// OgSampleViewerWindow 구현
void OgSampleViewerWindow::onRenderUI()
{
	// 초기 샘플 설정
	if (!GetSample())
	{
		switchSample(1); // 기본으로 모델 샘플 설정
		return;
	}

	ImGuiIO& io = ImGui::GetIO();

	// 레이아웃 설정
	const float leftPanelWidth = 250.0f;
	const float rightPanelWidth = 300.0f; // 모델 브라우저 패널 너비
	const float topPanelHeight = 200.0f;
	const float sampleSelectorHeight = 250.0f; // Sample Selector 높이 고정
	const float padding = 5.0f;

	// Light Controls 높이를 나머지 공간으로 계산
	const float lightControlHeight = io.DisplaySize.y - sampleSelectorHeight - 3 * padding;

	// 모델 브라우저 표시 여부 확인
	bool shouldShowModelBrowser = shouldShowBrowserPanel();

	// 중앙 영역 너비 계산 (브라우저가 표시될 때 조정)
	float centerAreaWidth = io.DisplaySize.x - leftPanelWidth - 2 * padding - padding;
	if (shouldShowModelBrowser)
	{
		centerAreaWidth -= rightPanelWidth + padding;
	}

	// 왼쪽 상단 패널 - Sample Selector
	ImGui::SetNextWindowPos(ImVec2(padding, padding), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(leftPanelWidth, sampleSelectorHeight), ImGuiCond_FirstUseEver);
	renderSampleSelector();

	// 왼쪽 하단 패널 - Sample Controls
	ImGui::SetNextWindowPos(ImVec2(padding, sampleSelectorHeight + 2 * padding), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(leftPanelWidth, lightControlHeight), ImGuiCond_FirstUseEver);
	renderSampleControls();

	// 중앙 상단 - Debug Info
	float centerPanelPosX = leftPanelWidth + 2 * padding;

	ImGui::SetNextWindowPos(ImVec2(centerPanelPosX, padding), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(centerAreaWidth, topPanelHeight), ImGuiCond_FirstUseEver);
	renderDebugInfo();

	// 중앙 하단 - Sample Viewer
	float viewerHeight = io.DisplaySize.y - topPanelHeight - 3 * padding;

	ImGui::SetNextWindowPos(ImVec2(centerPanelPosX, topPanelHeight + 2 * padding), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(centerAreaWidth, viewerHeight), ImGuiCond_FirstUseEver);
	renderSampleViewer();

	// 오른쪽 패널 - Sample Browser (필요한 경우)
	if (shouldShowModelBrowser)
	{
		float browserPosX = io.DisplaySize.x - rightPanelWidth - padding;
		ImGui::SetNextWindowPos(ImVec2(browserPosX, padding), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSize(ImVec2(rightPanelWidth, io.DisplaySize.y - 2 * padding), ImGuiCond_FirstUseEver);
		renderSampleBrowser();
	}
}

// 브라우저 패널을 표시해야 하는지 확인
bool OgSampleViewerWindow::shouldShowBrowserPanel() const
{
	if (!_currentSample)
		return false;

	// OgModelSample의 경우
	if (OgModelSample* modelSample = dynamic_cast<OgModelSample*>(_currentSample.get()))
	{
		// 모델이 선택되지 않았으면 항상 표시
		if (modelSample->GetSelectedModelIndex() == -1)
		{
			return true;
		}
		// 그렇지 않으면 사용자 설정에 따라
		return _showModelBrowser && (_currentSampleIndex == 1);
	}

	// 다른 샘플들은 브라우저가 필요 없음
	return false;
}

void OgSampleViewerWindow::renderSampleViewer()
{
	if (ImGui::Begin("Sample Viewer", nullptr))
	{
		OgSampleBase* sample = GetSample();

		// 샘플별 뷰어 렌더링
		if (OgModelSample* modelSample = dynamic_cast<OgModelSample*>(sample))
		{
			renderModelSampleViewer(modelSample);
		}
		else if (OgComputeSample* computeSample = dynamic_cast<OgComputeSample*>(sample))
		{
			renderComputeSampleViewer(computeSample);
		}
		else if (OgTriangleSample* triangleSample = dynamic_cast<OgTriangleSample*>(sample))
		{
			renderTriangleSampleViewer(triangleSample);
		}
		else if (OgRayTracingSample* rayTracingSample = dynamic_cast<OgRayTracingSample*>(sample))
		{
			renderRayTracingSampleViewer(rayTracingSample);
		}
		else
		{
			// 기본 렌더 타겟 표시
			renderDefaultViewer(sample);
		}
	}
	ImGui::End();
}

// 모델 샘플 뷰어 렌더링
void OgSampleViewerWindow::renderModelSampleViewer(OgModelSample* modelSample)
{
	// 모델이 선택되지 않았을 때
	if (modelSample->GetSelectedModelIndex() == -1)
	{
		// 중앙에 메시지 표시
		ImVec2 availSize = ImGui::GetContentRegionAvail();
		const char* message = "Please select a model from the browser";
		ImVec2 textSize = ImGui::CalcTextSize(message);
		ImGui::SetCursorPosX((availSize.x - textSize.x) * 0.5f);
		ImGui::SetCursorPosY((availSize.y - textSize.y) * 0.5f);
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.7f, 0.7f, 1.0f));
		ImGui::Text("%s", message);
		ImGui::PopStyleColor();
	}
	else
	{
		renderSampleImage(modelSample);
	}
}

// 컴퓨트 샘플 뷰어 렌더링
void OgSampleViewerWindow::renderComputeSampleViewer(OgComputeSample* computeSample)
{
	renderSampleImage(computeSample);
}

// 삼각형 샘플 뷰어 렌더링
void OgSampleViewerWindow::renderTriangleSampleViewer(OgTriangleSample* triangleSample)
{
	renderSampleImage(triangleSample);
}

// 기본 뷰어 렌더링
void OgSampleViewerWindow::renderDefaultViewer(OgSampleBase* sample)
{
	if (sample && sample->GetRenderTargetTexture())
	{
		renderSampleImage(sample);
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

// 샘플 이미지 렌더링
void OgSampleViewerWindow::renderSampleImage(OgSampleBase* sample)
{
	if (!sample || !sample->GetRenderTargetTexture())
		return;

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
	ImVec2 imagePos = ImGui::GetCursorScreenPos();
	ImGui::Image(
		reinterpret_cast<ImTextureID>(sample->GetRenderTargetTexture()),
		imageSize
	);

	// XYZ 축 기즈모 (오른쪽 상단)
	renderXYZGizmo(imagePos, imageSize);

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

void OgSampleViewerWindow::renderXYZGizmo(ImVec2 imagePos, ImVec2 imageSize)
{
	// 모델 샘플인 경우에만 동적 기즈모 표시
	OgSampleBase* sample = GetSample();
	OgModelSample* modelSample = dynamic_cast<OgModelSample*>(sample);

	ImDrawList* draw_list = ImGui::GetWindowDrawList();

	// 기즈모 크기와 위치 설정
	float gizmo_size = 80.0f;
	float margin = 20.0f;

	// 오른쪽 상단 위치
	ImVec2 gizmo_center = ImVec2(
		imagePos.x + imageSize.x - gizmo_size * 0.5f - margin,
		imagePos.y + gizmo_size * 0.5f + margin
	);

	if (modelSample)
	{
		// 카메라의 뷰 행렬 가져오기
		glm::mat4 viewMatrix = modelSample->GetViewMatrix();

		// 월드 공간의 축 벡터들
		glm::vec3 worldX = glm::vec3(1.0f, 0.0f, 0.0f);
		glm::vec3 worldY = glm::vec3(0.0f, 1.0f, 0.0f);
		glm::vec3 worldZ = glm::vec3(0.0f, 0.0f, 1.0f);

		// 뷰 공간으로 변환 (뷰 행렬을 직접 사용, 방향 벡터이므로 w=0)
		glm::vec4 viewX = viewMatrix * glm::vec4(worldX, 0.0f);
		glm::vec4 viewY = viewMatrix * glm::vec4(worldY, 0.0f);
		glm::vec4 viewZ = viewMatrix * glm::vec4(worldZ, 0.0f);

		// 2D 투영 (Y축은 스크린 좌표계에 맞게 뒤집기)
		glm::vec2 proj2D_X = glm::vec2(viewX.x, -viewX.y);  // Y축 뒤집기
		glm::vec2 proj2D_Y = glm::vec2(viewY.x, -viewY.y);  // Y축 뒤집기
		glm::vec2 proj2D_Z = glm::vec2(viewZ.x, -viewZ.y);  // Y축 뒤집기

		// 축의 길이 설정 - 3D 공간에서 길이 1인 축을 스케일링
		float scale_factor = gizmo_size * 0.3f;
		float line_thickness = 2.5f;

		// X축 그리기 (빨간색)
		ImVec2 x_end = ImVec2(
			gizmo_center.x + proj2D_X.x * scale_factor,
			gizmo_center.y + proj2D_X.y * scale_factor
		);
		ImU32 x_color = IM_COL32(220, 60, 60, 255);
		draw_list->AddLine(gizmo_center, x_end, x_color, line_thickness);

		// X 라벨
		ImVec2 x_label_pos = ImVec2(x_end.x + proj2D_X.x * 10, x_end.y + proj2D_X.y * 10);
		draw_list->AddText(x_label_pos, IM_COL32(255, 255, 255, 255), "X");

		// Y축 그리기 (녹색)
		ImVec2 y_end = ImVec2(
			gizmo_center.x + proj2D_Y.x * scale_factor,
			gizmo_center.y + proj2D_Y.y * scale_factor
		);
		ImU32 y_color = IM_COL32(60, 220, 60, 255);
		draw_list->AddLine(gizmo_center, y_end, y_color, line_thickness);

		// Y 라벨
		ImVec2 y_label_pos = ImVec2(y_end.x + proj2D_Y.x * 10, y_end.y + proj2D_Y.y * 10);
		draw_list->AddText(y_label_pos, IM_COL32(255, 255, 255, 255), "Y");

		// Z축 그리기 (파란색)
		ImVec2 z_end = ImVec2(
			gizmo_center.x + proj2D_Z.x * scale_factor,
			gizmo_center.y + proj2D_Z.y * scale_factor
		);
		ImU32 z_color = IM_COL32(60, 120, 220, 255);
		draw_list->AddLine(gizmo_center, z_end, z_color, line_thickness);

		// Z 라벨
		ImVec2 z_label_pos = ImVec2(z_end.x + proj2D_Z.x * 10, z_end.y + proj2D_Z.y * 10);
		draw_list->AddText(z_label_pos, IM_COL32(255, 255, 255, 255), "Z");
	}
	else
	{
		// 모델 샘플이 아닌 경우 - 간단한 고정 축 표시
		float axis_length = gizmo_size * 0.3f;
		float line_thickness = 2.5f;

		// X축 (빨간색) - 오른쪽
		ImVec2 x_end = ImVec2(gizmo_center.x + axis_length, gizmo_center.y);
		ImU32 x_color = IM_COL32(220, 60, 60, 255);
		draw_list->AddLine(gizmo_center, x_end, x_color, line_thickness);
		draw_list->AddText(ImVec2(x_end.x + 10, x_end.y - 6), IM_COL32(255, 255, 255, 255), "X");

		// Y축 (녹색) - 위쪽
		ImVec2 y_end = ImVec2(gizmo_center.x, gizmo_center.y - axis_length);
		ImU32 y_color = IM_COL32(60, 220, 60, 255);
		draw_list->AddLine(gizmo_center, y_end, y_color, line_thickness);
		draw_list->AddText(ImVec2(y_end.x - 6, y_end.y - 15), IM_COL32(255, 255, 255, 255), "Y");

		// Z축 (파란색) - 대각선 (화면에서 나오는 방향을 표현)
		ImVec2 z_end = ImVec2(gizmo_center.x - axis_length * 0.7f, gizmo_center.y + axis_length * 0.7f);
		ImU32 z_color = IM_COL32(60, 120, 220, 255);
		draw_list->AddLine(gizmo_center, z_end, z_color, line_thickness);
		draw_list->AddText(ImVec2(z_end.x - 15, z_end.y + 2), IM_COL32(255, 255, 255, 255), "Z");
	}
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
			const char* sampleType = "Unknown";
			switch (_currentSampleIndex) {
			case 0: sampleType = "Triangle"; break;
			case 1: sampleType = "glTF Model"; break;
			case 2: sampleType = "Compute"; break;
			case 3: sampleType = "Ray Tracing"; break;
			}
			ImGui::Text("Type: %s", sampleType);
			ImGui::Text("Render Target: %u x %u",
				sample->GetRenderTargetWidth(),
				sample->GetRenderTargetHeight());
			ImGui::Text("Status: %s", sample->IsInitialized() ? "Running" : "Not Initialized");

			// 샘플별 컨트롤 정보
			renderSampleControlsInfo();
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

// 샘플별 컨트롤 정보 렌더링
void OgSampleViewerWindow::renderSampleControlsInfo()
{
	OgSampleBase* sample = GetSample();
	if (!sample)
		return;

	ImGui::Spacing();
	ImGui::Text("Controls");
	ImGui::Separator();

	// 모델 샘플 컨트롤
	if (dynamic_cast<OgModelSample*>(sample))
	{
		ImGui::BulletText("Mouse: Rotate camera");
		ImGui::BulletText("Scroll: Zoom in/out");
		ImGui::BulletText("W/A/S/D: Move camera");
		ImGui::BulletText("F: Toggle fly camera");
		ImGui::BulletText("L: Toggle light controls");
		ImGui::BulletText("B: Toggle model browser (after model selected)");
	}
	// 컴퓨트 샘플 컨트롤
	else if (dynamic_cast<OgComputeSample*>(sample))
	{
		ImGui::BulletText("No interaction available");
		ImGui::BulletText("Compute shader running");
	}
	// 삼각형 샘플 컨트롤
	else if (dynamic_cast<OgTriangleSample*>(sample))
	{
		ImGui::BulletText("No interaction available");
		ImGui::BulletText("Static triangle rendering");
	}
	// 레이트레이싱 샘플 컨트롤
	else if (dynamic_cast<OgRayTracingSample*>(sample))
	{
		ImGui::BulletText("Mouse: Rotate camera");
		ImGui::BulletText("Scroll: Zoom in/out");
		ImGui::BulletText("W/A/S/D: Move camera");
		ImGui::BulletText("F: Toggle fly camera");
		ImGui::BulletText("D: Toggle debug visualization");
		ImGui::BulletText("Progressive rendering - quality improves over time");
	}
}

void OgSampleViewerWindow::renderSampleSelector()
{
	if (ImGui::Begin("Sample Selector", nullptr))
	{
		const char* sampleNames[] = {
		"Triangle Sample",
		"GLTF Model Sample",
		"Compute Sample",
		 "Ray Tracing Sample"
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
			ImGui::TextWrapped("Model Sample");
			ImGui::BulletText("Model rendering");
			ImGui::BulletText("Camera controls");
			ImGui::BulletText("Y-axis rotation animation");
			ImGui::BulletText("Mouse & keyboard input");
			break;
		case 2:
			ImGui::TextWrapped("Compute Sample");
			ImGui::BulletText("Compute shader demo");
			ImGui::BulletText("GPU computation");
			ImGui::BulletText("Texture processing");
			break;
		case 3:
			ImGui::TextWrapped("Ray Tracing Sample");
			ImGui::BulletText("Vulkan Ray Tracing");
			ImGui::BulletText("Path tracing rendering");
			ImGui::BulletText("Progressive refinement");
			ImGui::BulletText("DragonAttenuation model");
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
				// 샘플별 리셋 기능
				resetCurrentSample();
			}
		}
	}
	ImGui::End();
}

// 현재 샘플 리셋
void OgSampleViewerWindow::resetCurrentSample()
{
	OgSampleBase* sample = GetSample();
	if (!sample)
		return;

	// 모델 샘플 리셋
	if (OgModelSample* modelSample = dynamic_cast<OgModelSample*>(sample))
	{
		// 카메라 리셋 등의 기능 추가 가능
		// modelSample->ResetCamera();
	}
	// 컴퓨트 샘플 리셋
	else if (OgComputeSample* computeSample = dynamic_cast<OgComputeSample*>(sample))
	{
		// 컴퓨트 파라미터 리셋
		// computeSample->ResetParameters();
	}
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
		newSample = std::make_unique<OgModelSample>(_renderContext);
		break;
	case 2:
		newSample = std::make_unique<OgComputeSample>(_renderContext);
		break;
	case 3:
		newSample = std::make_unique<OgRayTracingSample>(_renderContext);
		break;
	default:
		return;
	}

	// 새 샘플로 교체
	SetSample(std::move(newSample));

	// 샘플별 초기화 처리는 SetSample에서 처리됨
}

// 샘플 컨트롤 렌더링 (Light Controls -> Sample Controls로 변경)
void OgSampleViewerWindow::renderSampleControls()
{
	OgSampleBase* sample = GetSample();
	if (!sample)
		return;

	// 모델 샘플인 경우 라이트 컨트롤 표시
	if (OgModelSample* modelSample = dynamic_cast<OgModelSample*>(sample))
	{
		renderLightControls(modelSample);
	}
	// 컴퓨트 샘플인 경우 컴퓨트 컨트롤 표시
	else if (OgComputeSample* computeSample = dynamic_cast<OgComputeSample*>(sample))
	{
		renderComputeControls(computeSample);
	}
	// 삼각형 샘플인 경우 기본 컨트롤 표시
	else if (OgTriangleSample* triangleSample = dynamic_cast<OgTriangleSample*>(sample))
	{
		renderTriangleControls(triangleSample);
	}
	// 레이트레이싱 샘플인 경우 레이트레이싱 컨트롤 표시
	else if (OgRayTracingSample* rayTracingSample = dynamic_cast<OgRayTracingSample*>(sample))
	{
		renderRayTracingControls(rayTracingSample);
	}
}

// 라이트 컨트롤 렌더링
void OgSampleViewerWindow::renderLightControls(OgModelSample* modelSample)
{
	if (ImGui::Begin("Light Controls", nullptr))
	{
		ImGui::Text("Light Settings");
		ImGui::Separator();

		bool lightDataChanged = false;
		auto& lightData = modelSample->GetLightUniformData();

		// 라이트 개수 조절
		int lightCount = lightData.lightCount;
		if (ImGui::SliderInt("Light Count", &lightCount, 0, 4))
		{
			lightData.lightCount = lightCount;
			lightDataChanged = true;
		}

		ImGui::Separator();

		// 각 라이트에 대한 컨트롤
		for (int i = 0; i < lightData.lightCount; ++i)
		{
			ImGui::PushID(i);

			char headerLabel[32];
			snprintf(headerLabel, sizeof(headerLabel), "Light %d", i);

			if (ImGui::CollapsingHeader(headerLabel, ImGuiTreeNodeFlags_DefaultOpen))
			{
				// 방향 (Directional Light)
				if (ImGui::DragFloat3("Direction", &lightData.lights[i].position.x, 0.1f))
				{
					lightDataChanged = true;
				}

				// 강도
				if (ImGui::SliderFloat("Intensity", &lightData.lights[i].intensity, 0.0f, 50.0f))
				{
					lightDataChanged = true;
				}

				// 색상
				if (ImGui::ColorEdit3("Color", &lightData.lights[i].color.x))
				{
					lightDataChanged = true;
				}
			}

			ImGui::PopID();
		}

		// 라이트 데이터가 변경되면 유니폼 버퍼 업데이트
		if (lightDataChanged)
		{
			modelSample->UpdateLightUniformBuffer();
		}

		// 프리셋 버튼들
		ImGui::Separator();
		ImGui::Text("Presets");
		ImGui::Separator();

		if (ImGui::Button("Default Lighting", ImVec2(-1, 0)))
		{
			// 기본 라이팅 설정 (Directional)
			lightData.lightCount = 4;

			lightData.lights[0].position = glm::normalize(glm::vec3(1.0f, -1.0f, -1.0f));
			lightData.lights[0].color = glm::vec3(1.0f, 1.0f, 1.0f);
			lightData.lights[0].intensity = 3.0f;

			lightData.lights[1].position = glm::normalize(glm::vec3(-0.5f, -1.0f, -0.5f));
			lightData.lights[1].color = glm::vec3(0.5f, 0.5f, 0.75f);
			lightData.lights[1].intensity = 1.5f;

			lightData.lights[2].position = glm::normalize(glm::vec3(0.5f, 1.0f, -0.3f));
			lightData.lights[2].color = glm::vec3(0.75f, 0.5f, 0.5f);
			lightData.lights[2].intensity = 1.0f;

			lightData.lights[3].position = glm::normalize(glm::vec3(0.0f, 0.0f, -1.0f));
			lightData.lights[3].color = glm::vec3(1.0f, 1.0f, 1.0f);
			lightData.lights[3].intensity = 0.5f;

			modelSample->UpdateLightUniformBuffer();
		}

		if (ImGui::Button("Single Key Light", ImVec2(-1, 0)))
		{
			// 단일 키 라이트 (Directional)
			lightData.lightCount = 1;
			lightData.lights[0].position = glm::normalize(glm::vec3(0.5f, -1.0f, -0.8f));
			lightData.lights[0].color = glm::vec3(1.0f, 1.0f, 1.0f);
			lightData.lights[0].intensity = 4.0f;

			modelSample->UpdateLightUniformBuffer();
		}

		if (ImGui::Button("Warm Lighting", ImVec2(-1, 0)))
		{
			// 따뜻한 조명 (Directional)
			lightData.lightCount = 2;
			lightData.lights[0].position = glm::normalize(glm::vec3(0.6f, -0.8f, -0.6f));
			lightData.lights[0].color = glm::vec3(1.0f, 0.8f, 0.6f);  // 따뜻한 색
			lightData.lights[0].intensity = 3.5f;

			lightData.lights[1].position = glm::normalize(glm::vec3(-0.3f, -0.5f, -0.8f));
			lightData.lights[1].color = glm::vec3(0.8f, 0.6f, 0.4f);  // 더 따뜻한 보조 조명
			lightData.lights[1].intensity = 1.5f;

			modelSample->UpdateLightUniformBuffer();
		}

		// Light Gizmo Visualization
		ImGui::Separator();
		ImGui::Text("Light Direction Gizmo");
		ImGui::Separator();

		// 기즈모 캔버스 영역
		ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
		ImVec2 canvas_size = ImGui::GetContentRegionAvail();
		canvas_size.y = std::min(canvas_size.y, 200.0f); // 최대 높이 제한

		if (canvas_size.x > 100 && canvas_size.y > 100)
		{
			ImDrawList* draw_list = ImGui::GetWindowDrawList();

			// 배경 그리기
			draw_list->AddRectFilled(canvas_pos,
				ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y),
				IM_COL32(40, 40, 40, 200), 5.0f);

			// 중심점
			ImVec2 center = ImVec2(canvas_pos.x + canvas_size.x * 0.5f, canvas_pos.y + canvas_size.y * 0.5f);

			// 참조 격자 그리기
			float grid_step = 20.0f;
			for (float x = canvas_pos.x; x <= canvas_pos.x + canvas_size.x; x += grid_step)
			{
				draw_list->AddLine(ImVec2(x, canvas_pos.y), ImVec2(x, canvas_pos.y + canvas_size.y),
					IM_COL32(60, 60, 60, 100), 1.0f);
			}
			for (float y = canvas_pos.y; y <= canvas_pos.y + canvas_size.y; y += grid_step)
			{
				draw_list->AddLine(ImVec2(canvas_pos.x, y), ImVec2(canvas_pos.x + canvas_size.x, y),
					IM_COL32(60, 60, 60, 100), 1.0f);
			}

			// 중심 십자선
			draw_list->AddLine(ImVec2(center.x - 10, center.y), ImVec2(center.x + 10, center.y),
				IM_COL32(100, 100, 100, 200), 2.0f);
			draw_list->AddLine(ImVec2(center.x, center.y - 10), ImVec2(center.x, center.y + 10),
				IM_COL32(100, 100, 100, 200), 2.0f);

			// 기즈모 크기 설정
			float gizmo_size = std::min(canvas_size.x, canvas_size.y) * 0.3f;
			gizmo_size = std::clamp(gizmo_size, 30.0f, 80.0f);

			// 카메라의 뷰 행렬 가져오기
			glm::mat4 viewMatrix = modelSample->GetViewMatrix();

			// 각 라이트에 대해 기즈모 그리기
			for (int i = 0; i < lightData.lightCount; ++i)
			{
				const auto& light = lightData.lights[i];

				// 라이트 방향을 world space에서 view space로 변환
				glm::vec3 worldLightDir = -glm::normalize(light.position);
				glm::vec4 viewLightDir = viewMatrix * glm::vec4(worldLightDir, 0.0f);

				// view space에서 2D로 투영 (카메라 방향을 고려)
				float dirX = -viewLightDir.x; // 화면 좌표계에 맞게 조정
				float dirY = viewLightDir.y;

				// 라이트 강도에 따른 화살표 길이
				float intensity = std::clamp(light.intensity / 4.0f, 0.3f, 1.0f);
				float arrow_length = gizmo_size * intensity;

				// 화살표 끝점
				ImVec2 arrow_end = ImVec2(
					center.x + dirX * arrow_length,
					center.y + dirY * arrow_length
				);

				// 라이트 색상 설정
				ImU32 light_color = IM_COL32(
					static_cast<int>(light.color.r * 255),
					static_cast<int>(light.color.g * 255),
					static_cast<int>(light.color.b * 255),
					255
				);

				// 화살표 선 그리기
				float line_thickness = 3.0f + intensity * 2.0f;
				draw_list->AddLine(center, arrow_end, light_color, line_thickness);

				// 화살표 머리 그리기
				float arrow_head_size = 10.0f + intensity * 5.0f;
				glm::vec2 arrow_dir = glm::normalize(glm::vec2(dirX, dirY));
				glm::vec2 perp_dir = glm::vec2(-arrow_dir.y, arrow_dir.x);

				ImVec2 arrow_head1 = ImVec2(
					arrow_end.x - arrow_dir.x * arrow_head_size + perp_dir.x * arrow_head_size * 0.5f,
					arrow_end.y - arrow_dir.y * arrow_head_size + perp_dir.y * arrow_head_size * 0.5f
				);
				ImVec2 arrow_head2 = ImVec2(
					arrow_end.x - arrow_dir.x * arrow_head_size - perp_dir.x * arrow_head_size * 0.5f,
					arrow_end.y - arrow_dir.y * arrow_head_size - perp_dir.y * arrow_head_size * 0.5f
				);

				// 화살표 머리 삼각형
				draw_list->AddTriangleFilled(arrow_end, arrow_head1, arrow_head2, light_color);

				// 라이트 인덱스 표시
				char light_label[16];
				snprintf(light_label, sizeof(light_label), "L%d", i);

				ImVec2 label_pos = ImVec2(
					arrow_end.x + dirX * 20.0f,
					arrow_end.y + dirY * 20.0f
				);

				// 라벨 배경
				ImVec2 label_size = ImGui::CalcTextSize(light_label);
				ImVec2 label_bg_min = ImVec2(label_pos.x - 3, label_pos.y - 2);
				ImVec2 label_bg_max = ImVec2(label_pos.x + label_size.x + 3, label_pos.y + label_size.y + 2);
				draw_list->AddRectFilled(label_bg_min, label_bg_max, IM_COL32(0, 0, 0, 180), 3.0f);

				// 라벨 텍스트
				draw_list->AddText(label_pos, IM_COL32(255, 255, 255, 255), light_label);
			}

			// 도움말 텍스트
			ImVec2 help_pos = ImVec2(canvas_pos.x + 5, canvas_pos.y + 5);
			draw_list->AddText(help_pos, IM_COL32(180, 180, 180, 255), "Light Direction Visualization");

			// 캔버스 공간 예약
			ImGui::Dummy(canvas_size);
		}
	}
	ImGui::End();
}

// 컴퓨트 컨트롤 렌더링
void OgSampleViewerWindow::renderComputeControls(OgComputeSample* computeSample)
{
	if (ImGui::Begin("Compute Controls", nullptr))
	{
		ImGui::Text("Compute Shader Settings");
		ImGui::Separator();

		// 컴퓨트 샘플에 특화된 컨트롤들
		ImGui::Text("Compute shader is running");
		ImGui::Text("No parameters available");

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Text("Information");
		ImGui::Separator();

		ImGui::BulletText("Workgroup size: 16x16");
		ImGui::BulletText("Dispatch groups: Auto");
		ImGui::BulletText("Output: Render target");
	}
	ImGui::End();
}

// 삼각형 컨트롤 렌더링
void OgSampleViewerWindow::renderTriangleControls(OgTriangleSample* triangleSample)
{
	if (ImGui::Begin("Triangle Controls", nullptr))
	{
		ImGui::Text("Triangle Sample Settings");
		ImGui::Separator();

		ImGui::Text("Static triangle rendering");
		ImGui::Text("No parameters available");

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Text("Information");
		ImGui::Separator();

		ImGui::BulletText("Vertices: 3");
		ImGui::BulletText("Colors: Interpolated");
		ImGui::BulletText("Shader: Basic vertex/fragment");
	}
	ImGui::End();
}

// 샘플 브라우저 렌더링 (Model Browser를 일반화)
void OgSampleViewerWindow::renderSampleBrowser()
{
	OgSampleBase* sample = GetSample();

	// 현재는 모델 샘플만 브라우저 지원
	if (OgModelSample* modelSample = dynamic_cast<OgModelSample*>(sample))
	{
		renderModelBrowser(modelSample);
	}
}

// 모델 브라우저 렌더링
void OgSampleViewerWindow::renderModelBrowser(OgModelSample* modelSample)
{
	const auto& availableModels = modelSample->GetAvailableModels();
	if (availableModels.empty())
		return;

	ImGuiIO& io = ImGui::GetIO();

	if (ImGui::Begin("glTF Model Browser", &_showModelBrowser, ImGuiWindowFlags_NoCollapse))
	{
		// 모델이 선택되지 않았을 때 메시지 표시
		if (modelSample->GetSelectedModelIndex() == -1)
		{
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 0.0f, 1.0f));
			ImGui::Text("Please select a model to continue");
			ImGui::PopStyleColor();
			ImGui::Separator();
		}

		ImGui::Text("Available Models: %zu", availableModels.size());
		ImGui::Text("Directory: %s", modelSample->GetGLTFDirectory().c_str());
		ImGui::Separator();

		// 현재 로드된 모델 표시
		if (!modelSample->GetCurrentModelPath().empty())
		{
			ImGui::Text("Current Model:");
			ImGui::SameLine();
			ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.0f, 1.0f), "%s", modelSample->GetCurrentModelPath().c_str());
			ImGui::Separator();
		}

		// 검색 필터
		static char searchBuffer[256] = { 0 };
		ImGui::Text("Search:");
		ImGui::SameLine();
		ImGui::InputText("##search", searchBuffer, sizeof(searchBuffer));
		ImGui::Separator();

		// 모델 리스트
		ImGui::BeginChild("ModelList", ImVec2(0, -40), true);
		{
			int selectedIndex = modelSample->GetSelectedModelIndex();
			std::string searchStr(searchBuffer);
			std::transform(searchStr.begin(), searchStr.end(), searchStr.begin(), ::tolower);

			for (int i = 0; i < static_cast<int>(availableModels.size()); ++i)
			{
				std::string modelName = availableModels[i];

				// 검색 필터 적용
				if (!searchStr.empty())
				{
					std::string lowerModelName = modelName;
					std::transform(lowerModelName.begin(), lowerModelName.end(), lowerModelName.begin(), ::tolower);
					if (lowerModelName.find(searchStr) == std::string::npos)
						continue;
				}

				// 폴더 구조 표시를 위한 들여쓰기 계산
				size_t depth = std::count(modelName.begin(), modelName.end(), '\\');
				depth += std::count(modelName.begin(), modelName.end(), '/');

				for (size_t d = 0; d < depth; ++d)
				{
					ImGui::Indent();
				}

				// 파일명만 추출
				size_t lastSlash = modelName.find_last_of("\\/");
				std::string displayName = (lastSlash != std::string::npos) ?
					modelName.substr(lastSlash + 1) : modelName;

				bool isSelected = (i == selectedIndex);
				if (ImGui::Selectable(displayName.c_str(), isSelected))
				{
					if (i != selectedIndex)
					{
						modelSample->LoadSelectedModel(i);
					}
				}

				// 툴팁 표시
				if (ImGui::IsItemHovered())
				{
					ImGui::SetTooltip("%s", modelName.c_str());
				}

				// 더블 클릭으로도 로드
				if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
				{
					modelSample->LoadSelectedModel(i);
				}

				for (size_t d = 0; d < depth; ++d)
				{
					ImGui::Unindent();
				}
			}
		}
		ImGui::EndChild();

		ImGui::Separator();

		// 하단 버튼들
		float buttonWidth = ImGui::GetContentRegionAvail().x;

		if (ImGui::Button("Refresh List", ImVec2(buttonWidth, 25)))
		{
			modelSample->ScanGLTFDirectory(modelSample->GetGLTFDirectory());
		}

		if (ImGui::Button("Reset View", ImVec2(buttonWidth, 25)))
		{
			// 카메라 리셋 기능 추가 가능
		}

		// 모델이 선택되지 않았으면 닫기 버튼 비활성화
		bool canClose = modelSample->GetSelectedModelIndex() != -1;
		if (!canClose)
		{
			ImGui::BeginDisabled();
		}

		if (ImGui::Button("Close Browser", ImVec2(buttonWidth, 25)))
		{
			_showModelBrowser = false;
		}

		if (!canClose)
		{
			ImGui::EndDisabled();
			if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
			{
				ImGui::SetTooltip("Please select a model first");
			}
		}
	}
	ImGui::End();
}

// 레이트레이싱 샘플 뷰어 렌더링
void OgSampleViewerWindow::renderRayTracingSampleViewer(OgRayTracingSample* rayTracingSample)
{
	renderSampleImage(rayTracingSample);
}

// 레이트레이싱 컨트롤 렌더링
void OgSampleViewerWindow::renderRayTracingControls(OgRayTracingSample* rayTracingSample)
{
	if (ImGui::Begin("Ray Tracing Controls", nullptr))
	{
		ImGui::Text("Ray Tracing Settings");
		ImGui::Separator();

		bool settingsChanged = false;

		// Max bounces
		uint32_t maxBounces = rayTracingSample->GetMaxBounces();
		if (ImGui::SliderInt("Max Bounces", reinterpret_cast<int*>(&maxBounces), 1, 10))
		{
			rayTracingSample->SetMaxBounces(maxBounces);
			settingsChanged = true;
		}

		// Samples per pixel
		uint32_t samplesPerPixel = rayTracingSample->GetSamplesPerPixel();
		if (ImGui::SliderInt("Samples Per Pixel", reinterpret_cast<int*>(&samplesPerPixel), 1, 16))
		{
			rayTracingSample->SetSamplesPerPixel(samplesPerPixel);
			settingsChanged = true;
		}

		ImGui::Separator();

		// Camera mode
		bool useFlyCamera = rayTracingSample->IsUsingFlyCamera();
		if (ImGui::Checkbox("Fly Camera Mode", &useFlyCamera))
		{
			rayTracingSample->SetUsingFlyCamera(useFlyCamera);
		}

		ImGui::Separator();
		ImGui::Text("Performance");
		ImGui::Separator();

		// 프레임 카운터 표시 (프로그레시브 렌더링 진행도)
		ImGui::Text("Progressive Frame: %u", 0); // TODO: 실제 프레임 카운터 가져오기
		ImGui::Text("Ray Tracing FPS: %.1f", ImGui::GetIO().Framerate);

		ImGui::Separator();
		ImGui::Text("Information");
		ImGui::Separator();

		ImGui::BulletText("VK_KHR_ray_tracing_pipeline");
		ImGui::BulletText("Path tracing with PBR materials");
		ImGui::BulletText("Progressive refinement");
		ImGui::BulletText("DragonAttenuation model");

		// 프리셋 버튼들
		ImGui::Separator();
		ImGui::Text("Presets");
		ImGui::Separator();

		if (ImGui::Button("Low Quality (Fast)", ImVec2(-1, 0)))
		{
			rayTracingSample->SetMaxBounces(2);
			rayTracingSample->SetSamplesPerPixel(1);
		}

		if (ImGui::Button("Medium Quality", ImVec2(-1, 0)))
		{
			rayTracingSample->SetMaxBounces(3);
			rayTracingSample->SetSamplesPerPixel(2);
		}

		if (ImGui::Button("High Quality (Slow)", ImVec2(-1, 0)))
		{
			rayTracingSample->SetMaxBounces(5);
			rayTracingSample->SetSamplesPerPixel(4);
		}

		if (ImGui::Button("Reset Camera", ImVec2(-1, 0)))
		{
			// TODO: 카메라 리셋 기능 구현
		}
	}
	ImGui::End();
}

OG_NAMESPACE_SAMPLE_END