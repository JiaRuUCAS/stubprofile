#include "thread.h"
#include "funccnt.h"
#include "util.h"

static struct range idx_range = {
	.min = 0,
	.max = -1,
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

void funcc_count_thread_init(void)
{
	if (state != FUNCC_STATE_INIT)
		return;

	if (idx_range.max == UINT_MAX)
		return;

	fprintf(stderr, "start init thread\n");
		
	unsigned size = idx_range.max - idx_range.min + 1;

	fprintf(stderr, "number of counters: %u\n", size);
	thread.counters = (struct funcc_counter *)malloc(
					size * sizeof(struct funcc_counter));
	if (thread.counters == NULL) {
		fprintf(stderr, "Failed to allocate memory for counters\n");
		thread.state = FUNCC_STATE_ERROR;
		return;
	}

	memset(thread.counters, 0,
					size * sizeof(struct funcc_counter));
	thread.state = FUNCC_STATE_INIT;
	fprintf(stderr, "thread inited\n");
}

void funcc_count_init(unsigned min, unsigned max)
{
	uint8_t exp_state = FUNCC_STATE_UNINIT;

	idx_range.min = min;
	idx_range.max = max;

	if (__atomic_compare_exchange_n(&state, &exp_state,
							FUNCC_STATE_INIT, false,
							__ATOMIC_ACQUIRE, __ATOMIC_ACQUIRE))
		return;

	fprintf(stderr, "start initialization\n");

	funcc_count_thread_init();
	fprintf(stderr, "finish initializing: min %u, max %u\n",
					min, max);
}

static void dump_counters(void)
{
	unsigned int i = 0, len = idx_range.max - idx_range.min + 1;
	struct funcc_counter *cnt = NULL;

	for (i = 0; i < len; i++) {
		cnt = &thread.counters[i];
		if (cnt->pre_count) {
			fprintf(stderr, "func[%u]: pre %lu, post %lu\n",
							i + idx_range.min,
							cnt->pre_count, cnt->post_count);
		}
	}
}

void funcc_count_thread_exit(void)
{
	fprintf(stderr, "thread exit\n");
	if (thread.state == FUNCC_STATE_INIT) {
		if (thread.counters) {
			dump_counters();
			free(thread.counters);
			thread.counters = NULL;
		}
	}
}

void funcc_count_exit(void)
{
	fprintf(stderr, "exiting\n");

	if (state == FUNCC_STATE_INIT) {
		fprintf(stderr, "inited\n");
		funcc_count_thread_exit();
	}
	fprintf(stderr, "finished\n");
}
