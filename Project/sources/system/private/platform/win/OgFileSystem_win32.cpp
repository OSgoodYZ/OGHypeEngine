#include <windows.h>
#include <fileapi.h>
#include <iterator>
#include <Lmcons.h>
#include <shlobj_core.h>
#include <string>

#include "system/OgVector.h"
#include "system/OgFileSystem.h"

#if defined(__SAFETY__)
#include <codecvt>
#include <locale>
#endif

using namespace Og;

#if !defined(DIRECTORY_SEPARATOR_WCHAR)
#define DIRECTORY_SEPARATOR_WCHAR L'\\'
#endif

// TODO