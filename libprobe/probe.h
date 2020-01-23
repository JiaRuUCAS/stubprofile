#ifndef _LIBPROBE_H_
#define _LIBPROBE_H_

#include "util.h"

struct global_info {
	/* process ID */
	int pid;
	/* File pointer to output log file
	 * If it is NULL, the standard stdout and stderr will be used
	 */
	FILE *flog;
	/* The range of target functions' indices */
	struct range idx_range;
	/* Flags */
	union {
		struct {
			/* Whether the funccnt is used */
			uint8_t use_funcct	: 1;
			/* Whether the pmu is used */
			uint8_t use_pmu		: 1;
			uint8_t reserved	: 6;
		};
		uint8_t flags;
	};
	/* */
};

extern struct global_info global_info;

/* Check the correctness of event list string.
 * This function is used by the instrumentation tool, and is not
 * instrumentable.
 */
LIB_EXPORT int probe_check_evlist(const char *evlist);

/* Global initialization */
LIB_EXPORT void probe_init();

/* Destroy all idle threads. If all threads in use are idle and
 * destroyed, it returns 0. Otherwise, it returns 1. The 'nb_exit'
 * in 'global_ctl' will records the number of threads that are
 * destroyed.
 * This function must be called when the thread-local data is not
 * in use. For example, it could be called in global destructor,
 * and when the execution is stopped.
 * When the function is called, the processing of other threads
 * may stop inside the probe (PROBE_STATE_RUNNING). In this case,
 * these threads cannot be released right now. We set the global
 * state to PROBE_STATE_EXIT, to inform all threads that, "don't
 * enter the probe anymore". Then the execution should be resumed.
 * After a short period, stop it again and try "probe_exit()".
 * Repeat these operations until all threads are destroyed.
 */
LIB_EXPORT unsigned probe_exit(void);

/* Per-thread initialization
 * This function will be called automatically at the first time
 * the process executes the probe. Before the execution of this
 * function, the probe-specified global initialization must be
 * done (the global state should be PROBE_STATE_RUNNING).
 */
LIB_EXPORT void probe_thread_init(void);





#endif /* _LIBPROBE_H */
