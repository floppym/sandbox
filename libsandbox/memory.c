/* memory.c
 * Minimal mmap-based malloc/free implementation to be used by libsandbox
 * internal routines, since we can't trust the current process to have a
 * malloc/free implementation that is sane and available at all times.
 *
 * Copyright 1999-2008 Gentoo Foundation
 * Licensed under the GPL-2
 */

#include "headers.h"
#include "libsandbox.h"
#include "sbutil.h"

#define SB_MALLOC_TO_MMAP(ptr) ((void*)(((size_t*)ptr) - 1))
#define SB_MMAP_TO_MALLOC(ptr) ((void*)(((size_t*)ptr) + 1))
#define SB_MALLOC_TO_SIZE(ptr) (*((size_t*)SB_MALLOC_TO_MMAP(ptr)))

void *malloc(size_t size)
{
	size_t *ret;
	size += sizeof(size_t);
	ret = mmap(0, size, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
	if (ret == MAP_FAILED)
		return NULL;
	*ret = size;
	return SB_MMAP_TO_MALLOC(ret);
}

void free(void *ptr)
{
	if (ptr == NULL)
		return;
	if (munmap(SB_MALLOC_TO_MMAP(ptr), SB_MALLOC_TO_SIZE(ptr))) {
		SB_EERROR("sandbox memory corruption", " free(%p): %s\n",
			ptr, strerror(errno));
#ifdef HAVE_BACKTRACE
		void *funcs[10];
		int num_funcs;
		num_funcs = backtrace(funcs, ARRAY_SIZE(funcs));
		backtrace_symbols_fd(funcs, num_funcs, STDERR_FILENO);
#endif
	}
}

void *calloc(size_t nmemb, size_t size)
{
	return xzalloc(nmemb * size); /* dont care about overflow */
}

void *realloc(void *ptr, size_t size)
{
	void *ret;
	size_t old_malloc_size;

	if (ptr == NULL)
		return xmalloc(size);
	if (size == 0) {
		free(ptr);
		return ptr;
	}

	old_malloc_size = SB_MALLOC_TO_SIZE(ptr);
	ret = xmalloc(size);
	memcpy(ret, ptr, MIN(size, old_malloc_size));
	free(ptr);
	return ret;
}

char *strdup(const char *s)
{
	size_t len;
	char *ret;

	if (s == NULL)
		return NULL;

	len = strlen(s);
	ret = xmalloc(len + 1);
	return memcpy(ret, s, len + 1);
}
