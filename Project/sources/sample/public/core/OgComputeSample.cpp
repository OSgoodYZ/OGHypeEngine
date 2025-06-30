#include "OgComputeSample.h"
#include "util/OgShaderCompiler.h"
#include <cstring>
#include <cmath>

using namespace std;
using namespace Render;

OG_NAMESPACE_SAMPLE_BEGIN

OgComputeSample::OgComputeSample(Render::OgRenderContext* renderContext)
    : OgSampleBase(renderContext)
{
}

OgComputeSample::~OgComputeSample()
{
    if (_isInitialized)
    {
        OnDestroy();
    }
}

void OgComputeSample::OnInit(Render::OgSwapChain* swapchain)
{
    if (_isInitialized)
        return;

    createResources();
    
    // 초기 compute 실행
    executeCompute(swapchain);
    
    _isInitialized = true;
}

void OgComputeSample::OnDestroy()
{
    if (!_isInitialized)
        return;
        
    destroyResources();
    _isInitialized = false;
}

void OgComputeSample::OnUpdate(float deltaTime)
{
    _elapsedTime += deltaTime;
}

void OgComputeSample::OnRender(Render::OgCommandEncoderHandle* encoder, Render::OgSwapChain* swapchain)
{
    if (!_isInitialized)
        return;

    // 주기적으로 compute shader 재실행 (1초마다)
    static float accumulatedTime = 0.0f;
    accumulatedTime += 0.016f; // 60 FPS 가정
    
    if (accumulatedTime > 1.0f)
    {
        accumulatedTime = 0.0f;
        executeCompute(swapchain);
    }
    
    // 결과 시각화
    uint32 width = _renderContext->GetSwapChainFrameBuffer(swapchain, 0)->width;
    uint32 height = _renderContext->GetSwapChainFrameBuffer(swapchain, 0)->height;
    renderResults(encoder, width, height);
}

void OgComputeSample::OnResize(uint32 width, uint32 height)
{
    // 이 샘플에서는 크기 변경 처리 필요 없음
}

void OgComputeSample::OnSuspend(Render::OgSwapChain* swapchain)
{
    _renderContext->Suspend(swapchain);
}

void OgComputeSample::OnRestore(Render::OgSwapChain* swapchain)
{
    _renderContext->Restore(swapchain);
}

bool OgComputeSample::GetComputeResults(float* outResults, uint32 count)
{
    if (!_computeExecuted || !_outputBuffer || count > ARRAY_SIZE)
        return false;
    
    // Map buffer and read results
    void* mappedData = _renderContext->MapBuffer(_outputBuffer, sizeof(float) * count);
    if (mappedData)
    {
        memcpy(outResults, mappedData, sizeof(float) * count);
        _renderContext->UnmapBuffer(_outputBuffer);
        return true;
    }
    
    return false;
}

void OgComputeSample::createResources()
{
    createBuffers();
    createComputeShader();
    createComputePipeline();
    createRenderingResources();
}

void OgComputeSample::destroyResources()
{
    _renderContext->WaitDeviceIdle();
    
    // Compute resources
    if (_computeResourceSet)
    {
        _computeResourceSet->Release();
        _computeResourceSet = nullptr;
    }
    
    if (_computeResourceLayout)
    {
        _computeResourceLayout->Release();
        _computeResourceLayout = nullptr;
    }
    
    if (_computePipeline)
    {
        _computePipeline->Release();
        _computePipeline = nullptr;
    }
    
    if (_computeShader)
    {
        _computeShader->Release();
        _computeShader = nullptr;
    }
    
    // Buffers
    if (_uniformBuffer)
    {
        _uniformBuffer->Release();
        _uniformBuffer = nullptr;
    }
    
    if (_outputBuffer)
    {
        _outputBuffer->Release();
        _outputBuffer = nullptr;
    }
    
    if (_inputBufferB)
    {
        _inputBufferB->Release();
        _inputBufferB = nullptr;
    }
    
    if (_inputBufferA)
    {
        _inputBufferA->Release();
        _inputBufferA = nullptr;
    }
    
    // Rendering resources
    if (_renderResourceSet)
    {
        _renderResourceSet->Release();
        _renderResourceSet = nullptr;
    }
    
    if (_renderResourceLayout)
    {
        _renderResourceLayout->Release();
        _renderResourceLayout = nullptr;
    }
    
    if (_renderPipeline)
    {
        _renderPipeline->Release();
        _renderPipeline = nullptr;
    }
    
    if (_program)
    {
        _program->Release();
        _program = nullptr;
    }
    
    if (_fragmentShader)
    {
        _fragmentShader->Release();
        _fragmentShader = nullptr;
    }
    
    if (_vertexShader)
    {
        _vertexShader->Release();
        _vertexShader = nullptr;
    }
    
    if (_renderUniformBuffer)
    {
        _renderUniformBuffer->Release();
        _renderUniformBuffer = nullptr;
    }
    
    if (_vertexBuffer)
    {
        _vertexBuffer->Release();
        _vertexBuffer = nullptr;
    }
}

