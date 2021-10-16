#pragma once
#ifndef _OG_RENDER_CONTEXT_H_
#define _OG_RENDER_CONTEXT_H_

#include "OgPrecompile.h"
#include "OgRenderDefinition.h"


LV_NS_RENDER_BEGIN
#ifdef __cplusplus
extern "C" {
#endif

	class LV_API LvRenderContext
	{
		friend class LvHandle;
	public:
		LvRenderPlatform platform;

		uint32 maxSubmitCount;

		static const uint32 SUBMISSION_INDEX_NONE = -1;	// Notice : -1 -> underflow

		LvSystemContext* context;

		/**
		* @brief Capability platform별로 여러가지 제약사항들을 담아두는 구조체입니다.
		* @details 예를 들어, uniformbuffer를 offset을 이용하여 업데이트하려고 할 때 platform(GL,VULKAN,METAL) 별로 다른 제약사항을 가지고 있습니다. 그러한 제약사항들을 Capability 구조체에 담아두었습니다.
		* @see https://www.geeks3d.com/20140704/gpu-buffers-introduction-to-opengl-3-1-uniform-buffers-objects/  // GL
		* @see https://www.khronos.org/registry/vulkan/specs/1.2-extensions/man/html/VkPhysicalDeviceLimits.html // VULKAN
		* @see https://developer.apple.com/documentation/metal/mtlfeatureset?language=objc // Metal
		* @see https://developer.apple.com/library/archive/documentation/Miscellaneous/Conceptual/MetalProgrammingGuide/MetalFeatureSetTables/MetalFeatureSetTables.html#//apple_ref/doc/uid/TP40014221-CH13-SW1 // Metal
		*/
		struct Capability
		{
		protected:
			uint32 _maxUniformBufferSize;
			uint32 _maxNumberOfUniformBlocks;
			uint32 _minUniformBufferOffsetAlignment; // GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT  minUniformBufferOffsetAlignment  256/16

			//uint32 maxUniformBufferBindings;
			//uint32 maxFragmentUniformBlocks;
			//uint32 maxGeometryUniformBlocks;
			//uint32 vertexBufferOffset; 
		public:
			virtual uint32 GetMaxUniformBufferSize() = 0;

			virtual uint32 GetMaxNumberOfUniformBlocks() = 0;

			virtual uint32 GetMinUniformBufferOffset() = 0;
		};

		Capability* capability;

		bool isInitialized = false;

		virtual ~LvRenderContext()
		{

		};
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
		* @see  http://devgit.com2us.com/TS/TPact/wikis/Lv1Engine-API/Render-Context
		*/
		virtual void Init(void) = 0;

		/**
		* @fn void InitSwapChain(const LvSwapChainInfo& swapchainInfo)
		* @brief Context의 SwapChain을 초기화 합니다.
		* @details 입력변수로 들어가는 LvSwapChainInf의 조건에 따라 자동으로 SwapChain이 정해집니다.
		* 즉 SwapChain이 Double Buffering or Triple Buffering으로 정해집니다.
		* @param LvSwapChainInfo
		* @return void
		* @see http://devgit.com2us.com/TS/TPact/wikis/LvRender/1)-Basic-Triangle#lvrender-module-%EC%84%A4%EB%AA%85-%EC%B4%88%EA%B8%B0%ED%99%94-%EC%88%9C%EC%84%9C%EB%A1%9C
		*/
		// TODO: reconsider
		virtual LvSwapChain* CreateSwapchain(LvNativeWindow* nativeWindow, const LvSwapChainInfo& swapchainInfo) = 0;

		virtual void DestroySwapchain(LvSwapChain* swapchain) = 0;

		// TODO: reconsider to delete
		//virtual LvSwapChain* GetSwapchain(LvNativeWindow* nativeWindow) = 0;


		/**
		* @fn LvFrameBufferHandle* GetSwapChainFrameBuffer(uint32 index)
		* @brief index에 해당하는 FrameBuffer의 handle을 얻을 수 있습니다.
		* @param uint | 얻고자 하는 FrameBuffer의 index
		* @return LvFrameBufferHandle* | 입력변수에 해당하는 FrameBuffer의 handle을 리턴
		* @see http://devgit.com2us.com/TS/TPact/wikis/LvRender/2)-Basic-Synchronization#acquire
		*/
		virtual LvFrameBufferHandle* GetSwapChainFrameBuffer(LvSwapChain* swapchain, uint32 index) = 0;
		/**
		* @fn uint32 AcquireNextImageIndex()
		* @brief 다음 모니터 화면에 보일 FrameBuffer의 index를 얻을 수 있습니다.
		* @details 만약 Triple buffering이라면 buffer의 index는 0,1,2로 주어질 것 입니다.
		* 이것들은 내부 Presentation Engine에 의해 임의의 순서대로 화면에 출력될 것 입니다.
		* (즉, 단순히 index순서에 따라 출력되는 것이 아니다.)
		* 따라서 AcquireNextImageIndex() 함수를 통해 우리는 다음번에 어떤 index의 SwapChain FrameBuffer를 가져올지 알 수 있습니다.
		* @return uint | FrameBuffer의 index
		* @see http://devgit.com2us.com/TS/TPact/wikis/LvRender/2)-Basic-Synchronization#acquire
		*/
		virtual uint32 AcquireNextImageIndex(LvSwapChain* swapchain) = 0;
		/**
		* @fn uint32 GetCurrentImageIndex()
		* @brief 현재 모니터 화면에 보일 FrameBuffer의 index를 얻을 수 있습니다.
		* @details 만약 Triple Buffering이라면 buffer의 index는 0,1,2로 주어질 것 입니다.
		* 이것들은 내부 Presentation Engine에 의해 임의의 순서대로 화면에 출력될 것 입니다.
		* (즉, 단순히 index순서에 따라 출력되는 것이 아니다.)
		* 따라서 GetCurrentImageIndex() 함수를 통해 우리는 현재 어떤 index의 SwapChain FrameBuffer를 가져올지 알 수 있습니다.
		* @return uint | FrameBuffer의 index
		* @see http://devgit.com2us.com/TS/TPact/wikis/LvRender/2)-Basic-Synchronization#acquire
		*/
		virtual uint32 GetCurrentImageIndex(LvSwapChain* swapchain) = 0;

		// https://developer.qualcomm.com/download/adrenosdk/vulkan-developer-guide.pdf
		// https://stackoverflow.com/questions/30641095/glmapbuffer-is-missing-from-opengl-es-2
		// https://developer.apple.com/documentation/metal/mtlresourceoptions?language=objc
		/**
		* @fn LvBufferHandle* CreateBuffer(void* data, size_t size, LvBufferUsage usage, LvMemoryOption option = LvMemoryOption::STAGING)
		* @brief buffer를 만들 수 있습니다.
		* @details 사용 용도에 따라 Uniform buffer, Vertex buffer, Index buffer용으로 buffer를 만들 수 있습니다.
		* @param void* | CPU에 있는 data의 정보
		* @param size_t | byte size
		* @param LvBufferUsage | buffer를 사용할 용도에 따라 uniform, index, vetex 로 설정 가능
		* @param LvMemoryOption | LvMemoryOption은 STAGING, MAP_MANAGED, PRIVATE_GPU가 있으며 STAGING은 한 번 data를 올린 후 변화가 없을 때 사용, MAP_MANAGED는 data가 자주 변화하는 경우 사용
		* @return LvBufferHandle*
		* @see http://devgit.com2us.com/TS/TPact/wikis/LvRender/1)-Basic-Triangle#buffer
		*/
		virtual LvBufferHandle* CreateBuffer(void* data, size_t size, LvBufferUsage usage, LvMemoryOption option = LvMemoryOption::PRIVATE_GPU) = 0;
		/**
		* @fn void DestroyBuffer(LvBufferHandle* buffer)
		* @brief 기존에 생성했던 buffer를 삭제할 수 있습니다.
		* @param LvBufferHandle*
		* @return void
		* @see http://devgit.com2us.com/TS/TPact/wikis/LvRender/1)-Basic-Triangle#buffer
		*/
		virtual void DestroyBuffer(LvBufferHandle* buffer) = 0;

		/**
		* @fn LvShaderHandle* CreateShader(LvShaderType flag, const char* text, uint32 codeSize, const char* funcName = nullptr)
		* @brief ShaderHandle를 만들기 위해 이용합니다.
		* @details 생성하고자 하는 Shader의 종류(ex vertex,frag)와 shadercode에서 읽어온 data를 넣어줌으로써 ShaderHandle울 만들어줄 수 있습니다.
		* @param LvShaderType | VERTEX, FRAGMENT 등을 의미
		* @param const char* | LvShaderCode의 data
		* @param uint | LvShaderCode의 size
		* @param const char* | LvShaderCode의 entryName
		* @return LvShaderHandle*
		* @see http://devgit.com2us.com/TS/TPact/wikis/LvRender/1)-Basic-Triangle#1-shaderdescriptor
		*/

		virtual LvShaderHandle* CreateShader(LvShaderType flag, const char* text, uint32 codeSize, const char* funcName = nullptr) = 0;
		/**
		* @fn void DestroyShader(LvShaderHandle* shader)
		* @brief 기존에 생성했던 ShaderHandle를 삭제할 수 있습니다.
		* @details GL에서는 Program을 만든 후에 Shader를 제거해도 상관없지만, LvRender에서는 Pipeline을 생성한 후에 Shader를 제거해야 합니다.
		* @param LvShaderHandle*
		* @return void
		* @remark Pipeline을 생성한 후에 Shader를 제거해야 합니다.
		* @see http://devgit.com2us.com/TS/TPact/wikis/LvRender/1)-Basic-Triangle#1-shaderdescriptor
		*/
		virtual void DestroyShader(LvShaderHandle* shader) = 0;

		/**
		* @fn LvHandle* CreateProgram(LvShaderHandle** shaders, uint32 shaderCount)
		* @brief Shader를 이용해 Program을 만들 수 있습니다. Program을 만든 후에 ShaderDescriptor에 넣어줄 것입니다.
		* @details Program을 만들기 앞서 해야할 것은 Shader를 만들고 Shader를 배열로 packing해주는 것이 필요합니다.
		* 가령, LvShaderHandle* vs, LvShaderHandle* fs 두 개를 만들었다면, LvShaderHandle* LvShaderContainer[2]; LvShaderContainer[0] = vs, LvShaderContainer[1] = fs; 와 같이 배열로 packing한 후에 Program을 만들 때 이용합니다.
		* @param LvShaderHandle**
		* @param uint | 이용하고자 하는 Shader의 갯수
		* @return LvHandle*
		* @see http://devgit.com2us.com/TS/TPact/wikis/LvRender/1)-Basic-Triangle#1-shaderdescriptor
		*/
		virtual LvProgramHandle* CreateProgram(LvShaderHandle** shaders, uint32 shaderCount) = 0;
		/**
		* @fn void DestroyProgram(LvHandle* handle)
		* @brief 기존에 생성했던 Program를 삭제할 수 있습니다.
		* @param LvHandle*
		* @return void
		* @see http://devgit.com2us.com/TS/TPact/wikis/LvRender/1)-Basic-Triangle#1-shaderdescriptor
		*/
		virtual void DestroyProgram(LvProgramHandle* handle) = 0;

		/**
		* @fn LvTextureHandle* CreateTexture(unsigned char* image, LvPixelFormat format, uint32 width, uint32 height, LvSamplerHandle* sampler = nullptr, bool generateMipmaps = false)
		* @brief TextureHandle* 을 만들 수 있습니다. 한 개의 texture를 이용하고자 할 때 사용하는 함수입니다.
		* @details 실제 Texture Image를 이용해야 할 경우에 이용할 수도 있고, offscreenTexture를 만들어야 할 때도 이용할 수 있습니다.  sampler을 통해 생성할 texture의 옵션을 조절할 수 있습니다.
		* 첫 번째 입력변수 unsigned char* image를 살펴보면 본 함수는 하나의 texture를 이용해서 하나의 TextureHandle*을 만들 수 있습니다.
		* 따라서, 만약 3개의 texture를 이용하고자 할 때 본 함수를 이용하면 3번의 각기 다른 호출이 필요하다.
		* 만약 spec이 같은 여러개의 texture를 이용해서 한 번에 LvTextureHandle* 만들고자 한다면 CreateTexture(unsigned char** image,...,) 함수를 이용할 수 있습니다.
		* 또한 입력 변수의 종류가 한계가 있으므로 좀 더 자세한 spec을 명시하여 texture를 만들고자 한다면 LvTextureHandle* CreateTexture(unsigned char** image, const LvTextureInfo& info, LvSamplerHandle* sampler = nullptr) 함수를 이용하는 것이 필요합니다.
		* @param unsigned char* | image data
		* @param LvPixelFormat | 해당 texture의 format
		* @param uint | width
		* @param uint | height
		* @param LvSamplerHandle* | sampler의 handle
		* @param bool | mipmap 사용 여부
		* @return LvTextureHandle*
		* @see http://devgit.com2us.com/TS/TPact/wikis/LvRender/4)-Basic-Texture-Sample#3-lvtexturehandle-%EC%83%9D%EC%84%B1
		*/
		virtual LvTextureHandle* CreateTexture(void* image, LvPixelFormat format, uint32 width, uint32 height, LvSamplerHandle* sampler = nullptr, bool generateMipmaps = false) = 0;
		/**
		* @fn LvTextureHandle* CreateTexture(unsigned char** image, LvPixelFormat format, uint32 width, uint32 height, uint32 layerCount, LvSamplerHandle* sampler = nullptr, bool generateMipmaps = false)
		* @brief TextureHandle* 을 만들 수 있습니다. 주로 three-dimensional texture image를 이용하고자 할 때 사용하는 함수입니다.
		* @details	unsigned char** 을 입력변수로 받는 이유는 image 여러 장을 받기 위해서 입니다.(본 함수를 이용해 한 장의 image를 texture로 만드는 것도 가능합니다.)
		* 입력 변수에 존재하는 layerCount는 image의 갯수를 의미합니다.
		* sampler을 통해 생성할 texture의 옵션을 조절할 수 있습니다.
		* @param unsigned char** | image data array
		* @param LvPixelFormat | 해당 texture의 format
		* @param uint | width
		* @param uint | height
		* @param uint | layerCount
		* @param LvSamplerHandle* | sampler의 handle
		* @param bool | Mipmap 사용 여부
		* @return LvTextureHandle*
		* @see http://devgit.com2us.com/TS/TPact/issues/209
		*/
		virtual LvTextureHandle* CreateTexture(void** image, LvPixelFormat format, uint32 width, uint32 height, uint32 layerCount, LvSamplerHandle* sampler = nullptr, bool generateMipmaps = false) = 0;
		/**
		* @fn LvTextureHandle* CreateTexture(unsigned char** image, const LvTextureInfo& info, LvSamplerHandle* sampler = nullptr)
		* @brief TextureHandle* 을 만들 수 있습니다.주로 three-dimensional texture image를 이용하고자 할 때 사용하는 함수입니다.
		* @details 입력변수로 LvTextureInfo을 구성하여 넣기 때문에 좀 더 자세한 spec을 지정하여 texture를 만들 수 있습니다.
		* @param unsigned char** | image data array
		* @param const LvTextureInfo& | unsigned char** image에 대한 자세한 spec으로 구성된 LvTextureInfo
		* @param LvSamplerHandle* | 원하는 Texture filtering 조건에 맞게 구성된 sampleHandle*
		* @return LvTextureHandle*
		* @see http://devgit.com2us.com/TS/TPact/wikis/LvRender/5)-Mipmap,-Cubemap-Samples
		*/
		virtual LvTextureHandle* CreateTexture(void** image, const LvTextureInfo& info, LvSamplerHandle* sampler = nullptr) = 0;
		/**
		* @fn void DestroyTexture(LvTextureHandle* texture)
		* @brief TextureHandle* 을 통해서 원하는 TextureHandle* 을 삭제할 수 있습니다.
		* @param LvTextureHandle*
		* @return void
		* @see http://devgit.com2us.com/TS/TPact/wikis/LvRender/4)-Basic-Texture-Sample#6-%ED%95%B4%EC%A0%9C
		*/
		virtual void DestroyTexture(LvTextureHandle* texture) = 0;

		/**
		* @fn void UpdateTexture(LvTextureHandle* texture, LvSamplerHandle* sampler, size_t offset, void* data, size_t size, bool useBarrier)
		* @brief LvTextureHandle*을 통해 해당 texture update를 할 수 있습니다.
		* @return void
		*/
		virtual void UpdateTexture(LvTextureHandle* texture, LvSamplerHandle* sampler, size_t offset, void** data, bool useBarrier) = 0;

		/**
		* @fn LvSamplerHandle* CreateSampler(LvSamplerInfo info)
		* @brief SamplerHandle*을 만들 수 있습니다.
		* @details Texture를 만들 때 LvSamplerHandle*이 입력변수로 들어가게 되는데 이는 생성 Texture의 옵션을 조절하기 위함입니다.
		* @param LvSamplerInfo | 원하는 조건에 맞게 구성된 LvSamplerInfo
		* @return LvSamplerHandle*
		* @see http://devgit.com2us.com/TS/TPact/wikis/LvRender/4)-Basic-Texture-Sample#3-lvtexturehandle-%EC%83%9D%EC%84%B1
		*/
		virtual LvSamplerHandle* CreateSampler(const LvSamplerInfo& info) = 0;
		/**
		* @fn void DestroySampler(LvSamplerHandle* sampler)
		* @brief SamplerHandle*을 삭제할 수 있습니다.
		* @param LvSamplerHandle*
		* @return void
		* @see http://devgit.com2us.com/TS/TPact/wikis/LvRender/4)-Basic-Texture-Sample#6-%ED%95%B4%EC%A0%9C
		*/
		virtual void DestroySampler(LvSamplerHandle* sampler) = 0;

		/**
		* @fn LvPipelineHandle* CreatePipeline(LvPipelineDescriptor& descriptor)
		* @brief Pipeline을 생성하기 위해 이용합니다.
		* @details LvPipelineDescriptor(pipeline state object)을 원하는 방식으로 작성한 후 넣어주면 하나의 Pipeline을 생성할 수 있습니다.
		* LvPipelineDescriptor에는 각각 type,name, renderPass, vertexInput, resourceLayout,shader, rasterize, colorBlend,depthStencil 가 멤버변수로 존재합니다.
		* type 은 GRAPHICS_PIPELINE 인지 COMPUTE_PIPELINE인지 명시.
		* name 은 단순히 Pipeline의 이름을 명시.
		* renderPass 는 해당 Pipeline의 RenderPass를 의미.
		* vertexInput 은 vertex assembler를 의미.(ex Shader에 들어갈 pos, normal 등의 정보로 구성.)
		* resourceLayout Shader에서 사용될 uniform, texture와 같은 정보들의 구성을 의미.
		* shader 는 LvShaderDescriptor로써 Shader의 구성을 의미.
		* rasterize 는 LvRasterizationDescriptor로써 Rasterization 조건 등으로 구성.
		* colorBlend 는 LvColorBlendDescriptor로써 ColorBlending 조건으로 구성.
		* depthStencil 는 LvDepthStencilDescriptor로써 DepthStencilTest 조건으로 구성.
		* @param LvPipelineDescriptor&
		* @return LvPipelineHandle*
		* @see http://devgit.com2us.com/TS/TPact/wikis/LvRender/1)-Basic-Triangle#7-pipeline
		*/
		virtual LvPipelineHandle* CreatePipeline(LvPipelineDescriptor& descriptor) = 0;
		/**
		* @fn void DestroyPipeline(LvPipelineHandle* pipeline)
		* @brief Pipeline을 제거하기 위해 이용합니다.
		* @details Pipeline을 위해 이용하던 memory를 반환합니다.
		* @param LvPipelineHandle*
		* @return void
		*/
		virtual void DestroyPipeline(LvPipelineHandle* pipeline) = 0;

		/**
		* @fn LvResourceLayoutHandle* CreateResourceLayout(LvResourceBinding* bindings, uint32 count)
		* @brief ResourceLayout을 만들기 위한 함수입니다.
		* @details ResourceLayout란? pipeline에서 이용할 Resource(Ex UniformData, Texture, Sampler)들의 구성을 의미합니다.
		* @param LvResourceBinding* | LvResourceBinding 형태의 배열
		* @param uint |  LvResourceBinding 배열의 크기
		* @return LvResourceLayoutHandle*
		* @remark LvResourceBinding은 graphics pipeline 순서에 맞게 구성되어야 합니다.
		* 예를 들어 Fragment -> Vertex 순서대로 넣으면 올바른 LvResourceLayout을 생성할 수 없습니다.
		* @see http://devgit.com2us.com/TS/TPact/wikis/LvRender/1)-Basic-Triangle#2-resourcelayout
		*/
		virtual LvResourceLayoutHandle* CreateResourceLayout(LvResourceBinding* bindings, uint32 count) = 0;
		/**
		* @fn void DestroyResourceLayout(LvResourceLayoutHandle* layout)
		* @brief ResourceLayout을 삭제하기 위한 함수입니다.
		* @param LvResourceLayoutHandle*
		* @return void
		* @see http://devgit.com2us.com/TS/TPact/wikis/LvRender/3)-Basic-Mesh-Sample#10-%ED%95%B4%EC%A0%9C
		*/
		virtual void DestroyResourceLayout(LvResourceLayoutHandle* layout) = 0;

		/**
		* @fn LvResourceSetHandle* CreateResourceSet(LvResourceLayoutHandle* resourceLayout, LvResourceUsage* usages, uint32 usageCount)
		* @brief 사용할 Resource의 handle의 집합을 생성하는 함수입니다.
		* @details 사용할 Resource의 handle의 집합을 ResourceSet이라고 합니다.
		* LvResourceLayout에 맞게 buffer의 handle들을 모아 하나의 LvResourceSetHandle을 만들어 줍니다.
		* @param LvResourceLayoutHandle*
		* @param LvResourceUsage*
		* @param uint | LvResourceUsage의 크기
		* @return LvResourceSetHandle*
		* @see http://devgit.com2us.com/TS/TPact/wikis/LvRender/1)-Basic-Triangle#resourceset
		*/
		virtual LvResourceSetHandle* CreateResourceSet(LvResourceLayoutHandle* resourceLayout,
			LvResourceUsage* usages, uint32 usageCount) = 0;
		/**
		* @fn void DestroyResourceSet(LvResourceSetHandle* resourceSet)
		* @brief LvResourceSetHandle*을 통해서 더이상 이용하지 않는 ResourceSet을 삭제할 수 있습니다.
		* @param LvResourceSetHandle*
		* @return void
		* @see http://devgit.com2us.com/TS/TPact/wikis/LvRender/3)-Basic-Mesh-Sample#10-%ED%95%B4%EC%A0%9C
		*/
		virtual void DestroyResourceSet(LvResourceSetHandle* resourceSet) = 0;

		/**
		* @fn LvCommandEncoderHandle* CreateCommandEncoder()
		* @brief Rendering Command를 담을 CommandEncoder를 생성 할 수 있습니다.
		* @details SwapChain의 갯수만큼 CommandEncoder를 만들어야 GPU가 처리하는 중인 자원들을 건드리지 않을 수 있습니다.
		* @return LvCommandEncoderHandle*
		* @remark SwapChain의 갯수만큼 CommandEncoder를 만들어야 합니다.
		* @see http://devgit.com2us.com/TS/TPact/wikis/LvRender/3)-Basic-Mesh-Sample#2-commandencoder-%EC%B4%88%EA%B8%B0%ED%99%94
		*/
		virtual LvCommandEncoderHandle* CreateCommandEncoder() = 0;
		/**
		* @fn void DestroyCommandEncoder(LvCommandEncoderHandle* encoder)
		* @brief 사용이 끝난 CommandEncoder를 삭제할 수 있습니다.
		* @param LvCommandEncoderHandle*
		* @return void
		* @see http://devgit.com2us.com/TS/TPact/wikis/LvRender/3)-Basic-Mesh-Sample#10-%ED%95%B4%EC%A0%9C
		*/
		virtual void DestroyCommandEncoder(LvCommandEncoderHandle* encoder) = 0;

		/**
		* @fn LvFrameBufferHandle* CreateFrameBuffer(LvFrameBufferInfo& info)
		* @brief CreateFrameBuffer는 의도적으로 FrameBuffer를 만들기 위해 이용합니다.
		* @details FrameBuffer란? buffer(ex Color Buffer,Depth_stencil Buffer) handle들의 집합입니다.
		* FrameBuffer가 가지고 있을 수 있는 Texture의 종류는 Color/DepthStencil 두 종류입니다.
		* ColorTexture는 1개 이상이 될 수 있고, DepthStencilTexture는 항상 1개입니다.
		* @param LvFrameBufferInfo&
		* @return LvFrameBufferHandle*
		* @remark 별도의 FrameBuffer를 생성할 때는 항상 한 개 이상의 RenderPass가 필요합니다.
		* @see http://devgit.com2us.com/TS/TPact/wikis/LvRender/1)-Basic-Triangle#framebuffer%EC%99%80-renderpass
		*/
		virtual LvFrameBufferHandle* CreateFrameBuffer(LvFrameBufferInfo& info) = 0;
		/**
		* @fn void DestroyFrameBuffer(LvFrameBufferHandle* framebuffer)
		* @brief LvFrameBufferHandle*을 통해 FrameBuffer을 삭제할 수 있습니다.
		* @param LvFrameBufferHandle*
		* @return void
		* @remark CreateFrameBuffer함수를 통해 의도적으로 생성해준 FrameBuffer는 DestroyFrameBuffer함수를 통해 지워줘야 합니다.
		*/
		virtual void DestroyFrameBuffer(LvFrameBufferHandle* framebuffer) = 0;

		/**
		* @fn LvRenderPassHandle* CreateRenderPass(LvRenderPassInfo& info)
		* @brief RenderPass를 만들기 위한 함수입니다.
		* @details RenderPass란 FrameBuffer상의 buffer들에 대한 operation의 집합입니다.
		* 가령 OpenGL의 관점에서 RenderPass를 설명하자면 glClearColor, glClear 등을 할 지 말지 우선적으로 정하는 것과 같습니다.
		* @param LvRenderPassInfo& | RenderPass의 spec
		* @return LvRenderPassHandle*
		* @remark 매개변수 LvRenderPassInfo의 멤버변수 isSwapChainRenderPass을 true로 설정시 SwapChain의 buffer가 default buffer로 설정됩니다.
		* @see http://devgit.com2us.com/TS/TPact/wikis/LvRender/1)-Basic-Triangle#framebuffer%EC%99%80-renderpass
		* @see http://devgit.com2us.com/TS/TPact/wikis/LvRender/3)-Basic-Mesh-Sample#3-default-framebufferswapchain-framebuffer%EC%97%90-%EB%8C%80%ED%95%9C-renderpass-%EC%83%9D%EC%84%B1
		*/
		virtual LvRenderPassHandle* CreateRenderPass(LvRenderPassInfo& info) = 0;
		/**
		* @fn void DestroyRenderPass(LvRenderPassHandle* renderPass)
		* @brief LvRenderPassHandle*을 통해 이동하던 RenderPass를 삭제할 수 있습니다.
		* @param LvRenderPassHandle*
		* @return void
		* @see http://devgit.com2us.com/TS/TPact/wikis/LvRender/3)-Basic-Mesh-Sample#10-%ED%95%B4%EC%A0%9C
		*/
		virtual void DestroyRenderPass(LvRenderPassHandle* renderPass) = 0;

		// --> Mapbuffer only
		/**
		* @fn void* MapBuffer(LvBufferHandle* buffer, LvBufferUsage usage, size_t size)
		* @brief GPU(디바이스)에 있는 buffer의 주소값을 받아오기 위한 함수입니다.
		* @details CPU(호스트)에 있는 데이터를 GPU(디바이스)쪽으로 넘기기 위해서는 GPU쪽 buffer의 주소값이 필요합니다.
		* 좀 더 정확히 얘기하자면 응용프로그램의 메모리 주소로 오브젝트의 메모리를 mapping합니다.
		* 이를 "호스트에 mapping되었다"고 표현합니다.
		* @param LvBufferHandle* | 주소값을 받아오기 위한 GPU(디바이스)쪽 buffer
		* @param LvBufferUsage | 이용할 buffer의 사용 용도
		* @param size_t | buffer의 크기
		* @return void* | buffer의 주소값을 반환
		* @remark 사용을 마친 buffer는 UnmapBuffer()을 이용해 CPU와 GPU의 연결관계를 끊어주어야 합니다.
		*/
		//https://www.seas.upenn.edu/~pcozzi/OpenGLInsights/OpenGLInsights-AsynchronousBufferTransfers.pdf
		virtual void* MapBuffer(LvBufferHandle* buffer, size_t size, size_t offset = 0) = 0;
		/**
		* @fn bool UnmapBuffer(LvBufferHandle* buffer, LvBufferUsage usage)
		* @brief MapBuffer() 함수를 이용해서 주소값을 받아와 이용하던 GPU(디바이스)쪽 buffer의 사용을 마무리한다는 의미의 함수입니다.
		* @details 가령 CPU(호스트)의 데이터를 GPU(디바이스)의 buffer쪽으로 복사한다고 할 때 GPU(디바이스) buffer의 주소값을 MapBuffer()함수를 통해 받아온 후
		* CPU(호스트)데이터의 복사를 진행할 것입니다. 복사를 완료한 후에는 CPU(호스트)와 GPU(디바이스)쪽의 연결관계를 끊는 작업이 필요합니다.(동기화 문제 때문에)
		* 따라서 이용하던 연결관계를 끊기 위해 사용되는 함수입니다.
		* @param LvBufferHandle* | 이용을 끝낸 BufferHandle*
		* @param LvBufferUsage
		* @return bool | LV가 buffer에 데이터를 성공적으로 매핑할 수 있었다면 true값을 리턴
		* @remark 대부분의 경우 MapBuffer()와 한 pair를 이루어 이용됩니다.
		*/
		//https://lifeisforu.tistory.com/408
		virtual bool UnmapBuffer(LvBufferHandle* buffer) = 0;

		/**
		* @fn void LvUpdateBuffer(LvBufferHandle* buffer, LvBufferUsage usage, LvMemoryOption option , size_t size, size_t offset, void* data)
		* @brief buffer를 특정 data로 업데이트하기 위해서 이용할 수 있는 함수입니다. LvMemoryOption이 GPU_PRIVATE, MAP_MANAGED인지 상관없이 내부에서 buffer를 업데이트 시켜줍니다.
		* @detail GPU_PRIVATE buffer도 업데이트 할 수 있습니다. GPU_PRIVATE buffer의 경우 플랫폼별로 내부적으로 stagingbuffer를 임시 생성하거나 하는 방법으로 업데이트를 진행합니다.
		* @detail 또한 offset과 size를 이용하여 버퍼의 원하는 부분(offset)에 원하는만큼(size) 업데이트를 할 수 있습니다.
		*/
		virtual void UpdateBuffer(LvBufferHandle* buffer, size_t offset, void* data, size_t size, bool useBarrier) = 0;

		/**
		* @fn void BlitFramebuffer(uint srcX0, uint srcY0, uint srcX1, uint srcY1,LvFrameBufferHandle* srcBuffer, uint dstX0, uint dstY0, uint dstX1, uint dstY1, LvFrameBufferHandle* dstBuffer)
		* @brief 일단 colorbuffer의 단순 복사만을 위해 구현되어 있다. depthFramebuffer나 stencilFramebuffer의 복사도 가능하도록 구현될 예정이다.
		*/
		virtual void BlitFramebuffer(uint srcX0, uint srcY0, uint srcX1, uint srcY1, LvFrameBufferHandle* srcBuffer, uint dstX0, uint dstY0, uint dstX1, uint dstY1, LvFrameBufferHandle* dstBuffer) = 0;

		//// -->
		///**
		//* @fn void Execute(LvByteBuffer& buffer)
		//* @brief 여러가지 Command들을 실행하기 위한 함수입니다.
		//* @details 대개 Present() 함수 내부에서 이용됩니다.
		//* @param LvByteBuffer&
		//* @return void
		//*/
		//virtual void Execute(LvByteBuffer& buffer) = 0;

		/**
		* @fn LvPixelFormat GetDefaultDepthFormat()
		* @brief Depth Format은 선택적인 것이기 때문에 사용하기에 알맞은 Depth Format을 선택하기 위해 사용되는 함수입니다.
		* @return LvPixelFormat
		*/
		virtual LvPixelFormat GetDefaultDepthFormat() = 0;

		/**
		* @fn void Submit(LvCommandEncoderHandle* encoder)
		* @brief EncoderBuffer를 완성한 후 RenderContext에 제출하기 위한 함수입니다.
		* @details 쉽게 말해, GPU에 실행할 명령어를 제출하는 것입니다.
		* encoder를 통해 EncoderBuffer을 완성하고 RenderContext의 내부 buffer에 해당 EncoderBuffer의 handle을 넣는 것입니다.
		* @param LvCommandEncoderHandle*
		* @return void
		* @see http://devgit.com2us.com/TS/TPact/wikis/LvRender/2)-Basic-Synchronization#submit
		*/
		virtual void Submit(LvSwapChain* swapchain, LvCommandEncoderHandle* encoder) = 0;

		/**
		* @fn void Present()
		* @brief FrameBuffer에 그렸던 것을 모니터에 보여달라는 요청입니다.
		* @return void
		* @remark Acquire-Submit-Present 의 순서대로 사용해야 올바르게 렌더링을 할 수 있습니다.
		* @see http://devgit.com2us.com/TS/TPact/wikis/LvRender/2)-Basic-Synchronization#present
		*/
		virtual void Present(LvSwapChain* swapchain) = 0;

		/**
		* @fn void Suspend()
		* @brief 잠시 렌더링를 멈추고 싶을 때 이용합니다.
		* @detail 가령 굳이 화면에 보이지 않아 연산이 필요하지 않을 때 이용할 수 있습니다.
		*/
		virtual void Suspend(LvSwapChain* swapchain) = 0;

		/**
		* @fn void Restore()
		* @brief 일시 정지 시켰던 렌더링을 다시 시작할 때 이용합니다.
		*/
		virtual void Restore(LvSwapChain* swapchain) = 0;


		// LvRender 3.0 Spec.
		virtual bool HasFeature(LvRenderFeature feature) = 0;

		/**
		* @fn void WaitDeviceIdle()
		* @brief GPU에서 돌아가는 명령(command)들이 다 끝날 때까지 대기하도록 합니다.
		* @detail 주로 렌더링 과정이 끝나서 할당했던 memory들을 해제하기 전에 이용합니다.
		* GPU에 명령들이 다 끝나지 않았는데 GPU에서 사용되고 있는 memory들을 해제하려고 한다면 에러가 발생할 것입니다.
		* 이러한 상황을 막기 위해 memory 해제 전에 이용하는 함수입니다.
		* @return void
		* @remark GPU에서 사용하고 있던 memory를 해제하기 전에 이용해야 합니다.
		* @see http://devgit.com2us.com/TS/TPact/wikis/LvRender/3)-Basic-Mesh-Sample#10-%ED%95%B4%EC%A0%9C
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

		virtual LvResourceSetPool* CreateResourceSetPool(uint32 maxSetFromPool, uint32 maxUniformBufferFromPool, uint32 maxTextureFromPool) = 0;

		virtual void DestroyResourceSetPool(LvResourceSetPool* resourceSetPool) = 0;

	};

#ifdef __cplusplus
}
#endif
LV_NS_RENDER_END

#endif // _LV_RENDER_CONTEXT_H_