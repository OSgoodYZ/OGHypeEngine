#pragma once
#ifndef _OG_RENDER_CONTEXT_H_
#define _OG_RENDER_CONTEXT_H_

#include "OgPrecompile.h"
#include "system/OgSystemContext.h"
#include "OgRenderDefinitions.h"

using namespace Og;

OG_NAMESPACE_RENDER_BEGIN

#ifdef __cplusplus
extern "C" {
#endif

class OG_API OgRenderContext
{
	friend class OgHandle;

public:

	OgRenderPlatform platform;

	uint32 maxSubmitCount;

	static const uint32 SUBMISSION_INDEX_NONE = -1;	// Notice : -1 -> underflow

	OgSystemContext* context;

	virtual ~OgRenderContext() {};
	
	/**
	* @fn void Load(void)
	* @brief Load 함수를 통해 library나 driver를 이용할 준비를 합니다.
	* @remark 모든 과정을 진행하기 앞서 반드시 Load()를 진행하여야 합니다.
	*/
	virtual void Load(void) = 0;
	/**
	* @fn void Init(void)
	* @brief 여러가지 초기화를 담당합니다. 가령 CommendPool이나 DescriptorPool 등을 초기화합니다.
	* @remark Load() 후 반드시 한번의 Init() 과정이 필요합니다.
	* @see  http://devgit.com2us.com/TS/TPact/wikis/Og1Engine-API/Render-Context
	*/
	virtual void Init(void) = 0;

	/**
	* @fn void InitSwapChain(const OgSwapChainInfo& swapchainInfo)
	* @brief Context의 SwapChain을 초기화 합니다.
	* @details 입력변수로 들어가는 OgSwapChainInf의 조건에 따라 자동으로 SwapChain이 정해집니다.
	* 즉 SwapChain이 Double Buffering or Triple Buffering으로 정해집니다.
	* @param OgSwapChainInfo
	* @return void
	* @see http://devgit.com2us.com/TS/TPact/wikis/OgRender/1)-Basic-Triangle#Ogrender-module-%EC%84%A4%EB%AA%85-%EC%B4%88%EA%B8%B0%ED%99%94-%EC%88%9C%EC%84%9C%EB%A1%9C
	*/
	virtual OgSwapChain* CreateSwapchain(OgNativeWindow* nativeWindow, const OgSwapChainInfo& swapchainInfo) = 0;

	virtual void DestroySwapchain(OgSwapChain* swapchain) = 0;

	/**
	* @fn OgFrameBufferHandle* GetSwapChainFrameBuffer(uint32 index)
	* @brief index에 해당하는 FrameBuffer의 handle을 얻을 수 있습니다.
	* @param uint | 얻고자 하는 FrameBuffer의 index
	* @return OgFrameBufferHandle* | 입력변수에 해당하는 FrameBuffer의 handle을 리턴
	* @see http://devgit.com2us.com/TS/TPact/wikis/OgRender/2)-Basic-Synchronization#acquire
	*/
	virtual OgFrameBufferHandle* GetSwapChainFrameBuffer(OgSwapChain* swapchain, uint32 index) = 0;
	/**
	* @fn uint32 AcquireNextImageIndex()
	* @brief 다음 모니터 화면에 보일 FrameBuffer의 index를 얻을 수 있습니다.
	* @details 만약 Triple buffering이라면 buffer의 index는 0,1,2로 주어질 것 입니다.
	* 이것들은 내부 Presentation Engine에 의해 임의의 순서대로 화면에 출력될 것 입니다.
	* (즉, 단순히 index순서에 따라 출력되는 것이 아니다.)
	* 따라서 AcquireNextImageIndex() 함수를 통해 우리는 다음번에 어떤 index의 SwapChain FrameBuffer를 가져올지 알 수 있습니다.
	* @return uint | FrameBuffer의 index
	* @see http://devgit.com2us.com/TS/TPact/wikis/OgRender/2)-Basic-Synchronization#acquire
	*/
	virtual uint32 AcquireNextImageIndex(OgSwapChain* swapchain) = 0;
	/**
	* @fn uint32 GetCurrentImageIndex()
	* @brief 현재 모니터 화면에 보일 FrameBuffer의 index를 얻을 수 있습니다.
	* @details 만약 Triple Buffering이라면 buffer의 index는 0,1,2로 주어질 것 입니다.
	* 이것들은 내부 Presentation Engine에 의해 임의의 순서대로 화면에 출력될 것 입니다.
	* (즉, 단순히 index순서에 따라 출력되는 것이 아니다.)
	* 따라서 GetCurrentImageIndex() 함수를 통해 우리는 현재 어떤 index의 SwapChain FrameBuffer를 가져올지 알 수 있습니다.
	* @return uint | FrameBuffer의 index
	* @see http://devgit.com2us.com/TS/TPact/wikis/OgRender/2)-Basic-Synchronization#acquire
	*/
	virtual uint32 GetCurrentImageIndex(OgSwapChain* swapchain) = 0;

	// https://developer.qualcomm.com/download/adrenosdk/vulkan-developer-guide.pdf
	// https://stackoverflow.com/questions/30641095/glmapbuffer-is-missing-from-opengl-es-2
	// https://developer.apple.com/documentation/metal/mtlresourceoptions?language=objc
	/**
	* @fn OgBufferHandle* CreateBuffer(void* data, size_t size, OgBufferUsage usage, OgMemoryOption option = OgMemoryOption::STAGING)
	* @brief buffer를 만들 수 있습니다.
	* @details 사용 용도에 따라 Uniform buffer, Vertex buffer, Index buffer용으로 buffer를 만들 수 있습니다.
	* @param void* | CPU에 있는 data의 정보
	* @param size_t | byte size
	* @param OgBufferUsage | buffer를 사용할 용도에 따라 uniform, index, vetex 로 설정 가능
	* @param OgMemoryOption | OgMemoryOption은 STAGING, MAP_MANAGED, PRIVATE_GPU가 있으며 STAGING은 한 번 data를 올린 후 변화가 없을 때 사용, MAP_MANAGED는 data가 자주 변화하는 경우 사용
	* @return OgBufferHandle*
	* @see http://devgit.com2us.com/TS/TPact/wikis/OgRender/1)-Basic-Triangle#buffer
	*/
	virtual OgBufferHandle* CreateBuffer(void* data, size_t size, OgBufferUsage usage, OgMemoryOption option = OgMemoryOption::PRIVATE_GPU) = 0;
	/**
	* @fn void DestroyBuffer(OgBufferHandle* buffer)
	* @brief 기존에 생성했던 buffer를 삭제할 수 있습니다.
	* @param OgBufferHandle*
	* @return void
	* @see http://devgit.com2us.com/TS/TPact/wikis/OgRender/1)-Basic-Triangle#buffer
	*/
	virtual void DestroyBuffer(OgBufferHandle* buffer) = 0;

	/**
	* @fn OgShaderHandle* CreateShader(OgShaderType flag, const char* text, uint32 codeSize, const char* funcName = nullptr)
	* @brief ShaderHandle를 만들기 위해 이용합니다.
	* @details 생성하고자 하는 Shader의 종류(ex vertex,frag)와 shadercode에서 읽어온 data를 넣어줌으로써 ShaderHandle울 만들어줄 수 있습니다.
	* @param OgShaderType | VERTEX, FRAGMENT 등을 의미
	* @param const char* | OgShaderCode의 data
	* @param uint | OgShaderCode의 size
	* @param const char* | OgShaderCode의 entryName
	* @return OgShaderHandle*
	* @see http://devgit.com2us.com/TS/TPact/wikis/OgRender/1)-Basic-Triangle#1-shaderdescriptor
	*/

	virtual OgShaderHandle* CreateShader(OgShaderType flag, const char* text, uint32 codeSize, const char* funcName = nullptr) = 0;
	/**
	* @fn void DestroyShader(OgShaderHandle* shader)
	* @brief 기존에 생성했던 ShaderHandle를 삭제할 수 있습니다.
	* @details GL에서는 Program을 만든 후에 Shader를 제거해도 상관없지만, OgRender에서는 Pipeline을 생성한 후에 Shader를 제거해야 합니다.
	* @param OgShaderHandle*
	* @return void
	* @remark Pipeline을 생성한 후에 Shader를 제거해야 합니다.
	* @see http://devgit.com2us.com/TS/TPact/wikis/OgRender/1)-Basic-Triangle#1-shaderdescriptor
	*/
	virtual void DestroyShader(OgShaderHandle* shader) = 0;

	/**
	* @fn OgHandle* CreateProgram(OgShaderHandle** shaders, uint32 shaderCount)
	* @brief Shader를 이용해 Program을 만들 수 있습니다. Program을 만든 후에 ShaderDescriptor에 넣어줄 것입니다.
	* @details Program을 만들기 앞서 해야할 것은 Shader를 만들고 Shader를 배열로 packing해주는 것이 필요합니다.
	* 가령, OgShaderHandle* vs, OgShaderHandle* fs 두 개를 만들었다면, OgShaderHandle* OgShaderContainer[2]; OgShaderContainer[0] = vs, OgShaderContainer[1] = fs; 와 같이 배열로 packing한 후에 Program을 만들 때 이용합니다.
	* @param OgShaderHandle**
	* @param uint | 이용하고자 하는 Shader의 갯수
	* @return OgHandle*
	* @see http://devgit.com2us.com/TS/TPact/wikis/OgRender/1)-Basic-Triangle#1-shaderdescriptor
	*/
	virtual OgProgramHandle* CreateProgram(OgShaderHandle** shaders, uint32 shaderCount) = 0;
	/**
	* @fn void DestroyProgram(OgHandle* handle)
	* @brief 기존에 생성했던 Program를 삭제할 수 있습니다.
	* @param OgHandle*
	* @return void
	* @see http://devgit.com2us.com/TS/TPact/wikis/OgRender/1)-Basic-Triangle#1-shaderdescriptor
	*/
	virtual void DestroyProgram(OgProgramHandle* handle) = 0;

	/**
	* @fn OgTextureHandle* CreateTexture(unsigned char* image, OgPixelFormat format, uint32 width, uint32 height, OgSamplerHandle* sampler = nullptr, bool generateMipmaps = false)
	* @brief TextureHandle* 을 만들 수 있습니다. 한 개의 texture를 이용하고자 할 때 사용하는 함수입니다.
	* @details 실제 Texture Image를 이용해야 할 경우에 이용할 수도 있고, offscreenTexture를 만들어야 할 때도 이용할 수 있습니다.  sampler을 통해 생성할 texture의 옵션을 조절할 수 있습니다.
	* 첫 번째 입력변수 unsigned char* image를 살펴보면 본 함수는 하나의 texture를 이용해서 하나의 TextureHandle*을 만들 수 있습니다.
	* 따라서, 만약 3개의 texture를 이용하고자 할 때 본 함수를 이용하면 3번의 각기 다른 호출이 필요하다.
	* 만약 spec이 같은 여러개의 texture를 이용해서 한 번에 OgTextureHandle* 만들고자 한다면 CreateTexture(unsigned char** image,...,) 함수를 이용할 수 있습니다.
	* 또한 입력 변수의 종류가 한계가 있으므로 좀 더 자세한 spec을 명시하여 texture를 만들고자 한다면 OgTextureHandle* CreateTexture(unsigned char** image, const OgTextureInfo& info, OgSamplerHandle* sampler = nullptr) 함수를 이용하는 것이 필요합니다.
	* @param unsigned char* | image data
	* @param OgPixelFormat | 해당 texture의 format
	* @param uint | width
	* @param uint | height
	* @param OgSamplerHandle* | sampler의 handle
	* @param bool | mipmap 사용 여부
	* @return OgTextureHandle*
	* @see http://devgit.com2us.com/TS/TPact/wikis/OgRender/4)-Basic-Texture-Sample#3-Ogtexturehandle-%EC%83%9D%EC%84%B1
	*/
	virtual OgTextureHandle* CreateTexture(void* image, OgPixelFormat format, uint32 width, uint32 height, OgSamplerHandle* sampler = nullptr, bool generateMipmaps = false) = 0;
	/**
	* @fn OgTextureHandle* CreateTexture(unsigned char** image, OgPixelFormat format, uint32 width, uint32 height, uint32 layerCount, OgSamplerHandle* sampler = nullptr, bool generateMipmaps = false)
	* @brief TextureHandle* 을 만들 수 있습니다. 주로 three-dimensional texture image를 이용하고자 할 때 사용하는 함수입니다.
	* @details	unsigned char** 을 입력변수로 받는 이유는 image 여러 장을 받기 위해서 입니다.(본 함수를 이용해 한 장의 image를 texture로 만드는 것도 가능합니다.)
	* 입력 변수에 존재하는 layerCount는 image의 갯수를 의미합니다.
	* sampler을 통해 생성할 texture의 옵션을 조절할 수 있습니다.
	* @param unsigned char** | image data array
	* @param OgPixelFormat | 해당 texture의 format
	* @param uint | width
	* @param uint | height
	* @param uint | layerCount
	* @param OgSamplerHandle* | sampler의 handle
	* @param bool | Mipmap 사용 여부
	* @return OgTextureHandle*
	* @see http://devgit.com2us.com/TS/TPact/issues/209
	*/
	virtual OgTextureHandle* CreateTexture(void** image, OgPixelFormat format, uint32 width, uint32 height, uint32 layerCount, OgSamplerHandle* sampler = nullptr, bool generateMipmaps = false) = 0;
	/**
	* @fn OgTextureHandle* CreateTexture(unsigned char** image, const OgTextureInfo& info, OgSamplerHandle* sampler = nullptr)
	* @brief TextureHandle* 을 만들 수 있습니다.주로 three-dimensional texture image를 이용하고자 할 때 사용하는 함수입니다.
	* @details 입력변수로 OgTextureInfo을 구성하여 넣기 때문에 좀 더 자세한 spec을 지정하여 texture를 만들 수 있습니다.
	* @param unsigned char** | image data array
	* @param const OgTextureInfo& | unsigned char** image에 대한 자세한 spec으로 구성된 OgTextureInfo
	* @param OgSamplerHandle* | 원하는 Texture filtering 조건에 맞게 구성된 sampleHandle*
	* @return OgTextureHandle*
	* @see http://devgit.com2us.com/TS/TPact/wikis/OgRender/5)-Mipmap,-Cubemap-Samples
	*/
	virtual OgTextureHandle* CreateTexture(void** image, const OgTextureInfo& info, OgSamplerHandle* sampler = nullptr) = 0;
	/**
	* @fn void DestroyTexture(OgTextureHandle* texture)
	* @brief TextureHandle* 을 통해서 원하는 TextureHandle* 을 삭제할 수 있습니다.
	* @param OgTextureHandle*
	* @return void
	* @see http://devgit.com2us.com/TS/TPact/wikis/OgRender/4)-Basic-Texture-Sample#6-%ED%95%B4%EC%A0%9C
	*/
	virtual void DestroyTexture(OgTextureHandle* texture) = 0;

	/**
	* @fn void UpdateTexture(OgTextureHandle* texture, OgSamplerHandle* sampler, size_t offset, void* data, size_t size, bool useBarrier)
	* @brief OgTextureHandle*을 통해 해당 texture update를 할 수 있습니다.
	* @return void
	*/
	virtual void UpdateTexture(OgTextureHandle* texture, OgSamplerHandle* sampler, size_t offset, void** data, bool useBarrier) = 0;

	/**
	* @fn OgSamplerHandle* CreateSampler(OgSamplerInfo info)
	* @brief SamplerHandle*을 만들 수 있습니다.
	* @details Texture를 만들 때 OgSamplerHandle*이 입력변수로 들어가게 되는데 이는 생성 Texture의 옵션을 조절하기 위함입니다.
	* @param OgSamplerInfo | 원하는 조건에 맞게 구성된 OgSamplerInfo
	* @return OgSamplerHandle*
	* @see http://devgit.com2us.com/TS/TPact/wikis/OgRender/4)-Basic-Texture-Sample#3-Ogtexturehandle-%EC%83%9D%EC%84%B1
	*/
	virtual OgSamplerHandle* CreateSampler(const OgSamplerInfo& info) = 0;
	/**
	* @fn void DestroySampler(OgSamplerHandle* sampler)
	* @brief SamplerHandle*을 삭제할 수 있습니다.
	* @param OgSamplerHandle*
	* @return void
	* @see http://devgit.com2us.com/TS/TPact/wikis/OgRender/4)-Basic-Texture-Sample#6-%ED%95%B4%EC%A0%9C
	*/
	virtual void DestroySampler(OgSamplerHandle* sampler) = 0;

	/**
	* @fn OgPipelineHandle* CreatePipeline(OgPipelineDescriptor& descriptor)
	* @brief Pipeline을 생성하기 위해 이용합니다.
	* @details OgPipelineDescriptor(pipeline state object)을 원하는 방식으로 작성한 후 넣어주면 하나의 Pipeline을 생성할 수 있습니다.
	* OgPipelineDescriptor에는 각각 type,name, renderPass, vertexInput, resourceLayout,shader, rasterize, colorBlend,depthStencil 가 멤버변수로 존재합니다.
	* type 은 GRAPHICS_PIPELINE 인지 COMPUTE_PIPELINE인지 명시.
	* name 은 단순히 Pipeline의 이름을 명시.
	* renderPass 는 해당 Pipeline의 RenderPass를 의미.
	* vertexInput 은 vertex assembler를 의미.(ex Shader에 들어갈 pos, normal 등의 정보로 구성.)
	* resourceLayout Shader에서 사용될 uniform, texture와 같은 정보들의 구성을 의미.
	* shader 는 OgShaderDescriptor로써 Shader의 구성을 의미.
	* rasterize 는 OgRasterizationDescriptor로써 Rasterization 조건 등으로 구성.
	* colorBlend 는 OgColorBlendDescriptor로써 ColorBlending 조건으로 구성.
	* depthStencil 는 OgDepthStencilDescriptor로써 DepthStencilTest 조건으로 구성.
	* @param OgPipelineDescriptor&
	* @return OgPipelineHandle*
	* @see http://devgit.com2us.com/TS/TPact/wikis/OgRender/1)-Basic-Triangle#7-pipeline
	*/
	virtual OgPipelineHandle* CreatePipeline(OgPipelineDescriptor& descriptor) = 0;
	/**
	* @fn void DestroyPipeline(OgPipelineHandle* pipeline)
	* @brief Pipeline을 제거하기 위해 이용합니다.
	* @details Pipeline을 위해 이용하던 memory를 반환합니다.
	* @param OgPipelineHandle*
	* @return void
	*/
	virtual void DestroyPipeline(OgPipelineHandle* pipeline) = 0;

	/**
	* @fn OgResourceLayoutHandle* CreateResourceLayout(OgResourceBinding* bindings, uint32 count)
	* @brief ResourceLayout을 만들기 위한 함수입니다.
	* @details ResourceLayout란? pipeline에서 이용할 Resource(Ex UniformData, Texture, Sampler)들의 구성을 의미합니다.
	* @param OgResourceBinding* | OgResourceBinding 형태의 배열
	* @param uint |  OgResourceBinding 배열의 크기
	* @return OgResourceLayoutHandle*
	* @remark OgResourceBinding은 graphics pipeline 순서에 맞게 구성되어야 합니다.
	* 예를 들어 Fragment -> Vertex 순서대로 넣으면 올바른 OgResourceLayout을 생성할 수 없습니다.
	* @see http://devgit.com2us.com/TS/TPact/wikis/OgRender/1)-Basic-Triangle#2-resourcelayout
	*/
	virtual OgResourceLayoutHandle* CreateResourceLayout(OgResourceBinding* bindings, uint32 count) = 0;
	/**
	* @fn void DestroyResourceLayout(OgResourceLayoutHandle* layout)
	* @brief ResourceLayout을 삭제하기 위한 함수입니다.
	* @param OgResourceLayoutHandle*
	* @return void
	* @see http://devgit.com2us.com/TS/TPact/wikis/OgRender/3)-Basic-Mesh-Sample#10-%ED%95%B4%EC%A0%9C
	*/
	virtual void DestroyResourceLayout(OgResourceLayoutHandle* layout) = 0;

	/**
	* @fn OgResourceSetHandle* CreateResourceSet(OgResourceLayoutHandle* resourceLayout, OgResourceUsage* usages, uint32 usageCount)
	* @brief 사용할 Resource의 handle의 집합을 생성하는 함수입니다.
	* @details 사용할 Resource의 handle의 집합을 ResourceSet이라고 합니다.
	* OgResourceLayout에 맞게 buffer의 handle들을 모아 하나의 OgResourceSetHandle을 만들어 줍니다.
	* @param OgResourceLayoutHandle*
	* @param OgResourceUsage*
	* @param uint | OgResourceUsage의 크기
	* @return OgResourceSetHandle*
	* @see http://devgit.com2us.com/TS/TPact/wikis/OgRender/1)-Basic-Triangle#resourceset
	*/
	virtual OgResourceSetHandle* CreateResourceSet(OgResourceLayoutHandle* resourceLayout,
		OgResourceUsage* usages, uint32 usageCount) = 0;
	/**
	* @fn void DestroyResourceSet(OgResourceSetHandle* resourceSet)
	* @brief OgResourceSetHandle*을 통해서 더이상 이용하지 않는 ResourceSet을 삭제할 수 있습니다.
	* @param OgResourceSetHandle*
	* @return void
	* @see http://devgit.com2us.com/TS/TPact/wikis/OgRender/3)-Basic-Mesh-Sample#10-%ED%95%B4%EC%A0%9C
	*/
	virtual void DestroyResourceSet(OgResourceSetHandle* resourceSet) = 0;

	/**
	* @fn OgCommandEncoderHandle* CreateCommandEncoder()
	* @brief Rendering Command를 담을 CommandEncoder를 생성 할 수 있습니다.
	* @details SwapChain의 갯수만큼 CommandEncoder를 만들어야 GPU가 처리하는 중인 자원들을 건드리지 않을 수 있습니다.
	* @return OgCommandEncoderHandle*
	* @remark SwapChain의 갯수만큼 CommandEncoder를 만들어야 합니다.
	* @see http://devgit.com2us.com/TS/TPact/wikis/OgRender/3)-Basic-Mesh-Sample#2-commandencoder-%EC%B4%88%EA%B8%B0%ED%99%94
	*/
	virtual OgCommandEncoderHandle* CreateCommandEncoder() = 0;
	/**
	* @fn void DestroyCommandEncoder(OgCommandEncoderHandle* encoder)
	* @brief 사용이 끝난 CommandEncoder를 삭제할 수 있습니다.
	* @param OgCommandEncoderHandle*
	* @return void
	* @see http://devgit.com2us.com/TS/TPact/wikis/OgRender/3)-Basic-Mesh-Sample#10-%ED%95%B4%EC%A0%9C
	*/
	virtual void DestroyCommandEncoder(OgCommandEncoderHandle* encoder) = 0;

	/**
	* @fn OgFrameBufferHandle* CreateFrameBuffer(OgFrameBufferInfo& info)
	* @brief CreateFrameBuffer는 의도적으로 FrameBuffer를 만들기 위해 이용합니다.
	* @details FrameBuffer란? buffer(ex Color Buffer,Depth_stencil Buffer) handle들의 집합입니다.
	* FrameBuffer가 가지고 있을 수 있는 Texture의 종류는 Color/DepthStencil 두 종류입니다.
	* ColorTexture는 1개 이상이 될 수 있고, DepthStencilTexture는 항상 1개입니다.
	* @param OgFrameBufferInfo&
	* @return OgFrameBufferHandle*
	* @remark 별도의 FrameBuffer를 생성할 때는 항상 한 개 이상의 RenderPass가 필요합니다.
	* @see http://devgit.com2us.com/TS/TPact/wikis/OgRender/1)-Basic-Triangle#framebuffer%EC%99%80-renderpass
	*/
	virtual OgFrameBufferHandle* CreateFrameBuffer(OgFrameBufferInfo& info) = 0;
	/**
	* @fn void DestroyFrameBuffer(OgFrameBufferHandle* framebuffer)
	* @brief OgFrameBufferHandle*을 통해 FrameBuffer을 삭제할 수 있습니다.
	* @param OgFrameBufferHandle*
	* @return void
	* @remark CreateFrameBuffer함수를 통해 의도적으로 생성해준 FrameBuffer는 DestroyFrameBuffer함수를 통해 지워줘야 합니다.
	*/
	virtual void DestroyFrameBuffer(OgFrameBufferHandle* framebuffer) = 0;

	/**
	* @fn OgRenderPassHandle* CreateRenderPass(OgRenderPassInfo& info)
	* @brief RenderPass를 만들기 위한 함수입니다.
	* @details RenderPass란 FrameBuffer상의 buffer들에 대한 operation의 집합입니다.
	* 가령 OpenGL의 관점에서 RenderPass를 설명하자면 glClearColor, glClear 등을 할 지 말지 우선적으로 정하는 것과 같습니다.
	* @param OgRenderPassInfo& | RenderPass의 spec
	* @return OgRenderPassHandle*
	* @remark 매개변수 OgRenderPassInfo의 멤버변수 isSwapChainRenderPass을 true로 설정시 SwapChain의 buffer가 default buffer로 설정됩니다.
	* @see http://devgit.com2us.com/TS/TPact/wikis/OgRender/1)-Basic-Triangle#framebuffer%EC%99%80-renderpass
	* @see http://devgit.com2us.com/TS/TPact/wikis/OgRender/3)-Basic-Mesh-Sample#3-default-framebufferswapchain-framebuffer%EC%97%90-%EB%8C%80%ED%95%9C-renderpass-%EC%83%9D%EC%84%B1
	*/
	virtual OgRenderPassHandle* CreateRenderPass(OgRenderPassInfo& info) = 0;
	/**
	* @fn void DestroyRenderPass(OgRenderPassHandle* renderPass)
	* @brief OgRenderPassHandle*을 통해 이동하던 RenderPass를 삭제할 수 있습니다.
	* @param OgRenderPassHandle*
	* @return void
	* @see http://devgit.com2us.com/TS/TPact/wikis/OgRender/3)-Basic-Mesh-Sample#10-%ED%95%B4%EC%A0%9C
	*/
	virtual void DestroyRenderPass(OgRenderPassHandle* renderPass) = 0;

	// --> Mapbuffer only
	/**
	* @fn void* MapBuffer(OgBufferHandle* buffer, OgBufferUsage usage, size_t size)
	* @brief GPU(디바이스)에 있는 buffer의 주소값을 받아오기 위한 함수입니다.
	* @details CPU(호스트)에 있는 데이터를 GPU(디바이스)쪽으로 넘기기 위해서는 GPU쪽 buffer의 주소값이 필요합니다.
	* 좀 더 정확히 얘기하자면 응용프로그램의 메모리 주소로 오브젝트의 메모리를 mapping합니다.
	* 이를 "호스트에 mapping되었다"고 표현합니다.
	* @param OgBufferHandle* | 주소값을 받아오기 위한 GPU(디바이스)쪽 buffer
	* @param OgBufferUsage | 이용할 buffer의 사용 용도
	* @param size_t | buffer의 크기
	* @return void* | buffer의 주소값을 반환
	* @remark 사용을 마친 buffer는 UnmapBuffer()을 이용해 CPU와 GPU의 연결관계를 끊어주어야 합니다.
	*/
	//https://www.seas.upenn.edu/~pcozzi/OpenGLInsights/OpenGLInsights-AsynchronousBufferTransfers.pdf
	virtual void* MapBuffer(OgBufferHandle* buffer, size_t size, size_t offset = 0) = 0;
	/**
	* @fn bool UnmapBuffer(OgBufferHandle* buffer, OgBufferUsage usage)
	* @brief MapBuffer() 함수를 이용해서 주소값을 받아와 이용하던 GPU(디바이스)쪽 buffer의 사용을 마무리한다는 의미의 함수입니다.
	* @details 가령 CPU(호스트)의 데이터를 GPU(디바이스)의 buffer쪽으로 복사한다고 할 때 GPU(디바이스) buffer의 주소값을 MapBuffer()함수를 통해 받아온 후
	* CPU(호스트)데이터의 복사를 진행할 것입니다. 복사를 완료한 후에는 CPU(호스트)와 GPU(디바이스)쪽의 연결관계를 끊는 작업이 필요합니다.(동기화 문제 때문에)
	* 따라서 이용하던 연결관계를 끊기 위해 사용되는 함수입니다.
	* @param OgBufferHandle* | 이용을 끝낸 BufferHandle*
	* @param OgBufferUsage
	* @return bool | Og가 buffer에 데이터를 성공적으로 매핑할 수 있었다면 true값을 리턴
	* @remark 대부분의 경우 MapBuffer()와 한 pair를 이루어 이용됩니다.
	*/
	//https://lifeisforu.tistory.com/408
	virtual bool UnmapBuffer(OgBufferHandle* buffer) = 0;

	/**
	* @fn void OgUpdateBuffer(OgBufferHandle* buffer, OgBufferUsage usage, OgMemoryOption option , size_t size, size_t offset, void* data)
	* @brief buffer를 특정 data로 업데이트하기 위해서 이용할 수 있는 함수입니다. OgMemoryOption이 GPU_PRIVATE, MAP_MANAGED인지 상관없이 내부에서 buffer를 업데이트 시켜줍니다.
	* @detail GPU_PRIVATE buffer도 업데이트 할 수 있습니다. GPU_PRIVATE buffer의 경우 플랫폼별로 내부적으로 stagingbuffer를 임시 생성하거나 하는 방법으로 업데이트를 진행합니다.
	* @detail 또한 offset과 size를 이용하여 버퍼의 원하는 부분(offset)에 원하는만큼(size) 업데이트를 할 수 있습니다.
	*/
	virtual void UpdateBuffer(OgBufferHandle* buffer, size_t offset, void* data, size_t size, bool useBarrier) = 0;

	/**
	* @fn void BlitFramebuffer(uint srcX0, uint srcY0, uint srcX1, uint srcY1,OgFrameBufferHandle* srcBuffer, uint dstX0, uint dstY0, uint dstX1, uint dstY1, OgFrameBufferHandle* dstBuffer)
	* @brief 일단 colorbuffer의 단순 복사만을 위해 구현되어 있다. depthFramebuffer나 stencilFramebuffer의 복사도 가능하도록 구현될 예정이다.
	*/
	virtual void BlitFramebuffer(uint srcX0, uint srcY0, uint srcX1, uint srcY1, OgFrameBufferHandle* srcBuffer, uint dstX0, uint dstY0, uint dstX1, uint dstY1, OgFrameBufferHandle* dstBuffer) = 0;

	//// -->
	///**
	//* @fn void Execute(OgByteBuffer& buffer)
	//* @brief 여러가지 Command들을 실행하기 위한 함수입니다.
	//* @details 대개 Present() 함수 내부에서 이용됩니다.
	//* @param OgByteBuffer&
	//* @return void
	//*/
	//virtual void Execute(OgByteBuffer& buffer) = 0;

	/**
	* @fn OgPixelFormat GetDefaultDepthFormat()
	* @brief Depth Format은 선택적인 것이기 때문에 사용하기에 알맞은 Depth Format을 선택하기 위해 사용되는 함수입니다.
	* @return OgPixelFormat
	*/
	virtual OgPixelFormat GetDefaultDepthFormat() = 0;

	/**
	* @fn void Submit(OgCommandEncoderHandle* encoder)
	* @brief EncoderBuffer를 완성한 후 RenderContext에 제출하기 위한 함수입니다.
	* @details 쉽게 말해, GPU에 실행할 명령어를 제출하는 것입니다.
	* encoder를 통해 EncoderBuffer을 완성하고 RenderContext의 내부 buffer에 해당 EncoderBuffer의 handle을 넣는 것입니다.
	* @param OgCommandEncoderHandle*
	* @return void
	* @see http://devgit.com2us.com/TS/TPact/wikis/OgRender/2)-Basic-Synchronization#submit
	*/
	virtual void Submit(OgSwapChain* swapchain, OgCommandEncoderHandle* encoder) = 0;

	/**
	* @fn void Present()
	* @brief FrameBuffer에 그렸던 것을 모니터에 보여달라는 요청입니다.
	* @return void
	* @remark Acquire-Submit-Present 의 순서대로 사용해야 올바르게 렌더링을 할 수 있습니다.
	* @see http://devgit.com2us.com/TS/TPact/wikis/OgRender/2)-Basic-Synchronization#present
	*/
	virtual void Present(OgSwapChain* swapchain) = 0;

	/**
	* @fn void Suspend()
	* @brief 잠시 렌더링를 멈추고 싶을 때 이용합니다.
	* @detail 가령 굳이 화면에 보이지 않아 연산이 필요하지 않을 때 이용할 수 있습니다.
	*/
	virtual void Suspend(OgSwapChain* swapchain) = 0;

	/**
	* @fn void Restore()
	* @brief 일시 정지 시켰던 렌더링을 다시 시작할 때 이용합니다.
	*/
	virtual void Restore(OgSwapChain* swapchain) = 0;

	/**
	* @fn void WaitDeviceIdle()
	* @brief GPU에서 돌아가는 명령(command)들이 다 끝날 때까지 대기하도록 합니다.
	* @detail 주로 렌더링 과정이 끝나서 할당했던 memory들을 해제하기 전에 이용합니다.
	* GPU에 명령들이 다 끝나지 않았는데 GPU에서 사용되고 있는 memory들을 해제하려고 한다면 에러가 발생할 것입니다.
	* 이러한 상황을 막기 위해 memory 해제 전에 이용하는 함수입니다.
	* @return void
	* @remark GPU에서 사용하고 있던 memory를 해제하기 전에 이용해야 합니다.
	* @see http://devgit.com2us.com/TS/TPact/wikis/OgRender/3)-Basic-Mesh-Sample#10-%ED%95%B4%EC%A0%9C
	*/
	virtual void WaitDeviceIdle() = 0;

	/**
	* @fn void Shutdown(void)
	* @brief 렌더링을 완전히 끝낼 때 이용합니다.
	*/
	virtual void Shutdown(void) = 0;

	/**
	* @brief GPU Resource 를 소거합니다.
	*/
	virtual void Collect() = 0;

private:
	
};
#ifdef __cplusplus
}
#endif
OG_NAMESPACE_RENDER_END
#endif // _OG_RENDER_CONTEXT_H_