void OgComputeSample::createComputeShader()
{
    // Compute shader GLSL 코드
    const char* computeShaderGLSL = R"(
        #version 450
        
        layout(local_size_x = 64, local_size_y = 1, local_size_z = 1) in;
        
        layout(binding = 0) uniform ComputeParams {
            uint arraySize;
            float multiplier;
        } params;
        
        layout(binding = 1, std430) readonly buffer InputBufferA {
            float dataA[];
        } inputA;
        
        layout(binding = 2, std430) readonly buffer InputBufferB {
            float dataB[];
        } inputB;
        
        layout(binding = 3, std430) writeonly buffer OutputBuffer {
            float result[];
        } output;
        
        void main() {
            uint index = gl_GlobalInvocationID.x;
            
            if (index >= params.arraySize)
                return;
            
            // 간단한 연산: (A[i] + B[i]) * multiplier + sin(index)
            float a = inputA.dataA[index];
            float b = inputB.dataB[index];
            float sinValue = sin(float(index) * 0.1);
            
            output.result[index] = (a + b) * params.multiplier + sinValue;
        }
    )";
    
    // GLSL을 SPIR-V로 컴파일
    std::vector<uint32_t> computeSPIRV;
    
    if (!OgShaderCompiler::CompileGLSLtoSPIRV(computeShaderGLSL, OgShaderType::COMPUTE, computeSPIRV))
    {
        LOGE(OG_ID, "Failed to compile compute shader");
        return;
    }
    
    // 컴파일된 SPIR-V로 셰이더 생성
    _computeShader = _renderContext->CreateShader(
        OgShaderType::COMPUTE, 
        reinterpret_cast<const char*>(computeSPIRV.data()), 
        computeSPIRV.size() * sizeof(uint32_t), 
        "main"
    );
    _computeShader->name = "ComputeSampleShader";
    _computeShader->Retain();
}

void OgComputeSample::createBuffers()
{
    // 입력 데이터 초기화
    std::vector<float> dataA(ARRAY_SIZE);
    std::vector<float> dataB(ARRAY_SIZE);
    
    for (uint32 i = 0; i < ARRAY_SIZE; ++i)
    {
        dataA[i] = static_cast<float>(i);
        dataB[i] = static_cast<float>(i) * 0.5f;
    }
    
    // Input Buffer A
    _inputBufferA = _renderContext->CreateBuffer(
        dataA.data(),
        sizeof(float) * ARRAY_SIZE,
        OgBufferUsage::STORAGE,
        OgMemoryOption::PRIVATE_GPU
    );
    _inputBufferA->name = "ComputeInputBufferA";
    _inputBufferA->Retain();
    
    // Input Buffer B  
    _inputBufferB = _renderContext->CreateBuffer(
        dataB.data(),
        sizeof(float) * ARRAY_SIZE,
        OgBufferUsage::STORAGE,
        OgMemoryOption::PRIVATE_GPU
    );
    _inputBufferB->name = "ComputeInputBufferB";
    _inputBufferB->Retain();
    
    // Output Buffer (읽기 위해 MAP_MANAGED 사용)
    _outputBuffer = _renderContext->CreateBuffer(
        nullptr,
        sizeof(float) * ARRAY_SIZE,
        OgBufferUsage::STORAGE,
        OgMemoryOption::MAP_MANAGED
    );
    _outputBuffer->name = "ComputeOutputBuffer";
    _outputBuffer->Retain();
    
    // Uniform Buffer
    struct ComputeParams {
        uint32 arraySize;
        float multiplier;
    } params = { ARRAY_SIZE, 2.0f };
    
    _uniformBuffer = _renderContext->CreateBuffer(
        &params,
        sizeof(ComputeParams),
        OgBufferUsage::UNIFORM,
        OgMemoryOption::MAP_MANAGED
    );
    _uniformBuffer->name = "ComputeUniformBuffer";
    _uniformBuffer->Retain();
}

