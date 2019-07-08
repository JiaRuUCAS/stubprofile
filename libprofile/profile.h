#ifndef __PROFILE_H__
#define __PROFILE_H__

#define PROF_EVENT_MAX	10

struct profile_info {
	/* List of events */
	struct perf_event_attr evlist[PROF_EVENT_MAX];
	/* Number of events */
	uint8_t nb_event;
	/* log file */
	FILE *flog;
};

enum {
	PROF_LOG_ERROR = 0,
	PROF_LOG_INFO,
	PROF_LOG_WARN,
	PROF_LOG_DEBUG,
	PROF_LOG_NUM,
};

void profile_log(uint8_t level, const char *format, ...);

#define LOG_ERROR(format, ...) profile_log(PROF_LOG_ERROR, format, ##__VA_ARGS__)
#define LOG_INFO(format, ...) profile_log(PROF_LOG_INFO, format, ##__VA_ARGS__)
#define LOG_WARN(format, ...) profile_log(PROF_LOG_WARN, format, ##__VA_ARGS__)

#ifdef PF_DEBUG
#define LOG_DEBUG(format, ...) profile_log(PROF_LOG_DEBUG, format, ##__VA_ARGS__)
#else
#define LOG_DEBUG(format, ...)
#endif

void *profile_init(char *evlist, char *logfile);

#endif	// __PROFILE_H__
