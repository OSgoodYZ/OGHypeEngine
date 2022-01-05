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

void og_path_utf8_to_system(void* dest_sys, const char* src_utf8, bool need_safe_conversion)
{
	char src[OG_CHAR_INIT_LENGTH] = { 0, };
	LPWSTR dest = (wchar_t*)dest_sys;

	og_path_to_win(src, src_utf8);
	int size = MultiByteToWideChar(CP_UTF8, 0, src, -1, NULL, 0);

	if (size > 0)
		MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, src, -1, dest, size + og_path_utf8_extra_length(src_utf8));

	if (need_safe_conversion)
	{
		bool isANSI = false;
		size_t destLen = wcslen(dest);
		for (size_t i = 0; i < destLen; ++i)
		{
			WCHAR w = dest[i];
			if (w == 0xFFFD)
			{
				isANSI = true;
				break;
			}
		}
		if (isANSI)
		{
			int size = MultiByteToWideChar(CP_ACP, 0, src, -1, NULL, 0);

			if (size > 0)
				MultiByteToWideChar(CP_ACP, MB_ERR_INVALID_CHARS, src, -1, dest, size + og_path_utf8_extra_length(src_utf8));
		}
	}
}
void og_path_system_to_utf8(char* dest_utf8, const void* src_sys)
{
	LPCWSTR inStr = (const wchar_t*)src_sys;
	wchar_t buf[OG_CHAR_INIT_LENGTH] = { 0, };

	NormalizeString(NormalizationC, inStr, -1, buf, OG_CHAR_INIT_LENGTH);
	int size = WideCharToMultiByte(CP_UTF8, WC_NO_BEST_FIT_CHARS, buf, -1, NULL, NULL, NULL, NULL);

	if (size > 0)
		WideCharToMultiByte(CP_UTF8, WC_NO_BEST_FIT_CHARS, buf, -1, dest_utf8, size, NULL, NULL);
}

// TODO