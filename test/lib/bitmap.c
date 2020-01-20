#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "bitmap.h"
#include <arpa/inet.h>

#ifdef UP_STATS
static int node_cpy;
static int node_alloc;
static int node_set;
static int st_f = 1;
void enable_stat(){
    st_f = 1;
}
void disable_stat() {
    st_f = 0;
}
#endif


#ifdef DEBUG_MEMORY_ALLOC
static int mem;
#endif

#define ROOT_CNT 1
#define CHILD_CNT (1<<STRIDE)
#define UP_CHILD(x) (x)
#define POINT(X) ((struct mb_node*)(X))
#define NODE_SIZE  (sizeof(struct mb_node))
#if __x86_64__  == 1
static uint32_t UP_RULE(uint32_t x)
{
        return (x*sizeof(void**) + NODE_SIZE - 1)/(NODE_SIZE);
}
#else
#define UP_RULE(X) ((X) & 0x01 ? (X/2 + 1) : (X/2))
#endif

void * new_node(struct mem_manager* mm, int mb_cnt, int result_cnt)
{
    // one for result table
    // one for child table
    int r_c = UP_RULE(result_cnt); 
    int size = (UP_CHILD(mb_cnt) + r_c) * NODE_SIZE;
    void *ret;
#ifndef USE_MM
    if (size != 0) 
        ret = calloc(1, size);
    else 
        ret = NULL;
#ifdef DEBUG_MEMORY_ALLOC
    mem += (UP_CHILD(mb_cnt) + r_c ) * NODE_SIZE;
#endif
    //node_num += UP_CHILD(mb_cnt) + r_c;
#else
    ret = alloc_node(mm, (UP_CHILD(mb_cnt) + r_c));
#endif
    ret = POINT(ret) + r_c;
    return ret;
}

void free_node(struct mem_manager *mm, void *ptr, uint32_t cnt)
{
    if (ptr){ 
#ifndef USE_MM
        free(ptr);
#ifdef DEBUG_MEMORY_ALLOC
        mem -= cnt * NODE_SIZE;
#endif
        //node_num -= cnt;
#else
        dealloc_node(mm, ptr);
#endif
    }
}


void mem_op()
{
#ifdef UP_STATS
    printf("node copy %d\n", node_cpy);
    printf("node alloc %d\n", node_alloc);
    printf("node set %d\n", node_set);
#endif
}
//count 1's from the right of the pos, not including the 1
//pos = 0 ~ 15
//pos == 16 means to counts how many 1's in the bitmap
//uint8_t count_ones(uint16_t bitmap, uint8_t pos)
//{
//    return bits_lookup[(uint16_t)(bitmap<<(16 - pos))];
//}
//
int count_ones(uint16_t bitmap, uint8_t pos)
{
    return __builtin_popcount((uint16_t)(bitmap<<(16 - pos)));
}


static uint32_t count_inl_bitmap(uint32_t bit, int cidr)
{
    uint32_t pos = (1<<cidr) + bit;
    return (pos - 1);
}

static uint32_t count_enl_bitmap(uint32_t bits)
{
    return (bits);
}

static void update_inl_bitmap(struct mb_node *node, uint32_t bit)
{
    node->internal |= bit;
}

static void update_enl_bitmap(struct mb_node *node, uint32_t bit)
{
    node->external |= bit;
}


// ----child_ptr-----
// --------|---------
// |rules  | child--| 
// |-------|--------|
// to get the head of the memory : POINT(x) - UP_RULE(x)
// 


//pos start from 1
//
void extend_rule(struct mem_manager *mm, struct mb_node *node, uint32_t pos,
        void *nhi)
{
    int child_num = count_ones(node->external, 16);
    int rule_num = count_ones(node->internal, 16) - 1;

    void **i;
    void **j;
    
    void *n = new_node(mm, child_num, rule_num + 1);

    if (child_num != 0){
        //copy the child
        memcpy(n,node->child_ptr,sizeof(struct mb_node)*UP_CHILD(child_num));
#ifdef UP_STATS
        if (st_f){
            node_cpy += UP_CHILD(child_num);
        }
#endif
    }
    

#ifdef UP_STATS
    if (st_f) {
        node_alloc++;
    }
#endif

    //insert the rule at the pos position

    if (rule_num != 0) {
        //copy the 1~pos-1 part
        i = (void**)node->child_ptr - pos + 1;
        j = (void**)n - pos + 1;
        
        memcpy(j,i,(pos-1)*sizeof(void**));

        //copy the pos~rule_num part
        i = (void**)node->child_ptr - rule_num;
        j = (void**)n - rule_num - 1;

        memcpy(j,i,(rule_num - pos + 1)*sizeof(void**));
#ifdef UP_STATS
        if (st_f) {
            node_cpy += UP_RULE(rule_num);
        }
#endif
    }

    i = (void**)n - pos;
    *i = nhi;
    
    //need to be atomic
    if (node->child_ptr) {
        free_node(mm, POINT(node->child_ptr) - UP_RULE(rule_num), UP_CHILD(child_num) + UP_RULE(rule_num));
    }
    node->child_ptr = n; 
}

