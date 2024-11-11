#pragma once
#ifndef __OG_IMGUI_CONTEXT_MANAGER_H__
#define __OG_IMGUI_CONTEXT_MANAGER_H__

#include "OgPrecompile.h"
#include "system//thirdparty/imgui/imgui.h"
#include "system//thirdparty/imgui/imgui_internal.h"

/* #include "engine/thirdparty/imgui/imgui.h" */ struct ImGuiViewport;
/* #include "engine/thirdparty/imgui/imgui.h" */ struct ImGuiContext;
/* #include "system/LvSystemContext.h" */ struct LvNativeWindow;
/* #include "engine/thirdparty/imgui/imgui_internal.h */ struct ImGuiDockNode;
/* #include "engine/thirdparty/imgui/imgui_internal.h */ struct ImGuiTabBar;
OG_NAMESPACE_SYSTEM_BEGIN

class OG_API OgImGuiContextManager
{
public:

	
	static void Initialize(bool loadDefaultFont = true);

	static void Finalize();

	static ImGuiContext* CreateContext(bool loadDefaultFont);

	static void DestroyContext(ImGuiContext* context);

	static void* GetMainImGuiContext();

	static void SetMainImGuiContext(ImGuiContext* context);

	static void* UpdateDockingLayout(void* ctx, const char* iniData, bool loadDefaultFont = true);
	

};

OG_NAMESPACE_SYSTEM_END

#endif