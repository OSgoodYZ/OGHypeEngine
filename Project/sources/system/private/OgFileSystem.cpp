#include "OgPrecompile.h"
#include "system/OgFileSystem.h"

#include <iterator>
#include <errno.h>

#if defined(__WIN32__)
#include <direct.h> // _getcwd
#define getcwd _getcwd
#else
#include <unistd.h>
#endif

const char* og_print_errno(int error_code)
{
	switch (error_code)
	{
	case EPERM: return "Operation not permitted";
	case ENOENT: return "No such file or directory";
	case ESRCH: return "No such process";
	case EINTR: return "Interrupted system call";
	case EIO: return "Error I/O";
	case ENXIO: return "No such device or address";
	case E2BIG: return "Argument list too long";
	case ENOEXEC: return "Exec format error";
	case EBADF: return "Bad file number";
	case ECHILD: return "No child processes";
	case EAGAIN: return "Try again";
	case ENOMEM: return "Out of memory";
	case EACCES: return "Access Permission denied";
	case EFAULT: return "Bad address";
	case EBUSY: return "Block device required";
	case EEXIST: return "File exists";
	case EXDEV: return "Cross-device link";
	case ENODEV: return "No such device";
	case ENOTDIR: return "Not a directory";
	case EISDIR: return "Is a directory";
	case EINVAL: return "Invalid argument";
	case ENFILE: return "File table overflow";
	case EMFILE: return "Too many open files";
	case ENOTTY: return "Not a typewriter";
	case ETXTBSY: return "Text file busy";
	case EFBIG: return "File too large";
	case ENOSPC: return "No space left on device";
	case ESPIPE: return "Illegal seek";
	case EROFS: return "Read-only file system";
	case EMLINK: return "Too many links";
	case EPIPE: return "Broken pipe, The process cannot access the file because it is being used by another process.";
	case EDOM: return "Math argument out of domain of func";
	case ERANGE: return "Math result not representable";
	case EDEADLK: return "Resource deadlock would occur";
	case ENAMETOOLONG: return "File name too long";
	case ENOLCK: return "No record locks available";
	case ENOSYS: return "Function not implemented";
	case ENOTEMPTY: return "Directory not empty";
#if defined(__WIN32__)
	case ERROR_INVALID_PARAMETER: return "The parameter is incorrect";
#endif
	}
	return "Unknown error, you should check https://man7.org/linux/man-pages/man3/errno.3.html page";
}

void og_str_replace_opt(char* dest, char const * const src, char const * const pattern, char const * const replace)
{
	size_t const replen = strlen(replace);
	size_t const patlen = strlen(pattern);

	size_t patcnt = 0;
	const char * oriptr;
	const char * patloc;

	// find how many times the pattern occurs in the original string
	for (oriptr = src; (patloc = strstr(oriptr, pattern)); oriptr = patloc + patlen)
	{
		patcnt++;
	}

	{
		// allocate memory for the new string

		char * const returned = dest;

		if (returned != NULL)
		{
			// copy the original string, 
			// replacing all the instances of the pattern
			char * retptr = returned;
			for (oriptr = src; (patloc = strstr(oriptr, pattern)); oriptr = patloc + patlen)
			{
				size_t const skplen = patloc - oriptr;
				// copy the section until the occurence of the pattern
				strncpy(retptr, oriptr, skplen);
				retptr += skplen;
				// copy the replacement 
				strncpy(retptr, replace, replen);
				retptr += replen;
			}
			// copy the rest of the string.
			strcpy(retptr, oriptr);
		}
	}
}

void og_path_to_system(char* dest, const char* src)
{
	const char sper[2] = { OG_DIRECTORY_SEPARATOR_CHAR, 0 };
#if defined(__WIN32__)
	og_str_replace_opt(dest, src, "/", sper);
#else
	og_str_replace_opt(dest, src, "\\", sper);
#endif
}


OG_API bool og_path_check_extension(const char* checkString, const char* extension)
{
	int32 checkStrLen = static_cast<int32>(strlen(checkString)) - 1;
	int32 extLen = static_cast<int32>(strlen(checkString)) - 1;

	while (checkStrLen >= 0 && extLen >= 0)
	{
		if (checkString[checkStrLen] != extension[extLen])
		{
			return false;
		}

		--checkStrLen;
		--extLen;
	}

	return (extLen == -1);
}

