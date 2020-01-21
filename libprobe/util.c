#include <time.h>
#include <stdarg.h>

#include "util.h"
#include "thread.h"

static char *log_tag[PROBE_LOG_NUM] = {
	[PROBE_LOG_ERROR] = "ERROR",
	[PROBE_LOG_INFO] = "INFO",
	[PROBE_LOG_WARN] = "WARN",
	[PROBE_LOG_DEBUG] = "DEBUG",
};

void probe_log(int pid, uint8_t level, const char *format, ...)
{
	va_list va;
	FILE *fout = NULL;
	time_t timer;
	struct tm *timeinfo;
	char buf[16] = {'\0'};

	if (global_ctl.flog)
		fout = global_ctl.flog;
	else if (level != PROBE_LOG_INFO)
		fout = stderr;
	else
		fout = stdout;

	timer = time(NULL);
	timeinfo = localtime(&timer);
	strftime(buf, 16, "%D %R", timeinfo);
	fprintf(fout, "[%s][%s][T%d]: ", buf, log_tag[level], pid);
	va_start(va, format);
	vfprintf(fout, format, va);
	va_end(va);
	fprintf(fout, "\n");
	fflush(fout);
}
