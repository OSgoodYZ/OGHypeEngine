#pragma once
#ifndef _OG_NATIVE_EVENT_H_
#define _OG_NATIVE_EVENT_H_

#include "OgPrecompile.h"

#ifdef __cplusplus
extern "C" {
#endif

	enum OgEventType
	{
		OG_WINDOW_CLOSE = 0,
		OG_WINDOW_MOVE = 1,
		OG_WINDOW_RESIZING = 2,
		OG_WINDOW_RESIZED = 3,
		OG_WINDOW_RESTORE = 4,
		OG_WINDOW_MINIMIZE = 5,
		OG_WINDOW_MAXMIZE = 6,
		OG_WINDOW_FOCUS_IN = 7,
		OG_WINDOW_FOCUS_OUT = 8,
		OG_KEY_PRESS = 9,
		OG_KEY_RELEASE = 10,
		OG_MOUSE_WHEEL_CHANGE = 11,
		OG_MOUSE_PRESS = 12,
		OG_MOUSE_RELEASE = 13,
		OG_MOUSE_MOVE = 14,
		OG_DROP_FILES = 15,
	};

	struct OgWindowEvent
	{
		int x, y;
		int width, height;
	};

	struct OgKeyEvent
	{
		int keyCode;
		bool alt;
		bool control;
		bool shift;
		bool system;
	};


	struct OgMouseEvent
	{
		struct
		{
			int x, y;
		} pos;

		struct
		{
			int x, y;

		} delta;

		int button;
		int mods;
		int wheelDelta;
	};


	struct OgDropEvent
	{
		void* paths;	// LvList<LvString>*
		uint64 count;
	};

	/**
	 * @file #include "system/LvNativeEvent.h"
	 */
	struct OgNativeEvent
	{
		enum OgEventType type;

		union
		{
			struct OgWindowEvent window;
			struct OgKeyEvent key;
			struct OgMouseEvent mouse;
			struct OgDropEvent drop;
		};

#ifdef __cplusplus
		OgNativeEvent() = default;
		OgNativeEvent& operator= (const OgNativeEvent& rhs) = default;
#endif
	};

#ifdef __cplusplus
}
#endif

#endif // _OG_NATIVE_EVENT_H_
