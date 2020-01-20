#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <syscall.h>
#include <limits.h>

#include "thread.h"

static struct thread_ctl thread_ctl = {
	.state = THREAD_STATE_IDLE,
	.nb_thread = 0,
	.thread = {{
		.tid = UINT8_MAX,
		.pid = 0,
		.state = 0,
		.data = NULL,
	}},
	.thread_data_init = NULL,
};


/* Constructor of the library */
void thread_global_init(void)
{
	fprintf(stdout, "constructor\n");
//	thread_ctl.thread_data_init = funcc_thread_init;
}

void thread_global_exit(void)
{
	fprintf(stdout, "destructor\n");
//	struct thread_ctl *ctl = &thread_ctl;
}
