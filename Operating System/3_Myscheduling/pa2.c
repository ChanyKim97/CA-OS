/**********************************************************************
 * Copyright (c) 2019-2021
 *  Sang-Hoon Kim <sanghoonkim@ajou.ac.kr>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTIABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 **********************************************************************/

/* THIS FILE IS ALL YOURS; DO WHATEVER YOU WANT TO DO IN THIS FILE */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "types.h"
#include "list_head.h"

/**
 * The process which is currently running
 */
#include "process.h"
extern struct process *current;


/**
 * List head to hold the processes ready to run
 */
extern struct list_head readyqueue;


/**
 * Resources in the system.
 */
#include "resource.h"
extern struct resource resources[NR_RESOURCES];


/**
 * Monotonically increasing ticks
 */
extern unsigned int ticks;


/**
 * Quiet mode. True if the program was started with -q option
 */
extern bool quiet;


/***********************************************************************
 * Default FCFS resource acquision function
 *
 * DESCRIPTION
 *   This is the default resource acquision function which is called back
 *   whenever the current process is to acquire resource @resource_id.
 *   The current implementation serves the resource in the requesting order
 *   without considering the priority. See the comments in sched.h
 ***********************************************************************/
bool fcfs_acquire(int resource_id)
{
	struct resource *r = resources + resource_id;

	if (!r->owner) {
		/* This resource is not owned by any one. Take it! */
		r->owner = current;
		return true;
	}

	/* OK, this resource is taken by @r->owner. */

	/* Update the current process state */
	current->status = PROCESS_WAIT;

	/* And append current to waitqueue */
	list_add_tail(&current->list, &r->waitqueue);

	/**
	 * And return false to indicate the resource is not available.
	 * The scheduler framework will soon call schedule() function to
	 * schedule out current and to pick the next process to run.
	 */
	return false;
}


/***********************************************************************
Priority acquire
***********************************************************************/
bool prio_acquire(int resource_id) { //fcfs와 차이 X
	struct resource* r = resources + resource_id;

	if (!r->owner) {
		r->owner = current;
		return true;
	}

	current->status = PROCESS_WAIT;

	list_add_tail(&current->list, &r->waitqueue);

	return false;
}


/***********************************************************************
Priority ceiling protocol acquire
***********************************************************************/
bool pcp_acquire(int resource_id) { //resource 잡는 순간 우선순위 최고로
	struct resource* r = resources + resource_id;

	if (!r->owner) {
		current->prio = MAX_PRIO;
		r->owner = current;
		return true;
	}

	current->status = PROCESS_WAIT;

	list_add_tail(&current->list, &r->waitqueue);

	return false;
}


/***********************************************************************
Priority inheritance protocol acquire
***********************************************************************/
bool pip_acquire(int resource_id) { //high가 low가 가진 resource원하면 low 우선순위 상승
	struct resource* r = resources + resource_id;

	if (!r->owner) {
		r->owner = current;
		return true;
	}

	//나보다 리소스를 갖고있는 놈 우선순위 낮으면 나한테 맞춰줌
	if (r->owner->prio < current->prio) {
		r->owner->prio = current->prio;
	}

	current->status = PROCESS_WAIT;

	list_add_tail(&current->list, &r->waitqueue);

	return false;
}


/***********************************************************************
 * Default FCFS resource release function
 *
 * DESCRIPTION
 *   This is the default resource release function which is called back
 *   whenever the current process is to release resource @resource_id.
 *   The current implementation serves the resource in the requesting order
 *   without considering the priority. See the comments in sched.h
 ***********************************************************************/
void fcfs_release(int resource_id)
{
	struct resource *r = resources + resource_id;

	/* Ensure that the owner process is releasing the resource */
	assert(r->owner == current);

	/* Un-own this resource */
	r->owner = NULL;

	/* Let's wake up ONE waiter (if exists) that came first */
	if (!list_empty(&r->waitqueue)) {
		struct process *waiter =
				list_first_entry(&r->waitqueue, struct process, list);

		/**
		 * Ensure the waiter is in the wait status
		 */
		assert(waiter->status == PROCESS_WAIT);

		/**
		 * Take out the waiter from the waiting queue. Note we use
		 * list_del_init() over list_del() to maintain the list head tidy
		 * (otherwise, the framework will complain on the list head
		 * when the process exits).
		 */
		list_del_init(&waiter->list);

		/* Update the process status */
		waiter->status = PROCESS_READY;

		/**
		 * Put the waiter process into ready queue. The framework will
		 * do the rest.
		 */
		list_add_tail(&waiter->list, &readyqueue);
	}
}


/***********************************************************************
Priority release
***********************************************************************/
void prio_release(int resource_id) {
	struct resource* r = resources + resource_id;

	assert(r->owner == current);

	r->owner = NULL;

	struct process* pos = NULL;

	if (!list_empty(&r->waitqueue)) {
		struct process* waiter =
			list_first_entry(&r->waitqueue, struct process, list);

		//waitqueue에 있는 process중 priority높은것 선택
		list_for_each_entry(pos, &r->waitqueue, list) {
			if (pos->prio > waiter->prio) waiter = pos;
		}

		assert(waiter->status == PROCESS_WAIT);

		list_del_init(&waiter->list);

		waiter->status = PROCESS_READY;

		list_add_tail(&waiter->list, &readyqueue);
	}
}


