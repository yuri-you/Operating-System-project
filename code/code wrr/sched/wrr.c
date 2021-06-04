#include "sched.h"

#include <linux/slab.h>
#include <linux/limits.h>
#define for_each_sched_wrr_entity(wrr_se) \
	for (; wrr_se; wrr_se = NULL)
static inline int on_wrr_rq(struct sched_wrr_entity *wrr_se)
{
	return !list_empty(&wrr_se->run_list);
}
static inline struct task_struct *wrr_task_of(struct sched_wrr_entity *wrr_se)
{
	return container_of(wrr_se, struct task_struct, wrr);
}
static inline struct wrr_rq *wrr_rq_of_se(struct sched_wrr_entity *wrr_se)
{
	struct task_struct *p=wrr_task_of(wrr_se);
	struct rq *rq=task_rq(p);
	return &rq->wrr;
}
static inline int wrr_rq_throttled(struct wrr_rq *wrr_rq)
{
	return wrr_rq->wrr_throttled;
}
static inline struct wrr_rq *group_wrr_rq(struct sched_wrr_entity *wrr_se)
{
	return NULL;
}
void init_wrr_rq(struct wrr_rq*wrr_rq,struct rq*rq){
	struct list_head *q=&wrr_rq->wrr;
	INIT_LIST_HEAD(q);
	wrr_rq->wrr_time=0;
	wrr_rq->wrr_throttled=0;
	wrr_rq->wrr_runtime=0;
	raw_spin_lock_init(&wrr_rq->wrr_runtime_lock);
}
// static void watchdog(struct rq *rq, struct task_struct *p)
// {
// 	unsigned long soft, hard;

// 	/* max may change after cur was read, this will be fixed next tick */
// 	soft = task_rlimit(p, RLIMIT_RTTIME);
// 	hard = task_rlimit_max(p, RLIMIT_RTTIME);

// 	if (soft != RLIM_INFINITY) {
// 		unsigned long next;

// 		p->rt.timeout++;
// 		next = DIV_ROUND_UP(min(soft, hard), USEC_PER_SEC/HZ);
// 		if (p->rt.timeout > next)
// 			p->cputime_expires.sched_exp = p->se.sum_exec_runtime;
// 	}
// }
static inline void list_add_leaf_wrr_rq(struct wrr_rq *wrr_rq){}//add one wrr to 
static inline void list_del_leaf_wrr_rq(struct wrr_rq *wrr_rq){}
static inline void sched_wrr_avg_update(struct rq *rq, u64 wrr_delta) {}
static inline
void inc_wrr_tasks(struct sched_wrr_entity *wrr_se, struct wrr_rq *wrr_rq)
{

	wrr_rq->wrr_nr_running++;//the number in wrr_rq increase
	// inc_rt_prio(rt_rq, prio);
	// inc_rt_migration(rt_se, rt_rq);
	// inc_rt_group(rt_se, rt_rq);
}
void dec_wrr_tasks(struct sched_wrr_entity *wrr_se, struct wrr_rq *wrr_rq)
{
	wrr_rq->wrr_nr_running--;

	// dec_rt_prio(rt_rq, rt_se_prio(rt_se));
	// dec_rt_migration(rt_se, rt_rq);
	// dec_rt_group(rt_se, rt_rq);
}
static inline u64 sched_wr_runtime(struct wrr_rq *wrr_rq)
{
	return wrr_rq->wrr_runtime;
}
static void update_curr_wrr(struct rq *rq)
{
	struct task_struct *curr = rq->curr;
	struct sched_wrr_entity *wrr_se = &curr->wrr;
	// struct wrr_rq *wrr_rq = wrr_rq_of_se(wrr_se);
	u64 delta_exec;

	if (curr->sched_class != &wrr_sched_class)
		return;//this task not use wrr

	delta_exec = rq->clock_task - curr->se.exec_start;
	if (unlikely((s64)delta_exec < 0))
		delta_exec = 0;

	schedstat_set(curr->se.statistics.exec_max,
		      max(curr->se.statistics.exec_max, delta_exec));

	curr->se.sum_exec_runtime += delta_exec;
	account_group_exec_runtime(curr, delta_exec);

	curr->se.exec_start = rq->clock_task;
	cpuacct_charge(curr, delta_exec);

	sched_wrr_avg_update(rq, delta_exec);

	// if (!rt_bandwidth_enabled())
	// 	return;

	// for_each_sched_wrr_entity(wrr_se) {
	// 	wrr_rq = wrr_rq_of_se(wrr_se);

	// 	if (sched_wrr_runtime(rt_rq) != RUNTIME_INF) {
	// 		raw_spin_lock(&rt_rq->rt_runtime_lock);
	// 		rt_rq->rt_time += delta_exec;
	// 		if (sched_rt_runtime_exceeded(rt_rq))
	// 			resched_task(curr);
	// 		raw_spin_unlock(&rt_rq->rt_runtime_lock);
	// 	}
	// }
}



