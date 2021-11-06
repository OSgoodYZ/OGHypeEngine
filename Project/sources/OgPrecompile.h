#pragma warning(disable : 4819)
#pragma warning(disable : 4251)

#pragma once
#ifndef __OG_PRECOMPILED_H_
#define __OG_PRECOMPILED_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
/************************************************************************************
 * Copyright (C) 2021 by osgood												*
 *																					*
 * Platform Macro 							    									*
 * 																					*
 ************************************************************************************/

#define OG_ID "osgoodProject"

typedef char int8;
typedef unsigned char uint8;
typedef short int16;
typedef int int32;
typedef unsigned short uint16;
typedef unsigned int uint32;
typedef unsigned int uint;
typedef unsigned long ulong;

 /* detect x86 32 bit platform */
#if defined(__i386__) || defined(_M_IX86)
	#if !defined(__X86__)
		#define __X86__
		#define __PLATFORM32__
	#endif

	#if !defined(__X86_32__)
		#define __X86_32_
		#define __PLATFORM32__
	#endif
#endif

/* detect x86 64 bit platform */
#if defined(__x86_64__) || defined(__ia64__) || defined(_M_X64)
	#if !defined(__X86__)
		#define __X86__
		#define __PLATFORM64__
	#endif

	#if !defined(__X86_64__)
		#define __X86_64__
		#define __PLATFORM64__
	#endif
#endif

/* detect ARM platform */
#if defined(__arm__) || defined(__arm64__) || defined(_M_ARM) || defined(__aarch64__)
#if !defined(__LINUX__)
#define __ARM__
#define __PLATFORM32__
#endif

#if defined(__arm64__) || defined(__aarch64__)
#define __ARM64__
#define __PLATFORM64__
#endif
#endif

/* detect Linux platform */
#if defined(linux) || defined(__linux__) || defined(__LINUX__)
#if !defined(__LINUX__)
#define __LINUX__
#endif
#if !defined(__UNIX__)
#define __UNIX__
#endif
#endif

/* detect Windows 95/98/NT/2000/XP/Vista/7 platform */
#if (defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)) && !defined(__CYGWIN__)
#if !defined(__WIN32__)
#define __WIN32__
#endif
#endif

/* detect Cygwin platform */
#if defined(__CYGWIN__)
#if !defined(__UNIX__)
#define __UNIX__
#endif
#endif

/* detect MacOS X & iOS/iOS simulator platform */
#if defined(__APPLE__)
#define __ASSERT_MACROS_DEFINE_VERSIONS_WITHOUT_UNDERSCORES 0 // conflict with boost_1_58_0 ?!?
#include <TargetConditionals.h>
#if TARGET_IPHONE_SIMULATOR == 1
#if !defined(__IOS_SIMULATOR__)
#define __IOS_SIMULATOR__
#define __IOS__
#endif
#elif TARGET_OS_IPHONE == 1
#if !defined(__IOS__)
#define __IOS__
#pragma message "[[ Device ]] IOS"
#endif
#elif TARGET_OS_MAC == 1 || defined(__OSX__)
#pragma message "[[ Device ]] OSX"
#define __MACOSX__
#if !defined(__MACOSX__)
#define __MACOSX__
#endif
#endif

#if !defined(__UNIX__)
#define __UNIX__
#endif
#endif

/* detect Android platform */
#if defined(ANDROID) || defined(__ANDROID__)
	#include <android/log.h>
	#if !defined(__ANDROID__)
		#pragma message "[[ Device ]] ANDROID"
		#define __ANDROID__
	#endif
#endif

/* try to detect other Unix systems */
#if defined(__unix__) || defined (unix) || defined(__unix) || defined(_unix)
	#if !defined(__UNIX__)
		#define __UNIX__
	#endif
#endif

#if defined(__MACOSX__) || defined(__WIN32__) || defined(__LINUX__)
#if !defined(__DESKTOP__)
#define __DESKTOP__
#endif
#endif

