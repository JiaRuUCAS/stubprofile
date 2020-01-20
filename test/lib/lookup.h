#ifndef _TS_LOOKUP_H_
#define _TS_LOOKUP_H_

#include <stdint.h>

struct ts_tree;

int ts_init(struct ts_tree *ts);
int ts_insert_prefix(struct ts_tree *ts, uint32_t ip, uint32_t cidr, uint32_t nhi);
uint32_t ts_search(struct ts_tree *ts, uint32_t ip);
int ts_delete_prefix(struct ts_tree *ts, uint32_t ip, uint32_t cidr);
void ts_destroy(struct ts_tree *ts);

#endif