static void __enqueue_wrr_entity(struct sched_wrr_entity *wrr_se, bool head)
{
	struct wrr_rq *wrr_rq = wrr_rq_of_se(wrr_se);//rq is belongs to
	struct list_head *q = &wrr_rq->wrr;//the task node

	/*
	 * Don't enqueue the group if its throttled, or when empty.
	 * The latter is a consequence of the former when a child group
	 * get throttled and the current group doesn't have any other
	 * active members.
	 */
	// if (group_rq && (rt_rq_throttled(group_rq) || !group_rq->rt_nr_running))
	// 	return;

	// if (!wrr_rq->wrr_nr_running)
	// 	list_add_leaf_wrr_rq(wrr_rq);
	struct task_struct *p=wrr_task_of(wrr_se);
	//obtain the task_struct that wrr_rq as its wrr variance.
	if (head){//add at head
		list_add(&wrr_se->run_list, q);//wrr_se->run_list is head of readyqueue
		//printk("Process %d has enqueued at the list head!\n",p->pid);
	}
	else{
		list_add_tail(&wrr_se->run_list, q);
		//printk("Process %d has enqueued at the list tail!\n",p->pid);
	}
	
	inc_wrr_tasks(wrr_se, wrr_rq);
}
static void __dequeue_wrr_entity(struct sched_wrr_entity *wrr_se)
{
	int times =0;
	struct wrr_rq *wrr_rq = wrr_rq_of_se(wrr_se);
	list_del_init(&wrr_se->run_list);
	dec_wrr_tasks(wrr_se, wrr_rq);
	if (!wrr_rq->wrr_nr_running){
		list_del_leaf_wrr_rq(wrr_rq);
	}
}

static void dequeue_wrr_stack(struct sched_wrr_entity *wrr_se)
{
	struct sched_wrr_entity *back = NULL;
	for_each_sched_wrr_entity(wrr_se) {
		wrr_se->back = back;
		back = wrr_se;
	}
	for (wrr_se = back; wrr_se; wrr_se = wrr_se->back) {
		if(on_wrr_rq(wrr_se)){//if in ready queue
			__dequeue_wrr_entity(wrr_se);//remove
		}
	}
}
static void enqueue_wrr_entity(struct sched_wrr_entity *wrr_se, bool head)
{	
	dequeue_wrr_stack(wrr_se);
	for_each_sched_wrr_entity(wrr_se){
		__enqueue_wrr_entity(wrr_se, head);
	}
}
static void dequeue_wrr_entity(struct sched_wrr_entity *wrr_se)
{
	dequeue_wrr_stack(wrr_se);
	for_each_sched_wrr_entity(wrr_se) {
		struct wrr_rq *wrr_rq = group_wrr_rq(wrr_se);

		if (wrr_rq && wrr_rq->wrr_nr_running)
			__enqueue_wrr_entity(wrr_se, false);//push at tail
	}
}

static void
enqueue_task_wrr(struct rq *rq, struct task_struct *p, int flags)
{
	struct sched_wrr_entity *wrr_se = &p->wrr;//the entity that p belongs to

	if (flags & ENQUEUE_WAKEUP)
		wrr_se->timeout = 0;
	enqueue_wrr_entity(wrr_se, flags & ENQUEUE_HEAD);
	inc_nr_running(rq);//the number in whole rq increase
}

static void dequeue_task_wrr(struct rq *rq, struct task_struct *p, int flags)
{
	struct sched_wrr_entity *wrr_se = &p->wrr;

	update_curr_wrr(rq);
	dequeue_wrr_entity(wrr_se);

	dec_nr_running(rq);
}
static void
requeue_wrr_entity(struct wrr_rq *wrr_rq, struct sched_wrr_entity *wrr_se, int head)
{
	if (on_wrr_rq(wrr_se)) {
		struct list_head *queue = &wrr_rq->wrr;

		if (head)
			list_move(&wrr_se->run_list, queue);
		else
			list_move_tail(&wrr_se->run_list, queue);
	}
}
static void requeue_task_wrr(struct rq *rq, struct task_struct *p, int head)
{
	struct sched_wrr_entity *wrr_se = &p->wrr;
	struct wrr_rq *wrr_rq;

	for_each_sched_wrr_entity(wrr_se) {
		wrr_rq = wrr_rq_of_se(wrr_se);
		requeue_wrr_entity(wrr_rq, wrr_se, head);
	}
}
static void yield_task_wrr(struct rq *rq)
{
	requeue_task_wrr(rq, rq->curr, 0);
}


static void check_preempt_curr_wrr(struct rq *rq, struct task_struct *p, int flags)
{
	//no preempt in wrr
}
static struct sched_wrr_entity *pick_next_wrr_entity(struct rq *rq,
						   struct wrr_rq *wrr_rq)
{
	struct sched_wrr_entity *next = NULL;
	struct list_head *queue=&wrr_rq->wrr;
	next = list_entry(queue->next, struct sched_wrr_entity, run_list);

