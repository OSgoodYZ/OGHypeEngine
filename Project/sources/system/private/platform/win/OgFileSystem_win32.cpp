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
using namespace Og::System;


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

std::string og_path_normalize(const char* path)
{
	std::string s;
	// TODO
	return std::move(s);
}

OG_API Og::System::OgFileHandle og_file_create(const char* dest)
{
	char conv[OG_CHAR_INIT_LENGTH] = { 0 };
	og_path_to_system(conv, dest);

	wchar_t wc[OG_CHAR_INIT_LENGTH];
	og_path_utf8_to_system((void*)wc, conv, true);

	OgFileHandle s;
	s.nativeHandle = CreateFile(wc, GENERIC_READ | GENERIC_WRITE, 0, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (s.nativeHandle == OG_INVALID_HANDLE)
	{
		const int error_code = GetLastError();
		OG_THROW("lv_file_create Code = %i : %s (%s)", error_code, og_print_errno(error_code), dest);
	}
	return s;

	
}

Og::System::OgFileHandle og_file_open(const char* path, Og::System::OgFileAccess accessFlag, Og::System::OgFileMode modeFlag)
{
	char conv[OG_CHAR_INIT_LENGTH] = { 0 };
	og_path_to_system(conv, path);

	wchar_t wc[OG_CHAR_INIT_LENGTH];
	og_path_utf8_to_system((void*)wc, conv, true);

	DWORD acc = 0;
	DWORD share = 0;
	DWORD disp = 0;
	constexpr DWORD attrs = FILE_ATTRIBUTE_NORMAL;


	switch (accessFlag)
	{
	case OgFileAccess::READ_ONLY:
		acc = GENERIC_READ;
		share = FILE_SHARE_READ | FILE_SHARE_WRITE;
		break;
	case OgFileAccess::WRITE_ONLY:
		acc = GENERIC_WRITE;
		share = FILE_SHARE_READ;
		break;
	case OgFileAccess::READ_WRITE:
		acc = GENERIC_READ | GENERIC_WRITE;
		share = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
		break;
	}

	switch (modeFlag)
	{
	case OgFileMode::APPEND:
		acc |= FILE_APPEND_DATA;
		disp = OPEN_ALWAYS;
		break;
	case OgFileMode::CREATE:
		disp = CREATE_ALWAYS;
		break;
	case OgFileMode::NEW:
		disp = CREATE_NEW;
		break;
	case OgFileMode::OPEN_CREATE:
		disp = OPEN_ALWAYS;
		break;
	case OgFileMode::OPEN:
		disp = OPEN_EXISTING;
		break;
	case OgFileMode::TRUNCATE:
		disp = TRUNCATE_EXISTING;
		break;
	}

	OgFileHandle s;
	s.nativeHandle = CreateFile(wc, acc, share, nullptr, disp,
		attrs, nullptr);

	if (s.nativeHandle == OG_INVALID_HANDLE)
	{
		const DWORD errorCode = GetLastError();
		OG_THROW("lv_file_open Code = %lu : %s (%s)", errorCode, og_print_errno(errorCode), path);
	}
	return s;
}


bool og_file_close(OgFileHandle handle)
{
	return CloseHandle(handle.nativeHandle);
}

int64 og_file_read(OgFileHandle file, void* buffer, size_t nbytes)
{
	DWORD readBytes = 0;
	return ReadFile(file.nativeHandle, buffer, nbytes, &readBytes, nullptr) ? static_cast<int64>(readBytes) : -1;
}

int64 og_file_write(OgFileHandle file, const void* buffer, size_t nbytes)
{
	DWORD writeBytes = 0;
	return WriteFile(file.nativeHandle, buffer, nbytes, &writeBytes, nullptr) ? static_cast<int64>(writeBytes) : -1;
}

OG_API bool og_file_seek(Og::System::OgFileHandle file, int64 offset, Og::System::OgSeekMode mode)
{
	LARGE_INTEGER largeOffset;
	largeOffset.QuadPart = offset;
	DWORD method = 0;
	switch (mode)
	{
	case OgSeekMode::BEGIN: method = FILE_BEGIN; break;
	case OgSeekMode::CURRENT: method = FILE_CURRENT; break;
	case OgSeekMode::END: method = FILE_END; break;
	}

	return SetFilePointerEx(file.nativeHandle, largeOffset, nullptr, method);
}


OG_API bool og_file_get_pos(Og::System::OgFileHandle file, int64& pos)
{
	LARGE_INTEGER largeOffset;
	largeOffset.QuadPart = 0;
	LARGE_INTEGER newOffset;
	if (SetFilePointerEx(file.nativeHandle, largeOffset, &newOffset, FILE_CURRENT))
	{
		pos = newOffset.QuadPart;
		return true;
	}

	return false;
}


OG_API bool og_file_set_pos(Og::System::OgFileHandle file, const int64 pos)
{
	LARGE_INTEGER largeOffset;
	largeOffset.QuadPart = pos;

	return SetFilePointerEx(file.nativeHandle, largeOffset, nullptr, FILE_BEGIN);
}


OG_API bool og_file_flush(Og::System::OgFileHandle file)
{
	return FlushFileBuffers(file.nativeHandle);
}

OG_API bool og_file_eof(OgFileHandle file)
{
	LARGE_INTEGER zero, pos, size;
	zero.QuadPart = 0;
	if (!SetFilePointerEx(file.nativeHandle, zero, &pos, FILE_CURRENT))
	{
		OG_THROW("Error!");
	}
	if (!GetFileSizeEx(file.nativeHandle, &size))
	{
		OG_THROW("Error!");
	}

	return (pos.QuadPart >= size.QuadPart);
}
