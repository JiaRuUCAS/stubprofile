#include <stdarg.h>

#include "util.h"
#include "list.h"
#include "evlist.h"
#include "threadmap.h"
#include "profile.h"

struct prof_info globalinfo = {
	.evlist = NULL,
	.max_index = 0,
	.flog = NULL,
};

__thread struct prof_tinfo tinfo = {
	.pid = -1,
	.tid = -1,
	.state = PROF_STATE_UNINIT,
	.func_counters = NULL,
};

static char *log_tag[PROF_LOG_NUM] = {
	[PROF_LOG_ERROR] = "ERROR",
	[PROF_LOG_INFO] = "INFO",
	[PROF_LOG_WARN] = "WARN",
	[PROF_LOG_DEBUG] = "DEBUG",
};

void prof_log(uint8_t level, const char *format, ...)
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

static int __create_evlist(struct prof_info *info,
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
	return 0;

fail_destroy_evlist:
	prof_evlist__delete(evlist);
	return -1;
}

static void __init_thread(void)
{
	struct prof_evlist *evlist = globalinfo.evlist;
	struct prof_tinfo *info = &tinfo;

	info->pid = syscall(__NR_gettid);
	info->tid = thread_map__getindex(evlist->threads, info->pid);
	if (info->tid == -1) {
		LOG_ERROR("No thread %d found in threadmap", info->pid);
		info->state = PROF_STATE_ERROR;
		return;
	}

	// allocate counter
	info->func_counters = (uint64_t *)malloc(
					sizeof(uint64_t) * (globalinfo.max_index + 1));
	if (!info->func_counters) {
		LOG_ERROR("Failed to allocate memory for function counters,"
				  "its desired length is %u",
				  globalinfo.max_index + 1);
		info->state = PROF_STATE_ERROR;
		return;
	}
	// insert this data into global list
	INIT_LIST_HEAD(&info->node);
	list_add_tail(&info->node, &globalinfo.thread_data);

	tinfo.state = PROF_STATE_INIT;
	LOG_INFO("Thread %d index %d, address %p",
					tinfo.pid, tinfo.tid, info);
	return;
}

void *prof_init(char *evlist_str, char *logfile, unsigned max)
{
	int pid;
	struct prof_info *info = &globalinfo;
	char buf[32];
	FILE *fp = NULL;

	pid = getpid();
	tinfo.pid = syscall(__NR_gettid);
	INIT_LIST_HEAD(&info->thread_data);
	info->max_index = max;

	if (strlen(logfile) == 0)
		snprintf(buf, 32, "prof_%d.log", pid);
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

static void __destroy_thread_data(void)
{
	struct prof_tinfo *iter = NULL, *info = NULL;
	struct list_head *list = &globalinfo.thread_data;

	list_for_each_entry_safe(info, iter, list, node) {
		list_del_init(&info->node);
		// clear counters
		if (info->func_counters)
			free(info->func_counters);
		info->func_counters = NULL;
		info->state = PROF_STATE_UNINIT;
		LOG_INFO("Destroy data for thread, address %p",
						info);
	}
}

static void __destroy_evlist(void)
{
	struct prof_evlist *evlist = globalinfo.evlist;

	if (!evlist)
		return;

	prof_evlist__delete(evlist);
	globalinfo.evlist = NULL;
}

void prof_exit(void)
{
	LOG_INFO("Profile exit.");

	__destroy_evlist();
	__destroy_thread_data();

	if (globalinfo.flog) {
		fclose(globalinfo.flog);
		globalinfo.flog = NULL;
	}
}
