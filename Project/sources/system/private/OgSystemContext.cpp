#include "system/OgSystemContext.h"
#include "system/OgMemory.h"

#if defined(__WIN32__)
#include "system/private/platform/win/OgPlatform_win32.h"
#elif defined(__MACOSX__)
// TODO
#elif defined(__IOS__)
// TODO
#elif defined(__ANDROID__)
// TODO
#endif

//OG_NAMESPACE_SYSTEM_BEGIN

static OgSystemContext* _context;

bool og_system_init(OgSystemContext* context, void* platformHandle)
{
	_context = context;

	if (_context->isInitialized)
		return true;

#if defined(OG_USE_CRT_CHASE_MEMORY_LEAK)
	_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
	_CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDOUT);
	_CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE);
	_CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDOUT);
	_CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
	_CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDOUT);
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	// memset(context, 0, sizeof(LvSystemContext));

	LOGD(OG_ID, "OgContext has set.");

	_context->isInitialized = true;

#if defined(_DEBUG)
	_context->isDebug = true;
#else
	_context->isDebug = false;
#endif

	if (!og_platform_system_init(_context, platformHandle))
	{
		//terminate();
		return false;
	}
	

	return true;
}

void og_system_terminate(OgSystemContext* context)
{
	//TODO
}
OgSystemContext* og_system_get_context()
{
	return _context;
}
void og_system_poll_events() 
{
	//TODO
}
void og_system_monitor_set_callback(OgSystemContext*, MonitorCallback cb)
{
	//TODO
}
void og_monitor_get_pos(OgMonitor* monitor, int* outXPos, int* outYPos)
{
	if (outXPos)
		*outXPos = 0;

	if (outYPos)
		*outYPos = 0;

	og_platform_monitor_get_pos(monitor, outXPos, outYPos);
}
void og_monitor_get_video_mode(OgMonitor* monitor, OgVideoMode* outVideoMode)
{
	//TODO
}
void og_monitor_get_content_scale(OgMonitor* monitor, float* xscale, float* ysceale)
{
	//TODO
}
void og_system_change_executable_path(OgSystemContext* context, const char* path) {
	//TODO
}
void* og_file_relative_load_from_package(OgSystemContext* context, const char* path, bool binary, unsigned* outSize)
{
	//TODO
	return nullptr;
}
void* og_file_absolute_load_from_package(OgSystemContext* context, const char* path, bool binary, unsigned* outSize) 
{
	//TODO
	return nullptr;
}
void og_file_unload_from_package(void* file)
{
	//TODO
}
bool og_file_exist_from_package(OgSystemContext* context, const char* path)
{
	//TODO
	return true;
}
void og_package_folder_get_with_seperator(OgSystemContext* context, char* outBuf, unsigned outBufSize)
{
	//TODO
}
OgCursor* og_cursor_create(OgSystemContext* context, int shape)
{
	//TODO
	return nullptr;
}
void og_cursor_destroy(OgSystemContext* context, OgCursor* handle)
{
	//TODO
}
// cpu device info
// http://blog.naver.com/PostView.nhn?blogId=sorkelf&logNo=221126168094
void og_cpu_get_vender(const char* out)
{
	//TODO
}
void og_cpu_get_brand(const char* out)
{
	//TODO
}
bool og_cpu_is_support(const char* instruction)
{
	//TODO
	return true;
}
int og_key_get_scancode(int key)
{
	//TODO
	return 1;
}
//OG_NAMESPACE_SYSTEM_END