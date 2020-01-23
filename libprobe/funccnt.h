#ifndef __LIBPROBE_FUNCC_H__
#define __LIBPROBE_FUNCC_H__

#include "util.h"

struct funcc_counter {
	uint64_t pre_count;
	uint64_t post_count;
};

struct funcc_thread {
	unsigned state;
	struct funcc_counter counters[];
};

LIB_EXPORT void funcc_count_pre(unsigned int func);
LIB_EXPORT void funcc_count_post(unsigned int func);
LIB_EXPORT void funcc_init(unsigned min, unsigned max);

void funcc_data_free(void);
void funcc_data_init(void);

#endif // __LIBPROBE_FUNCC_H__
