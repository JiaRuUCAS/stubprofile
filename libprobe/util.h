#ifndef _LIBPROBE_UTIL_H_
#define _LIBPROBE_UTIL_H_

#define _GNU_SOURCE

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
#include <time.h>
#include <sys/syscall.h>
#include <sys/resource.h>

#define PAGE_SIZE 4096

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

#ifndef is_dir_sep
#define is_dir_sep(c) ((c) == '/')
#endif

#if !defined(LIB_EXPORT)
#define LIB_EXPORT __attribute__((visibility ("default")))
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

#define zfree(x) \
		do {				\
			if (x) {		\
				free(x);	\
				x = NULL;	\
			}				\
		} while (0);

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

static inline ssize_t readn(int fd, void *buf, size_t n)
{
	size_t left = n;

	while (left) {
		ssize_t ret = 0;

		ret = read(fd, buf, left);
		if (ret < 0 && errno == EINTR)
			continue;
		if (ret <= 0)
			return ret;

		left -= ret;
		buf += ret;
	}
	return n;
}

/* For the buffer size of strerror_r */
#define STRERR_BUFSIZE	128

#define offsetof(type, member) ((size_t) & ((type*)0)->member)

#define container_of(ptr, type, member) ({ \
			typeof(((type *)0)->member) *__mptr = (ptr); \
			(type*)((char*)__mptr - offsetof(type,member)); })

#define ARRAY_SIZE(x) (sizeof(x)/sizeof(x[0]))

/********** LOG ***********/
enum {
	PROBE_LOG_ERROR = 0,
	PROBE_LOG_INFO,
	PROBE_LOG_WARN,
	PROBE_LOG_DEBUG,
	PROBE_LOG_NUM,
};

#define PROBE_LOGFILE "probe.log"

void probe_log(int pid, uint8_t level,
				const char *format, ...);

#define LOG_ERROR(tid, format, ...) \
		probe_log(tid, PROBE_LOG_ERROR, format, ##__VA_ARGS__)
#define LOG_INFO(tid, format, ...) \
		probe_log(tid, PROBE_LOG_INFO, format, ##__VA_ARGS__)
#define LOG_WARN(tid, format, ...) \
		probe_log(tid, PROBE_LOG_WARN, format, ##__VA_ARGS__)
#ifdef PROBE_DEBUG
#define LOG_DEBUG(tid, format, ...) \
		probe_log(tid, PROBE_LOG_DEBUG, format, ##__VA_ARGS__)
#else
#define LOG_DEBUG(tid, format, ...)
#endif /* ifdef PROBE_DEBUG */

struct range {
	uint16_t min;
	uint16_t max;
};

#endif /* _LIBPROBE_UTIL_H_ */
