#include "thread.h"
#include "funccnt.h"
#include "util.h"

static struct range idx_range = {
	.min = 0,
	.max = UINT16_MAX,
};

#define FUNC_IDX(x) ((x)-idx_range.min)

void funcc_count_pre(unsigned int func)
{
	struct thread_info *thread = probe_get_thread();
	struct funcc_thread *info = NULL;

	if (thread->state == PROBE_STATE_UNINIT)
		probe_thread_init();
	else if (thread->state != PROBE_STATE_IDLE)
		return;

	thread->state = PROBE_STATE_RUNNING;
	info = (struct funcc_thread *)thread->data;
	info->counters[FUNC_IDX(func)].pre_count++;
	thread->state = PROBE_STATE_IDLE;
}

void funcc_count_post(unsigned int func)
{
	struct thread_info *thread = probe_get_thread();
	struct funcc_thread *info = NULL;

	if (thread->state == PROBE_STATE_UNINIT)
		probe_thread_init();
	else if (thread->state != PROBE_STATE_IDLE)
		return;

	thread->state = PROBE_STATE_RUNNING;
	info = (struct funcc_thread *)thread->data;
	info->counters[FUNC_IDX(func)].post_count++;
	thread->state = PROBE_STATE_IDLE;
}

/* Initialize the funcc-specified thread-local data */
void funcc_data_init(void)
{
	struct global_ctl *ctl = &global_ctl;
	struct thread_info *thread = probe_get_thread();
	struct funcc_thread *data = NULL;
	unsigned size = 0, nb_counter = 0;


	/* Check whether the global configuration is initialized */
	if (ctl->state != PROBE_STATE_RUNNING)
		return;

	/* Check whether this thread is already initialized */
	if (thread->state != PROBE_STATE_UNINIT)
		return;

	/* Check if the global configuration is valid */
	if (idx_range.max == UINT16_MAX)
		return;

	nb_counter = idx_range.max - idx_range.min + 1;
	size = sizeof(struct funcc_thread)
			+ nb_counter * sizeof(struct funcc_counter);

	data = (struct funcc_thread *)malloc(size);
	if (!data) {
		LOG_ERROR(thread->pid,
				"Failed to allocate memory for counters\n");
		thread->state = PROBE_STATE_ERROR;
		return;
	}

	memset(data, 0, size);

	thread->data = data;
	thread->state = PROBE_STATE_IDLE;

	LOG_INFO(thread->pid, "Initialize thread %u, with %u counters",
				   thread->tid, size);
}

/* This function must be called manually after the global
 * initialization and before the initialization of each thread.
 * It configures the range of the indices of target functions.
 */
void funcc_init(unsigned min, unsigned max)
{
	if (global_ctl.state != PROBE_STATE_UNINIT)
		return;

	idx_range.min = min;
	idx_range.max = max;

	global_ctl.state = PROBE_STATE_RUNNING;

	LOG_INFO(global_ctl.pid, "Initialize funcc, min %u, max %u",
					idx_range.min, idx_range.max);

	probe_thread_init();
}

static void dump_counters(int pid, struct funcc_counter *cnt)
{
	unsigned int i = 0, len = idx_range.max - idx_range.min + 1;

	for (i = 0; i < len; i++) {
		LOG_INFO(pid, "func[%u]: pre %lu, post %lu",
						i + idx_range.min,
						cnt->pre_count, cnt->post_count);
	}
}

void funcc_data_free(void)
{
	struct thread_info *thread = probe_get_thread();
	struct funcc_thread *data =
			(struct funcc_thread *)thread->data;

	LOG_INFO(thread->pid, "Release thread-local data");
	if (data) {
		dump_counters(thread->pid, data->counters);
		free(data);
		thread->data = NULL;
	}
}
