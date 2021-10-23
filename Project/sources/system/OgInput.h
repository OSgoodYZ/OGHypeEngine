#pragma warning(disable: 4819)
#pragma once
#ifndef _OG_INPUT_H_
#define _OG_INPUT_H_

#include "OgPrecompile.h"

#ifdef __cplusplus
extern "C" {
#endif

	/////////////////// Input Macro ///////////////////////

#define OG_KEY_UNKNOWN            -1
/**
* 터치, 마우스, 키보드의 누름 해제.
*/
#define OG_RELEASE                0

/**
* 터치, 마우스, 키보드의 누름.
*/
#define OG_PRESS                  1

// OG_STATIONARY
/**
* 터치, 마우스, 키보드 계속 누름.
*/
#define OG_REPEAT                 2


/* Printable keys */
#define OG_KEY_SPACE              32
#define OG_KEY_APOSTROPHE         39  /* ' */
#define OG_KEY_COMMA              44  /* , */
#define OG_KEY_MINUS              45  /* - */
#define OG_KEY_PERIOD             46  /* . */
#define OG_KEY_SLASH              47  /* / */
#define OG_KEY_0                  48
#define OG_KEY_1                  49
#define OG_KEY_2                  50
#define OG_KEY_3                  51
#define OG_KEY_4                  52
#define OG_KEY_5                  53
#define OG_KEY_6                  54
#define OG_KEY_7                  55
#define OG_KEY_8                  56
#define OG_KEY_9                  57
#define OG_KEY_SEMICOLON          59  /* ; */
#define OG_KEY_EQUAL              61  /* = */
#define OG_KEY_A                  65
#define OG_KEY_B                  66
#define OG_KEY_C                  67
#define OG_KEY_D                  68
#define OG_KEY_E                  69
#define OG_KEY_F                  70
#define OG_KEY_G                  71
#define OG_KEY_H                  72
#define OG_KEY_I                  73
#define OG_KEY_J                  74
#define OG_KEY_K                  75
#define OG_KEY_L                  76
#define OG_KEY_M                  77
#define OG_KEY_N                  78
#define OG_KEY_O                  79
#define OG_KEY_P                  80
#define OG_KEY_Q                  81
#define OG_KEY_R                  82
#define OG_KEY_S                  83
#define OG_KEY_T                  84
#define OG_KEY_U                  85
#define OG_KEY_V                  86
#define OG_KEY_W                  87
#define OG_KEY_X                  88
#define OG_KEY_Y                  89
#define OG_KEY_Z                  90
#define OG_KEY_LEFT_BRACKET       91  /* [ */
#define OG_KEY_BACKSLASH          92  /* \ */
#define OG_KEY_RIGHT_BRACKET      93  /* ] */
#define OG_KEY_GRAVE_ACCENT       96  /* ` */
#define OG_KEY_WORLD_1            161 /* non-US #1 */
#define OG_KEY_WORLD_2            162 /* non-US #2 */

/* FunctOGkeys */
#define OG_KEY_ESCAPE             256
#define OG_KEY_ENTER              257
#define OG_KEY_TAB                258
#define OG_KEY_BACKSPACE          259
#define OG_KEY_INSERT             260
#define OG_KEY_DELETE             261
#define OG_KEY_RIGHT              262
#define OG_KEY_LEFT               263
#define OG_KEY_DOWN               264
#define OG_KEY_UP                 265
#define OG_KEY_PAGE_UP            266
#define OG_KEY_PAGE_DOWN          267
#define OG_KEY_HOME               268
#define OG_KEY_END                269
#define OG_KEY_CAPS_LOCK          280
#define OG_KEY_SCROLL_LOCK        281
#define OG_KEY_NUM_LOCK           282
#define OG_KEY_PRINT_SCREEN       283
#define OG_KEY_PAUSE              284
#define OG_KEY_F1                 290
#define OG_KEY_F2                 291
#define OG_KEY_F3                 292
#define OG_KEY_F4                 293
#define OG_KEY_F5                 294
#define OG_KEY_F6                 295
#define OG_KEY_F7                 296
#define OG_KEY_F8                 297
#define OG_KEY_F9                 298
#define OG_KEY_F10                299
#define OG_KEY_F11                300
#define OG_KEY_F12                301
#define OG_KEY_F13                302
#define OG_KEY_F14                303
#define OG_KEY_F15                304
#define OG_KEY_F16                305
#define OG_KEY_F17                306
#define OG_KEY_F18                307
#define OG_KEY_F19                308
#define OG_KEY_F20                309
#define OG_KEY_F21                310
#define OG_KEY_F22                311
#define OG_KEY_F23                312
#define OG_KEY_F24                313
#define OG_KEY_F25                314
#define OG_KEY_KP_0               320
#define OG_KEY_KP_1               321
#define OG_KEY_KP_2               322
#define OG_KEY_KP_3               323
#define OG_KEY_KP_4               324
#define OG_KEY_KP_5               325
#define OG_KEY_KP_6               326
#define OG_KEY_KP_7               327
#define OG_KEY_KP_8               328
#define OG_KEY_KP_9               329
#define OG_KEY_KP_DECIMAL         330
#define OG_KEY_KP_DIVIDE          331
#define OG_KEY_KP_MULTIPLY        332
#define OG_KEY_KP_SUBTRACT        333
#define OG_KEY_KP_ADD             334
#define OG_KEY_KP_ENTER           335
#define OG_KEY_KP_EQUAL           336
#define OG_KEY_LEFT_SHIFT         340
#define OG_KEY_LEFT_CONTROL       341
#define OG_KEY_LEFT_ALT           342
#define OG_KEY_LEFT_SUPER         343
#define OG_KEY_RIGHT_SHIFT        344
#define OG_KEY_RIGHT_CONTROL      345
#define OG_KEY_RIGHT_ALT          346
#define OG_KEY_RIGHT_SUPER        347
#define OG_KEY_MENU               348

#define OG_KEY_LAST               OG_KEY_MENU

/*! @} */

/*! @defgroup mods Modifier key flags
*  @brief Modifier key flags.
*
*  See [key input](@ref input_key) for how these are used.
*
*  @ingroup input
*  @{ */

/*! @brief If this bit is set one or more Shift keys were held down.
*/
#define OG_MOD_SHIFT           0x0001

/*! @brief If this bit is set one or more Control keys were held down.
*/
#define OG_MOD_CONTROL         0x0002
/*! @brief If this bit is set one or more Alt keys were held down.
*/
#define OG_MOD_ALT             0x0004
/*! @brief If this bit is set one or more Super keys were held down.
*/
#define OG_MOD_SUPER         0x0008
/*! @brief If this bit is set the Caps Lock key is enabled.
 *
 *  If this bit is set the Caps Lock key is enabled and the @ref
 *  GLFW_LOCK_KEY_MODS input mode is set.
 */
#define OG_MOD_CAPS_LOCK       0x0010
 /*! @brief If this bit is set the Num Lock key is enabled.
  *
  *  If this bit is set the Num Lock key is enabled and the @ref
  *  GLFW_LOCK_KEY_MODS input mode is set.
  */
#define OG_MOD_NUM_LOCK        0x0020

  /*! @} */

  /*! @defgroup buttons Mouse buttons
  *  @brief Mouse button IDs.
  *
  *  See [mouse button input](@ref input_mouse_button) for how these are used.
  *
  *  @ingroup input
  *  @{ */
#define OG_MOUSE_BUTTON_1         0
#define OG_MOUSE_BUTTON_2         1
#define OG_MOUSE_BUTTON_3         2
#define OG_MOUSE_BUTTON_4         3
#define OG_MOUSE_BUTTON_5         4
#define OG_MOUSE_BUTTON_6         5
#define OG_MOUSE_BUTTON_7         6
#define OG_MOUSE_BUTTON_8         7
#define OG_MOUSE_BUTTON_COUNT	  8
#define OG_MOUSE_BUTTON_LAST      OG_MOUSE_BUTTON_8
#define OG_MOUSE_BUTTON_LEFT      OG_MOUSE_BUTTON_1
#define OG_MOUSE_BUTTON_RIGHT     OG_MOUSE_BUTTON_2
#define OG_MOUSE_BUTTON_MIDDLE    OG_MOUSE_BUTTON_3

#define OG_CURSOR                 0x00033001
#define OG_STICKY_KEYS            0x00033002
#define OG_STICKY_MOUSE_BUTTONS   0x00033003
#define OG_LOCK_KEY_MODS          0x00033004
#define OG_RAW_MOUSE_MOTION       0x00033005

#define OG_CURSOR_NORMAL          0x00034001
#define OG_CURSOR_HIDDEN          0x00034002
#define OG_CURSOR_DISABLED        0x00034003


  /**
  *  일반적인 커서
  */
#define OG_ARROW_CURSOR           0x00036001
  /**
  *  I-beam 커서
  */
#define OG_IBEAM_CURSOR           0x00036002
  /**
  *  crosshair
  */
#define OG_CROSSHAIR_CURSOR       0x00036003
  /*! @brief The pointing hand cursor shape.
   *
   *  The pointing hand cursor shape.
   */
#define OG_POINTING_HAND_CURSOR   0x00036004
   /*! @brief The horizontal resize/move arrow shape.
	*
	*  The horizontal resize/move arrow shape.  This is usually a horizontal
	*  double-headed arrow.
	*/
#define OG_RESIZE_EW_CURSOR       0x00036005
	/*! @brief The vertical resize/move arrow shape.
	 *
	 *  The vertical resize/move shape.  This is usually a vertical double-headed
	 *  arrow.
	 */
#define OG_RESIZE_NS_CURSOR       0x00036006
	 /*! @brief The top-left to bottom-right diagonal resize/move arrow shape.
	  *
	  *  The top-left to bottom-right diagonal resize/move shape.  This is usually
	  *  a diagonal double-headed arrow.
	  *
	  *  @note @macos This shape is provided by a private system API and may fail
	  *  with @ref GLFW_CURSOR_UNAVAILABLE in the future.
	  *
	  *  @note @x11 This shape is provided by a newer standard not supported by all
	  *  cursor themes.
	  *
	  *  @note @wayland This shape is provided by a newer standard not supported by
	  *  all cursor themes.
	  */
#define OG_RESIZE_NWSE_CURSOR     0x00036007
	  /*! @brief The top-right to bottom-left diagonal resize/move arrow shape.
	   *
	   *  The top-right to bottom-left diagonal resize/move shape.  This is usually
	   *  a diagonal double-headed arrow.
	   *
	   *  @note @macos This shape is provided by a private system API and may fail
	   *  with @ref GLFW_CURSOR_UNAVAILABLE in the future.
	   *
	   *  @note @x11 This shape is provided by a newer standard not supported by all
	   *  cursor themes.
	   *
	   *  @note @wayland This shape is provided by a newer standard not supported by
	   *  all cursor themes.
	   */
#define OG_RESIZE_NESW_CURSOR     0x00036008
	   /*! @brief The omni-directional resize/move cursor shape.
		*
		*  The omni-directional resize cursor/move shape.  This is usually either
		*  a combined horizontal and vertical double-headed arrow or a grabbing hand.
		*/
#define OG_RESIZE_ALL_CURSOR      0x00036009
		/*! @brief The operation-not-allowed shape.
		 *
		 *  The operation-not-allowed shape.  This is usually a circle with a diagonal
		 *  line through it.
		 *
		 *  @note @x11 This shape is provided by a newer standard not supported by all
		 *  cursor themes.
		 *
		 *  @note @wayland This shape is provided by a newer standard not supported by
		 *  all cursor themes.
		 */
#define OG_NOT_ALLOWED_CURSOR     0x0003600A
		 /*! @brief Legacy name for compatibility.
		 *
		 *  This is an alias for compatibility with earlier versions.
		 */
#define OG_HAND_CURSOR            OG_RESIZE_EW_CURSOR
		 /*! @brief Legacy name for compatibility.
		 *
		 *  This is an alias for compatibility with earlier versions.
		 */
#define OG_HRESIZE_CURSOR         OG_RESIZE_NS_CURSOR
		 /*! @brief Legacy name for compatibility.
		 *
		 *  This is an alias for compatibility with earlier versions.
		 */
#define OG_VRESIZE_CURSOR         OG_POINTING_HAND_CURSOR


	typedef struct OgNativeWindow OGNativeWindow;

	typedef struct OgInput OGInput;

	typedef void(*OgOnKeyCallback)(OgNativeWindow* window, int keyCode, int scanCode, int action, int modifierKeys);

	typedef void(*OgOnCharacterInput)(OgNativeWindow* window, unsigned int codepoint);

	typedef void(*OgOnCharacterModsInput)(OgNativeWindow* window, unsigned int codepoint, int modifierKeys);

	typedef void(*OgOnMouseButton)(OgNativeWindow* window, int button, int action, int modifierKeys);

	typedef void(*OgOnCursorPos)(OgNativeWindow* window, double xpos, double ypos);

	OG_API const char* OG_get_character_string(unsigned int codepoint);

	struct OgInput
	{
		char pointers[OG_MOUSE_BUTTON_LAST + 1]; // the last pointer event saved here
		char keys[OG_KEY_LAST + 1];	 // the last key event saved here

		// Mouse State in this frame. This will be clear every frame by OG_window_event_poll
		// The clear value is -1 (0xff) 
		char currentPointers[OG_MOUSE_BUTTON_LAST + 1];

		// Key State in this frame. This will be clear every frame by OG_window_event_poll
		// The clear value is -1 (0xff) 
		char currentKeys[OG_KEY_LAST + 1];

		// window cursor position which starts at upper-left on the window
		double virtualCursorPosX, virtualCursorPosY;

		struct Wheel
		{
			double x, y;
		} wheel;

		OgOnKeyCallback onKey;
		OgOnCharacterInput onCharacterInput;
		OgOnCharacterModsInput onCharacterModsInput;
		OgOnMouseButton onMouseButton;
		OgOnCursorPos onCursorPos;
	};

#ifdef __cplusplus
}
#endif

#endif
