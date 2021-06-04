#include "sched.h"

#include <linux/slab.h>
#include <linux/limits.h>
#define for_each_sched_prr_entity(prr_se) \
	for (; prr_se; prr_se = NULL)
static inline int on_prr_rq(struct sched_prr_entity *prr_se)
{
	return !list_empty(&prr_se->run_list);
}
static inline struct task_struct *prr_task_of(struct sched_prr_entity *prr_se)
{
	return container_of(prr_se, struct task_struct, prr);
}
static inline struct prr_rq *prr_rq_of_se(struct sched_prr_entity *prr_se)
{
	struct task_struct *p=prr_task_of(prr_se);
	struct rq *rq=task_rq(p);
	return &rq->prr;
}
static inline int prr_rq_throttled(struct prr_rq *prr_rq)
{
	return prr_rq->prr_throttled;
}
static inline struct prr_rq *group_prr_rq(struct sched_prr_entity *prr_se)
{
	return NULL;
}
void init_prr_rq(struct prr_rq*prr_rq,struct rq*rq){
	struct list_head *q=&prr_rq->prr;
	INIT_LIST_HEAD(q);
	prr_rq->prr_time=0;
	prr_rq->prr_throttled=0;
	prr_rq->prr_runtime=0;
	raw_spin_lock_init(&prr_rq->prr_runtime_lock);
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
static inline void list_add_leaf_prr_rq(struct prr_rq *prr_rq){}//add one prr to 
static inline void list_del_leaf_prr_rq(struct prr_rq *prr_rq){}
static inline void sched_prr_avg_update(struct rq *rq, u64 prr_delta) {}
static inline
void inc_prr_tasks(struct sched_prr_entity *prr_se, struct prr_rq *prr_rq)
{

	prr_rq->prr_nr_running++;//the number in prr_rq increase
	// inc_rt_prio(rt_rq, prio);
	// inc_rt_migration(rt_se, rt_rq);
	// inc_rt_group(rt_se, rt_rq);
}
void dec_prr_tasks(struct sched_prr_entity *prr_se, struct prr_rq *prr_rq)
{
	prr_rq->prr_nr_running--;

	// dec_rt_prio(rt_rq, rt_se_prio(rt_se));
	// dec_rt_migration(rt_se, rt_rq);
	// dec_rt_group(rt_se, rt_rq);
}
static inline u64 sched_wr_runtime(struct prr_rq *prr_rq)
{
	return prr_rq->prr_runtime;
}
static void update_curr_prr(struct rq *rq)
{
	struct task_struct *curr = rq->curr;
	struct sched_prr_entity *prr_se = &curr->prr;
	// struct prr_rq *prr_rq = prr_rq_of_se(prr_se);
	u64 delta_exec;

	if (curr->sched_class != &prr_sched_class)
		return;//this task not use prr

	delta_exec = rq->clock_task - curr->se.exec_start;
	if (unlikely((s64)delta_exec < 0))
		delta_exec = 0;

	schedstat_set(curr->se.statistics.exec_max,
		      max(curr->se.statistics.exec_max, delta_exec));

	curr->se.sum_exec_runtime += delta_exec;
	account_group_exec_runtime(curr, delta_exec);

	curr->se.exec_start = rq->clock_task;
	cpuacct_charge(curr, delta_exec);

	sched_prr_avg_update(rq, delta_exec);

	// if (!rt_bandwidth_enabled())
	// 	return;

	// for_each_sched_prr_entity(prr_se) {
	// 	prr_rq = prr_rq_of_se(prr_se);

	// 	if (sched_prr_runtime(rt_rq) != RUNTIME_INF) {
	// 		raw_spin_lock(&rt_rq->rt_runtime_lock);
	// 		rt_rq->rt_time += delta_exec;
	// 		if (sched_rt_runtime_exceeded(rt_rq))
	// 			resched_task(curr);
	// 		raw_spin_unlock(&rt_rq->rt_runtime_lock);
	// 	}
	// }
}



static void __enqueue_prr_entity(struct sched_prr_entity *prr_se, bool head)
{
	struct prr_rq *prr_rq = prr_rq_of_se(prr_se);//rq is belongs to
	struct list_head *q = &prr_rq->prr;//the task node

	/*
	 * Don't enqueue the group if its throttled, or when empty.
	 * The latter is a consequence of the former when a child group
	 * get throttled and the current group doesn't have any other
	 * active members.
	 */
	// if (group_rq && (rt_rq_throttled(group_rq) || !group_rq->rt_nr_running))
	// 	return;

	// if (!prr_rq->prr_nr_running)
	// 	list_add_leaf_prr_rq(prr_rq);
	struct task_struct *p=prr_task_of(prr_se);
	//obtain the task_struct that prr_rq as its prr variance.
	if (head){//add at head
		list_add(&prr_se->run_list, q);//prr_se->run_list is head of readyqueue
		//printk("Process %d has enqueued at the list head!\n",p->pid);
	}
	else{
		list_add_tail(&prr_se->run_list, q);
		//printk("Process %d has enqueued at the list tail!\n",p->pid);
	}
	
	inc_prr_tasks(prr_se, prr_rq);
}
static void __dequeue_prr_entity(struct sched_prr_entity *prr_se)
{
	int times =0;
	struct prr_rq *prr_rq = prr_rq_of_se(prr_se);
	list_del_init(&prr_se->run_list);
	dec_prr_tasks(prr_se, prr_rq);
	if (!prr_rq->prr_nr_running){
		list_del_leaf_prr_rq(prr_rq);
	}
}

static void dequeue_prr_stack(struct sched_prr_entity *prr_se)
{
	struct sched_prr_entity *back = NULL;
	for_each_sched_prr_entity(prr_se) {
		prr_se->back = back;
		back = prr_se;
	}
	for (prr_se = back; prr_se; prr_se = prr_se->back) {
		if(on_prr_rq(prr_se)){//if in ready queue
			__dequeue_prr_entity(prr_se);//remove
		}
	}
}
static void enqueue_prr_entity(struct sched_prr_entity *prr_se, bool head)
{	
	dequeue_prr_stack(prr_se);
	for_each_sched_prr_entity(prr_se){
		__enqueue_prr_entity(prr_se, head);
	}
}
static void dequeue_prr_entity(struct sched_prr_entity *prr_se)
{
	dequeue_prr_stack(prr_se);
	for_each_sched_prr_entity(prr_se) {
		struct prr_rq *prr_rq = group_prr_rq(prr_se);

		if (prr_rq && prr_rq->prr_nr_running)
			__enqueue_prr_entity(prr_se, false);//push at tail
	}
}

static void
enqueue_task_prr(struct rq *rq, struct task_struct *p, int flags)
{
	struct sched_prr_entity *prr_se = &p->prr;//the entity that p belongs to

	if (flags & ENQUEUE_WAKEUP)
		prr_se->timeout = 0;
	enqueue_prr_entity(prr_se, flags & ENQUEUE_HEAD);
	inc_nr_running(rq);//the number in whole rq increase
}

static void dequeue_task_prr(struct rq *rq, struct task_struct *p, int flags)
{
	struct sched_prr_entity *prr_se = &p->prr;

	update_curr_prr(rq);
	dequeue_prr_entity(prr_se);

	dec_nr_running(rq);
}
static void
requeue_prr_entity(struct prr_rq *prr_rq, struct sched_prr_entity *prr_se, int head)
{
	if (on_prr_rq(prr_se)) {
		struct list_head *queue = &prr_rq->prr;

		if (head)
			list_move(&prr_se->run_list, queue);
		else
			list_move_tail(&prr_se->run_list, queue);
	}
}
static void requeue_task_prr(struct rq *rq, struct task_struct *p, int head)
{
	struct sched_prr_entity *prr_se = &p->prr;
	struct prr_rq *prr_rq;

	for_each_sched_prr_entity(prr_se) {
		prr_rq = prr_rq_of_se(prr_se);
		requeue_prr_entity(prr_rq, prr_se, head);
	}
}
static void yield_task_prr(struct rq *rq)
{
	requeue_task_prr(rq, rq->curr, 0);
}


static void check_preempt_curr_prr(struct rq *rq, struct task_struct *p, int flags)
{
	//no preempt in prr
}
static struct sched_prr_entity *pick_next_prr_entity(struct rq *rq,
						   struct prr_rq *prr_rq)
{
	struct sched_prr_entity *next = NULL;
	struct list_head *queue=&prr_rq->prr;
	next = list_entry(queue->next, struct sched_prr_entity, run_list);

	return next;
}
static struct task_struct *pick_next_task_prr(struct rq *rq)
{
	struct sched_prr_entity *prr_se;
	struct task_struct *p;
	struct prr_rq *prr_rq=&rq->prr;


	if (!prr_rq->prr_nr_running)//not this schedule method
		return NULL;

	do {
		prr_se = pick_next_prr_entity(rq, prr_rq);
		BUG_ON(!prr_se);
		prr_rq = group_prr_rq(prr_se);
	} while (prr_rq);

	p = prr_task_of(prr_se);
	p->se.exec_start = rq->clock_task;

	return p;
}
static void put_prev_task_prr(struct rq *rq, struct task_struct *p)
{	
}
static void set_curr_task_prr(struct rq *rq)
{
	struct task_struct *p = rq->curr;
	p->se.exec_start = rq->clock_task;
}
char group_path_prr[PATH_MAX];
static char *task_group_path_prr(struct task_group *tg)
{
	if (autogroup_path(tg, group_path_prr, PATH_MAX))
		return group_path_prr;

	/*
	 * May be NULL if the underlying cgroup isn't fully-created yet
	 */
	if (!tg->css.cgroup) {
		group_path_prr[0] = '\0';
		return group_path_prr;
	}
	cgroup_path(tg->css.cgroup, group_path_prr, PATH_MAX);
	return group_path_prr;
}
char path_1[PATH_MAX];
static int foreground(struct task_struct *p){
	char *path_1=task_group_path_prr(p->sched_task_group);
	const char* fore="/";
	const char* back="/bg_non_interactive";
	if(strcmp(path_1,fore)==0){
		printk("foreground\n");
		return 1;
	}
	else{
		if(strcmp(path_1,back)==0){
			printk("background\n");
			return 0;
		}
		else printk("Not foregroud or background\n");
		return 0;
	}
}
static void task_tick_prr(struct rq *rq, struct task_struct *p, int queued)
{

	struct sched_rt_entity *prr_se = &p->prr;

	update_curr_prr(rq);
	/*
	 * PRR
	 */
	if (p->policy != SCHED_PRR)
		return;

	if (--p->prr.time_slice)
		return;
	int prio=p->priority;
	if(foreground(p)){
		p->prr.time_slice=F_MIN_TIMESLICE+(F_MAX_TIMESLICE-F_MIN_TIMESLICE)*(PRR_PRIORITY-1-prio)/(PRR_PRIORITY-1);
	}
	else{
		p->prr.time_slice=B_MIN_TIMESLICE+(B_MAX_TIMESLICE-B_MIN_TIMESLICE)*(PRR_PRIORITY-1-prio)/(PRR_PRIORITY-1);
	}
	/*
	 * Requeue to the end of queue if we (and all of our ancestors) are the
	 * only element on the queue
	 */
	for_each_sched_prr_entity(prr_se) {
		if (prr_se->run_list.prev != prr_se->run_list.next) {
			requeue_task_prr(rq, p, 0);
			set_tsk_need_resched(p);
			return;
		}
	}
}
static unsigned int get_prr_interval_prr(struct rq *rq, struct task_struct *task)
{
	int prio=task->priority;
	if(task->policy==SCHED_PRR){
			if(foreground(task)){
				return F_MIN_TIMESLICE+(F_MAX_TIMESLICE-F_MIN_TIMESLICE)*(PRR_PRIORITY-1-prio)/(PRR_PRIORITY-1);
			}
			else{
				return B_MIN_TIMESLICE+(B_MAX_TIMESLICE-B_MIN_TIMESLICE)*(PRR_PRIORITY-1-prio)/(PRR_PRIORITY-1);
			}
	}
		return 0;
}
static void
prio_changed_prr(struct rq *rq, struct task_struct *p, int oldprio)
{}
static void switched_to_prr(struct rq *rq, struct task_struct *p)
{}
const struct sched_class prr_sched_class = {
    .next           = &fair_sched_class,        /*Required*/
    .enqueue_task       = enqueue_task_prr,     /*Required*/
    .dequeue_task       = dequeue_task_prr,     /*Required*/
    .yield_task     = yield_task_prr,       /*Required*/

    .check_preempt_curr = check_preempt_curr_prr,       /*Required*/

    .pick_next_task     = pick_next_task_prr,       /*Required*/
    .put_prev_task      = put_prev_task_prr,        /*Required*/
    // .task_fork          = task_fork_prr,
#ifdef CONFIG_SMP
    .select_task_rq     = select_task_rq_prr,          /*Never need impl */

    .set_cpus_allowed       = set_cpus_allowed_prr,        /*Never need impl */
    .rq_online              = rq_online_prr,           /*Never need impl */
    .rq_offline             = rq_offline_prr,          /*Never need impl */
    .pre_schedule       = pre_schedule_prr,        /*Never need impl */
    .post_schedule      = post_schedule_prr,           /*Never need impl */
    .task_woken     = task_woken_prr,          /*Never need impl */
    .switched_from      = switched_from_prr,           /*Never need impl */
#endif

    .set_curr_task          = set_curr_task_prr,        /*Required*/
    .task_tick      = task_tick_prr,        /*Required*/
    .get_rr_interval    = get_prr_interval_prr,

    .prio_changed       = prio_changed_prr,        /*Never need impl */
    .switched_to        = switched_to_prr,         /*Never need impl */
};


