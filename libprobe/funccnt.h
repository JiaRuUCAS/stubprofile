#ifndef __LIBPROBE_FUNCC_H__
#define __LIBPROBE_FUNCC_H__

#include "util.h"

struct funcc_counter {
	uint64_t pre_count;
	uint64_t post_count;
};

struct funcc_thread {
	struct funcc_counter *counters;
	unsigned state;
};

LIB_EXPORT void funcc_count_pre(unsigned int func);
LIB_EXPORT void funcc_count_post(unsigned int func);
LIB_EXPORT void funcc_count_init(unsigned min, unsigned max);
LIB_EXPORT void funcc_count_exit(void);
LIB_EXPORT void funcc_count_thread_exit(void);

void funcc_count_thread_init(void);

#endif // __LIBPROBE_FUNCC_H__