void OgComputeSample::createComputePipeline()
{
    // Resource Layout 생성
    OgResourceBinding bindings[4];
    
    // Uniform buffer binding
    bindings[0].type = OgResourceType::UNIFORM_BUFFER;
    bindings[0].stage = OgShaderType::COMPUTE;
    bindings[0].binding = 0;
    bindings[0].arrayCount = 0;
    bindings[0].name = "ComputeParams";
    
    // Storage buffer bindings
    bindings[1].type = OgResourceType::STORAGE_BUFFER;
    bindings[1].stage = OgShaderType::COMPUTE;
    bindings[1].binding = 1;
    bindings[1].arrayCount = 0;
    bindings[1].name = "InputBufferA";
    
    bindings[2].type = OgResourceType::STORAGE_BUFFER;
    bindings[2].stage = OgShaderType::COMPUTE;
    bindings[2].binding = 2;
    bindings[2].arrayCount = 0;
    bindings[2].name = "InputBufferB";
    
    bindings[3].type = OgResourceType::STORAGE_BUFFER;
    bindings[3].stage = OgShaderType::COMPUTE;
    bindings[3].binding = 3;
    bindings[3].arrayCount = 0;
    bindings[3].name = "OutputBuffer";
    
    _computeResourceLayout = _renderContext->CreateResourceLayout(bindings, 4);
    _computeResourceLayout->name = "ComputeSampleResourceLayout";
    _computeResourceLayout->Retain();
    
    // Resource Set 생성
    OgResourceUsage usages[4];
    uint32 offset = 0;
    uint32 uniformSize = sizeof(uint32) + sizeof(float);
    uint32 storageSize = sizeof(float) * ARRAY_SIZE;
    
    usages[0].binding = bindings[0];
    usages[0].buffer.handle = &_uniformBuffer;
    usages[0].buffer.offset = &offset;
    usages[0].buffer.range = &uniformSize;
    
    usages[1].binding = bindings[1];
    usages[1].buffer.handle = &_inputBufferA;
    usages[1].buffer.offset = &offset;
    usages[1].buffer.range = &storageSize;
    
    usages[2].binding = bindings[2];
    usages[2].buffer.handle = &_inputBufferB;
    usages[2].buffer.offset = &offset;
    usages[2].buffer.range = &storageSize;
    
    usages[3].binding = bindings[3];
    usages[3].buffer.handle = &_outputBuffer;
    usages[3].buffer.offset = &offset;
    usages[3].buffer.range = &storageSize;
    
    _computeResourceSet = _renderContext->CreateResourceSet(_computeResourceLayout, usages, 4);
    _computeResourceSet->Retain();
    
    // Compute Pipeline 생성
    OgPipelineDescriptor descriptor;
    descriptor.type = OgPipelineType::COMPUTE_PIPELINE;
    descriptor.name = "ComputeSamplePipeline";
    descriptor.resourceLayout = _computeResourceLayout;
    
    OgShaderDescriptor shaderDesc;
    shaderDesc.shaders[0] = _computeShader;
    shaderDesc.shaderCount = 1;
    descriptor.shader = shaderDesc;
    
    _computePipeline = _renderContext->CreatePipeline(descriptor);
    _computePipeline->Retain();
}

