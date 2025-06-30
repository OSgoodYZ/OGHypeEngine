#pragma once
#ifndef _OG_PLAY_WINDOW_H__
#define _OG_PLAY_WINDOW_H__

#include "OgEditorWindow.h"
#include "OgImguiRenderer.h"
#include "sample/public/core/OgSampleBase.h"
#include "system/OgImGUIManager.h"
#include "system/thirdparty/imgui/imgui.h"
#include "system/thirdparty/imgui/backends/imgui_impl_win32.h"
#include <memory>

// Forward declare message handler
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

OG_NAMESPACE_SAMPLE_BEGIN

// Forward declarations
class OgModelSample;
class OgComputeSample;
class OgTriangleSample;

/**
 * @brief ImGui를 사용하는 플레이 윈도우
 */
class OG_API OgPlayWindow : public OgEditorWindow
{
public:
	OgPlayWindow(Render::OgRenderContext* renderContext, const Config& config);
	~OgPlayWindow() override;

	// 샘플 관리
	void SetSample(std::unique_ptr<OgSampleBase> sample);
	OgSampleBase* GetSample() const { return _currentSample.get(); }

protected:
	// OgEditorWindow 오버라이드
	void onInit() override;
	void onDestroy() override;
	void onUpdate(float deltaTime) override;
	void onRender(Render::OgCommandEncoderHandle* encoder) override;
	void onResize(uint32 width, uint32 height) override;
	void onEvent(const OgNativeEvent& evt) override;

	// ImGui 관련
	void initImGui();
	void shutdownImGui();
	void beginImGuiFrame();
	void endImGuiFrame();
	void renderImGui(Render::OgCommandEncoderHandle* encoder);

	// UI 렌더링 (서브클래스에서 오버라이드)
	virtual void onRenderUI() {}

	// 샘플별 처리 메서드
	void handleSampleSpecificInit();
	bool shouldRenderSample() const;
	void handleSampleSpecificEvent(const OgNativeEvent& evt, const ImGuiIO& io);

	// 샘플별 이벤트 핸들러
	void handleModelSampleEvent(OgModelSample* modelSample, const OgNativeEvent& evt, const ImGuiIO& io);
	void handleComputeSampleEvent(OgComputeSample* computeSample, const OgNativeEvent& evt, const ImGuiIO& io);

protected:
	// 브라우저 표시 여부 (서브클래스에서 접근 가능)
	bool _showModelBrowser = false;
	std::unique_ptr<OgSampleBase> _currentSample;

private:
	// ImGui 렌더러
	std::unique_ptr<OgImguiRenderer> _imguiRenderer;
	bool _imguiWin32Initialized = false;

	
	
};

/**
 * @brief 샘플 뷰어 윈도우 - 실제 구현 예제
 */
class OG_API OgSampleViewerWindow : public OgPlayWindow
{
public:
	OgSampleViewerWindow(Render::OgRenderContext* renderContext, const Config& config)
		: OgPlayWindow(renderContext, config)
	{
	}

protected:
	void onRenderUI() override;

private:
	// UI 렌더링 메서드들
	void renderSampleViewer();
	void renderDebugInfo();
	void renderSampleSelector();
	void renderSampleControls();
	void renderSampleBrowser();

	// 샘플별 뷰어 렌더링
	void renderModelSampleViewer(OgModelSample* modelSample);
	void renderComputeSampleViewer(OgComputeSample* computeSample);
	void renderTriangleSampleViewer(OgTriangleSample* triangleSample);
	void renderDefaultViewer(OgSampleBase* sample);
	void renderSampleImage(OgSampleBase* sample);

	// 샘플별 컨트롤 렌더링
	void renderLightControls(OgModelSample* modelSample);
	void renderComputeControls(OgComputeSample* computeSample);
	void renderTriangleControls(OgTriangleSample* triangleSample);

	// 샘플별 브라우저 렌더링
	void renderModelBrowser(OgModelSample* modelSample);

	// 헬퍼 메서드들
	void switchSample(int index);
	bool shouldShowBrowserPanel() const;
	void renderSampleControlsInfo();
	void resetCurrentSample();

	// XYZ 축 기즈모 렌더링
	void renderXYZGizmo(ImVec2 imagePos, ImVec2 imageSize);

	int _currentSampleIndex = 1; // 기본은 Model 샘플
};

OG_NAMESPACE_SAMPLE_END

#endif // _OG_PLAY_WINDOW_H__