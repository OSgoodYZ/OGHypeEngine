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

//OG_NAMESPACE_SYSTEM_END