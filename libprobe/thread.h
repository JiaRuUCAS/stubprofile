#ifndef _LIBPROBE_THREAD_H_
#define _LIBPROBE_THREAD_H_

#include "util.h"

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

struct thread_ctl {
	/* global state */
	uint8_t state;

	/* thread infomations */
	uint8_t nb_thread;
	uint8_t nb_exit;
	struct thread_info *threads[PROBE_THREAD_NB_MAX];

	/* Pointor to thread-local data initialization function */
	void* (*thread_data_init)(void);
	/* Pointor to thread-local data destructor */
	void (*thread_data_free)(void);
	/* Pointer to global destructor */
	void (*global_exit)(void);
};

extern struct global_ctl global_ctl;

/* Constructor
 * This function will be called automatically during the program
 * loading.
 */
void __attribute__((constructor)) thread_global_init(void);


/* Destructor
 * If it is not completely destroyed, destroy it. This function is
 * called automatically at the end of the process. It means when
 * it is called, all the threads except the main thread, already
 * existed. So it's safe to use 'probe_exit()' directly. And since
 * all threads are idle, they can be destroyed by only one call.
 * For dynamic instrumentation, the tracer is responsible to
 * safely release resources by invoking 'probe_exit()' manually.
 * For the static case, all the resources should be released by
 * this function.
 */
void __attribute__((destructor)) thread_global_exit(void);

/* Get thread-local structure */
struct thread_info *probe_get_thread(void);

#endif /* _LIBPROBE_THREAD_H_  */
