/*
 *
 * DKM: Zur Dokumentation werden nur die modifizierten Funktionen ausgedruckt.
 *
 */

/*
 * Some day this will be a full-fledged user tracking system..
 * Right now it is only used to track how many processes a
 * user has, but it has the potential to track memory usage etc.
 */
struct user_struct;

struct task_struct {
/* these are hardcoded - don't touch */
	volatile long state;	/* -1 unrunnable, 0 runnable, >0 stopped */
	unsigned long flags;	/* per process flags, defined below */
	int sigpending;
	mm_segment_t addr_limit;	/* thread address space:
					 	0-0xBFFFFFFF for user-thead
						0-0xFFFFFFFF for kernel-thread
					 */
	struct exec_domain *exec_domain;
	long need_resched;

/* für die Dokumentation gekürzt */

	struct signal_queue *sigqueue, **sigqueue_tail;
	unsigned long sas_ss_sp;
	size_t sas_ss_size;
	
/* Thread group tracking */
   	u32 parent_exec_id;
   	u32 self_exec_id;

/* dkm specific entries */
	struct dkm_struct *dkm;
};
