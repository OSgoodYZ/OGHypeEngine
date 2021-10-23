#include "OgSystemContext.h"

struct OgMonitor;

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