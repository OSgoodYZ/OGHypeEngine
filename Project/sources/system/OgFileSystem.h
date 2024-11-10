#pragma once
#ifndef __OG_FILESYSTEM_H__
#define __OG_FILESYSTEM_H__

#include "system/OgVector.h"
#include "system/OgStream.h"
#include <string>
#include <fstream>


OG_NAMESPACE_SYSTEM_BEGIN

/**
* @file #include "system/LvFileSystem.h"
* @brief 플랫폼 독립적인 파일 핸들
*/
struct OG_API OgFileHandle final
{
#if defined(WIN32)
	using OgNativeHandleType = HANDLE;
#define OG_INVALID_HANDLE ((HANDLE)(LONG_PTR)-1)
#else
	using OgNativeHandleType = int;
#define OG_INVALID_HANDLE -1
#endif

	OgNativeHandleType nativeHandle = OG_INVALID_HANDLE;
};

/**
* @file #include "system/LvFileSystem.h"
* @brief 매핑된 메모리에 어떤 모드로 접근할지에 대한 mmap관련 플래그.
*/
enum class ogMmapFileAccess :uint8
{
	READ_ONLY,	// 읽기모드로 접근
	READ_WRITE	// 쓰지모드로 접근
};

/**
* https://github.com/LMDB/lmdb/blob/mdb.master/libraries/liblmdb/mdb.c
* @file #include "system/LvFileSystem.h"
* @brief file Access 관련 플래그
*/
enum class OgFileAccess : uint8
{
	READ_ONLY,	// 읽기모드로 접근 (Read Only)
	WRITE_ONLY,	// 쓰기모드로 접근 (Write Only)
	READ_WRITE	// 읽기 쓰기모드로 접근 (Read Write)
};


/**
* @file #include "system/LvFileSystem.h"
* @brief file open mode 관련 플래그
* @details Windows: https://learn.microsoft.com/ko-kr/windows/win32/api/fileapi/nf-fileapi-createfilea
* @details linux[POSIX]: https://www.joinc.co.kr/w/man/2/open
* @details mac: https://developer.apple.com/library/archive/documentation/System/Conceptual/ManPages_iPhoneOS/man2/open.2.html
*/
enum class OgFileMode : uint8
{
	APPEND,			// 파일을 Append모드로 오픈, 파일이 없으면 새로 생성. [FILE_APPEND_DATA, OPEN_ \ O_APPEND]
	CREATE,			// 파일을 항상 생성한다. 이미 있는 경우에도 새로 생성. [CREATE_ALWAYS \ O_CREATE | O_TRUNC]
	NEW,			// 파일이 없을 경우에만 생성한다. 이미 있는 경우에는 에러. [CREATE_NEW \ O_CREATE | O_EXCL]
	OPEN_CREATE,	// 파일을 항상 연다. 파일이 없는 경우에는 생성한다. [OPEN_ALWAYS \  O_CREATE]
	OPEN,			// 파일이 있는 경우에만 연다. 없는 경우에는 에러. [OPEN_EXISTING \ normal]
	TRUNCATE,		// 파일이 있는 경우 사이즈를 0으로 만들어 연다. 없는 경우는 에러. [TRUNCATE_EXISTING \ O_TRUNC]
};


/**
* @file #include "system/LvFileSystem.h"
* @brief 파일의 커서의 기준
*/
enum class OgSeekMode : uint8
{
	BEGIN,		// 파일의 맨 앞
	CURRENT,	// 현재
	END			// 파일의 끝
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
	OgFileStream(OgFileHandle file);

	OgFileStream(const char* path, OgFileMode mode);

	OgFileStream(const OgFileStream& o) = delete;

	OgFileStream(OgFileStream&& o) noexcept;

	~OgFileStream() override;

	int64 GetPosition() override;

	void SetPosition(int64 pos) override;

	void WriteRaw(const void* ptr, size_t size) override;

	void ReadRaw(void* ptr, size_t size) override;

	void WriteChar(const char* c);

	void WriteWChar(const wchar_t* c);

	void Flush() const;

	void Close() override;

	size_t Length() const override;

	OgFileStream& operator=(const OgFileStream& o) = delete;

	OgFileStream& operator=(OgFileStream&& o) noexcept;

private:

	//fpos_t _position;
	size_t _position;

	size_t _length;

	OgFileHandle _file;

	//LvFileMode _mode;
};

OG_NAMESPACE_SYSTEM_END

/*
 * @file #include "system/LvFileSystem.h"
 */
OG_API const char* og_print_errno(int error_code);

OG_API const char* og_path_current();

OG_API const char* og_path_executable_current();


OG_API bool og_path_check_extension(const char* checkString, const char* extension);

