#ifndef _LIBFUNCC_THREAD_H_
#define _LIBFUNCC_THREAD_H_

enum {
	THREAD_STATE_IDLE = 0,
	THREAD_STATE_ERROR,
	THREAD_STATE_RUNNING,
};

struct thread_info {
	uint8_t tid;
	uint32_t pid;

	union {
		struct {
			uint8_t init 	:1;
			uint8_t quit 	:1;
			uint8_t running	:1;
			uint8_t error	:1;
			uint8_t reserved:4;
		};
		uint8_t state;
	};

	/* thread-local data */
	void *data;
};

#define THREAD_NB_MAX 64

struct thread_ctl {
	uint8_t state;
	uint8_t nb_thread;
	struct thread_info thread[THREAD_NB_MAX];
	/* Pointor to thread-local data initialization function */
	int (*thread_data_init)(void);
};

void __attribute__((constructor)) thread_global_init(void);
void __attribute__((destructor)) thread_global_exit(void);

#endif /* _LIBFUNCC_THREAD_H_  */