/***********************************************************************
Priority ceiling protocol release
***********************************************************************/
void pcp_release(int resource_id) {
	struct resource* r = resources + resource_id;

	assert(r->owner == current);

	r->owner = NULL;

	struct process* pos = NULL;

	//resource release 하면 우선순위 원래대로
	current->prio = current->prio_orig;

	if (!list_empty(&r->waitqueue)) {
		struct process* waiter =
			list_first_entry(&r->waitqueue, struct process, list);

		list_for_each_entry(pos, &r->waitqueue, list) {
			if (pos->prio > waiter->prio) waiter = pos;
		}

		assert(waiter->status == PROCESS_WAIT);

		list_del_init(&waiter->list);

		waiter->status = PROCESS_READY;

		list_add_tail(&waiter->list, &readyqueue);
	}
}


/***********************************************************************
Priority inheritance protocol release
***********************************************************************/
void pip_release(int resource_id) {
	struct resource* r = resources + resource_id;

	assert(r->owner == current);

	//해제하기 전에 우선순위를 원래 자기 우선순위로 돌려주도록함
	r->owner->prio = r->owner->prio_orig;

	r->owner = NULL;

	struct process* pos = NULL;

	if (!list_empty(&r->waitqueue)) {
		struct process* waiter =
			list_first_entry(&r->waitqueue, struct process, list);

		list_for_each_entry(pos, &r->waitqueue, list) {
			if (pos->prio > waiter->prio) waiter = pos;
		}

		assert(waiter->status == PROCESS_WAIT);

		list_del_init(&waiter->list);

		waiter->status = PROCESS_READY;

		list_add_tail(&waiter->list, &readyqueue);
	}
}


#include "sched.h"
/***********************************************************************
 * FIFO scheduler
 ***********************************************************************/
static int fifo_initialize(void)
{
	return 0;
}

static void fifo_finalize(void)
{
}

static struct process *fifo_schedule(void)
{
	struct process *next = NULL;

	/* You may inspect the situation by calling dump_status() at any time */
	//dump_status();

	/**
	 * When there was no process to run in the previous tick (so does
	 * in the very beginning of the simulation), there will be
	 * no @current process. In this case, pick the next without examining
	 * the current process. Also, when the current process is blocked
	 * while acquiring a resource, @current is (supposed to be) attached
	 * to the waitqueue of the corresponding resource. In this case just
	 * pick the next as well.
	 */
	if (!current || current->status == PROCESS_WAIT) { // process wait이면 pick_next로
		goto pick_next;
	}

	/* The current process has remaining lifetime. Schedule it again */
	if (current->age < current->lifespan) { //age보다 lifespan큰 상태면 현재프로세스 유지
		return current;
	}

pick_next:
	/* Let's pick a new process to run next */

	if (!list_empty(&readyqueue)) { //readyqueue 안 비어있으면 실행
		/**
		 * If the ready queue is not empty, pick the first process
		 * in the ready queue
		 */
		next = list_first_entry(&readyqueue, struct process, list); //process순번대로 next에 넣기

		/**
		 * Detach the process from the ready queue. Note we use list_del_init()
		 * instead of list_del() to maintain the list head tidy. Otherwise,
		 * the framework will complain (assert) on process exit.
		 */
		list_del_init(&next->list); //readyqueue에서 빼기
	}

	/* Return the next process to run */
	return next;
}

struct scheduler fifo_scheduler = {
	.name = "FIFO",
	.acquire = fcfs_acquire,
	.release = fcfs_release,
	.initialize = fifo_initialize,
	.finalize = fifo_finalize,
	.schedule = fifo_schedule,
};


/***********************************************************************
 * SJF scheduler
 ***********************************************************************/
static struct process *sjf_schedule(void)
{
	/**
	 * Implement your own SJF scheduler here.
	 */
	struct process* next = NULL;

	if (!current || current->status == PROCESS_WAIT) {
		goto pick_next;
	}

	if (current->age < current->lifespan) {
		return current;
	}

	struct process* pos = NULL;

pick_next:
	if (!list_empty(&readyqueue)) {
		next = list_first_entry(&readyqueue, struct process, list);

		//fcfs와 다른 선택방법
		//제일 작은 lifespan찾아서 next로
		list_for_each_entry(pos, &readyqueue, list) {
			if (next->lifespan > pos->lifespan) next = pos;
		}

		list_del_init(&next->list);
	}

	return next;
}

struct scheduler sjf_scheduler = {
	.name = "Shortest-Job First",
	.acquire = fcfs_acquire, /* Use the default FCFS acquire() */
	.release = fcfs_release, /* Use the default FCFS release() */
	.schedule = sjf_schedule		 /* TODO: Assign sjf_schedule()
								to this function pointer to activate
								SJF in the system */
};


/***********************************************************************
 * SRTF scheduler
 ***********************************************************************/
static struct process* srtf_schedule(void) {
	struct process* next = NULL;

