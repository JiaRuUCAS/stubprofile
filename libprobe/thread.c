#include "util.h"
#include "thread.h"

struct global_ctl global_ctl = {
	.state = PROBE_STATE_UNINIT,
	.nb_thread = 0,
//	.thread = {{
//		.tid = UINT8_MAX,
//		.pid = 0,
//		.state = 0,
//		.data = NULL,
//	}},
	.threads = {NULL},
	.thread_data_init = NULL,
	.flog = NULL,
};

static __thread struct thread_info localinfo = {
	.tid = UINT8_MAX,
	.pid = -1,
	.state = PROBE_STATE_UNINIT,
	.data = NULL,
};

/* Constructor
 * The global state will not be updated to PROBE_STATE_IDLE. It
 * should be updated after the initialization of specified probe
 * scheme (funcc or pmuprobe).
 */
void thread_global_init(void)
{
	struct global_ctl *ctl = &global_ctl;
	int pid = -1;

	pid = getpid();

	/* open log file */
	ctl->flog = fopen(PROBE_LOGFILE, "a");
	if (ctl->flog == NULL) {
		LOG_WARN(pid,
				"Failed to open log file, use stdout/stderr");
	}

	LOG_INFO(pid, "Global initialization");

//	global_ctl.thread_data_init = funcc_thread_init;
//	ctl->state = PROBE_STATE_IDLE;
}

/* destructor */
void thread_global_exit(void)
{
	if (global_ctl.state != PROBE_STATE_UNINIT) {
		probe_exit();
		LOG_INFO(0, "Destroy %u(%u) threads",
						global_ctl.nb_exit,
						global_ctl.nb_thread);
	}
}

/* This function must be called when the thread-local data is not
 * in use. For example, it could be called in global destructor,
 * and when the execution is stopped.
 * It tries to destroy all idle threads. If all threads are idle
 * and destroyed, it returns 0. Otherwise, it returns 1. The
 * 'nb_exit' in 'global_ctl' will records the number of threads
 * that are destroyed.
 */
unsigned probe_exit(void)
{
	int pid = -1;
	uint8_t i = 0, nb_thread, nb_exit;
	struct global_ctl *ctl = &global_ctl;
	struct thread_info *thread = NULL;

	ctl->state = PROBE_STATE_EXIT;
	if (ctl->nb_exit == ctl->nb_thread)
		return 0;

	pid = getpid();

	/* destroy thread-local data */
	for (i = 0; i < ctl->nb_thread; i++) {
		thread = ctl->threads[i];

		if (thread == NULL || thread->state == PROBE_STATE_IDLE)
			continue;

		if (ctl->thread_data_exit) {
			ctl->thread_data_exit(thread->data);
			thread->data = NULL;
		}
		thread->state = PROBE_STATE_UNINIT;
		ctl->threads[i] = NULL;
		ctl->nb_exit++;
		LOG_INFO(pid, "Thread %u (%d) exits.",
						thread->tid, thread->pid);
	}

	/* check whether all threads are destroyed. */
	if (ctl->nb_exit >= ctl->nb_thread) {
		if (ctl->global_exit)
			ctl->global_exit();

		LOG_INFO(pid, "Finished.");

		/* close log file */
		if (ctl->flog) {
			fclose(ctl->flog);
			ctl->flog = NULL;
		}

		ctl->state = PROBE_STATE_UNINIT;
		return 0;
	}
	else {
		LOG_INFO(pid, "%u(%u) threads are destroyed",
						ctl->nb_exit, ctl->nb_thread);
		return 1;
	}
}

/* Per-thread initialization */
void probe_thread_init(void)
{
	struct global_ctl *ctl = &global_ctl;
	struct thread_info *thread = &localinfo;

	/* check global state */
	if (ctl->state != PROBE_STATE_RUNNING &&
					ctl->state != PROBE_STATE_IDLE)
		return;

	/* check thread state, it must be UNINIT. */
	if (thread->state != PROBE_STATE_UNINIT)
		return;

	/* check tid. If tid is invalid, generate it safely */
	if (thread->tid > ctl->nb_thread) {
		/* if tid is invalid, generate it safely. */
		thread->tid = __atomic_fetch_add(&ctl->nb_thread, 1,
						__ATOMIC_SEQ_CST);
		ctl->threads[thread->tid] = thread;
	}

	/* get pid */
	thread->pid = syscall(SYS__gettid);

	LOG_INFO(thread->pid, "This is thread %u", thread->tid);

	/* create thread-local data */
	if (ctl->thread_data_init) {
		thread->data = ctl->thread_data_init();
		if (!thread->data) {
			LOG_ERROR(thread->pid, "Failed to init thread-local"
							"data, nb_exit++");
			thread->state = PROBE_STATE_ERROR;
			ctl->nb_exit ++;
			return;
		}
	}

	thread->state = PROBE_STATE_IDLE;
	LOG_INFO(thread->pid, "Finish initialization");
}

struct thread_info *probe_get_thread(void)
{
	return &localinfo;
}