#if defined(__ANDROID__) || defined(__IOS__)
#if !defined(__MOBILE__)
#define __MOBILE__
#undef __DESKTOP__
#endif
#endif


#ifndef OG_API
	#if defined(OG_API_EXPORT)
		#if defined(_WIN32) || defined(__CYGWIN__) || defined(__MINGW__)
			#if defined(__GNUC__)
				#define OG_API __attribute__ ((dllexport))
			#else
				#define OG_API __declspec(dllexport)
			#endif
		#elif defined(__GNUC__) || defined(__clang__)
			#define OG_API __attribute__ ((visibility ("default")))
		#else
			#define OG_API
		#endif
	#elif defined(OG_API_IMPORT)
		#if defined(__GNUC__)
			#define OG_API __attribute__ ((dllimport))
		#else
			#define OG_API __declspec(dllimport)
		#endif
	#else
		#define OG_API
	#endif
#endif

#if defined(__DESKTOP__)
#if defined(__WIN32__)
#ifndef WIN32_LEAN_AND_MEAN
	// for Winsock2 Library https://docs.microsoft.com/en-us/windows/win32/winsock/creating-a-basic-winsock-application
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#elif defined(__MACOSX__)
#include <dlfcn.h>
#endif
#endif


#if !defined(__cplusplus) && !defined(__clang__) || defined(__GNUC__)
#if !defined(bool)
//typedef enum { false, true } bool;
#include <stdbool.h>
#endif
#endif


#if defined(__GNUC__) || defined(__clang__)
#define OG_DEPRECATED __attribute__((deprecated))
#define OG_NOVTABLE
#elif defined(_MSC_VER)
#define OG_DEPRECATED __declspec(deprecated)
#define OG_NOVTABLE __declspec(novtable)
#else
#pragma message("WARNING: You need to implement DEPRECATED for this compiler")
#define OG_DEPRECATED
#define OG_NOVTABLE
#endif

#define OG_STRINGIFY(X) #X

#define _Args(...) __VA_ARGS__
#define _STRIP_PARENS(X) X
#define OG_PASS_PARAMETERS(X) _STRIP_PARENS( _Args X )

#define OG_EXPAND(x) x
#define OG_CONCAT(arg1, arg2) OG_CONCAT1(arg1, arg2)
#define OG_CONCAT1(arg1, arg2) arg1##arg2

#define OG_FOR_EACH_0(what, x, ...) 

#define OG_FOR_EACH_1(what, x, ...)\
what(OG_PASS_PARAMETERS(x))

#define OG_FOR_EACH_2(what, x, ...)\
what(OG_PASS_PARAMETERS(x)) OG_FOR_EACH_1(what, __VA_ARGS__)

#define OG_FOR_EACH_3(what, x, ...)\
what(OG_PASS_PARAMETERS(x)) OG_FOR_EACH_2(what, __VA_ARGS__)

#define OG_FOR_EACH_4(what, x, ...)\
what(OG_PASS_PARAMETERS(x)) OG_FOR_EACH_3(what, __VA_ARGS__)

#define OG_FOR_EACH_5(what, x, ...)\
what(OG_PASS_PARAMETERS(x)) OG_FOR_EACH_4(what, __VA_ARGS__)

#define OG_FOR_EACH_6(what, x, ...)\
what(OG_PASS_PARAMETERS(x)) OG_FOR_EACH_5(what, __VA_ARGS__)

#define OG_FOR_EACH_7(what, x, ...)\
what(OG_PASS_PARAMETERS(x)) OG_FOR_EACH_6(what, __VA_ARGS__)

#define OG_FOR_EACH_8(what, x, ...)\
what(OG_PASS_PARAMETERS(x)) OG_FOR_EACH_7(what, __VA_ARGS__)

#if defined(_MSC_VER)
#define VA_ARGS(...) unused, __VA_ARGS__
#else
#define VA_ARGS(...) ,## __VA_ARGS__
#endif

