#ifndef _LIBPROBE_THREAD_H_
#define _LIBPROBE_THREAD_H_

#include <stdbool.h>

enum {
	PROBE_STATE_UNINIT = 0,
	PROBE_STATE_IDLE,
	PROBE_STATE_ERROR,
	PROBE_STATE_RUNNING,
	PROBE_STATE_EXIT,
};

struct thread_info {
	uint8_t tid;
	int pid;

	uint8_t state;

	/* thread-local data */
	void *data;
};

#define PROBE_THREAD_NB_MAX 64

struct global_ctl {
	/* global state */
	uint8_t state;

	/* thread infomations */
	uint8_t nb_thread;
	uint8_t nb_exit;
	struct thread_info *threads[PROBE_THREAD_NB_MAX];

	/* Pointor to thread-local data initialization function */
	void* (*thread_data_init)(void);
	/* Pointor to thread-local data destructor */
	void (*thread_data_exit)(void *data);
	/* Pointer to global destructor */
	void (*global_exit)(void);

	/* log file */
	FILE *flog;
};

extern struct global_ctl global_ctl;

void __attribute__((constructor)) thread_global_init(void);
void __attribute__((destructor)) thread_global_exit(void);

LIB_EXPORT bool probe_thread_init(void);
LIB_EXPORT unsigned probe_exit(void);

#endif /* _LIBPROBE_THREAD_H_  */