//return the extended node
//
struct mb_node * extend_child(struct mem_manager *mm, struct mb_node *node,
        uint32_t pos)
{
    int child_num = count_ones(node->external, 16) - 1;
    int rule_num = count_ones(node->internal, 16);

    void *n = new_node(mm, child_num +1 ,rule_num);

    void **i;
    void **j;
    
    if (rule_num != 0) {
        //copy the rules
        i = (void **)node->child_ptr - rule_num;
        j = (void **)n - rule_num;
        memcpy(j,i,(rule_num)*sizeof(void**));
#ifdef UP_STATS
        if (st_f)
            node_cpy += UP_RULE(rule_num);
#endif
    }

    if (child_num != 0) {
    //copy the  0~pos-1 part
        memcpy(n,node->child_ptr,(pos)*sizeof(struct mb_node));
    //copy the pos~child_num-1 part to pos+1~child_num 
        memcpy((struct mb_node*)n+pos+1,(struct mb_node*)node->child_ptr + pos, (child_num - pos) * sizeof(struct mb_node));
#ifdef UP_STATS
        if(st_f)
            node_cpy += UP_CHILD(child_num);
#endif
    }

#ifdef UP_STATS
    if(st_f)
        node_alloc ++;
#endif

    //need to be atomic
    if (node->child_ptr) {
        free_node(mm, POINT(node->child_ptr) - UP_RULE(rule_num), UP_CHILD(child_num) + UP_RULE(rule_num));
    }
    node->child_ptr = n;
    return (struct mb_node*)n + pos;

}


//notes: cidr > 0 

void insert_entry(
        struct mem_manager *mm,
        struct mb_node *node,
        uint32_t ip, int cidr, 
        void *nhi
        )
{
    uint8_t pos;
    uint8_t stride;
    void **i;

    for (;;) {

        if (cidr < STRIDE) {
            // if the node has the prefix already
            //need to be atomic
            //
            stride = ip >> (LENGTH - cidr);
            pos = count_inl_bitmap(stride, cidr);
            if( node->internal & (1 << pos)) {
                //already has the rule, need to update the rule
                i = (void**)node->child_ptr - count_ones(node->internal, pos) -1;
                *i = nhi;
                return;
            }
            else {
                update_inl_bitmap(node, (1 << pos));
                //rules pos starting at 1, so add 1 to offset
                pos = count_ones(node->internal, pos) + 1;
                extend_rule(mm, node, pos, nhi); 
                break;
            }
        }
        //push the "cidr == stride" node into the next child
        else {
            stride = ip >> (LENGTH - STRIDE);
            pos = count_enl_bitmap(stride);

            if ( node->external & (1<< pos)){
                node = (struct mb_node*) node->child_ptr + count_ones(node->external, pos); 
            } 
            else {

                update_enl_bitmap(node, 1<<pos);
                //iteration
                //child pos starting at 0, so add 0 to offset
                pos = count_ones(node->external, pos);
                node = extend_child(mm, node, pos); 
            }
            cidr -= STRIDE;
            ip <<= STRIDE;
        }
    }
}



void reduce_child(struct mem_manager *mm, struct mb_node *node, int pos)
{
    int child_num = count_ones(node->external, 16);
    int rule_num = count_ones(node->internal, 16);

    if (child_num < 1){
        printf("reduce_rule: error!\n");
    }

    void *n = new_node(mm, child_num -1 ,rule_num);

    void **i;
    void **j;
    
    if (rule_num != 0) {
        //copy the rules
        i = (void **)node->child_ptr - rule_num;
        j = (void **)n - rule_num;
        memcpy(j,i,(rule_num)*sizeof(void**));
#ifdef UP_STATS
        if (st_f)
            node_cpy += UP_RULE(rule_num);
#endif
    }

    if (child_num > 1) {
    //copy the  0~pos-1 part
        memcpy(n,node->child_ptr,(pos)*sizeof(struct mb_node));
    //copy the pos+1~child_num part to pos~child_num-1 
        memcpy((struct mb_node*)n+pos,(struct mb_node*)node->child_ptr + pos + 1, (child_num - pos -1) * sizeof(struct mb_node));
#ifdef UP_STATS
        if(st_f)
            node_cpy += UP_CHILD(child_num);
#endif
    }
#ifdef UP_STATS
    if(st_f)
        node_alloc++;
#endif

    //need to be atomic
    if (node->child_ptr) {
        free_node(mm, POINT(node->child_ptr) - UP_RULE(rule_num), UP_CHILD(child_num) + UP_RULE(rule_num));
    }
    node->child_ptr = n;
}