#define __NARGS(_1, _2, _3, _4, _5, _6, _7, _8, N, ...) N
#define NARGS_1(...) OG_EXPAND(__NARGS(__VA_ARGS__, 7, 6, 5, 4, 3, 2, 1, 0))

#define OG_FOR_EACH_NARG(...) NARGS_1(VA_ARGS(__VA_ARGS__))

#define OG_HAS_COMMA(...) OG_EXPAND(__NARGS(__VA_ARGS__, 1, 1, 1, 1, 1, 1, 1, 0, 0))

#define OG_FOR_EACH(what, ...) \
OG_EXPAND(OG_CONCAT(OG_FOR_EACH_,OG_FOR_EACH_NARG(__VA_ARGS__))(what, __VA_ARGS__)) 

#define OG_COMPILE_VERSION __DATE__ " " __TIME__

typedef char int8;
typedef unsigned char uint8;
typedef short int16;
typedef int int32;
typedef unsigned short uint16;
typedef unsigned int uint32;
typedef unsigned int uint;
typedef unsigned long ulong;

#if defined(__WIN32__)
typedef long long int64;
typedef unsigned long long uint64;
#define OG_ALIGN_BEGIN(_align) __declspec(align(_align))
#define OG_ALIGN_END(_align)
#else
#ifdef __PLATFORM32__
typedef long int64;
typedef unsigned long uint64;
#else
typedef long long int64;
typedef unsigned long long uint64;
#endif
#define OG_ALIGN_BEGIN(_align)
#define OG_ALIGN_END(_align) __attribute__( (aligned(_align) ) )
#endif

#ifdef __PLATFORM64__
typedef uint64 uintptr;
#else
typedef uint32 uintptr;
#endif


// https://github.com/scottt/debugbreak
// https://stackoverflow.com/questions/173618/is-there-a-portable-equivalent-to-debugbreak-debugbreak
#if defined(_DEBUG)
#ifdef __WIN32__
#define OG_DEBUG_BREAK() __debugbreak()
#elif defined(__APPLE__) && defined(__aarch64__)
#define OG_DEBUG_BREAK()  __builtin_trap()
#elif defined(__arm__)
__attribute__((gnu_inline, always_inline))
__inline__ static void trap_instruction(void)
{
	/* See 'arm-linux-tdep.c' in GDB source,
	* 'eabi_linux_arm_le_breakpoint' */
	__asm__ volatile(".inst 0xe7f001f0");
	/* Known problem:
	* Same problem and workaround as Thumb mode */
}
#define OG_DEBUG_BREAK()  trap_instruction()
#else
#include <signal.h>
#define OG_DEBUG_BREAK()  raise(SIGTRAP)
#endif
#else
#define OG_DEBUG_BREAK() 
#endif

#if defined(__ANDROID__)
	#define LOGD(tag, fmt, ...) __android_log_print(ANDROID_LOG_DEBUG, tag, fmt "\n - %s (at %s:%d)", ##__VA_ARGS__, __FILE__, __FUNCTION__, __LINE__)
	#define LOGW(tag, fmt, ...) __android_log_print(ANDROID_LOG_WARN, tag, fmt "\n - %s (at %s:%d)", ##__VA_ARGS__, __FILE__, __FUNCTION__, __LINE__)
	#define LOGE(tag, fmt, ...) __android_log_print(ANDROID_LOG_ERROR, tag, fmt "\n - %s (at %s:%d)", ##__VA_ARGS__, __FILE__, __FUNCTION__, __LINE__)
#else
	#define LOGD(tag, fmt, ...) fprintf(stdout, ANSI_COLOR_RESET "[D " tag "] " fmt "\n  - %s (at %s:%d)\n" ANSI_COLOR_RESET, ##__VA_ARGS__, __FILE__, __FUNCTION__, __LINE__)

