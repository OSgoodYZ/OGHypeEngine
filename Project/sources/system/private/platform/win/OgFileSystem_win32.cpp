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

char s_executablePath[OG_CHAR_INIT_LONG_LENGTH] = { 0, };
const char* og_path_executable_current()
{
	if (0 == strcmp("", s_executablePath))
	{
		GetModuleFileNameA(NULL, s_executablePath, MAX_PATH);
	}
	return s_executablePath;
}

// TODO