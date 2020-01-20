#ifndef __LIB_FUNC_CNT_H__
#define __LIB_FUNC_CNT_H__

#if !defined(LIB_EXPORT)
#define LIB_EXPORT __attribute__((visibility ("default")))
#endif

struct funcc_counter {
	uint64_t pre_count;
	uint64_t post_count;
};

enum {
	FUNCC_STATE_UNINIT = 0,
	FUNCC_STATE_INIT,
	FUNCC_STATE_ERROR,
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

#endif // __LIB_FUNC_CNT_H__
