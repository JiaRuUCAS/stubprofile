#ifndef MEMORY_MANAGEMENT
#define MEMORY_MANAGEMENT

#include "list.h"
#include <stdint.h>


//this is a big memory pools
//one memory cube is 4MB
#define CUBE_SIZE (1024*1024*64)


struct mem_cube{
    void *mem;
    void *left;
    uint32_t space;
};
//This mem manager is specific for the
//bitmap trie. 
//the internal data structure is a cross list table
//it's kinda like a buddy system, but we don't use the memory cube which 
//size is a exp of 2. 
//we use consective node size, like 1,2,3,4,5,...
//

struct mem_head{
    struct list_head list;
};

struct mem_manager{
    struct mem_cube cube;
    struct mem_head *head;
    uint32_t node_size;
#ifdef STATS
    uint32_t mem;
    uint32_t mem_node;
#endif

};

struct mem_info{
    uint32_t node_num;
    struct list_head list;
};
    

int init_mem_manager(struct mem_manager *mm, int stride, int node_size);
void *alloc_node(struct mem_manager *mm, uint32_t node_num);
void dealloc_node(struct mem_manager *mm, void *mem);

void mem_usage(struct mem_manager *mm);


#endif
