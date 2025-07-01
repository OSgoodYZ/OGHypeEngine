#include "OgComputeSample.h"
#include "sample/public/core/util/OgShaderCompiler.h"
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

    // 스왑체인의 크기로 리소스 생성
    const uint16 width = _renderContext->GetSwapChainFrameBuffer(swapchain, 0)->width;
    const uint16 height = _renderContext->GetSwapChainFrameBuffer(swapchain, 0)->height;

    createResources(width, height);
    
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
    _accumulatedTime += deltaTime;
}

void OgComputeSample::OnRender(Render::OgCommandEncoderHandle* encoder, Render::OgSwapChain* swapchain)
{
    if (!_isInitialized || !_renderTargetFrameBuffer)
        return;

    // 주기적으로 compute shader 재실행 (0.5초마다)
    if (_accumulatedTime > 0.5f)
    {
        _accumulatedTime = 0.0f;
        executeCompute(swapchain);
    }
    
    // 결과를 렌더 타겟에 시각화
    renderResults(encoder, _renderTargetWidth, _renderTargetHeight);
}

void OgComputeSample::OnResize(uint32 width, uint32 height)
{
    if (!_isInitialized)
        return;

    // 크기가 변경되면 렌더 타겟 재생성
    destroyRenderTarget();
    createRenderTarget(static_cast<uint16>(width), static_cast<uint16>(height));
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

void OgComputeSample::createResources(uint16 width, uint16 height)
{
    // 렌더 타겟을 먼저 생성
    createRenderTarget(width, height);
    
    createBuffers();
    createComputeShader();
    createComputePipeline();
    createRenderingResources();
}

void OgComputeSample::destroyResources()
{
    _renderContext->WaitDeviceIdle();
    
    // Rendering resources
    if (_renderPipeline)
    {
        _renderPipeline->Release();
        _renderPipeline = nullptr;
    }
    
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
    
    // Render target
    destroyRenderTarget();
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
            float time;
            float padding;
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
            
            // 더 흥미로운 연산: 사인파 + 코사인파 조합
            float a = inputA.dataA[index];
            float b = inputB.dataB[index];
            float phase = float(index) * 0.1 + params.time;
            float sinValue = sin(phase);
            float cosValue = cos(phase * 0.7);
            
            // 결과는 0~1 범위로 정규화
            float result = (a + b) * params.multiplier + sinValue * 0.5 + cosValue * 0.3;
            output.result[index] = clamp(result / 100.0, 0.0, 1.0);
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
        float time;
        float padding;
    } params = { ARRAY_SIZE, 2.0f, 0.0f, 0.0f };
    
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
    bindings[0].name = nullptr;
    
    // Storage buffer bindings
    bindings[1].type = OgResourceType::STORAGE_BUFFER;
    bindings[1].stage = OgShaderType::COMPUTE;
    bindings[1].binding = 1;
    bindings[1].arrayCount = 0;
    bindings[1].name = nullptr;
    
    bindings[2].type = OgResourceType::STORAGE_BUFFER;
    bindings[2].stage = OgShaderType::COMPUTE;
    bindings[2].binding = 2;
    bindings[2].arrayCount = 0;
    bindings[2].name = nullptr;
    
    bindings[3].type = OgResourceType::STORAGE_BUFFER;
    bindings[3].stage = OgShaderType::COMPUTE;
    bindings[3].binding = 3;
    bindings[3].arrayCount = 0;
    bindings[3].name = nullptr;
    
    _computeResourceLayout = _renderContext->CreateResourceLayout(bindings, 4);
    _computeResourceLayout->name = "ComputeSampleResourceLayout";
    _computeResourceLayout->Retain();
    
    // Resource Set 생성
    OgResourceUsage usages[4];
    uint32 offset = 0;
    uint32 uniformSize = sizeof(uint32) + sizeof(float) * 3;
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
    // 결과를 시각화하기 위한 렌더링 파이프라인 생성
    
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
            float time;
            float arraySize;
            float padding1;
            float padding2;
        } params;
        
        layout(binding = 1, std430) readonly buffer ComputeResult {
            float data[];
        } computeResult;
        
        vec3 hsv2rgb(vec3 c) {
            vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
            vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
            return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
        }
        
        void main()
        {
            // 배열 인덱스 계산
            uint index = uint(v_UV.x * params.arraySize);
            if (index >= uint(params.arraySize)) index = uint(params.arraySize) - 1;
            
            // Compute 결과 읽기
            float value = computeResult.data[index];
            
            // 시각화: 값에 따라 색상 결정
            float hue = value * 0.8 + params.time * 0.1; // 색조
            float saturation = 0.8;
            float brightness = 0.5 + value * 0.5;
            
            vec3 color = hsv2rgb(vec3(hue, saturation, brightness));
            
            
            float wave = sin(v_UV.x * 10.0 + params.time) * 0.1;

            float intensity = smoothstep(0.0, 1.0, 1.0 - abs(v_UV.y - 0.5 - value * 0.3 - wave));
            
            FragColor = vec4(color * intensity, 1.0);
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
    float renderParams[] = { 0.0f, static_cast<float>(ARRAY_SIZE), 0.0f, 0.0f };
    _renderUniformBuffer = _renderContext->CreateBuffer(
        renderParams,
        sizeof(renderParams),
        OgBufferUsage::UNIFORM,
        OgMemoryOption::MAP_MANAGED
    );
    _renderUniformBuffer->Retain();
    
    // Render Resource Layout
    OgResourceBinding renderBindings[2];
    
    renderBindings[0].type = OgResourceType::UNIFORM_BUFFER;
    renderBindings[0].stage = OgShaderType::FRAGMENT;
    renderBindings[0].binding = 0;
    renderBindings[0].arrayCount = 0;
    renderBindings[0].name = nullptr;
    
    renderBindings[1].type = OgResourceType::STORAGE_BUFFER;
    renderBindings[1].stage = OgShaderType::FRAGMENT;
    renderBindings[1].binding = 1;
    renderBindings[1].arrayCount = 0;
    renderBindings[1].name = nullptr;
    
    _renderResourceLayout = _renderContext->CreateResourceLayout(renderBindings, 2);
    _renderResourceLayout->name = "ComputeRenderResourceLayout";
    _renderResourceLayout->Retain();
    
    // Render Resource Set
    OgResourceUsage renderUsages[2];
    uint32 offset = 0;
    uint32 uniformSize = sizeof(float) * 4;
    uint32 storageSize = sizeof(float) * ARRAY_SIZE;
    
    renderUsages[0].binding = renderBindings[0];
    renderUsages[0].buffer.handle = &_renderUniformBuffer;
    renderUsages[0].buffer.offset = &offset;
    renderUsages[0].buffer.range = &uniformSize;
    
    renderUsages[1].binding = renderBindings[1];
    renderUsages[1].buffer.handle = &_outputBuffer;
    renderUsages[1].buffer.offset = &offset;
    renderUsages[1].buffer.range = &storageSize;
    
    _renderResourceSet = _renderContext->CreateResourceSet(_renderResourceLayout, renderUsages, 2);
    _renderResourceSet->Retain();
    
    // Render Pipeline
    OgColorBlendDescriptor cbDesc{};
    cbDesc.attachmentCount = 1;
    cbDesc.attachments[0].blendEnable = false;
    
    OgRasterizationDescriptor rsDesc{};
    rsDesc.polygonMode = OgPolygonMode::FILL;
    rsDesc.cullMode = OgCullMode::NONE;
    rsDesc.frontFace = OgFrontFace::CLOCKWISE;
    rsDesc.scissorTest = false;
    rsDesc.primitiveType = OgPrimitiveType::TRIANGLE_LIST;
    
    OgDepthStencilDescriptor dsDesc{};
    dsDesc.depthTest = false;
    dsDesc.depthWrite = false;
    dsDesc.stencilTest = false;
    
    OgShaderDescriptor shDesc{};
    shDesc.shaderCount = 2;
    shDesc.shaders[0] = _vertexShader;
    shDesc.shaders[1] = _fragmentShader;
    shDesc.program = _program;
    
    OgVertexInputDescriptor viDesc{};
    OgVertexBufferLayoutDescriptor vblDesc[1] = {
        OgVertexBufferLayoutDescriptor(0, sizeof(float) * 2)
    };
    OgVertexAttributeDescriptor vaDesc[1] = {
        OgVertexAttributeDescriptor(0, 0, OgVertexFormat::FLOAT2, 0)
    };
    viDesc.attributes = vaDesc;
    viDesc.attributeCount = 1;
    viDesc.layouts = vblDesc;
    viDesc.layoutCount = 1;
    
    OgPipelineDescriptor pipeDesc{};
    pipeDesc.name = "ComputeRenderPipeline";
    pipeDesc.type = OgPipelineType::GRAPHICS_PIPELINE;
    pipeDesc.colorBlend = cbDesc;
    pipeDesc.depthStencil = dsDesc;
    pipeDesc.rasterize = rsDesc;
    pipeDesc.vertexInput = viDesc;
    pipeDesc.renderPass = _renderTargetRenderPass;
    pipeDesc.shader = shDesc;
    pipeDesc.resourceLayout = _renderResourceLayout;
    
    _renderPipeline = _renderContext->CreatePipeline(pipeDesc);
    _renderPipeline->Retain();
}

