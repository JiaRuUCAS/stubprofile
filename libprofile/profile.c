#include <stdarg.h>

#include "util.h"
#include "evlist.h"
#include "threadmap.h"
#include "profile.h"

struct profile_info globalinfo = {
	.evlist = NULL,
	.nb_event = 0,
	.flog = NULL,
};

__thread struct profile_tinfo tinfo = {
	.pid = -1,
	.tid = -1,
	.state = PROF_STATE_UNINIT,
};

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

	fprintf(fout, "[%s][%d]: ", log_tag[level], tinfo.pid);
	va_start(va, format);
	vfprintf(fout, format, va);
	va_end(va);
	fprintf(fout, "\n");
}

static int __create_evlist(struct profile_info *info,
				char *evlist_str, int pid)
{
	struct prof_evlist *evlist = NULL;

	evlist = prof_evlist__new();
	if (!evlist) {
		LOG_ERROR("Failed to create evlist (%s) for process %d",
						evlist_str, pid);
		return -1;
	}

	// parse and add events
	if (prof_evlist__add_from_str(evlist, evlist_str) < 0) {
		LOG_ERROR("Wrong event list %s", evlist_str);
		goto fail_destroy_evlist;
	}
	LOG_INFO("Add %d events", evlist->nr_entries);

	// create threadmap
	if (prof_evlist__create_threadmap(evlist, pid) < 0) {
		LOG_ERROR("Failed to create thread map");
		goto fail_destroy_evlist;
	}
	LOG_INFO("%d threads detected.",
					thread_map__nr(evlist->threads));

	info->evlist = evlist;
	info->nb_event = evlist->nr_entries;
	return 0;

fail_destroy_evlist:
	prof_evlist__delete(evlist);
	return -1;
}

static void __init_thread(void)
{
	struct prof_evlist *evlist = globalinfo.evlist;

	tinfo.pid = syscall(__NR_gettid);
	tinfo.tid = thread_map__getindex(evlist->threads, tinfo.pid);
	if (tinfo.tid == -1) {
		LOG_ERROR("No thread %d found in threadmap", tinfo.pid);
		tinfo.state = PROF_STATE_ERROR;
		return;
	}
	tinfo.state = PROF_STATE_INIT;
	LOG_INFO("Thread %d index %d", tinfo.pid, tinfo.tid);
}

void *profile_init(char *evlist_str, char *logfile)
{
	int pid;
	struct profile_info *info = &globalinfo;
	char buf[32];
	FILE *fp = NULL;

	pid = getpid();
	tinfo.pid = syscall(__NR_gettid);

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

	LOG_INFO("Create event list %s", evlist_str);
	if (__create_evlist(info, evlist_str, pid) < 0) {
		LOG_ERROR("Failed to create event list %s", evlist_str);
		goto fail_close_log;
	}

	// init current thread
	__init_thread();

	return (void *)0;

fail_close_log:
	fclose(info->flog);
	info->flog = NULL;

	return (void *)-1;
}

static void __destroy_evlist(void)
{
	struct prof_evlist *evlist = globalinfo.evlist;

	if (!evlist)
		return;

	prof_evlist__delete(evlist);
	globalinfo.evlist = NULL;
}

void profile_exit(void)
{
	LOG_INFO("Profile exit.");

	__destroy_evlist();

	if (globalinfo.flog) {
		fclose(globalinfo.flog);
		globalinfo.flog = NULL;
	}
}
