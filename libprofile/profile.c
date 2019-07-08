#include <stdarg.h>

#include "util.h"
#include "profile.h"

struct profile_info globalinfo = {
	.nb_event = 0,
	.flog = NULL,
};

__thread int thread_pid = 0;

static char *log_tag[PROF_LOG_NUM] = {
	[PROF_LOG_ERROR] = "ERROR",
	[PROF_LOG_INFO] = "INFO",
	[PROF_LOG_WARN] = "WARN",
	[PROF_LOG_DEBUG] = "DEBUG",
};

void profile_log(uint8_t level, const char *format, ...)
{
	va_list va;
	FILE *fout = NULL;

	if (globalinfo.flog)
		fout = globalinfo.flog;
	else if (level == PROF_LOG_ERROR)
		fout = stderr;
	else
		fout = stdout;

	fprintf(fout, "[%s][%d]:", log_tag[level], thread_pid);
	va_start(va, format);
	vfprintf(fout, format, va);
	va_end(va);
	fprintf(fout, "\n");
}

void *profile_init(char *evlist, char *logfile)
{
	int pid;
	struct profile_info *info = &globalinfo;
	char buf[32];
	FILE *fp = NULL;

	pid = getpid();
	thread_pid = syscall(__NR_gettid);

	if (strlen(logfile) == 0)
		snprintf(buf, 32, "profile_%d.log", pid);
	else
		snprintf(buf, 32, "%s", logfile);

	fp = fopen(buf, "w");
	if (!fp) {
		LOG_ERROR("Failed to open log file %s, using stdout/stderr",
						buf);
		return (void *)-1;
	}
	LOG_INFO("Create log file %s", buf);
	info->flog = fp;

	LOG_INFO("Test log file");
	return (void *)0;
}
