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



#endif // _OG_SYSTEM_CONTEXT_H_