void reduce_rule(struct mem_manager *mm, struct mb_node *node, uint32_t pos)
{
    int child_num = count_ones(node->external, 16);
    int rule_num = count_ones(node->internal, 16);

    void **i;
    void **j;
    
    if (rule_num < 1){
        printf("reduce_rule: error!\n");
        return;
    }

    void *n = new_node(mm, child_num, rule_num - 1);

    if (child_num != 0){
        //copy the child
        memcpy(n,node->child_ptr,sizeof(struct mb_node)*UP_CHILD(child_num));
#ifdef UP_STATS
        if(st_f)
            node_cpy += UP_CHILD(child_num);
#endif
    }
    


    //delete the rule at the pos position

    if (rule_num > 1) {
        //copy the 1~pos-1 part
        i = (void**)node->child_ptr - pos + 1;
        j = (void**)n - pos + 1;
        
        memcpy(j,i,(pos-1)*sizeof(void**));

        //copy the pos~rule_num part
        i = (void**)node->child_ptr - rule_num;
        j = (void**)n - rule_num + 1;

        memcpy(j,i,(rule_num - pos)*sizeof(void**));
#ifdef UP_STATS
        if(st_f)
            node_cpy += UP_RULE(rule_num);
#endif
    }

#ifdef UP_STATS
    if(st_f)
        node_alloc++;
#endif

    //need to be atomic
    if (node->child_ptr) {
        free_node(mm, POINT(node->child_ptr) - UP_RULE(rule_num), UP_CHILD(child_num) + UP_RULE(rule_num));
    }
    node->child_ptr = n; 
}

static void clear_bitmap(uint16_t *bitmap, uint16_t bitmap_set){
    *bitmap &= (~bitmap_set);
}

struct trace{
    struct mb_node *node;
    uint32_t pos;
};


int update_nodes(struct mem_manager *mm, struct trace *t, int total)
{
    int i;
    int node_need_to_del = 0;
    for(i=total;i >= 0;i--){
        if(i==total){
            reduce_rule(mm, t[i].node, count_ones((t[i].node)->internal, t[i].pos) + 1);
            clear_bitmap(&(t[i].node)->internal, (1 << (t[i].pos)));
            if((t[i].node)->internal == 0 && (t[i].node)->external == 0)
            {
                node_need_to_del = 1;
            }
        }
        else{
            if(node_need_to_del){
                reduce_child(mm, t[i].node, count_ones((t[i].node)->external, t[i].pos));
                clear_bitmap(&(t[i].node)->external, (1 << (t[i].pos)));
            }
            if((t[i].node)->internal == 0 && (t[i].node)->external == 0){
                node_need_to_del = 1;
            }
            else{
                node_need_to_del = 0;
            }
        }
    }
    return node_need_to_del;


}
/*
typedef void (*traverse_func) (struct mb_node *node, uint32_t cur_ip, int cidr, void *data);

static void traverse_trie(struct mb_node *node, uint32_t ip, int cidr,
        traverse_func func, void *user_data)
{

    uint8_t pos;
    uint8_t stride;

    for (;;) {

        if (cidr < STRIDE) {
            stride = ip >> (LENGTH - cidr);
            pos = count_inl_bitmap(stride, cidr);
            func(node, ip, cidr, user_data);

            break;
        }
        else {
            stride = ip >> (LENGTH - STRIDE);
            pos = count_enl_bitmap(stride);

            func(node, ip, cidr, user_data);
            node = (struct mb_node*) node->child_ptr + count_ones(node->external, pos); 

            cidr -= STRIDE;
            ip <<= STRIDE;
        }
    }
}


typedef void (*destroy_func)(void*);

struct overlap_nhi_data{
    destroy_func func;
    void *nhi_near;
};

static void overlap_nhi(struct mb_node *node, uint32_t ip, int cidr, void *user_data)
{

    if (cidr < STRIDE) {
        uint8_t stride;
        uint8_t pos;
        void ** nhi;
        struct overlap_nhi_data *ond  = (struct overlap_nhi_data *)(user_data);

        stride = ip >> (LENGTH - cidr);
        pos = count_inl_bitmap(stride, cidr);


        nhi = (void **)node->child_ptr - 
            count_ones(node->internal, pos) - 1; 

        if (ond->func)
            ond->func(*nhi);
        *nhi = ond->nhi_near;
    }
}
*/



