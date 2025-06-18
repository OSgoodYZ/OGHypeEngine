#pragma once
#ifndef _OG_SAMPLE_APPLICATION_H__
#define _OG_SAMPLE_APPLICATION_H__

#include "OgPrecompile.h"
#include "system/OgSystemContext.h"
#include "system/OgImGUIManager.h"
#include "render/OgRenderContext.h"
#include "sample/public/editor/OgPlayWindow.h"
#include <memory>
#include <vector>

OG_NAMESPACE_SAMPLE_BEGIN

/**
 * @brief 샘플 애플리케이션 관리 클래스
 */
class OG_API OgSampleApplication
{
public:
    OgSampleApplication();
    ~OgSampleApplication();

    // 애플리케이션 실행
    void Run(OgSystemContext* systemContext);

private:
    // 초기화/종료
    void initialize(OgSystemContext* systemContext);
    void shutdown();
    
    // 서브시스템 초기화/종료
    void initRenderContext(OgSystemContext* systemContext);
    void shutdownRenderContext();
    void initImGui();
    void shutdownImGui();
    void createMainWindow();
    void destroyWindows();
    
    // 메인 루프
    void mainLoop();
    void processEvents();
    void updateWindows(float deltaTime);
    void renderWindows();
    void collectGarbage();

    // 유틸리티
    float calculateDeltaTime();

private:
    // 렌더 컨텍스트
    std::unique_ptr<Render::OgRenderContext> _renderContext;
    
    // 윈도우들
    std::vector<std::unique_ptr<OgEditorWindow>> _windows;
    
    // 시간 관리
    uint32 _lastFrameTime;
    float _deltaTime;
    
    // 상태
    bool _isRunning;
};

OG_NAMESPACE_SAMPLE_END

#endif // _OG_SAMPLE_APPLICATION_H__