#if defined(__cplusplus)
	#if defined(_DEBUG)
		#define LOGE(tag, fmt, ...) fprintf(stderr, ANSI_COLOR_RED "[D " tag "] " fmt "\n  - %s (at %s:%d)\n", ##__VA_ARGS__, __FILE__, __FUNCTION__, __LINE__), OG_DEBUG_BREAK()
	#else
		#define LOGE(tag, fmt, ...) fprintf(stderr, ANSI_COLOR_RED "[D " tag "] " fmt "\n  - %s (at %s:%d)\n", ##__VA_ARGS__, __FILE__, __FUNCTION__, __LINE__)
	#endif
#else
	#if defined(_DEBUG)
			#define LOGE(tag, fmt, ...) fprintf(stderr, ANSI_COLOR_RED "[D " tag "] " fmt "\n  - %s (at %s:%d)\n", ##__VA_ARGS__, __FILE__, __FUNCTION__, __LINE__), OG_DEBUG_BREAK()
	#else
			#define LOGE(tag, fmt, ...) fprintf(stderr, ANSI_COLOR_RED "[D " tag "] " fmt "\n  - %s (at %s:%d)\n", ##__VA_ARGS__, __FILE__, __FUNCTION__, __LINE__)
	#endif
#endif
#endif

#if !defined(ASSERT)
#if !defined(_DEBUG)
#define ASSERT(x) { false ? (void)(x) : (void)0; }
#else
#define ASSERT(x) do { const volatile bool og_assert_b____ = !(x); if(og_assert_b____) { OG_DEBUG_BREAK(); } } while (false)
#endif
#endif

#if defined(_DEBUG)
#define OG_CHECK(x, fmt, ...) do { const volatile bool og_assert_b____ = !(x); if(og_assert_b____) { LOGE(OG_ID, fmt, ##__VA_ARGS__); } } while (false)
#elif defined(_RELEASE_INFO)
#define OG_CHECK(x, fmt, ...) { if(!(x)) LOGE(OG_ID, fmt, ##__VA_ARGS__); }
#elif !defined(_DEBUG) || defined(_NDEBUG) || defined(_RELEASE)
#define OG_CHECK(x, fmt, ...) 
#else
#error fail to detect mode
#endif