OG_API std::string og_path_combine(const char* a, const char* b)
{

	int32 aLen = a ? static_cast<int32>(strlen(a)) : -1;
	int32 bLen = b ? static_cast<int32>(strlen(b)) : -1;
	const char* pa = aLen == -1 ? nullptr : a + aLen - 1;
	const char* pb = bLen == -1 ? nullptr : b + bLen - 1;

	while (aLen >= 0 && *pa == OG_DIRECTORY_SEPARATOR_CHAR)
	{
		--pa;
		--aLen;
	}

	while (bLen >= 0 && *pb == OG_DIRECTORY_SEPARATOR_CHAR)
	{
		--pb;
		--bLen;
	}

	size_t newCapacity = aLen + bLen + 2; // 1 : DIRECTORY_SEPARATOR_CHAR, 2 : null limiter

	
	std::string s;
	s.reserve(newCapacity);
	if (aLen >= 0)
	{
		s.append(a, 0, aLen);
		s.push_back(OG_DIRECTORY_SEPARATOR_CHAR);
		
	}

	if (bLen >= 0)
	{
		s.append(b, 0, bLen);
	}


#if defined(WIN32)
	size_t pos = 0;
	while ((pos = s.find("\\\\", pos)) != std::string::npos) {
		s.replace(pos, 2, "\\");
		pos += 1; // Move past the replaced backslash
	}
#else
	size_t pos = 0;
	while ((pos = s.find("//", pos)) != std::string::npos) {
		s.replace(pos, 2, "/");
		pos += 1; // Move past the replaced forward slash
	}
#endif

	return s;
}

std::string og_path_parent(const char* path)
{
	std::string result;
	const char ds = OG_DIRECTORY_SEPARATOR_CHAR;
	char conv[OG_CHAR_INIT_LENGTH] = { 0 };

	og_path_to_system(conv, path);

	const char* last_separator = strrchr(conv, ds);

	if (last_separator == nullptr || last_separator == conv)
	{
		// 구분자가 없거나 경로가 루트인 경우
		return result;
	}
	else
	{
		result.assign(conv, last_separator - conv);
		return result;
	}
}

std::string s_executableDirectoryPath;
const char* og_path_current()
{

	if (s_executableDirectoryPath.empty())
	{
		s_executableDirectoryPath = og_path_parent(og_path_executable_current());
	}
	return s_executableDirectoryPath.c_str();
}

void og_path_to_win(char* dest, const char* src)
{
	og_str_replace_opt(dest, src, "/", "\\");
}


void og_path_to_absolute(char* dest, const char* relative, const char* root)
{
	const char sper[2] = { OG_DIRECTORY_SEPARATOR_CHAR, 0 };
	// TODO
}

bool og_path_contains(const char* parent, const char* child)
{
	// TODO
	return false;
}

int og_path_utf8_extra_length(const char* src_utf8)
{
	int added = 0;
	size_t srcLen = strlen(src_utf8);
	for (size_t i = 0; i < srcLen; ++i)
	{
		if (static_cast<int>(src_utf8[i]) < 0)
		{
			int charI = static_cast<uint>(src_utf8[i]) % 0x100;
			int addedBytes = 0;

			//https://en.wikipedia.org/wiki/UTF-8#Description
			//flags is Byte1 checker
			int flags = ((charI & 0x40) >> 4) + ((charI & 0x20) >> 4) + ((charI & 0x10) >> 4);

			if (flags & 0b0100)
			{
				if (flags & 0b0010)
				{
					if (flags & 1)
					{
						addedBytes = 3;
					}
					else
					{
						addedBytes = 2;
					}
				}
				else
				{
					addedBytes = 1;
				}
				}

			added += addedBytes;
			i += addedBytes;
			}
		}
	return added;
}


#pragma region >> OgFileSystem

OG_NAMESPACE_SYSTEM_BEGIN


OgFileStream::OgFileStream(OgFileHandle file)
	: _position(0)
	, _length(0)
	, _file(file)
{
	if (file.nativeHandle == OG_INVALID_HANDLE)
	{
		const int errorCode = errno;
		OG_THROW("Code = %i : %s", errorCode, og_print_errno(errorCode));
	}

	og_file_seek(_file, 0, OgSeekMode::END);
	int64 length = 0;
	if (og_file_get_pos(_file, length))
	{
		_length = static_cast<size_t>(length);
	}

	og_file_seek(_file, 0, OgSeekMode::BEGIN);
}

