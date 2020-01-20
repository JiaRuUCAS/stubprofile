#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <sys/syscall.h>
#include <signal.h>
#include <pthread.h>

#include "./lib/random.h"

#define ELEM_NUM 4096

static uint8_t is_stop = 0;

static __always_inline unsigned long rdtsc(void)
{
	unsigned long low, high;

	asm volatile("rdtsc" : "=a" (low),
						   "=b" (high));

	return (low | high << 32);
}

void func()
{
	int *arr = NULL;

//	prof_count_pre(1);

	arr = (int*)malloc(ELEM_NUM * sizeof(int));
	memset(arr, 0, ELEM_NUM * sizeof(int));
	free(arr);
	arr = NULL;
}

void sighandler(int signo)
{
	if (signo == SIGINT) {
		is_stop = 1;
		fprintf(stdout, "recv sigint\n");
	}
}

void test(void *arg)
{
	unsigned idx = (unsigned)(uintptr_t)arg;

	struct timespec start, end;
	long nano = 0;
	int tid = 0;
	unsigned int cnt = 0;
//	ret = atoi(argv[1]);

	cpu_set_t mask;

	CPU_ZERO(&mask);
	CPU_SET(12 + idx, &mask);
	pthread_setaffinity_np(pthread_self(), sizeof(mask), &(mask));

	tid = syscall(SYS_gettid);
	fprintf(stdout, "thread %u (%d) is running on core %u\n",
					idx, tid, 12 + idx);

	while (!is_stop) {
		clock_gettime(CLOCK_MONOTONIC, &start);
		for(int i = 0; i < 100000; i++) {
			func();
			cnt++;
		}
		clock_gettime(CLOCK_MONOTONIC, &end);
		nano = (end.tv_nsec > start.tv_nsec) ? (end.tv_nsec - start.tv_nsec) : (start.tv_nsec - end.tv_nsec);
		fprintf(stdout, "Thread %u: %lu\n", idx, nano);

		test_random_ips("/home/jr/program/test-probe/ret_0");
//		usleep(1000);
	}

	fprintf(stdout, "Thread %u: func() runs %u times\n",
					idx, cnt);
}

#define THREAD_NUM 3

int main(int argc, char **argv)
{
	unsigned i = 0;
	unsigned long tids[THREAD_NUM];

	signal(SIGINT, sighandler);

	fprintf(stdout, "pid %u\n", (unsigned int)getpid());

	for (i = 0; i < THREAD_NUM; i++) {
		if (pthread_create(&tids[i], NULL, (void*)test,
								(void*)(uintptr_t)i) != 0) {
			fprintf(stderr, "failed to create thread %u\n", i);
			return 1;
		}
	}

	for (i = 0; i < THREAD_NUM; i++) {
		pthread_join(tids[i], NULL);
	}

	fprintf(stdout, "exit\n");
	return 0;
}
