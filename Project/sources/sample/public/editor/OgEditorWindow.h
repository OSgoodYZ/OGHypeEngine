#pragma once
#ifndef _OG_EDITOR_WINDOW_H__
#define _OG_EDITOR_WINDOW_H__

#include "OgPrecompile.h"
#include "system/OgNativeWindow.h"
#include "system/OgNativeEvent.h"
#include "render/OgRenderContext.h"
#include <string>
#include <vector>

OG_NAMESPACE_SAMPLE_BEGIN

/**
 * @brief 에디터 윈도우의 기본 클래스
 */
class OG_API OgEditorWindow
{
public:
    struct Config
    {
        std::string title = "Editor Window";
        uint32 width = 800;
        uint32 height = 600;
        bool resizable = true;
        bool decorated = true;
        OgEditorWindow* parent = nullptr;
    };

    OgEditorWindow(Render::OgRenderContext* renderContext, const Config& config);
    virtual ~OgEditorWindow();

    // 윈도우 라이프사이클
    void Open();
    void Close();
    bool ShouldClose() const;
    
    // 메인 루프 메서드
    void PeekEvents();
    void Update(float deltaTime);
    void Render();
    void Present();

    // 속성 접근자
    OgNativeWindow* GetNativeWindow() { return _nativeWindow; }
    uint32 GetWidth() const { return _nativeWindow ? _nativeWindow->width : 0; }
    uint32 GetHeight() const { return _nativeWindow ? _nativeWindow->height : 0; }
    bool IsMinimized() const { return _nativeWindow ? _nativeWindow->minimized : false; }

protected:
    // 서브클래스에서 오버라이드할 메서드들
    virtual void onInit() {}
    virtual void onDestroy() {}
    virtual void onOpen() {}
    virtual void onClose() {}
    virtual void onUpdate(float deltaTime) {}
    virtual void onRender(Render::OgCommandEncoderHandle* encoder) {}
    virtual void onResize(uint32 width, uint32 height) {}
    virtual void onEvent(const OgNativeEvent& evt) {}

    // 내부 메서드
    void processEvent(const OgNativeEvent& evt);
    void createSwapchain();
    void destroySwapchain();
    void createCommandEncoders();
    void destroyCommandEncoders();

protected:
    // 기본 멤버
    Render::OgRenderContext* _renderContext;
    OgNativeWindow* _nativeWindow;
    Render::OgSwapChain* _swapchain;
    
    // 커맨드 인코더 (트리플 버퍼링)
    std::vector<Render::OgCommandEncoderHandle*> _encoders;
    uint32 _currentEncoderIndex;
    
    // 상태
    bool _isOpen;
    bool _shouldClose;
    bool _canRender;
    
    // 설정
    Config _config;
};

OG_NAMESPACE_SAMPLE_END

#endif // _OG_EDITOR_WINDOW_H__