void OgComputeSample::executeCompute(Render::OgSwapChain* swapchain)
{
    // Uniform buffer 업데이트 (시간 값 업데이트)
    struct ComputeParams {
        uint32 arraySize;
        float multiplier;
        float time;
        float padding;
    } params = { ARRAY_SIZE, 2.0f + sinf(_elapsedTime), _elapsedTime, 0.0f };
    
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
    
    // Memory barrier - compute 결과가 fragment shader에서 읽기 가능하도록
    encoder->MemoryBarrier(
        (uint32)OgAccessFlag::SHADER_WRITE,
        (uint32)OgAccessFlag::SHADER_READ,
        (uint32)OgPipelineStageFlag::COMPUTE_SHADER,
        (uint32)OgPipelineStageFlag::FRAGMENT_SHADER
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
    if (!_renderTargetFrameBuffer || !_renderPipeline)
        return;
        
    // 렌더 uniform buffer 업데이트
    float renderParams[] = { _elapsedTime, static_cast<float>(ARRAY_SIZE), 0.0f, 0.0f };
    void* mappedData = _renderContext->MapBuffer(_renderUniformBuffer, sizeof(renderParams));
    if (mappedData)
    {
        memcpy(mappedData, renderParams, sizeof(renderParams));
        _renderContext->UnmapBuffer(_renderUniformBuffer);
    }
    
    // Clear values
    OgCommandEncoderHandle::ClearValue colorClear;
    colorClear.color.value[0] = 0.1f;
    colorClear.color.value[1] = 0.1f;
    colorClear.color.value[2] = 0.15f;
    colorClear.color.value[3] = 1.0f;
    
    OgCommandEncoderHandle::ClearValue depthStencilClear;
    depthStencilClear.depthStencil.depth = 1.0f;
    depthStencilClear.depthStencil.stencil = 0;
    
    // 렌더 타겟 프레임버퍼 사용
    OgCommandEncoderHandle::Area area(0, 0, width, height);
    
    encoder->BeginDebugMarker("Sample - Compute Visualization", colorClear.color.value);
    
    // Begin render pass
    encoder->BeginRenderPass(_renderTargetRenderPass, _renderTargetFrameBuffer, area, 1, &colorClear, 0, nullptr, &depthStencilClear);
    
    // Set viewport
    encoder->SetViewport(static_cast<float>(area.x), static_cast<float>(area.y),
        static_cast<float>(area.width), static_cast<float>(area.height));
    encoder->SetScissor(area.x, area.y, area.width, area.height);
    
    // Bind pipeline
    encoder->BindPipeline(_renderPipeline);
    
    // Bind resource set
    encoder->BindResourceSet(_renderResourceSet);
    
    // Bind vertex buffer
    encoder->BindVertexBuffers(&_vertexBuffer, 0, 1);
    
    // Draw full screen triangle
    encoder->DrawArrays(0, 3, 1);
    
    // End render pass
    encoder->EndRenderPass();
    encoder->EndDebugMarker();
}

void OgComputeSample::createRenderTarget(uint16 width, uint16 height)
{
    _renderTargetWidth = width;
    _renderTargetHeight = height;
    
    // 샘플러 생성
    OgSamplerInfo samplerInfo{};
    samplerInfo.type = OgSamplerType::TEX_2D;
    samplerInfo.addressU = OgSamplerAddressMode::REPEAT;
    samplerInfo.addressV = OgSamplerAddressMode::REPEAT;
    samplerInfo.magFilter = OgFilter::LINEAR;
    samplerInfo.minFilter = OgFilter::LINEAR;
    samplerInfo.mipmapMode = OgSamplerMipmapMode::NEAREST;
    
    OgSamplerHandle* sampler = _renderContext->CreateSampler(samplerInfo);
    
    // 렌더 타겟 텍스처 생성
    OgTextureInfo texInfo{};
    texInfo.type = OgTextureType::TEX_2D;
    texInfo.format = OgPixelFormat::R8G8B8A8_UNORM;
    texInfo.extent.width = width;
    texInfo.extent.height = height;
    texInfo.usage = OgTextureUsage::COLOR_ATTACHMENT | OgTextureUsage::SAMPLED;
    texInfo.isGenerateMipmaps = false;
    
    _renderTargetTexture = _renderContext->CreateTexture((void**)nullptr, texInfo, sampler);
    _renderTargetTexture->name = "ComputeRenderTarget";
    _renderTargetTexture->Retain();
    
    // 깊이 텍스처 생성
    OgTextureInfo depthTexInfo{};
    depthTexInfo.type = OgTextureType::TEX_2D;
    depthTexInfo.format = OgPixelFormat::D24_UNORM_S8_UINT;
    depthTexInfo.extent.width = width;
    depthTexInfo.extent.height = height;
    depthTexInfo.usage = OgTextureUsage::DEPTH_STENCIL_ATTACHMENT;
    
    OgSamplerHandle* depthSampler = _renderContext->CreateSampler(samplerInfo);
    _depthTexture = _renderContext->CreateTexture(nullptr, depthTexInfo, depthSampler);
    _depthTexture->name = "ComputeDepthBuffer";
    _depthTexture->Retain();
    
    // 렌더 패스 생성
    OgAttachment rtColor{};
    rtColor.isDepthStencilAttachment = false;
    rtColor.format = OgRenderTextureFormat::R8G8B8A8_UNORM;
    rtColor.load = OgRenderBufferLoadAction::CLEAR;
    rtColor.store = OgRenderBufferStoreAction::STORE;
    
    OgAttachment rtDepth{};
    rtDepth.isDepthStencilAttachment = true;
    rtDepth.format = OgRenderTextureFormat::DEPTH24_STENCIL8;
    rtDepth.load = OgRenderBufferLoadAction::CLEAR;
    rtDepth.store = OgRenderBufferStoreAction::STORE;
    
    OgRenderPassInfo rtRpInfo{};
    rtRpInfo.isSwapchainRenderPass = false;
    rtRpInfo.outputColorAttachmentCount = 1;
    rtRpInfo.outputColorAttachments = &rtColor;
    rtRpInfo.useDepthStencilAttachment = true;
    rtRpInfo.outputDepthStencilAttachment = rtDepth;
    rtRpInfo.resolveColorAttachmentCount = 0;
    
    _renderTargetRenderPass = _renderContext->CreateRenderPass(rtRpInfo);
    _renderTargetRenderPass->name = "ComputeRenderTargetPass";
    _renderTargetRenderPass->Retain();
    
    // 프레임버퍼 생성
    OgVector<OgTextureHandle*> rtTextures;
    rtTextures.Add(_renderTargetTexture);
    
    OgFrameBufferInfo rtFbInfo{};
    rtFbInfo.width = width;
    rtFbInfo.height = height;
    rtFbInfo.renderPass = _renderTargetRenderPass;
    rtFbInfo.colorBuffers = rtTextures;
    rtFbInfo.depthStencilBuffer = _depthTexture;
    
    _renderTargetFrameBuffer = _renderContext->CreateFrameBuffer(rtFbInfo);
    _renderTargetFrameBuffer->name = "ComputeRenderTargetFrameBuffer";
    _renderTargetFrameBuffer->Retain();
}

void OgComputeSample::destroyRenderTarget()
{
    if (_renderTargetFrameBuffer)
    {
        _renderTargetFrameBuffer->Release();
        _renderTargetFrameBuffer = nullptr;
    }
    
    if (_renderTargetRenderPass)
    {
        _renderTargetRenderPass->Release();
        _renderTargetRenderPass = nullptr;
    }
    
    if (_depthTexture)
    {
        _depthTexture->Release();
        _depthTexture = nullptr;
    }
    
    if (_renderTargetTexture)
    {
        _renderTargetTexture->Release();
        _renderTargetTexture = nullptr;
    }
}

OG_NAMESPACE_SAMPLE_END