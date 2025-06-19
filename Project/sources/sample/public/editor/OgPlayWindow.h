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

	// ImGui 관련
	void initImGui();
	void shutdownImGui();
	void beginImGuiFrame();
	void endImGuiFrame();
	void renderImGui(Render::OgCommandEncoderHandle* encoder);

	// UI 렌더링 (서브클래스에서 오버라이드)
	virtual void onRenderUI() {}

private:
	// ImGui 렌더러
	std::unique_ptr<OgImguiRenderer> _imguiRenderer;
	bool _imguiWin32Initialized = false;

	// 현재 샘플
	std::unique_ptr<OgSampleBase> _currentSample;
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
	void renderSampleViewer();
	void renderDebugInfo();
	void renderSampleSelector();
	void switchSample(int index);
	
	int _currentSampleIndex = 1; // 기본은 FBX 샘플
};

OG_NAMESPACE_SAMPLE_END

#endif // _OG_PLAY_WINDOW_H__