OgFileStream::OgFileStream(const char* path, OgFileMode mode)
	: OgFileStream(og_file_open(path, OgFileAccess::READ_WRITE, mode))
{
	//_mode = mode;
}

OgFileStream::OgFileStream(OgFileStream&& o) noexcept
	: _position(o._position)
	, _length(o._length)
	, _file(o._file)
	//, _mode(o._mode)
{
	o._file.nativeHandle = OG_INVALID_HANDLE;
}

OgFileStream::~OgFileStream()
{
	if (_file.nativeHandle != OG_INVALID_HANDLE)
	{
		og_file_close(_file);
		_file.nativeHandle = OG_INVALID_HANDLE;
	}
}

void OgFileStream::WriteRaw(const void* ptr, size_t size)
{
	// TODO : fwrite left over using while loop
	og_file_write(_file, ptr, size);
	//fwrite(ptr, size, 1, _file.handle);
	_position += size;
	_length = OG_MAX(_position, static_cast<int64>(_length + 1));
}

void OgFileStream::WriteChar(const char* c)
{
	// TODO : fwrite left over using while loop
	const size_t s = strlen(c);
	og_file_write(_file, c, s);
	//fwrite(c, s, 1, _file.handle);
	_position += s;
	_length = OG_MAX(_position, static_cast<int64>(_length + 1));
}

void OgFileStream::WriteWChar(const wchar_t* c)
{
	// TODO : fwrite left over using while loop
	const size_t s = wcslen(c) * sizeof(wchar_t);
	og_file_write(_file, c, s);
	//fwrite(c, s, 1, _file.handle);
	_position += s;
	_length = OG_MAX(_position, static_cast<int64>(_length + 1));
}

void OgFileStream::ReadRaw(void* ptr, size_t size)
{
	if (size == 0) OG_THROW("size > 0 Size should not be zero");

	uint8* movePointer = static_cast<uint8*>(ptr);
	size_t leftOverReadSize = size;
	size_t currentReadSize = 0;
	size_t totalReadSize = 0;
	while (totalReadSize < size)
	{
		//currentReadSize = fread(movePointer, 1, leftOverReadSize, _file.handle);
		currentReadSize = og_file_read(_file, movePointer, leftOverReadSize);
		totalReadSize += currentReadSize;
		if (totalReadSize >= size) break;

		movePointer += currentReadSize;
		leftOverReadSize -= currentReadSize;

		if (/*feof(_file.handle)*/ og_file_eof(_file) || currentReadSize <= 0)
		{
			// Fail to read for some problems
			break;
		}
	}

	_position += totalReadSize;

	if (totalReadSize != size)
	{
		const int errorCode = errno;
		if (errorCode != 0)
		{
			OG_THROW("File read failed Code = %i : %s", errorCode, og_print_errno(errorCode));
		}
	}
}

int64 OgFileStream::GetPosition()
{
	int64 position = 0;
	//if (fgetpos(_file.handle, &_position) != 0)
	if (!og_file_get_pos(_file, position))
	{
		const int errorCode = errno;
		if (errorCode != 0)
		{
			OG_THROW("File getpos failed Code = %i : %s", errorCode, og_print_errno(errorCode));
		}
	}

	_position = static_cast<size_t>(position);
	return position;
}

void OgFileStream::SetPosition(int64 pos)
{
	_position = static_cast<size_t>(pos);

	//if (fsetpos(_file.handle, &_position) != 0)
	if (!og_file_set_pos(_file, pos))
	{
		const int errorCode = errno;
		if (errorCode != 0)
		{
			OG_THROW("File setpos failed Code = %i : %s", errorCode, og_print_errno(errorCode));
		}
	}
}

OgFileStream& OgFileStream::operator=(OgFileStream&& o) noexcept
{
	if (this != &o)
	{
		this->~OgFileStream();
		new (this) OgFileStream(std::move(o));
	}

	return *this;
}


size_t OgFileStream::Length() const
{
	return _length;
}

void OgFileStream::Flush() const
{
	og_file_flush(_file);
	//fflush(_file.handle);
}

void OgFileStream::Close()
{
	og_file_close(_file);
	_file.nativeHandle = OG_INVALID_HANDLE;
}


OG_NAMESPACE_SYSTEM_END
#pragma endregion