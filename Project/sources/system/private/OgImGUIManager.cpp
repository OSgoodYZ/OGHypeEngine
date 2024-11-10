#include "..\OgImGUIManager.h"

#include "system/OgFileSystem.h"
#include "system/OgSystemContext.h"
#include "system//thirdparty/imgui/imgui.h"
#include "system//thirdparty/imgui/imgui_internal.h"
#include "system/thirdparty/IconsFontAwesome6.h"

OG_NAMESPACE_SYSTEM_BEGIN
static bool s_managerInit = false;

static ImGuiContext* s_imguiContext = nullptr;

static void imgui_context_setting(ImGuiContext* context, bool loadDefaultFont)
{
	ImGuiIO& io = context->IO;

	if (loadDefaultFont)
	{
		
		io.Fonts->AddFontFromFileTTF(og_path_combine(og_system_get_context()->executableDirectoryPath, "fonts/NotoSansCJKkr-Regular.otf").c_str(), 18, nullptr, io.Fonts->GetGlyphRangesKorean());

		ImFontConfig config;
		config.MergeMode = true;
		config.GlyphMinAdvanceX = 13.0f; // Use if you want to make the icon monospaced
		ImWchar icon_ranges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };
		io.Fonts->AddFontFromFileTTF(og_path_combine(og_system_get_context()->executableDirectoryPath, "fonts/" FONT_ICON_FILE_NAME_FAR).c_str(), 13.0f, &config, icon_ranges);
		io.Fonts->AddFontFromFileTTF(og_path_combine(og_system_get_context()->executableDirectoryPath, "fonts/" FONT_ICON_FILE_NAME_FAS).c_str(), 13.0f, &config, icon_ranges);
	}

	// In order to build fonts data
	unsigned char* fontData;
	int texWidth, texHeight;
	io.Fonts->GetTexDataAsRGBA32(&fontData, &texWidth, &texHeight);

	/*const LvString& path = "D:/Users/spacejwj/Documents/Resource/default.png";

	stbi_write_png(path.c_str(), texWidth, texHeight, 1, fontData, texWidth*1);*/

#if defined(__DESKTOP__)
	//ini 파일을 수동으로 저장하겠다는 것을 선언함
	io.IniFilename = NULL; //io.WantSaveIniSettings를 save 후에 false로 clear해줘야 한다.
#endif

	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;   // Enable Gamepad Controls
#if defined(__DESKTOP__)
	//io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	//io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
#endif
	io.ConfigWindowsMoveFromTitleBarOnly = true;	// Multi Window 때는 TitleBar로만 움직일 수 있게 해야 드래깅할 수 있음.

	// TODO
	//if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	//{
	//	ImGuiStyle& style = context->Style;
	//	style.WindowRounding = 0.f;
	//	style.Colors[ImGuiCol_WindowBg].w = 1.f;
	//}

	// Setup Back-End capabilities flags
#if defined(__DESKTOP__)
	io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;		// We can honor GetMouseCursor() values (optional)
	io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;		// We can honor io.WantSetMousePos requests (optional, rarely used)

	const int monitorCount = og_system_get_context()->monitorCount;
	if (0 < monitorCount)
	{
		//io.BackendFlags |= ImGuiBackendFlags_PlatformHasViewports;	// We can create multi-viewports on the Platform side (optional)
		//io.BackendFlags |= ImGuiBackendFlags_RendererHasViewports;	// We can create multi-viewports on the Renderer side (optional)
		
	}
	else
	{
		
	}
#endif

	io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;	// We can honor the ImDrawCmd::VtxOffset field, allowing for large meshes.
	// Keyboard mapping. ImGui will use those indices to peek into the io.KeysDown[] array.

}

//TODO
//void(*LvImGuiContextManager::dockNodeWindowMenuHandler)(ImGuiContext* ctx, ImGuiDockNode*, ImGuiTabBar*) = nullptr;

void OgImGuiContextManager::Initialize(bool loadDefaultFont)
{
	if (s_managerInit == true)
		return;

	IMGUI_CHECKVERSION();
	//ImGui::SetAllocatorFunctions(imgui_malloc, imgui_free);

	s_managerInit = true;
}

void OgImGuiContextManager::Finalize()
{
	if (s_managerInit == false)
		return;

	if (s_imguiContext != nullptr)
		DestroyContext(s_imguiContext);
	s_managerInit = false;
}

ImGuiContext* OgImGuiContextManager::CreateContext(bool loadDefaultFont)
{
	ImGuiContext* ctx = ImGui::CreateContext();

	// TODO
	//if (nullptr != dockNodeWindowMenuHandler)
	//{
	//	ctx->DockNodeWindowMenuHandler = dockNodeWindowMenuHandler;
	//}

	if (s_imguiContext == nullptr)
	{
		s_imguiContext = ctx;
	}

	imgui_context_setting(ctx, loadDefaultFont);
	
	return ctx;
}

void OgImGuiContextManager::DestroyContext(ImGuiContext* context)
{
	if (s_imguiContext == context)
		s_imguiContext = nullptr;

	context->Viewports[0]->PlatformHandle = nullptr;
	context->Viewports[0]->PlatformHandleRaw = nullptr;

	ImGui::DestroyContext(context);
}

void* OgImGuiContextManager::GetMainImGuiContext()
{
	return s_imguiContext;
}

void OgImGuiContextManager::SetMainImGuiContext(ImGuiContext* context)
{
	s_imguiContext = context;
	ImGui::SetCurrentContext(context);
}

void* OgImGuiContextManager::UpdateDockingLayout(void* ctx, const char* iniData, bool loadDefaultFont)
{
	if (ctx == nullptr)
		return nullptr;

	ImGuiContext* context = (ImGuiContext*)ctx;

	ImGui::LoadIniSettingsFromMemory(iniData);

	return (void*)s_imguiContext;
}


OG_NAMESPACE_SYSTEM_END