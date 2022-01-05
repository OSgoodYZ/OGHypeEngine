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


std::string og_path_parent(const char* path)
{
	std::string result("");
	const char ds[2] = { OG_DIRECTORY_SEPARATOR_CHAR, 0 };
	char conv[OG_CHAR_INIT_LENGTH] = { 0 };

	og_path_to_system(conv, path);
	
	const char* ptr = strstr(conv, ds);
	//strcpy(conv, (const char*)(ptr - conv));

	if (ptr == nullptr) 
		return result;
	else
	{
		result.append(conv, 0, (int)(ptr - conv));
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

int og_path_utf8_extra_length(const char* src_utf8)
{
	int added = 0;
	size_t srcLen = strlen(src_utf8);
	for (size_t i = 0; i < srcLen; ++i)
	{
		if ((int)src_utf8[i] < 0)
		{
			int charI = (uint)src_utf8[i] % 0x100;
			int addedBytes = 0;

			//https://en.wikipedia.org/wiki/UTF-8#Description
			//flags is Byte1 checker
			int flags = ((charI & 0x40) >> 4) + ((charI & 0x20) >> 4) + ((charI & 0x10) >> 4);

			if (flags & 0b0100)
			{
				if (flags & 0b0010)
				{
					if (flags & 1)
						addedBytes = 3;
					else
						addedBytes = 2;
				}
				else
					addedBytes = 1;
			}

			added += addedBytes;
			i += addedBytes;
		}
	}
	return added;
}


#pragma region >> OgFileSystem

OG_NAMESPACE_SYSTEM_BEGIN

OgFileStream::OgFileStream(FILE * file)
	: _file(file), _position(0), _length(0)
{
	if (_file == NULL)
	{
		int error_code = errno;
		LOGE(OG_ID, "Code = %i : %s", error_code, og_print_errno(error_code));
	}

	fseek(_file, 0, SEEK_END);
	_length = ftell(_file);
	fseek(_file, 0, SEEK_SET);
}

OgFileStream::OgFileStream(FILE* file, OgFileMode mode)
	: _file(file), mode(mode), _position(0), _length(0)
{
	if (_file == NULL)
	{
		int error_code = errno;
		LOGE(OG_ID, "Code = %i : %s", error_code, og_print_errno(error_code));
	}

	fseek(_file, 0, SEEK_END);
	_length = ftell(_file);
	fseek(_file, 0, SEEK_SET);
}

OgFileStream::OgFileStream(const char* path, OgFileMode mode)
	: _file(nullptr), mode(mode), _position(0), _length(0)
{
	switch (mode)
	{
	case OgFileMode::APPEND:
		_file = og_file_open(path, "ab");
		break;
	case OgFileMode::CREATE:
		_file = og_file_open(path, "wb");
		break;
	case OgFileMode::NEW:
		if (og_file_exist(path))
			LOGE(OG_ID, "%s file exists", path);
		_file = og_file_open(path, "w+b");
		break;
	case OgFileMode::OPEN:
		_file = og_file_open(path, "rb");
		break;
	case OgFileMode::OPEN_CREATE:
		_file = og_file_open(path, "w+b");
		break;
	case OgFileMode::TRUNCATE:
		_file = og_file_open(path, "w+b");
		break;
	}

	if (_file == NULL)
	{
		int error_code = errno;
		LOGE(OG_ID, "Code = %i : %s", error_code, og_print_errno(error_code));
	}

	fseek(_file, 0, SEEK_END);
	_length = ftell(_file);
	fseek(_file, 0, SEEK_SET);
}

OgFileStream::OgFileStream(const OgFileStream & stream)
	: _file(stream._file), _position(stream._position), mode(stream.mode), _length(stream._length)
{
}

OgFileStream::~OgFileStream()
{
	_file = nullptr;
}

void OgFileStream::WriteRaw(const void * ptr, size_t size)
{
	// TODO : fwrite left over using while loop
	fwrite(ptr, size, 1, _file);
	_position += size;
	_length = OG_MAX(_position, _length + 1);
}

void OgFileStream::WriteChar(const char * c)
{
	// TODO : fwrite left over using while loop
	size_t s = og_strlen(c);
	fwrite(c, s, 1, _file);
	_position += s;
	_length = OG_MAX(_position, _length + 1);
}

void OgFileStream::WriteWChar(const wchar_t* c)
{
	// TODO : fwrite left over using while loop
	size_t s = og_strlen(c) * sizeof(wchar_t);
	fwrite(c, s, 1, _file);
	_position += s;
	_length = OG_MAX(_position, _length + 1);
}

void OgFileStream::ReadRaw(void* ptr, size_t size)
{
	uint8* movePointer = (uint8*)ptr;
	size_t leftOverReadSize = size;
	size_t currentReadSize = 0;
	size_t totalReadSize = 0;
	while (totalReadSize < size)
	{
		currentReadSize = fread(movePointer, 1, leftOverReadSize, _file);
		totalReadSize += currentReadSize;
		if (totalReadSize >= size) break;

		movePointer += currentReadSize;
		leftOverReadSize -= currentReadSize;

		if (feof(_file) || currentReadSize <= 0)
		{
			// Fail to read for some problems
			break;
		}
	}

	_position += totalReadSize;

	if (totalReadSize != size)
	{
		int error_code = errno;
		if (error_code != 0)
			LOGE(OG_ID, "File read failed Code = %i : %s", error_code, og_print_errno(error_code));
	}
}

int64 OgFileStream::GetPosition()
{
	if (fgetpos(_file, &_position) != 0)
	{
		int error_code = errno;
		if (error_code != 0)
			LOGE(OG_ID, "File getpos failed Code = %i : %s", error_code, og_print_errno(error_code));
	}
	return _position;
}


void OgFileStream::SetPosition(int64 pos)
{
	_position = pos;

	if (fsetpos(_file, &_position) != 0)
	{
		int error_code = errno;
		if (error_code != 0)
			LOGE(OG_ID, "File setpos failed Code = %i : %s", error_code, og_print_errno(error_code));
	}
}

size_t OgFileStream::Length() const
{
	return _length;
}

void OgFileStream::Flush()
{
	fflush(_file);
}

void OgFileStream::Close()
{
	fclose(_file);
}

OG_NAMESPACE_SYSTEM_END
#pragma endregion