int delete_entry(struct mem_manager* mm, struct mb_node *node, uint32_t ip, int cidr, 
        void (*destroy_nhi)(void *nhi))
{
    uint8_t pos;
    uint8_t stride;
    struct trace trace_node[UPDATE_LEVEL];
    int i = 0;

    for (;;) {

        if (cidr < STRIDE) {
            // if the node has the prefix already
            //need to be atomic
            //
            stride = ip >> (LENGTH - cidr);
            pos = count_inl_bitmap(stride, cidr);
            if (destroy_nhi) {
                void ** nhi;
                nhi = (void **)node->child_ptr - 
                    count_ones(node->internal, pos) - 1; 
                destroy_nhi(*nhi);
            }

            trace_node[i].node = node;
            trace_node[i].pos =  pos; 

            break;
        }
        //push the "cidr == stride" node into the next child
        else {
            stride = ip >> (LENGTH - STRIDE);
            pos = count_enl_bitmap(stride);

            
            trace_node[i].node = node;
            trace_node[i].pos  = pos; 

            node = (struct mb_node*) node->child_ptr + count_ones(node->external, pos); 

            cidr -= STRIDE;
            ip <<= STRIDE;
        }
        i++;
    }
    return update_nodes(mm, trace_node, i);
}


uint8_t detect_overlap(struct mb_node *up_aux, uint32_t ip, uint8_t cidr, uint32_t leaf_pushing_bits, void **nhi_over);

//return 1 means the prefix exists
int prefix_exsit(struct mb_node *up_aux, uint32_t ip, uint8_t cidr)
{
    uint8_t pos;
    uint8_t stride;
    struct mb_node *node = up_aux;

    for (;;) {

        if (cidr < STRIDE) {
            // if the node has the prefix already
            //need to be atomic
            //
            stride = ip >> (LENGTH - cidr);
            pos = count_inl_bitmap(stride, cidr);
            if ( !(node->internal & ( 1 << pos))) {
                printf("ERROR: try to delete a non-exist prefix\n");
                return 0;
            }

            break;
        }
        //push the "cidr == stride" node into the next child
        else {
            stride = ip >> (LENGTH - STRIDE);
            pos = count_enl_bitmap(stride);

            if( ! (node->external & ( 1<< pos))) {
                printf("ERROR: try to delete a non-exist prefix\n");
                return 0;
            }

            node = (struct mb_node*) node->child_ptr + count_ones(node->external, pos); 

            cidr -= STRIDE;
            ip <<= STRIDE;
        }
    }

    return 1;
}


//static int tree_function(uint16_t bitmap, uint8_t stride)
int tree_function(uint16_t bitmap, uint8_t stride)
{
    int i;
    int pos;
    if (bitmap == 0)
        return -1;

    for(i=STRIDE-1;i>=0;i--){
        stride >>= 1;
        pos = count_inl_bitmap(stride, i); 
        if (bitmap & (1 << pos)){
            return pos;
        }
    }

    return -1;
}


   
void* do_search(struct mb_node *n, uint32_t ip)
{
    uint8_t stride;
    int pos;
    void **longest = NULL;
    //int depth = 1;

    for (;;){
        stride = ip >> (LENGTH - STRIDE);
        pos = tree_function(n->internal, stride);

        if (pos != -1){
            longest = (void**)n->child_ptr - count_ones(n->internal, pos) - 1;
        }
        if (n->external & (1 << count_enl_bitmap(stride))) {
            //printf("%d %p\n", depth, n);
            n = (struct mb_node*)n->child_ptr + count_ones(n->external, count_enl_bitmap(stride));
            ip = (uint32_t)(ip << STRIDE);
            //depth ++;
        }
        else {
            break;
        }
    }
//    printf("depth %d\n",depth);
    return (longest == NULL)?NULL:*longest;
}

