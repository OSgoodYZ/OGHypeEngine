#pragma warning(disable:4595)
#pragma once

#ifndef _OG_MEMORY_H_
#define _OG_MEMORY_H_

#include "OgPrecompile.h"

// TODO
//#include "OgDynamicLib.h"
#include <stdlib.h>

#if defined(__DESKTOP__) && defined(__WIN32__)
#if _DEBUG && _MSC_VER >= 1700 // Visual Studio Versio more than 2012
#define OG_USE_CRT_CHASE_MEMORY_LEAK
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif // DEBUG && MSC_VER >= 1700
#endif // DESKTOP && WIN32

#ifdef __cplusplus
extern "C"
{
#endif

#if defined(OG_USE_CRT_CHASE_MEMORY_LEAK)

#define og_malloc(s) _malloc_dbg(s, _NORMAL_BLOCK, __FILE__, __LINE__)
#define og_realloc(p, s) _realloc_dbg(p, s, _NORMAL_BLOCK, __FILE__, __LINE__)
#define og_calloc(c, s) _calloc_dbg(c, s, _NORMAL_BLOCK, __FILE__, __LINE__)
#define og_free(p) _free_dbg(p, _NORMAL_BLOCK)
#define og_aligned_alloc(s, a) _aligned_malloc_dbg(s, a, __FILE__, __LINE__)
#define og_aligned_free(p) _aligned_free_dbg(p)                                                          
#define og_aligned_realloc(p, a, s) _aligned_realloc_dbg(p, s, a, __FILE__, __LINE__)

#elif defined(OG_USE_JEMALLOC)

	OG_API void* og_malloc(size_t size);
	OG_API void* og_realloc(void * dst, size_t size);
	OG_API void* og_calloc(size_t bufferCount, size_t size);
	OG_API void* og_aligned_alloc(size_t size, size_t alignment);
	OG_API void* og_aligned_realloc(void *ptr, size_t alignment, size_t new_size);
	OG_API void  og_aligned_free(void * pointer);

#else

#define og_malloc(s) malloc(s) 
#define og_realloc(p, s) realloc(p, s)
#define og_calloc(b, s) calloc(b, s)
#define og_free(p) free(p)

	OG_API void* og_aligned_alloc(size_t size, size_t alignment);
	OG_API void* og_aligned_realloc(void *ptr, size_t alignment, size_t new_size);
	OG_API void  og_aligned_free(void * pointer);

#endif

	OG_API void og_memory_init();
	OG_API void og_memory_release();
	OG_API void og_memory_status();

	OG_API void og_memory_print(unsigned char* mem, size_t total, size_t align);

#ifdef __cplusplus
}
#endif // __cpp


// CPP new/delete wrapping

#ifdef __cplusplus

#include <new>
#include <iostream>
#include <iomanip>
#include <exception>
#include <cstdlib>

// Placement New (POOL)
// https://stackoverflow.com/questions/222557/what-uses-are-there-for-placement-new
// struct NewPlaceholder {};

// https://github.com/jemalloc/jemalloc/blob/d3d7a8ef09b6fa79109e8930aaba7a677f8b24ac/src/jemalloc_cpp.cpp

OG_FORCEINLINE void * operator new(std::size_t n)
#if !defined(_MSC_VER)
throw(std::bad_alloc)
#endif
{
	//std::printf("global op new called, size = %zu\n", n);
	void *ptr = og_malloc(n);
	if (ptr)
	{
		return ptr;
	}
#if defined(__cpp_exceptions)
	throw std::bad_alloc{};
#else
	LOGE(LV_ID, "bad_alloc");
#endif
}

OG_FORCEINLINE void * operator new[](std::size_t s)
#if !defined(_MSC_VER)
throw(std::bad_alloc)
#endif
{
	//std::printf("global op new called, size = %zu\n", s);
	void *ptr = og_malloc(s);
	if (ptr)
	{
		return ptr;
	}
	// TODO : move to LvPrecompiled.h
#if defined(__cpp_exceptions)
	throw std::bad_alloc{};
#else
	LOGE(LV_ID, "bad_alloc");
#endif	
}

OG_FORCEINLINE void operator delete(void * p)
#if !defined(_MSC_VER)
throw()
#endif
{
	og_free(p);
}

OG_FORCEINLINE void operator delete[](void *p)
#if !defined(_MSC_VER)
throw()
#endif
{
	og_free(p);
}

#if defined(OG_USE_CRT_CHASE_MEMORY_LEAK)
#define DBG_NEW new (_NORMAL_BLOCK, __FILE__, __LINE__)
//#define new DBG_NEW
#endif

#endif


#endif //_OG_MEMORY_H_