/**
 * @brief 두개의 경로를 경로 구분자와 함께 합성합니다.
 * @param path 경로
 * @return 합성된 경로
 * @file #include "system/LvFileSystem.h"
 */
OG_API std::string og_path_combine(const char* a, const char* b);


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


/**
 * @brief 절대경로를 리턴합니다.
 * @param root 상대 경로 까지의 절대 경로
 * @param src 상대 경로
 * @param dest 변경 이후 경로
 * @file #include "system/LvFileSystem.h"
 */
OG_API void og_path_to_absolute(char* dest, const char* relative, const char* root);

/**
 * @brief 자식 패스가 포함하는지를 알 수 있습니다.
 * @param parent 부모 패스
 * @param child 자식 패스
 * @return LvString
 * @file #include "system/LvFileSystem.h"
 */

OG_API bool og_path_contains(const char* parent, const char* child);

/**
 * @brief 디렉토리를 UTF-8 (NFC)로 통일합니다.
 * @param 대상 경로 (Window : char*(ANSI), Mac : char*)
 * @return LvString
 * @file #include "system/LvFileSystem.h"
 */
OG_API std::string og_path_normalize(const char* path);

/**
 * @brief platform os 에 알맞게 경로를 변환합니다.
 * @param dest 변경 이후 경로
 * @param src 변경 이전 경로
 * @file #include "system/LvFileSystem.h"
 */
OG_API void og_path_to_system(char* dest, const char* src);
/**
 * @brief 파일을 생성합니다.
 * @param dest 생성경로
 * @return FILE 핸들. 실패하면 og_INVALID_HANDLE 리턴
 * @file #include "system/LvFileSystem.h"
 */
OG_API Og::System::OgFileHandle og_file_create(const char* dest);

/**
 * @brief 파일을 엽니다.
 * @param path 열어야할 경로
 * @param accessFlag 접근 권한 플래그
 * @param modeFlag 파일 모드 플래스
 * @param mask 파일생성시 접근제어권한 마스크비트(Windows에서는 사용X)
 * @return FILE 핸들. 실패하면 og_INVALID_HANDLE 리턴
 * @file #include "system/LvFileSystem.h"
 */
OG_API Og::System::OgFileHandle og_file_open(const char* path, Og::System::OgFileAccess accessFlag, Og::System::OgFileMode modeFlag);

/**
* @file #include "system/LvFileSystem.h"
* @brief 파일을 닫습니다.
* @param handle 열려있는 파일 핸들
*/
OG_API bool og_file_close(Og::System::OgFileHandle handle);

/**
* @file #include "system/LvFileSystem.h"
* @brief 파일로 부터 데이터를 읽습니다.
* @param file 읽을 파일핸들
* @param buffer 데이터를 읽어서 넣을 버퍼
* @param nbytes 읽어올 바이트 갯수
* @return 실패하면 -1, 성공하면 읽은 바이트수 리턴
*/
OG_API int64 og_file_read(Og::System::OgFileHandle file, void* buffer, size_t nbytes);

/**
* @file #include "system/LvFileSystem.h"
* @brief 파일에 데이터를 씁니다.
* @param file 쓸 파일핸들
* @param buffer 쓸 데이터를 넣은 버퍼
* @param nbytes 쓸 바이트 갯수
* @return 실패하면 -1, 성공하면 쓴 바이트 수 리턴
*/
OG_API int64 og_file_write(Og::System::OgFileHandle file, const void* buffer, size_t nbytes);


/**
* @brief 파일의 커서를 변경합니다.
* @param file 커서를 변경할 파일
* @param offset 기준점으로 부터의 거리
* @param mode 위치 기준점
* @return 성공하면 true 리턴
*/
OG_API bool og_file_seek(Og::System::OgFileHandle file, int64 offset, Og::System::OgSeekMode mode);

/**
* @brief 파일의 커서 위치를 가져옵니다.
* @param file 커서위치를 조회할 파일
* @param pos positon out값
* @return 성공하면 true 리턴
*/
OG_API bool og_file_get_pos(Og::System::OgFileHandle file, int64& pos);


/**
* @brief 파일의 커서 위치를 설정합니다.
* @param file 설정할 파일
* @param pos 설정할 위치.(음수일 경우 무시)
* @return 성공하면 true 리턴
*/
OG_API bool og_file_set_pos(Og::System::OgFileHandle file, const int64 pos);

/**
* @brief 버퍼에 존재하는 데이터를 flush한다
* @param file 설정할 파일
* @return 성공하면 true 리턴
*/
OG_API bool og_file_flush(Og::System::OgFileHandle file);

/**
* @brief 파일이 현재 EOF인지 확인합니다.
* @param file 설정할 파일
* @return 성공하면 true 리턴
*/
OG_API bool og_file_eof(Og::System::OgFileHandle file);

#endif // __OG_FILESYSTEM_H__

