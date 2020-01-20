#ifndef _DYNINSTTEST_UTIL_H_
#define _DYNINSTTEST_UTIL_H_

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#include "BPatch.h"

#ifndef __maybe_unused
#define __maybe_unused __attribute__((unused))
#endif

#ifndef __packed
#define __packed __attribute__((__packed__))
#endif

#ifndef likely
#define likely(x) __builtin_expect(!!(x), 1)
#endif

#ifndef unlikely
#define unlikely(x) __builtin_expect(!!(x), 0)
#endif

#define STUBPROFILE_DEBUG

#define LOG_ERROR(format, ...) \
		fprintf(stderr, "[ERROR] %s %d: " format "\n", \
						__FILE__, __LINE__, ##__VA_ARGS__);

#define LOG_INFO(format, ...) \
		fprintf(stdout, "[INFO] %s %d: " format "\n", \
						__FILE__, __LINE__, ##__VA_ARGS__);

#ifdef STUBPROFILE_DEBUG
#define LOG_DEBUG(format, ...) \
		fprintf(stderr, "[DEBUG] %s %d: " format "\n", \
						__FILE__, __LINE__, ##__VA_ARGS__);
#else
#define LOG_DEBUG(format, ...)
#endif


static inline void *zalloc(size_t size)
{
	void *ptr = NULL;

	ptr = malloc(size);
	if (ptr == NULL)
		return NULL;

	memset(ptr, 0, size);
	return ptr;
}

static inline unsigned int __roundup_2(unsigned int num)
{
	num--;

	num |= (num >> 1);
	num |= (num >> 2);
	num |= (num >> 4);
	num |= (num >> 8);
	num |= (num >> 16);
	num++;
	return num;
}

/* For the buffer size of strerror_r */
#define STRERR_BUFSIZE	128

#ifndef offsetof
#define offsetof(type, member) ((size_t) & ((type*)0)->member)
#endif

#ifndef container_of
#define container_of(ptr, type, member) ({ \
			const typeof(((type *)0)->member) *__mptr = (ptr); \
			(type*)((char*)__mptr - offsetof(type,member)); })
#endif

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x)/sizeof(x[0]))
#endif

#endif /* _DYNINSTTEST_UTIL_H_ */
