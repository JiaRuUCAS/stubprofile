#ifndef __PROFILE_H__
#define __PROFILE_H__

#define PROF_EVENT_MAX	10

#include "list.h"

struct prof_info {
	/* List of events */
	struct prof_evlist *evlist;
	/* Max index of traced function */
	unsigned max_index;
	/* log file */
	FILE *flog;
	/* list of per-thread data.
	 * It's used for safely termination.
	 */
	struct list_head thread_data;
};

enum {
	PROF_STATE_UNINIT = 0,
	PROF_STATE_INIT,
	PROF_STATE_STOP,
	PROF_STATE_RUNNING,
	PROF_STATE_ERROR,
};

// per-thread data
struct prof_tinfo {
	struct list_head node;

	int pid;
	int tid;
	uint8_t state;

	/* Per-function counters
	 * Its length is (max_index + 1)
	 */
	uint64_t *func_counters;
};

enum {
	PROF_LOG_ERROR = 0,
	PROF_LOG_INFO,
	PROF_LOG_WARN,
	PROF_LOG_DEBUG,
	PROF_LOG_NUM,
};

void prof_log(uint8_t level, const char *format, ...);

#define LOG_ERROR(format, ...) prof_log(PROF_LOG_ERROR, format, ##__VA_ARGS__)
#define LOG_INFO(format, ...) prof_log(PROF_LOG_INFO, format, ##__VA_ARGS__)
#define LOG_WARN(format, ...) prof_log(PROF_LOG_WARN, format, ##__VA_ARGS__)

#ifdef PF_DEBUG
#define LOG_DEBUG(format, ...) prof_log(PROF_LOG_DEBUG, format, ##__VA_ARGS__)
#else
#define LOG_DEBUG(format, ...)
#endif

void *prof_init(char *evlist_str, char *logfile, unsigned max_id);

void prof_exit(void);

#endif	// __PROFILE_H__
