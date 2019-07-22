#ifndef _PROFILE_ARRAY_H_
#define _PROFILE_ARRAY_H_

#include <stdint.h>

struct array {
	uint32_t elem_num;
	uint32_t elem_size;
	uint8_t contents[];
};

struct array *array__new(uint32_t elem_num, uint32_t elem_size);
void array__delete(struct array *arr);

static inline void *array__entry(struct array *arr, unsigned idx)
{
	if (idx >= arr->elem_num)
		return NULL;
	return &arr->contents[idx * arr->elem_size];
}

#endif /* _PROFILE_ARRAY_H_ */