#ifdef __cplusplus
#if __cplusplus >= 201103L
#define STATIC_ASSERT(expr) \
			static_assert(expr, \
						"static assert failed:" \
						#expr)
#else

#ifdef __WIN32__
#define STATIC_ASSERT(expr, msg) static_assert(expr, msg)
#else
		// declare a template but only define the
		// true case (via specialization)
template<bool> class TStaticAssert;
template<> class TStaticAssert<true> {};

#define _ASSERT_GLUE(a, b) a ## b
#define ASSERT_GLUE(a, b) _ASSERT_GLUE(a, b)

#define STATIC_ASSERT(expr, fail) \
				enum {  ASSERT_GLUE(g_assert_fail_ " " #fail, __LINE__) = sizeof(TStaticAssert<!!(expr)>) }
#endif

#endif

#ifdef __WIN32__
static_assert(sizeof(uintptr) == sizeof(void*), "Incorrect Size of uintptr");
static_assert(sizeof(int64) == 8, "Incorrect Size of i64");
static_assert(sizeof(int32) == 4, "Incorrect Size of i32");
static_assert(sizeof(int16) == 2, "Incorrect Size of i16");
static_assert(sizeof(int8) == 1, "Incorrect Size of i8");
#endif

#endif


#if defined(__WIN32__)
#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"
#else
#define ANSI_COLOR_RED
#define ANSI_COLOR_GREEN
#define ANSI_COLOR_YELLOW
#define ANSI_COLOR_BLUE
#define ANSI_COLOR_MAGENTA
#define ANSI_COLOR_CYAN
#define ANSI_COLOR_RESET   
#endif


// TODO : https://github.com/therocode/smartenum/blob/master/smartenum.hpp
// https://github.com/seleznevae/smart_enum/blob/master/smart_enum.h

#define OG_DECLARE_ENUM(E, ...)											\
struct E {																\
		enum Enum { __VA_ARGS__ };										\
		static const Enum values[] = { __VA_ARGS__ };			\
		static const char* const names[] = { #__VA_ARGS__ };		\
		static const char* ToString(Enum v)) {							\
			for(int i=0;i<sizeof(values)/sizeof(value[0]);++i){			\
				if(values[i] == v)										\
					return names[i];									\
			}															\
		}																\
};																		\

#define OG_ALIGNOF(...) __alignof(__VA_ARGS__)

/* GCC version 2.96 required for branch prediction expectation. */
#if RS_GCC_VERSION > 29600
#define OG_EXPECT(expr, val) __builtin_expect(expr, val)
#else
#define OG_EXPECT(expr, val) (expr)
#endif

#define OG_LIKELY(expr) OG_EXPECT(expr, 1)
#define OG_UNLIKELY(expr) OG_EXPECT(expr, 0)


#if defined(_MSC_VER) // msvc
#ifndef FORCEINLINE
#if (_MSC_VER >= 1200)
#define FORCEINLINE __forceinline
#else
#define FORCEINLINE __inline
#endif
#endif
#else // gcc
#ifndef FORCEINLINE
#define FORCEINLINE inline __attribute__((always_inline))
#endif
#endif
#define OG_FORCEINLINE FORCEINLINE

// Utility Macro
#define OG_MAX(a, b) ((a) > (b) ? (a) : (b))
#define OG_MIN(a, b) ((a) > (b) ? (b) : (a))
#define OG_CLAMP(v, lo, hi) OG_MAX( OG_MIN(v, hi), lo)
#define OG_SWAP(x, y, T) do { T t = x; x = y; y = t; } while (0)

#define OG_CHAR_INIT_SHORT_LENGTH 256
#define OG_CHAR_INIT_LENGTH 512
#define OG_CHAR_INIT_LONG_LENGTH 2048

#if !defined(OG_DIRECTORY_SEPARATOR_CHAR)
#if defined(__WIN32__)	
#define OG_DIRECTORY_SEPARATOR_CHAR '\\'
#else 
#define OG_DIRECTORY_SEPARATOR_CHAR '/'
#endif
#endif

	#if defined(__cplusplus)
	
		#define OG_NAMESPACE_BEGIN namespace Og {
		#define OG_NAMESPACE_END } 
	
	#if defined(OG_SAMPLE_BUILD) 
		#define OG_NAMESPACE_SAMPLE_BEGIN namespace OG { namespace Sample {
		#define OG_NAMESPACE_SAMPLE_END } }  // OG::Render namespace End
	#endif

		#define OG_NAMESPACE_SYSTEM_BEGIN namespace OG { namespace System {
		#define OG_NAMESPACE_SYSTEM_END } }  // OG::Render namespace End

		#define OG_NAMESPACE_RENDER_BEGIN namespace OG { namespace Render {
		#define OG_NAMESPACE_RENDER_END } }  // OG::Render namespace End
		
		#define OG_NAMESPACE_ENGINE_BEGIN namespace OG { namespace Engine {
		#define OG_NAMESPACE_ENGINE_END } }  // OG::Engine namespace End
		
		#define OG_NAMESPACE_APPLICATION_BEGIN namespace OG { namespace Application {
		#define OG_NAMESPACE_APPLICATION_END } } // OG::Application namespace End
	

	#else
	
		#define OG_NAMESPACE_BEGIN
		#define OG_NAMESPACE_END
		
		#define OG_NAMESPACE_RENDER_BEGIN
		#define OG_NAMESPACE_RENDER_END
		
		#define OG_NAMESPACE_ENGINE_BEGIN
		#define OG_NAMESPACE_ENGINE_END
		
		#define OG_NAMESPACE_APPLICATION_BEGIN
		#define OG_NAMESPACE_APPLICATION_END
	
	#endif

	#define EXIT_SUCCESS 0
	#define EXIT_FAILURE 1

#endif // __OG_PRECOMPILED_H_
