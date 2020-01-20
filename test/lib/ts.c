#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ts.h"
#include "lookup.h"

int ts_init(struct ts_tree *ts)
{
    ts->l1 = (struct entry*)calloc((1<<16) , sizeof(struct entry));
    if(!ts->l1){
        ERROR("No memory for l1 entry");
        return -1;
    }

    init_aux_trie(&ts->up_aux, &ts->aux_mm);

    ts->mm.curr_cube = 0;
    ts->mm.cube[ts->mm.curr_cube] = calloc(MEM_CUBE_SIZE , sizeof(struct entry));
    INIT_LIST_HEAD(&ts->mm.free_index);

    if(!ts->mm.cube[ts->mm.curr_cube]){
        ERROR("No memory for cube");
        return -1;
    }

    return 0;
}

// * ts-mem manager code

struct entry * get_entry(struct mem_cache *mm, uint32_t index) 
{
    if(cube_no(index) > MEM_CUBE) {
        return NULL;
    }

    struct entry *e;
    e = ((struct entry*)mm->cube[cube_no(index)] + (cube_index(index)<<8));
    return e;
}

struct entry *alloc_entry(struct mem_cache *mm, uint32_t *index)
{

    struct entry *alloc_e = NULL;
    //first query the free list
    if(!list_empty(&mm->free_index)){
        struct list_head *ph = &mm->free_index; 
        ph = ph->next;
        struct free_blocks *fb = (struct free_blocks*)list_entry(ph, struct free_blocks, list);
        
        int i = 11;
        for(; i>=0; i--) {
            if(fb->free_index[i] != INVALID_INDEX) {
                *index = fb->free_index[i];
                fb->free_index[i] = INVALID_INDEX;
                alloc_e = get_entry(mm, *index);
                break;
            }
        }
        //the last one is consumed
        if(i==0) {
            list_del(&fb->list);
            free(fb);
        }
    }
    else {
        *index = mm->curr_index;
        alloc_e = get_entry(mm, *index);

        if(cube_index(mm->curr_index) == MEM_CUBE_SIZE/(1<<8) - 1) {
            mm->curr_cube += 1;
            mm->cube[mm->curr_cube] = calloc(MEM_CUBE_SIZE, sizeof(struct entry)); 
            mm->curr_index = (mm->curr_cube << CUBE_LOG_SIZE) | 0; 
        }
        else {
            mm->curr_index += 1;
        }
    }
    return alloc_e;
}

void dealloc_entry(struct mem_cache *mm, uint32_t index)
{
//    if(index == 0) {
//        printf("here");
//    }
    if(list_empty(&mm->free_index)) {
        struct free_blocks *fb = (struct free_blocks *)calloc(1, sizeof(struct free_blocks));
        if(!fb) {
            ERROR("No memory for free_blocks");
            return;
        }
        INIT_LIST_HEAD(&fb->list);
        list_add(&fb->list, &mm->free_index);
        int i; 
        for(i = 0; i < 12; i++) {
            fb->free_index[i] = INVALID_INDEX;
        }
        
    }

    struct free_blocks *fb;
    int add_flag = 0;
    list_for_each_entry(fb, &mm->free_index, list) { 
        int i;
        for(i = 0; i < 12; i++) {
            if(fb->free_index[i] == INVALID_INDEX) {
                fb->free_index[i] = index;
                add_flag = 1;
                break;
            }
        }
        if(i!= 12) {
            add_flag = 1;
            break;
        }
    }

    if(!add_flag) {
        struct free_blocks *fb = (struct free_blocks *)calloc(1, sizeof(struct free_blocks));
        if(!fb) {
            ERROR("No memory for free_blocks");
            return;
        }
        INIT_LIST_HEAD(&fb->list);
        list_add(&fb->list, &mm->free_index);
        fb->free_index[0] = index;
        int i;
        for(i = 1 ; i< 12; i++) {
            fb->free_index[i] = INVALID_INDEX;
        }
    }
}

void destory_mem_cache(struct mem_cache *mm) 
{
    if(!list_empty(&mm->free_index)) {
        struct free_blocks *fb;
        struct free_blocks *fbtmp;
        list_for_each_entry_safe(fb, fbtmp, &mm->free_index, list) {
            list_del(&fb->list);
            free(fb);
        }
    }
    int i = 0;
    for(; i< mm->curr_cube + 1; i++) {
        free(mm->cube[i]);
    }

    mm->curr_cube = 0;
    mm->curr_index = 0;

}





//modified: return 1
//unmodified: return 0
//error: return -1

