#ifndef __BITMAP_H__
#define __BITMAP_H__

#include <stdint.h>
#include "mm.h"



//use the bitmap configruation comes from the Will Eatherton paper: Tree bitmap
//use the 13,4,4,4,3 trie

//you can not adjust the STRIDE value
//if you ajust the other value, the macro LEVEL should be changed.

#define LENGTH 32
#define STRIDE 4 
#define INITIAL_BITS 13 
#define PREFIX_HI 4 
#define LEVEL 5

#define UPDATE_LEVEL 16 

#define INLINE __attribute__((always_inline))

//the multi-bit node
//the stride is 4, so the external bitmap is 16 bits long.
//the internal bitmap is 15 bits long
//the total size of mb_node is 8Byte 
//one cache line is 64Bytes, we can add some bit flags in the mb_node
struct mb_node{
    uint16_t external;
    uint16_t internal;
    void     *child_ptr;
};


//void initial_table_init();
//void init_bits_lookup();




//return 1 means the prefix exists.

void init_aux_trie(struct mb_node *up_aux, struct mem_manager *mm);
void destroy_aux_trie(struct mb_node *up_aux);

void print_aux_prefix(struct mb_node *up_aux, void (*print_next_hop)(void *nhi));

uint8_t detect_overlap(struct mb_node *aux, uint32_t ip, uint8_t cidr, uint32_t leaf_pushing_bits, void **nhi_over);

void insert_aux_entry(struct mem_manager *mm, struct mb_node *up_aux, uint32_t ip, int cidr, uint32_t nhi);

void delete_aux_entry(struct mem_manager *mm, struct mb_node *up_aux, uint32_t ip, int cidr);

void mem_alloc_stat();
void mem_op();

//#define UP_STATS
//#define USE_MM
#define DEBUG_MEMORY_FREE
#define DEBUG_MEMORY_ALLOC

#endif
