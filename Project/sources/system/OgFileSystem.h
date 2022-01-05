#pragma once
#ifndef __OG_FILESYSTEM_H__
#define __OG_FILESYSTEM_H__

#include "system/OgVector.h"
#include "system/OgStream.h"
#include <string>
#include <fstream>


OG_NAMESPACE_SYSTEM_BEGIN

/*
 * @file #include "system/LvFileSystem.h"
 */
enum class OgFileMode
{
	// 해당 파일이 있을 경우 파일을 열고 파일의 끝까지 검색하거나 새 파일을 만듭니다.
	APPEND,
	// 새 파일을 만들도록 지정합니다. 파일이 이미 있으면 덮어씁니다. 
	// 접근하는 순간 바로 filewatcher event modify 호출됨
	CREATE,
	// 새 파일을 만들도록 지정합니다. 없으면 에러.
	NEW,
	//  기존 파일을 열도록 지정합니다.
	OPEN,
	// 파일이 있으면 운영 체제에서 파일을 열고 그렇지 않으면 새 파일을 만들도록 지정합니다.
	// 접근하는 순간 바로 filewatcher event modify 호출됨
	OPEN_CREATE,
	// 파일을 열면서 파일의 내용을 모두 지웁니다.
	// 접근하는 순간 바로 filewatcher event modify 호출됨
	TRUNCATE
};

/*
 * @file #include "system/LvFileSystem.h"
 */
struct OG_API OgFileAttribute
{
	std::string name;

	size_t size;

	time_t lastModifedTime;

	time_t creationTime;
};

/*
 * @file #include "system/LvFileSystem.h"
 */
class OG_API OgFileStream : public OgStream
{

public:

	OgFileMode mode;
	OgFileStream() { }
	OgFileStream(FILE* file);
	OgFileStream(FILE* file, OgFileMode mode);
	OgFileStream(const char* path, OgFileMode mode);
	OgFileStream(const OgFileStream& stream);
	~OgFileStream();

	int64 GetPosition();

	void SetPosition(int64 pos);

	void WriteRaw(const void* ptr, size_t size);

	void ReadRaw(void* ptr, size_t size);

	void WriteChar(const char* c);

	void WriteWChar(const wchar_t* c);

	void Flush();

	void Close();

	size_t Length() const;

private:

	fpos_t _position;

	FILE* _file;

	size_t _length;

};

OG_NAMESPACE_SYSTEM_END

OG_API const char* og_path_current();

OG_API const char* og_path_executable_current();

OG_API std::string og_path_parent(const char* path);

/**
 * @brief window 경로로 변환합니다.
 * @param dest 변경 이후 경로
 * @param src 변경 이전 경로
 * @file #include "system/LvFileSystem.h"
 */
OG_API void og_path_to_win(char* dest, const char* src);

/**
 * @brief char(utf8) path에서 os system path로 변환합니다.
 * @param dest_sys 변경 이후 경로 (windows: wchar_t, mac: NSString)
 * @param src_utf8 변경 이전 경로
 * @param need_safe_conversion utf-8이 아닌 ANSI type까지 처리 (only for window)
 * @file #include "system/LvFileSystem.h"
 */
OG_API void og_path_utf8_to_system(void* dest_sys, const char* src_utf8, bool need_safe_conversion = false);


OG_API void og_path_system_to_utf8(char* dest_utf8, const void* src_sys);

/**
 * @brief char(utf8(nfc)) path에서 가변 byte로 인해 추가된 byte수를 계산합니다.
 * @param src_utf8 utf8 경로
 * @return 1byte보다 긴 가변 bytes 수
 * @file #include "system/LvFileSystem.h"
 */
OG_API int og_path_utf8_extra_length(const char* src_utf8);
#endif // __OG_FILESYSTEM_H__