//TS INSERT TO LEVEL X 
int ts_insert_lx(struct mem_cache *mm,  
        struct entry *l1e, 
        uint32_t cidr, 
        uint32_t nhi, 
        uint32_t level)
{
    int i = 0;
    int modi = 0;
    struct entry *e = l1e;

    //if(level < cidr) {
    //    return 1;
    //}

    for(; i < (1<<(level-cidr));i++, e++) {
        if(e->child) {
            if(e->cidr > cidr) {
                continue;
            }
            else {
                e->cidr = cidr;
                //push to next level;
                struct entry *ne = get_entry(mm, e->index); 
                modi = ts_insert_lx(mm, ne, cidr, nhi, cidr + 8);
            }
        }
        else {
            if(e->cidr > cidr) {
                continue;
            }
            else {
                e->cidr = cidr;
                e->index = nhi;
                modi = 1;
            }
        }
    }

    return modi;
}

int ts_insert_iter(struct ts_tree *ts, 
        struct entry *e, 
        uint32_t ip, int cidr, uint32_t nhi, uint32_t level, int depth) 
{
    int state = 0;
    struct entry *ne = NULL;

    if(e->child) {
        ne = get_entry(&ts->mm, e->index);  
        ne += (ip >>24);
        if(cidr <= 8)
            state |= ts_insert_lx(&ts->mm, ne, cidr + depth * 8, nhi, level);
    }
    else {
        //two cases: 
        //1 empty blocks, one needs to alloc the memory.
        //2 has small cidr, needs to push to the next level;
        //
        if(e->cidr == 0) {
            e->child = 1;
            uint32_t tmp_index;
            ne = alloc_entry(&ts->mm, &tmp_index);
            e->index = tmp_index;
            ne += (ip >> 24);
            if(cidr <= 8)
                state |= ts_insert_lx(&ts->mm, ne, cidr + depth * 8, nhi, level);
        }
        else {
            e->child = 1;
            uint32_t push_nhi = e->index;
            uint32_t push_cidr = e->cidr;
            uint32_t tmp_index;
            ne = alloc_entry(&ts->mm, &tmp_index);
            e->index = tmp_index;
            state |= ts_insert_lx(&ts->mm, ne, push_cidr, push_nhi, push_cidr + 8); 

            ne += (ip >>24);
            if(cidr <=8)
                state |= ts_insert_lx(&ts->mm, ne, cidr + depth * 8, nhi, level);
        }
    }

    if(cidr > 8) {
        state |= ts_insert_iter(ts, ne, ip << 8, cidr - 8, nhi, level + 8, depth +1);     
    }

    
    return state;

}


int ts_insert_prefix(struct ts_tree *ts, uint32_t ip, 
        uint32_t cidr, uint32_t nhi)
{
    if(nhi & (1<<25)) {
       INFO("too large nhi (> 2^25)"); 
       return -1;
    }

    if(cidr != 0) {
        ip = ip & (0xffffffff << (LENGTH - cidr));
    }
    else {
        INFO("default ip prefix should not be insert");
        return 0;
    }

    if(cidr <= 16) {
        int index = ip >> 16;
        struct entry *l1e = &ts->l1[index];
        ts_insert_lx(&ts->mm, l1e, cidr, nhi, 16);
    }
    else {
        int index = ip >> 16;
        struct entry *l1e = &ts->l1[index];
        ts_insert_iter(ts, l1e, (ip << 16), cidr - 16, nhi, 24, 2);
    }

    insert_aux_entry(&ts->aux_mm, &ts->up_aux, ip, cidr, nhi); 
    return 0;
}

int can_recycle(struct mem_cache *mm, struct entry *e, uint32_t level)
{
    //e can be recycled due to two resons:
    //1: all e children are the same
    //2: all e->cidr is actually smaller than the level bottom bound;

    int i = 1;
    int recycle = 1;

    struct entry *ne = get_entry(mm, e->index);
    if(ne->child) {
        return 0;
    }

    uint32_t cidr = ne->cidr;
    uint32_t index = ne->index;
    if(cidr > level) {
        return 0;
    }

    ne++;
    for(; i < 256; i++, ne++) {
        if(ne->child != 0 || ne->cidr != cidr || ne->index != index) {
            recycle = 0;
            break;
        }
    }

    return recycle;

}

void clear_entries(struct mem_cache *mm, struct entry *e)
{
    struct entry *ne = get_entry(mm, e->index);
    memset(ne, 0, 256*sizeof(struct entry));
}


