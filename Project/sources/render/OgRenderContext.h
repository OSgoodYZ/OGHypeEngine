#pragma once
#ifndef _OG_RENDER_CONTEXT_H_
#define _OG_RENDER_CONTEXT_H_

#include "OgPrecompile.h"
#include "OgRenderDefinitions.h"


OG_NAMESPACE_RENDER_BEGIN

#ifdef __cplusplus
extern "C" {
#endif

class OG_API OgRenderContext
{
public:
	

	virtual ~OgRenderContext()
	{	};
	
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
	virtual OgSwapChain* CreateSwapchain(OgNativeWindow* nativeWindow, const OgSwapChainInfo& swapchainInfo) = 0;

	virtual void DestroySwapchain(LvSwapChain* swapchain) = 0;

	// TODO: reconsider to delete
	//virtual LvSwapChain* GetSwapchain(LvNativeWindow* nativeWindow) = 0;
private:
	
};

#ifdef __cplusplus
}
#endif

OG_NAMESPACE_RENDER_END
#endif // _OG_RENDER_CONTEXT_H_