	return next;
}
static struct task_struct *pick_next_task_wrr(struct rq *rq)
{
	struct sched_wrr_entity *wrr_se;
	struct task_struct *p;
	struct wrr_rq *wrr_rq=&rq->wrr;


	if (!wrr_rq->wrr_nr_running)//not this schedule method
		return NULL;

	do {
		wrr_se = pick_next_wrr_entity(rq, wrr_rq);
		BUG_ON(!wrr_se);
		wrr_rq = group_wrr_rq(wrr_se);
	} while (wrr_rq);

	p = wrr_task_of(wrr_se);
	p->se.exec_start = rq->clock_task;

	return p;
}
static void put_prev_task_wrr(struct rq *rq, struct task_struct *p)
{	
}
static void set_curr_task_wrr(struct rq *rq)
{
	struct task_struct *p = rq->curr;
	p->se.exec_start = rq->clock_task;
}
char group_path[PATH_MAX];
static char *task_group_path(struct task_group *tg)
{
	if (autogroup_path(tg, group_path, PATH_MAX))
		return group_path;

	/*
	 * May be NULL if the underlying cgroup isn't fully-created yet
	 */
	if (!tg->css.cgroup) {
		group_path[0] = '\0';
		return group_path;
	}
	cgroup_path(tg->css.cgroup, group_path, PATH_MAX);
	return group_path;
}
static int foreground(struct task_struct *p){
	char *path=task_group_path(p->sched_task_group);
	const char* fore="/";
	const char* back="/bg_non_interactive";
	if(strcmp(path,fore)==0){
		printk("foreground\n");
		return 1;
	}
	else{
		if(strcmp(path,back)==0){
			printk("background\n");
			return 0;
		}
		else printk("Not foregroud or background\n");
		return 0;
	}
}
static void task_tick_wrr(struct rq *rq, struct task_struct *p, int queued)
{

	struct sched_rt_entity *wrr_se = &p->wrr;

	update_curr_wrr(rq);
	/*
	 * WRR
	 */
	if (p->policy != SCHED_WRR)
		return;

	if (--p->wrr.time_slice)
		return;
	if(foreground(p)){
		p->wrr.time_slice=F_WRR_TIMESLICE;
	}
	else{
		p->wrr.time_slice=B_WRR_TIMESLICE;
	}
	/*
	 * Requeue to the end of queue if we (and all of our ancestors) are the
	 * only element on the queue
	 */
	for_each_sched_wrr_entity(wrr_se) {
		if (wrr_se->run_list.prev != wrr_se->run_list.next) {
			requeue_task_wrr(rq, p, 0);
			set_tsk_need_resched(p);
			return;
		}
	}
}
static unsigned int get_wrr_interval_wrr(struct rq *rq, struct task_struct *task)
{
	if(task->policy==SCHED_WRR){
			if(foreground(task)){
				return F_WRR_TIMESLICE;
			}
			else{
				return B_WRR_TIMESLICE;
			}
	}
		return 0;
}
static void
prio_changed_wrr(struct rq *rq, struct task_struct *p, int oldprio)
{}
static void switched_to_wrr(struct rq *rq, struct task_struct *p)
{}
const struct sched_class wrr_sched_class = {
    .next           = &fair_sched_class,        /*Required*/
    .enqueue_task       = enqueue_task_wrr,     /*Required*/
    .dequeue_task       = dequeue_task_wrr,     /*Required*/
    .yield_task     = yield_task_wrr,       /*Required*/

    .check_preempt_curr = check_preempt_curr_wrr,       /*Required*/

    .pick_next_task     = pick_next_task_wrr,       /*Required*/
    .put_prev_task      = put_prev_task_wrr,        /*Required*/
    // .task_fork          = task_fork_wrr,
#ifdef CONFIG_SMP
    .select_task_rq     = select_task_rq_wrr,          /*Never need impl */

    .set_cpus_allowed       = set_cpus_allowed_wrr,        /*Never need impl */
    .rq_online              = rq_online_wrr,           /*Never need impl */
    .rq_offline             = rq_offline_wrr,          /*Never need impl */
    .pre_schedule       = pre_schedule_wrr,        /*Never need impl */
    .post_schedule      = post_schedule_wrr,           /*Never need impl */
    .task_woken     = task_woken_wrr,          /*Never need impl */
    .switched_from      = switched_from_wrr,           /*Never need impl */
#endif

    .set_curr_task          = set_curr_task_wrr,        /*Required*/
    .task_tick      = task_tick_wrr,        /*Required*/
    .get_rr_interval    = get_wrr_interval_wrr,

    .prio_changed       = prio_changed_wrr,        /*Never need impl */
    .switched_to        = switched_to_wrr,         /*Never need impl */
};