int find_overlap_in_node(uint16_t bitmap, uint8_t stride, uint8_t *mask, int limit_inside)
{
    int i;
    int pos;
    if (bitmap == 0)
        return -1;
    //calulate the beginning bits
    stride >>= (STRIDE - limit_inside);

    for(i=limit_inside-1;i>=0;i--){
        stride >>= 1;
        *mask = i;
        pos = count_inl_bitmap(stride, i); 
        if (bitmap & (1 << pos)){
            return pos;
        }
    }


    return -1;
}


uint8_t detect_overlap(struct mb_node *aux, uint32_t ip, uint8_t cidr, uint32_t leaf_pushing_bits, void **nhi_over)
{
    //uint8_t ret;
    uint8_t stride;
    uint8_t mask = 0;
    uint8_t final_mask = 0;
    uint8_t curr_mask = 0;

    uint8_t step = 0;
    int pos;
    void **longest = NULL;

    struct mb_node *n = aux;

    //limit the bits to detect
    int limit;
    int limit_inside;
    int org_limit;
    
    limit = (cidr > leaf_pushing_bits) ? leaf_pushing_bits + 1 : cidr;
    org_limit = limit;
    while(limit>0) {

        stride = ip >> (LENGTH - STRIDE);
        limit_inside = (limit > STRIDE) ? STRIDE: limit;
        pos = find_overlap_in_node(n->internal, stride, &mask, limit_inside);

        if (pos != -1){
            curr_mask = step * STRIDE + mask;
            if (curr_mask < org_limit) {  
                final_mask = curr_mask;
                longest = (void**)n->child_ptr - count_ones(n->internal, pos) - 1;
            }
        }

        limit -= STRIDE;
        step ++;

        if (n->external & (1 << count_enl_bitmap(stride))) {
            //printf("%d %p\n", depth, n);
            n = (struct mb_node*)n->child_ptr + count_ones(n->external, count_enl_bitmap(stride));
            ip = (uint32_t)(ip << STRIDE);
        }
        else {
            break;
        }
    }
    //printf("limit %d, total_mask %d\n", limit, final_mask);

    if(final_mask != 0) {
        *nhi_over = *longest;
    }
    
    //printf("detect_prefix error\n");
    return final_mask;
}


//skip node as far as possible
//don't check the internal bitmap as you have to check

struct lazy_travel {
    struct mb_node *lazy_p;
    uint32_t stride;
};

struct lazy_travel lazy_mark[LEVEL]; 

static void * do_search_lazy(struct mb_node *n, uint32_t ip)
{
    uint8_t stride;
    int pos;
    void **longest = NULL;
    int travel_depth = -1;
//    int depth = 1;

    for (;;){
        stride = ip >> (LENGTH - STRIDE);

        if (n->external & (1 << count_enl_bitmap(stride))) {
            travel_depth++;
            lazy_mark[travel_depth].lazy_p = n;
            lazy_mark[travel_depth].stride = stride;
            n = (struct mb_node*)n->child_ptr + count_ones(n->external, count_enl_bitmap(stride));
            ip = (uint32_t)(ip << STRIDE);
//           depth ++;
        }
        else {

            pos = tree_function(n->internal, stride);
            if (pos != -1) {
                longest = (void**)n->child_ptr - count_ones(n->internal, pos) - 1;
                //already the longest match 
                goto out;
            }

            for(;travel_depth>=0;travel_depth --) {
                n = lazy_mark[travel_depth].lazy_p;
                stride = lazy_mark[travel_depth].stride;
                pos = tree_function(n->internal, stride);
                if (pos != -1) {
                    longest = (void**)n->child_ptr - count_ones(n->internal, pos) - 1;
                    //already the longest match 
                    goto out;
                }
            }
            //anyway we have to go out
            break;
        }
    }
//    printf("depth %d\n",depth);
out:
    return (longest == NULL)?NULL:*longest;

}


struct print_key {
    uint32_t ip;
    uint32_t cidr;
};


void print_ptr(struct print_key *key, void (*print_next_hop)(void *nhi), void *nhi)
{
    struct in_addr addr;
    addr.s_addr = htonl(key->ip);
    printf("%s/%d ", inet_ntoa(addr), key->cidr);

    if (print_next_hop)
        print_next_hop(nhi);

    printf("\n");
}