	if (!current || current->status == PROCESS_WAIT) {
		goto pick_next;
	}

	if (current->age < current->lifespan) { //non-preemptive 특징 preemptive해짐
		list_add_tail(&current->list, &readyqueue);
	}

	struct process* pos = NULL;

pick_next:
	if (!list_empty(&readyqueue)) {
		next = list_first_entry(&readyqueue, struct process, list);

		list_for_each_entry(pos, &readyqueue, list) { //남은 일을 계산
			if (next->lifespan - next->age > pos->lifespan - pos->age) next = pos;
		}

		list_del_init(&next->list);
	}

	return next;
}
struct scheduler srtf_scheduler = {
	.name = "Shortest Remaining Time First",
	.acquire = fcfs_acquire, /* Use the default FCFS acquire() */
	.release = fcfs_release, /* Use the default FCFS release() */
	/* You need to check the newly created processes to implement SRTF.
	 * Use @forked() callback to mark newly created processes */
	/* Obviously, you should implement srtf_schedule() and attach it here */
	.schedule = srtf_schedule
};


/***********************************************************************
 * Round-robin scheduler
 ***********************************************************************/
static struct process* rr_schedule(void) {
	struct process* next = NULL;

	if (!current || current->status == PROCESS_WAIT) {
		goto pick_next;
	}

	if (current->age < current->lifespan) { //한 건 readyqueue의 마지막으로 보내서 rr구현
		list_add_tail(&current->list, &readyqueue);
	}

pick_next:
	if (!list_empty(&readyqueue)) {
		next = list_first_entry(&readyqueue, struct process, list);

		list_del_init(&next->list); 
	}

	return next;
}
struct scheduler rr_scheduler = {
	.name = "Round-Robin",
	.acquire = fcfs_acquire, /* Use the default FCFS acquire() */
	.release = fcfs_release, /* Use the default FCFS release() */
	/* Obviously, you should implement rr_schedule() and attach it here */
	.schedule = rr_schedule
};


/***********************************************************************
 * Priority scheduler
 ***********************************************************************/
static struct process* prio_schedule(void) {
	struct process* next = NULL;
	//dump_status();

	if (!current || current->status == PROCESS_WAIT) {
		goto pick_next;
	}

	if (current->age < current->lifespan) { //rr기반
		list_add_tail(&current->list, &readyqueue);
	}

	struct process* pos = NULL;

pick_next:
	if (!list_empty(&readyqueue)) {
		next = list_first_entry(&readyqueue, struct process, list);

		//priority 높은것 next로
		list_for_each_entry(pos, &readyqueue, list) {
			if (pos->prio > next->prio) next = pos;
		}

		list_del_init(&next->list);
	}
	return next;
}

struct scheduler prio_scheduler = {
	.name = "Priority",
	/**
	 * Implement your own acqure/release function to make priority
	 * scheduler correct.
	 */
	.acquire = prio_acquire,
	.release = prio_release,
	/* Implement your own prio_schedule() and attach it here */
	.schedule = prio_schedule
};


/***********************************************************************
 * Priority scheduler with aging
 ***********************************************************************/
static struct process* prio_aging_schedule(void) {
	struct process* next = NULL;
	//dump_status();

	if (!current || current->status == PROCESS_WAIT) {
		goto pick_next;
	}

	if (current->age < current->lifespan) {
		list_add_tail(&current->list, &readyqueue);
	}

	struct process* pos = NULL;

pick_next:
	if (!list_empty(&readyqueue)) {
		next = list_first_entry(&readyqueue, struct process, list);

		//prio와 같이 우선순위 높은거 선택
		//차이: readyqueue에 있는 모든 process에 1씩 aging해서 starvation 없애기
		list_for_each_entry(pos, &readyqueue, list) {
			pos->prio++;
			if (pos->prio > next->prio) next = pos;
			//pos->prio++; 16에 4번 process실행 틀림 why?
		}

		list_del_init(&next->list);
		//다음으로 선택된 프로세스의 우선순위는 정상으로 돌려놓기
		next->prio = next->prio_orig;
	}
	return next;
}
struct scheduler pa_scheduler = {
	.name = "Priority + aging",
	/**
	 * Implement your own acqure/release function to make priority
	 * scheduler correct.
	 */
	.acquire = prio_acquire,
	.release = prio_release,
	/* Implement your own prio_aging_schedule() and attach it here */
	.schedule = prio_aging_schedule
};


/***********************************************************************
 * Priority scheduler with priority ceiling protocol
 ***********************************************************************/
struct scheduler pcp_scheduler = {
	.name = "Priority + PCP Protocol",
	/**
	 * Implement your own acqure/release function too to make priority
	 * scheduler correct.
	 */
	.acquire = pcp_acquire,
	.release = pcp_release,
	.schedule = prio_schedule
};


/***********************************************************************
 * Priority scheduler with priority inheritance protocol
 ***********************************************************************/
struct scheduler pip_scheduler = {
	.name = "Priority + PIP Protocol",
	/**
	 * Ditto
	 */
	.acquire = pip_acquire,
	.release = pip_release,
	.schedule = prio_schedule
};