int ts_delete_lx(struct ts_tree *ts,  
        struct entry *l1e, 
        uint32_t cidr,
        uint32_t over_cidr, 
        uint32_t over_nhi,
        uint32_t level) 
{

    int i = 0;
    struct entry *e = l1e;
    int ret;
    int flag = 1; 
    
    for(; i < (1<<(level-cidr)); i++, e++) {
        if(e->child) {
            if(e->cidr > cidr) {
                flag = 0;
                continue;
            }
            else {
                e->cidr = over_cidr;
                struct entry *ne = get_entry(&ts->mm, e->index);
                ret = ts_delete_lx(ts, ne, cidr, over_cidr, over_nhi, cidr + 8);
                //ret == 1, means the next 256 entries equals to 
                //(over_cidr, over_nhi), in this case, 
                //we should not push the (over_cidr, over_nhi) to the 
                //next level, in fact, we should recycle the next 256 entries;
                if(ret) {
                    clear_entries(&ts->mm, e);
                    dealloc_entry(&ts->mm, e->index);
                    e->child = 0;
                    e->index = over_nhi;
                    //if(over_nhi == 229324) {
                    //    printf("here\n");
                    //}
                }
                else
                    flag = 0;
            }
        }
        else {
            if(e->cidr > cidr) {
                flag = 0;
                continue;
            }
            else {
                e->cidr = over_cidr;
                e->index = over_nhi;
                //if(over_nhi == 229324) {
                //    printf("here\n");
                //}
            }
        }
    }

    return flag;
}

static void find_overlap_prefix(struct ts_tree *ts, uint32_t ip, uint32_t cidr, 
        uint32_t pushing_level, uint32_t *over_cidr, uint32_t *over_nhi)
{

    void *nhi_over = NULL;
    *over_cidr = detect_overlap(&ts->up_aux, ip, cidr, pushing_level, &nhi_over);
    *over_nhi = 0;

    if(nhi_over == NULL) {
        *over_cidr = 0;
    }
    else {
        *over_nhi = (uint32_t)nhi_over;
    }


}
//this is quite write-only code I have ever written
//I am still unware the problem

int ts_delete_iter(struct ts_tree *ts, 
        struct entry *e, 
        uint32_t ip, 
        uint32_t cidr, 
        uint32_t level,
        int depth)
{
    int ret = 0;
    uint32_t over_nhi, over_cidr;
    find_overlap_prefix(ts, ip, cidr + 8 * depth, level, &over_cidr, &over_nhi);

    if(cidr > 8) {
        struct entry *ne = get_entry(&ts->mm, e->index);
        ne += (((uint32_t)(ip << level)) >> 24);
        ret = ts_delete_iter(ts, ne, ip, cidr - 8, level+8, depth+1);
        if(ret && can_recycle(&ts->mm, e, level)) {
            clear_entries(&ts->mm, e);
            dealloc_entry(&ts->mm, e->index);
            e->child = 0;
            e->index = over_nhi;
            e->cidr = over_cidr;
            return 1;
        }
        return 0;
    }
    
    ret = ts_delete_lx(ts, e, cidr + 8 * depth, over_cidr, over_nhi, level);
    return ret;
}

int ts_delete_prefix(struct ts_tree *ts, uint32_t ip, uint32_t cidr)
{
    if(cidr != 0) {
        ip = ip & (0xffffffff << (LENGTH - cidr));
    }
    else {
        INFO("default ip prefix should not be deleted");
        return 0;
    }

    uint32_t index = ip >> 16;
    struct entry *l1e = &ts->l1[index];

    uint32_t over_cidr, over_nhi;
    find_overlap_prefix(ts, ip, cidr, 16, &over_cidr, &over_nhi);
    
    if(cidr <= 16) {
        ts_delete_lx(ts, l1e, cidr, over_cidr, over_nhi, 16); 
    }
    else {
        //should be a child entry
        struct entry *ne = get_entry(&ts->mm, l1e->index);
        ne += ((uint32_t)(ip << 16) >> 24);
        int ret = ts_delete_iter(ts, ne, ip, cidr - 16, 24, 2);
        if(ret && can_recycle(&ts->mm, l1e, 16)) {
            //if(index == 14673) {
            //    printf("here\n");
            //}
            dealloc_entry(&ts->mm, l1e->index);
            clear_entries(&ts->mm, l1e);
            l1e->child = 0;
            l1e->index = over_nhi;
            l1e->cidr = over_cidr;
        }
    }

    delete_aux_entry(&ts->aux_mm, &ts->up_aux, ip, cidr);
    return 0;
}



uint32_t ts_search(struct ts_tree *ts, uint32_t ip) 
{
    uint32_t index = ip >> 16;
    ip <<= 16;
    struct entry *e = &(ts->l1[index]);
    while(e->child) {
        e = fast_get_entry(&ts->mm, e->index);
        e += ip >> 24;
        ip <<= 8;
    }
    return e->index;
}

void ts_destroy(struct ts_tree *ts)
{
    free(ts->l1);
    destory_mem_cache(&ts->mm);

    destroy_aux_trie(&ts->up_aux);
}

