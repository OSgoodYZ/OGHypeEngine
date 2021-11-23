#include "system/OgMemory.h"

// Issue
// http://devgit.com2us.com/TS/TPact/issues/39
// Fundamental
// https://developer.apple.com/library/archive/documentation/Darwin/Conceptual/KernelProgramming/vm/vm.html
// Jemalloc Manual
// http://jemalloc.net/jemalloc.3.html

#include <errno.h>

void og_memory_init() { }

void og_memory_release() { }

void og_memory_status() { }

void og_memory_print(unsigned char* mem, size_t total, size_t align)
{
	unsigned char* e = &mem[total];

	for (int i = 0; &mem[i] != e; ++i)
	{
		if (i % align == 0)
		{
			printf("0x%p | [", &mem[i]);
		}

		printf(" %*u ", 3, mem[i]);

		if ((i + 1) % align == 0)
		{
			printf("%c", ']');
			if (i < total - 1)
				printf("\n");
		}
	}
}
