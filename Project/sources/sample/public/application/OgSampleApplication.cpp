#include "OgSampleApplication.h"
#include "render/private/vulkan/OgRenderContext_Vulkan.h"
#include "sample/public/core/OgTriangleSample.h"
#include "sample/public/core/OgModelSample.h"
#include "sample/public/core/OgComputeSample.h"
#include "sample/public/core/OgRayTracingSample.h"

OG_NAMESPACE_SAMPLE_BEGIN

OgSampleApplication::OgSampleApplication()
    : _lastFrameTime(0)
    , _deltaTime(0.0f)
    , _isRunning(false)
{
}

OgSampleApplication::~OgSampleApplication()
{
    shutdown();
}

void OgSampleApplication::Run(OgSystemContext* systemContext)
{
    initialize(systemContext);
    mainLoop();
    shutdown();
}

void OgSampleApplication::initialize(OgSystemContext* systemContext)
{
    // 1. 렌더 컨텍스트 초기화
    initRenderContext(systemContext);
    
    // 2. ImGui 초기화
    initImGui();
    
    // 3. 메인 윈도우 생성
    createMainWindow();
    
    _isRunning = true;
}

void OgSampleApplication::initRenderContext(OgSystemContext* systemContext)
{
    _renderContext = std::make_unique<Render::OgRenderContextVulkan>(systemContext);
    _renderContext->Load();
    _renderContext->Init();
}

void OgSampleApplication::initImGui()
{
    OgImGuiContextManager::Initialize();
    ImGuiContext* ctx = OgImGuiContextManager::CreateContext(true);
    OgImGuiContextManager::SetMainImGuiContext(ctx);
}

void OgSampleApplication::createMainWindow()
{
    // 윈도우 설정
    OgEditorWindow::Config config;
    config.title = "Vulkan Sample Viewer";
    config.width = 2560;
    config.height = 1440;
    config.resizable = true;
    
    // 샘플 뷰어 윈도우 생성
    auto window = std::make_unique<OgSampleViewerWindow>(_renderContext.get(), config);
    
    // 레이트레이싱 샘플 설정 (기본으로 레이트레이싱 샘플 표시)
    //auto rayTracingSample = std::make_unique<OgRayTracingSample>(_renderContext.get());
    //window->SetSample(std::move(rayTracingSample));
    
    // 다른 샘플들도 사용 가능:
     auto modelSample = std::make_unique<OgModelSample>(_renderContext.get());
     window->SetSample(std::move(modelSample));
    
    // auto triangleSample = std::make_unique<OgTriangleSample>(_renderContext.get());
    // window->SetSample(std::move(triangleSample));
    
    // Compute Shader 샘플:
     //auto computeSample = std::make_unique<OgComputeSample>(_renderContext.get());
     //window->SetSample(std::move(computeSample));
    
    // 윈도우 열기
    window->Open();
    
    _windows.push_back(std::move(window));
}

void OgSampleApplication::mainLoop()
{
    while (_isRunning)
    {
        float deltaTime = calculateDeltaTime();
        
        processEvents();
        updateWindows(deltaTime);
        renderWindows();
        collectGarbage();
        
        // 모든 윈도우가 닫혔으면 종료
        if (_windows.empty())
        {
            _isRunning = false;
        }
    }
}

void OgSampleApplication::processEvents()
{
    og_system_poll_events();
    
    // 각 윈도우의 이벤트 처리
    for (auto& window : _windows)
    {
        window->PeekEvents();
    }
}

void OgSampleApplication::updateWindows(float deltaTime)
{
    for (auto& window : _windows)
    {
        if (!window->ShouldClose())
        {
            window->Update(deltaTime);
        }
    }
}

void OgSampleApplication::renderWindows()
{
    for (auto& window : _windows)
    {
        if (!window->ShouldClose())
        {
            window->Render();
            window->Present();
        }
    }
    
    _renderContext->Collect();
}

void OgSampleApplication::collectGarbage()
{
    // 닫힌 윈도우 제거
    _windows.erase(
        std::remove_if(_windows.begin(), _windows.end(),
            [](const std::unique_ptr<OgEditorWindow>& window) {
                return window->ShouldClose();
            }),
        _windows.end()
    );
}

float OgSampleApplication::calculateDeltaTime()
{
    // TODO: 실제 시간 계산 구현
    // uint32 currentTime = og_time_milli();
    // _deltaTime = (currentTime - _lastFrameTime) / 1000.0f;
    // _lastFrameTime = currentTime;
    
    _deltaTime = 1.0f / 60.0f; // 임시로 60 FPS 가정
    return _deltaTime;
}

void OgSampleApplication::shutdown()
{
    destroyWindows();
    shutdownImGui();
    shutdownRenderContext();
}

void OgSampleApplication::destroyWindows()
{
    for (auto& window : _windows)
    {
        window->Close();
    }
    _windows.clear();
}

void OgSampleApplication::shutdownImGui()
{
    OgImGuiContextManager::Finalize();
}

void OgSampleApplication::shutdownRenderContext()
{
    if (_renderContext)
    {
        _renderContext->Shutdown();
        _renderContext.reset();
    }
}

OG_NAMESPACE_SAMPLE_END
