#include "bitmap.h"
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include "ts.h"

#include <unistd.h>
#include <sys/ioctl.h>
#include <asm/unistd.h>

#include "random.h"

struct ip_rules{
    uint32_t ip;
    uint32_t cidr;
    uint32_t test_ip;
    uint32_t key;
};

#if 0
static __always_inline unsigned long rdtsc(void)
{
	unsigned long low, high;

	asm volatile("rdtsc" : "=a" (low),
						   "=b" (high));

	return (low | high << 32);
}
#endif

#define RAND_SIZE  1000000 

//werid things: a 32-bit interger left shift 32 will equals to itself
//uint32_t i;
//i == i<<32
//
void test_random_ips(char *file)
{
	FILE *fp = fopen(file,"r");
    if (fp == NULL) {
		fprintf(stdout, "failed to open %s\n", file);
		return;
//        pthread_exit(NULL);
	}

    char *line = NULL;
    ssize_t read = 0;
    size_t len = 0;
    int cnt = 0;
    int i = 0;
    uint32_t ip = 0, cidr;
    struct ts_tree tree, *trie;
	char buf[128], *pbuf;
//	cpu_set_t mask;
    
//	CPU_ZERO(&mask);
//	CPU_SET(1, &mask);
//	pthread_setaffinity_np(pthread_self(), sizeof(mask), &(mask));

	trie = &tree;
	memset(trie, 0, sizeof(struct ts_tree));
    ts_init(trie);

    uint32_t *random_ips = (uint32_t *)malloc(RAND_SIZE * sizeof(uint32_t));
    uint32_t *ips = (uint32_t *)malloc(RAND_SIZE * sizeof(uint32_t));
    uint32_t *cidrs = (uint32_t *)malloc(RAND_SIZE * sizeof(uint32_t));

    while((read = getline(&line, &len, fp)) != -1){
        if (i & 0x01) {
            cidr = atoi(line);
            ip = ip & (0xffffffff << (32-cidr));
			ips[i/2] = ip;
			cidrs[i/2] = cidr;
        }
        else {
            ip = inet_network(line);
        }
        
        i++;
    }
    cnt = i/2;
//	printf("finish read\n");

    for (i=0;i<RAND_SIZE;i++){
        random_ips[i] = ips[random()%cnt];
    }

    struct timespec tp_b;
    struct timespec tp_a;
	long nano = 0;
//	uint64_t cyc = 0, cyc2 = 0;
//	int nwriten = 0;
	pbuf = buf;

	ts_insert_prefix(trie, ips[0], cidrs[0], (0));

	clock_gettime(CLOCK_MONOTONIC, &tp_b);
//	cyc = rdtsc();
	for (i = 1; i < cnt; i++)
        ts_insert_prefix(trie, ips[i], cidrs[i],(i));
//	cyc2 = rdtsc();
//	fprintf(stdout, "%lu,", cyc2 - cyc);
	clock_gettime(CLOCK_MONOTONIC, &tp_a);
//	printf("insert: \n");
    nano = (tp_a.tv_nsec > tp_b.tv_nsec) ? (tp_a.tv_nsec -tp_b.tv_nsec) : (tp_a.tv_nsec - tp_b.tv_nsec + 1000000000ULL);
//    printf("sec %ld, nano %ld\n", tp_b.tv_sec, tp_b.tv_nsec);
//    printf("sec %ld, nano %ld\n", tp_a.tv_sec, tp_a.tv_nsec);
//    printf("nano %ld\n", nano);
	pbuf += sprintf(pbuf, "%ld,", nano);
    //int j;
    //for (j=0;j<10;j++){
	ts_search(trie, random_ips[0]);

    clock_gettime(CLOCK_MONOTONIC, &tp_b);
//	cyc = rdtsc();
    for (i=0;i<RAND_SIZE;i++){
        ts_search(trie, random_ips[i]);
        //hash_trie_search(random_ips[i]);
        //compact_search(random_ips[i]);
    }
//	cyc2 = rdtsc();
 //	fprintf(stdout, "%lu,", cyc2 - cyc);
   clock_gettime(CLOCK_MONOTONIC, &tp_a);
//	printf("search: \n");
    nano = (tp_a.tv_nsec > tp_b.tv_nsec) ? (tp_a.tv_nsec -tp_b.tv_nsec) : (tp_a.tv_nsec - tp_b.tv_nsec + 1000000000ULL);
//    printf("sec %ld, nano %ld\n", tp_b.tv_sec, tp_b.tv_nsec);
//    printf("sec %ld, nano %ld\n", tp_a.tv_sec, tp_a.tv_nsec);
//    printf("nano %ld\n", nano);
	pbuf += sprintf(pbuf, "%ld,", nano);
    //}

//	cyc = rdtsc();
	ts_delete_prefix(trie, ips[0], cidrs[0]);
	clock_gettime(CLOCK_MONOTONIC, &tp_b);
	for (i = 1; i < cnt; i++)
		ts_delete_prefix(trie, ips[i], cidrs[i]);
//	cyc2 = rdtsc();
//	fprintf(stdout, "%lu\n", cyc2 - cyc);
	clock_gettime(CLOCK_MONOTONIC, &tp_a);
//	printf("delete: \n");
    nano = (tp_a.tv_nsec > tp_b.tv_nsec) ? (tp_a.tv_nsec -tp_b.tv_nsec) : (tp_a.tv_nsec - tp_b.tv_nsec + 1000000000ULL);
//    printf("sec %ld, nano %ld\n", tp_b.tv_sec, tp_b.tv_nsec);
//    printf("sec %ld, nano %ld\n", tp_a.tv_sec, tp_a.tv_nsec);
//    printf("nano %ld\n", nano);
	sprintf(pbuf, "%ld\n", nano);
	fprintf(stdout, "%s", buf);

    free(random_ips);
	free(ips);
	free(cidrs);
    fclose(fp);
	ts_destroy(trie);

//	pthread_exit(NULL);
}
