#ifndef _TS_H_
#define _TS_H_

#include "bitmap.h"
#include "list.h"

#define MEM_CUBE 32
#define MEM_CUBE_SIZE (1<<CUBE_LOG_SIZE)
#define CUBE_LOG_SIZE 20
#define INVALID_INDEX 0xffffffff



#define ERROR(x) perror("TS-LOOKUP: " #x);
#define INFO(x)  printf("TS-LOOKUP: " #x);


struct free_blocks
{
    uint32_t free_index[12];
    struct list_head list;
};

struct mem_cache 
{
    struct list_head free_index;
    uint32_t curr_cube;
    uint32_t curr_index;
    void *cube[MEM_CUBE];
};

struct ts_tree
{
    struct entry *l1;
    struct mb_node up_aux;
    struct mem_manager aux_mm;
    struct mem_cache mm; 
};

//L1 & L2 ENTRY
///////////////////////////////////
//   CHILD |  CIDR  | INDEX  | 
//   1bits |  6bits | 25bits | 



struct entry
{
    uint32_t child :1;
    uint32_t cidr  :6;
    uint32_t index :25;
};

static inline uint32_t cube_no(uint32_t index) 
{
    return (index >> CUBE_LOG_SIZE);
}

static inline uint32_t cube_index(uint32_t index)
{
    return (index & (0xffffffff >> (LENGTH - CUBE_LOG_SIZE)));
}

static inline struct entry * fast_get_entry(struct mem_cache *mm, uint32_t index)
{
    struct entry *e; 
    e = ((struct entry*)mm->cube[cube_no(index)] + (cube_index(index)<<8));
    return e;
}


int ts_init(struct ts_tree *ts);
int ts_insert_prefix(struct ts_tree *ts, uint32_t ip, uint32_t cidr, uint32_t nhi);
int ts_delete_prefix(struct ts_tree *ts, uint32_t ip, uint32_t cidr);
void ts_destroy(struct ts_tree *ts);

uint32_t ts_search(struct ts_tree *ts, uint32_t ip); 
#endif
