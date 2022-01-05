#pragma once
#ifndef _OG_SYSTEM_CONTEXT_H_
#define _OG_SYSTEM_CONTEXT_H_

#include "OgPrecompile.h"

#if defined(__WIN32__)
#include "system/platform/win/OgPlatformDefinition_win32.h"
#elif defined(__MACOSX__)
#include "system/platform/mac/OgPlatformDefinitions_cocoa.h"
#elif defined(__IOS__)
#include "system/platform/ios/OgPlatformDefinitions_ios.h"
#elif defined(__ANDROID__)
#include "system/platform/android/OgPlatformDefinitions_android.h"
#endif


#define OG_DONT_CARE -1
//OG_NAMESPACE_SYSTEM_BEGIN

struct OgMonitor;
struct OgSystemContext;
typedef void(*MonitorCallback)(OgMonitor*, int);

// GLFW Reference
struct OG_API OgGraphicLibrary
{
	void *handle;

	int major;
	int minor;

	const char* name;

	void* (*GetFunction)(const char* name);
};

struct OG_API OgSystemInitConfig
{
	bool hatButtons;
	int  angleType;

	struct
	{
		bool menubar;
		bool chdir;
	} mac;
};


enum OG_API OgPlatformType
{
	WINDOWS,
	//MACOS,
	//ANDROID,
	//IOS,
};

struct OG_API OgFrameBufferConfig
{
	int redBits;
	int greenBits;
	int blueBits;
	int alphaBits;
	int depthBits;
	int stencilBits;
	int accumRedBits;
	int accumGreenBits;
	int accumBlueBits;
	int accumAlphaBits;
	int auxBuffers;
	bool stereo;
	int samples;
	bool sRGB;
	bool doublebuffer;
	bool transparent;
	uintptr_t handle;
};

struct OG_API OgWindowConfig
{
	int width;
	int height;
	const char* title;
	bool resizable;
	bool visible;
	bool decorated;
	bool focused;
	bool autoIconify;
	bool floating;
	bool maximized;
	bool centerCursor;
	bool focusOnShow;
	bool mousePassThrough;
	bool scaleToMonitor;
	struct
	{
		bool retina;
		char frameName[256];
	} mac;

	struct
	{
		bool keymenu;
	} win32;
	bool debug;

	struct OgMonitor* monitor;
	struct OgNativeWindow* share;
};

struct OG_API OgVideoMode
{
	int width;
	int height;
	int redBits;
	int greenBits;
	int blueBits;
	int refreshRate;
};

// Notifies shared code of a monitor connection or disconnection
enum OG_API OgMonitorAction
{
	CONNECTED,
	DISCONNECTED,

	INSERT_FIRST,
	INSERT_LAST
};

struct OG_API OgMonitor
{
	char* name;
	int physicalWidth;
	int	physicalHeight;

	// The window whose video mode is current on this monitor
	OgNativeWindow* window;

	OgVideoMode* modes;
	int modeCount;

	OgVideoMode currentMode;

	_OG_PLATFORM_MONITOR_STATE
};

struct OG_API OgCursor
{
	OgCursor*	next;

	_OG_PLATFORM_CURSOR_STATE
};

struct OG_API OgNativeWindow
{
	OgNativeWindow* next;

	// windo settings and state
	char* title;
	bool resizable;
	bool decorated;
	bool autoIconify;
	bool floating;
	bool shouldClose;
	bool minimized;
	bool focused;

	OgVideoMode videoMode;
	OgMonitor* monitor;
	OgCursor* cursor;

	int x, y;
	int width, height;

	int minWidth, minHeight;
	int maxWidth, maxHeight;
	int numer, denom;

	bool stickyKeys;
	bool stickyMouseButtons;
	bool lockKeyMods;
	int cursorMode;
	bool rawMouseMotion;
	OgInput input;

	OgSystemContext* context;

	_OG_PLATFORM_WINDOW
};


struct OG_API OgSystemContext
{
	// Default configuration for graphics interface.
	struct
	{
		OgSystemInitConfig init;

		OgFrameBufferConfig frameBuffer;

		OgWindowConfig window;

		int refreshRate;

	} configs;

	bool isInitialized;

	int platform;

	bool isDebug;

	OgNativeWindow* headWindow;

	OgCursor* headCursor;

	OgMonitor** monitors;

	int monitorCount;

	MonitorCallback monitorCallback;

	// 실행파일의 경로
	const char* executablePath;

	// 실행파일의 디렉토리 경로
	const char* executableDirectoryPath;

	// 작업이 실행된 경로, 실행파일의 디렉토리 경로가 아닐 수 있음
	const char* currentDirectoryPath;

	OgGraphicLibrary graphicLibrary;

	uint frameRate;

	_OG_PLATFORM_CONTEXT
};

OG_API bool og_system_init(OgSystemContext* context, void* platformHandle);
OG_API void og_system_terminate(OgSystemContext* context);
OG_API OgSystemContext* og_system_get_context();
OG_API void og_system_poll_events();
OG_API void og_system_monitor_set_callback(OgSystemContext*, MonitorCallback cb);
OG_API void og_monitor_get_pos(OgMonitor* monitor, int* outXPos, int* outYPos);
OG_API void og_monitor_get_video_mode(OgMonitor* monitor, OgVideoMode* outVideoMode);
OG_API void og_monitor_get_content_scale(OgMonitor* monitor, float* xscale, float* ysceale);
OG_API void og_system_change_executable_path(OgSystemContext* context, const char* path);
OG_API void* og_file_relative_load_from_package(OgSystemContext* context, const char* path, bool binary, unsigned* outSize);
OG_API void* og_file_absolute_load_from_package(OgSystemContext* context, const char* path, bool binary, unsigned* outSize);
OG_API void og_file_unload_from_package(void* file);
OG_API bool og_file_exist_from_package(OgSystemContext* context, const char* path);
// This is the same with the OgSystemContext's executablePath, except Android Platform.
// Because Android's AAsset_open function does not need absolute path.
// It means this function will not fill the outBuf in Android Platform
// So When you try to get package folder correctly for all platforms (win/mac/ios/android)
// you should use this.
OG_API void og_package_folder_get_with_seperator(OgSystemContext* context, char* outBuf, unsigned outBufSize);
OG_API OgCursor* og_cursor_create(OgSystemContext* context, int shape);
OG_API void og_cursor_destroy(OgSystemContext* context, OgCursor* handle);
// cpu device info
// http://blog.naver.com/PostView.nhn?blogId=sorkelf&logNo=221126168094
OG_API void og_cpu_get_vender(const char* out);
OG_API void og_cpu_get_brand(const char* out);
OG_API bool og_cpu_is_support(const char* instruction);
OG_API int og_key_get_scancode(int key);

//OG_NAMESPACE_SYSTEM_END


#endif // _OG_SYSTEM_CONTEXT_H_