void print_mb_node_iter(struct mb_node *node, uint32_t ip, uint32_t left_bits, 
        uint32_t cur_cidr, void (*print_next_hop)(void *nhi)
        )
{
    int bit=0;
    int cidr=0;
    int stride = 0;
    uint32_t iptmp;
    int pos;
    void **nhi;
    struct mb_node *next;
    struct print_key key;

    //internal prefix first
    for (cidr=0;cidr<= STRIDE -1;cidr ++ ){
        for (bit=0;bit< (1<<cidr);bit++) {
            pos = count_inl_bitmap(bit,cidr);
            if (node->internal & (1 << pos)) {
                nhi = (void**)node->child_ptr - count_ones(node->internal, pos) - 1;
                iptmp = ip;
                iptmp |= bit << (left_bits - cidr);
                key.ip = iptmp;
                key.cidr = cur_cidr + cidr; 
                print_ptr(&key, print_next_hop, *nhi); 
            }
        }
    }

    for (stride = 0; stride < (1<<STRIDE); stride ++ ){
        pos = count_enl_bitmap(stride);
        if (node->external & (1 << pos)) {
            //ip |= stride << (left_bits - STRIDE);
            next = (struct mb_node *)node->child_ptr + count_ones(node->external, pos);
            print_mb_node_iter(next, ip | (stride << (left_bits - STRIDE)), left_bits - STRIDE, cur_cidr + STRIDE, print_next_hop); 
        }
    }
}



#ifdef DEBUG_MEMORY_FREE
int mem_destroy;
#endif



void destroy_subtrie(struct mb_node *node, void (*destroy_nhi)(void *))
{
    int bit;
    int cidr;
    int pos;
    void ** nhi = NULL;
    int stride;
    struct mb_node *next = NULL;

    
    int cnt_rules;
    struct mb_node *first = NULL;

    for (cidr=0;cidr<= STRIDE -1;cidr ++ ){
        for (bit=0;bit< (1<<cidr);bit++) {
            pos = count_inl_bitmap(bit,cidr);
            if (node->internal & (1 << pos)) {
                nhi = (void**)node->child_ptr - count_ones(node->internal, pos) - 1;
                if (destroy_nhi && *nhi != NULL) {
                    destroy_nhi(*nhi);
                }
                *nhi = NULL;
            }
        }
    }


    for (stride = 0; stride < (1<<STRIDE); stride ++ ){
        pos = count_enl_bitmap(stride);
        if (node->external & (1 << pos)) {
            next = (struct mb_node *)node->child_ptr + count_ones(node->external, pos);
            destroy_subtrie(next, destroy_nhi);
        }
    }

    cnt_rules = count_ones(node->internal, 16);
    first = POINT(node->child_ptr) - UP_RULE(cnt_rules);

#ifdef DEBUG_MEMORY_FREE
    int cnt = count_ones(node->internal, 16);
    mem_destroy += UP_RULE(cnt) * NODE_SIZE;
    cnt = count_ones(node->external, 16);
    mem_destroy += UP_CHILD(cnt) * NODE_SIZE;
#endif


    node->internal = 0;
    node->external = 0;
    node->child_ptr = NULL;

    free(first);

}


void init_aux_trie(struct mb_node *up_aux, struct mem_manager *mm)
{
    if(!up_aux)
        return;

    up_aux->internal = 0;
    up_aux->external = 0;
    up_aux->child_ptr = NULL;

    if(!mm)
        return;
#ifdef USE_MM
    init_mem_manager(mm, STRIDE, sizeof(struct mb_node));
#else
    memset(mm, 0, sizeof(*mm));
#endif
}

void destroy_aux_trie(struct mb_node *up_aux) {
    destroy_subtrie(up_aux, NULL);
}

void insert_aux_entry(struct mem_manager *mm, struct mb_node *up_aux, uint32_t ip, int cidr, uint32_t nhi) 
{
#if __x86_64__  == 1
    insert_entry(mm, up_aux, ip, cidr, (void*)(uint64_t)nhi);
#else
    insert_entry(mm, up_aux, ip, cidr, (void*)(uint32_t)nhi);
#endif
}

void delete_aux_entry(struct mem_manager *mm, struct mb_node *up_aux, uint32_t ip, int cidr) {
    delete_entry(mm, up_aux, ip, cidr, NULL);
}

void print_aux_prefix(struct mb_node *up_aux, void (*print_next_hop)(void *nhi))
{
    print_mb_node_iter(up_aux, 0, LENGTH, 0, print_next_hop); 
}

void mem_stat()
{
#ifdef DEBUG_MEMORY_ALLOC
    printf("mem alloc %d\n", mem);
#endif
#ifdef DEBUG_MEMORY_FREE
    printf("mem destory %d\n", mem_destroy);
#endif
}


