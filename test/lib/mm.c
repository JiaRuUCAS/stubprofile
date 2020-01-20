#include "mm.h"
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include "list.h"


int init_mem_manager(struct mem_manager *mm, int stride, int node_size)
{
    if(!mm) {
        printf("wrong agument\n");
        return -1;
    }

    mm->cube.mem = calloc(1, CUBE_SIZE);
    if(!((mm->cube).mem)) {
        perror("no memory for cube\n");
        return -1;
    }
    mm->cube.left = mm->cube.mem;
    mm->cube.space = CUBE_SIZE;

    mm->head = (struct mem_head*)calloc(sizeof(struct mem_head), (1<<stride)*2);
    if (!mm->head) {
        perror("no memory for head\n");
        return -1;
    }
    int i;
    for(i=0;i<(1<<stride)*2;i++){
        INIT_LIST_HEAD(&(mm->head[i].list));
    }

    mm->node_size = node_size;
#ifdef STATS
    mm->mem = 0;
    mm->mem_node =0;
#endif
    return 0;
}    

void *alloc_node(struct mem_manager *mm, uint32_t node_num)
{
    uint8_t *p;
    uint8_t *ret;
    struct list_head *pl;
    struct mem_info *info;
    uint32_t size = 0;

    if(node_num == 0) {
        return NULL;
    }

    //first try to alloc it from the mem_head list

    if (node_num > 31) {
        printf("ssss\n");
    }

    if (!list_empty(&(mm->head[node_num-1].list))) {
        pl = &(mm->head[node_num-1].list);
        pl = pl->next;
        list_del(pl);  
        info = (struct mem_info*)list_entry(pl, struct mem_info, list);
        ret = (uint8_t *)info + sizeof(struct mem_info);
        memset(ret, 0, node_num * mm->node_size);
#ifdef STATS
        mm->mem += node_num * mm->node_size;
        mm->mem_node += node_num;
#endif
        return (void*) ret; 
    }
    //need to alloc it from the mem_cube
    //first to see if there is enough space in mem cube

    size = (node_num * mm->node_size) + sizeof(struct mem_info);

    if (mm->cube.space < size) {
        printf("out of memory\n");
        return NULL;
    }

    p = mm->cube.left;
    mm->cube.left = (void*)(p + size);
    mm->cube.space -= size;

    info = (struct mem_info*)p;
    INIT_LIST_HEAD(&info->list);
    info->node_num = node_num;

    ret = p + sizeof(struct mem_info);
#ifdef STATS
    mm->mem += node_num * mm->node_size;
    mm->mem_node += node_num;
#endif
    return (void*) ret;
}

void dealloc_node(struct mem_manager *mm, void *mem)
{
    struct mem_info *info;
    uint8_t *p = (uint8_t *)mem;
    p = p - sizeof(struct mem_info);
    info = (struct mem_info*)p;
    

#ifdef STATS
    mm->mem -= info->node_num * mm->node_size;
    mm->mem_node -= info->node_num;
#endif

    list_add(&info->list, &(((mm->head)[info->node_num-1]).list));
}


void mem_usage(struct mem_manager *mm)
{
#ifdef STATS
    printf("mem %d mem %dK\n", mm->mem, mm->mem/1024);
    printf("node_num %d\n", mm->mem_node);
#endif
}


