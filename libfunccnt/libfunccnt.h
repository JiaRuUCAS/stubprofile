#ifndef __LIB_FUNC_CNT_H__
#define __LIB_FUNC_CNT_H__

#if !defined(LIB_EXPORT)
	#define LIB_EXPORT __attribute__((visibility ("default")))
#endif

LIB_EXPORT void funcc_count_pre(unsigned int func);
LIB_EXPORT void funcc_count_post(unsigned int func);

#endif // __LIB_FUNC_CNT_H__
