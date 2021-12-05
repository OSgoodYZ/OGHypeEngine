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