void OgComputeSample::createRenderingResources()
{
    // 결과를 시각화하기 위한 간단한 렌더링 파이프라인 생성
    
    // Vertex shader
    const char* vertexShaderGLSL = R"(
        #version 450
        
        layout(location = 0) in vec2 a_Position;
        layout(location = 0) out vec2 v_UV;
        
        void main()
        {
            gl_Position = vec4(a_Position, 0.0, 1.0);
            v_UV = a_Position * 0.5 + 0.5;
        }
    )";
    
    // Fragment shader - compute 결과를 시각화
    const char* fragmentShaderGLSL = R"(
        #version 450
        
        layout(location = 0) in vec2 v_UV;
        layout(location = 0) out vec4 FragColor;
        
        layout(binding = 0) uniform RenderParams {
            float maxValue;
            float padding1;
            float padding2;
            float padding3;
        } params;
        
        void main()
        {
            // UV를 기반으로 색상 결정 (compute 결과 시각화 예시)
            float intensity = v_UV.x;
            FragColor = vec4(intensity, 1.0 - intensity, 0.5, 1.0);
        }
    )";
    
    // 셰이더 컴파일
    std::vector<uint32_t> vertexSPIRV, fragmentSPIRV;
    
    if (!OgShaderCompiler::CompileGLSLtoSPIRV(vertexShaderGLSL, OgShaderType::VERTEX, vertexSPIRV) ||
        !OgShaderCompiler::CompileGLSLtoSPIRV(fragmentShaderGLSL, OgShaderType::FRAGMENT, fragmentSPIRV))
    {
        LOGE(OG_ID, "Failed to compile rendering shaders");
        return;
    }
    
    _vertexShader = _renderContext->CreateShader(
        OgShaderType::VERTEX,
        reinterpret_cast<const char*>(vertexSPIRV.data()),
        vertexSPIRV.size() * sizeof(uint32_t),
        "main"
    );
    _vertexShader->Retain();
    
    _fragmentShader = _renderContext->CreateShader(
        OgShaderType::FRAGMENT,
        reinterpret_cast<const char*>(fragmentSPIRV.data()),
        fragmentSPIRV.size() * sizeof(uint32_t),
        "main"
    );
    _fragmentShader->Retain();
    
    OgShaderHandle* handles[] = { _vertexShader, _fragmentShader };
    _program = _renderContext->CreateProgram(handles, 2);
    _program->Retain();
    
    // 전체 화면 쿼드를 위한 vertex buffer
    float vertices[] = {
        -1.0f, -1.0f,
         3.0f, -1.0f,
        -1.0f,  3.0f
    };
    
    _vertexBuffer = _renderContext->CreateBuffer(
        vertices,
        sizeof(vertices),
        OgBufferUsage::VERTEX,
        OgMemoryOption::PRIVATE_GPU
    );
    _vertexBuffer->Retain();
    
    // Render uniform buffer
    float renderParams[] = { 100.0f, 0.0f, 0.0f, 0.0f };
    _renderUniformBuffer = _renderContext->CreateBuffer(
        renderParams,
        sizeof(renderParams),
        OgBufferUsage::UNIFORM,
        OgMemoryOption::MAP_MANAGED
    );
    _renderUniformBuffer->Retain();
}

void OgComputeSample::executeCompute(Render::OgSwapChain* swapchain)
{
    // Uniform buffer 업데이트 (multiplier를 시간에 따라 변경)
    struct ComputeParams {
        uint32 arraySize;
        float multiplier;
    } params = { ARRAY_SIZE, 2.0f + sinf(_elapsedTime) };
    
    void* mappedData = _renderContext->MapBuffer(_uniformBuffer, sizeof(ComputeParams));
    if (mappedData)
    {
        memcpy(mappedData, &params, sizeof(ComputeParams));
        _renderContext->UnmapBuffer(_uniformBuffer);
    }
    
    // Command encoder 생성
    OgCommandEncoderHandle* encoder = _renderContext->CreateCommandEncoder();
    encoder->Begin();
    
    // Compute pipeline 바인딩
    encoder->BindComputePipeline(_computePipeline);
    
    // Resource set 바인딩
    encoder->BindResourceSet(_computeResourceSet);
    
    // Dispatch compute shader
    uint32 groupCount = (ARRAY_SIZE + WORKGROUP_SIZE - 1) / WORKGROUP_SIZE;
    encoder->Dispatch(groupCount, 1, 1);
    
    // Memory barrier - compute 결과가 읽기 가능하도록
    encoder->MemoryBarrier(
        (uint32)OgAccessFlag::SHADER_WRITE,
        (uint32)OgAccessFlag::HOST_READ,
        (uint32)OgPipelineStageFlag::COMPUTE_SHADER,
        (uint32)OgPipelineStageFlag::HOST
    );
    
    encoder->End();
    
    // Submit
    _renderContext->Submit(swapchain, encoder);
    
    // Cleanup
    _renderContext->DestroyCommandEncoder(encoder);
    
    _computeExecuted = true;
}

void OgComputeSample::renderResults(Render::OgCommandEncoderHandle* encoder, uint32 width, uint32 height)
{
    // 이 부분은 실제 렌더링 파이프라인이 필요합니다.
    // 현재는 간단히 주석 처리
    
    // 실제로는:
    // 1. Render pass 생성
    // 2. Pipeline 바인딩
    // 3. Vertex buffer 바인딩
    // 4. Draw call
    
    // 예시:
    /*
    encoder->BeginRenderPass(...);
    encoder->SetViewport(0, 0, width, height);
    encoder->BindPipeline(_renderPipeline);
    encoder->BindResourceSet(_renderResourceSet);
    encoder->BindVertexBuffers(&_vertexBuffer, nullptr, 1);
    encoder->DrawArrays(0, 3, 1);
    encoder->EndRenderPass();
    */
}

OG_NAMESPACE_SAMPLE_END
