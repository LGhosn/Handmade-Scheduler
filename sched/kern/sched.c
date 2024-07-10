#include <inc/assert.h>
#include <inc/x86.h>
#include <kern/spinlock.h>
#include <kern/env.h>
#include <kern/pmap.h>
#include <kern/monitor.h>
#define MASK_SET_ENVINDEX 1023
void sched_halt(void);

int cycles = 0;

void show_sched_stats(void);
// stats
struct sched_stats {
	int total_cycles;
	int priorities_runs[MIN_PRIORITY];
	int priorities_boosted[MIN_PRIORITY];
	int sched_runs;
	int sched_halt_runs;
};

struct sched_stats stats = { 0 };

void
boost()
{
	cycles = 0;
	for (envid_t iter_envid = 0; iter_envid < NENV; iter_envid++) {
		if (envs[iter_envid].priority > MAX_PRIORITY) {
			stats.priorities_boosted[envs[iter_envid].priority]++;
			envs[iter_envid].priority--;
		}
	}
}


// Choose a user environment to run and run it.
void
sched_yield(void)
{
	// Implement simple round-robin scheduling.
	//
	// Search through 'envs' for an ENV_RUNNABLE environment in
	// circular fashion starting just after the env this CPU was
	// last running.  Switch to the first such environment found.
	//
	// If no envs are runnable, but the environment previously
	// running on this CPU is still ENV_RUNNING, it's okay to
	// choose that environment.
	//
	// Never choose an environment that's currently running on
	// another CPU (env_status == ENV_RUNNING). If there are
	// no runnable environments, simply drop through to the code
	// below to halt the cpu.

	// Your code here
	// Wihtout scheduler, keep runing the last environment while it exists

	stats.sched_runs++;
	envid_t iter_envid = 0;

#define SCHED_PRIORITY

	// Start after the env this CPU was running, if there was any
	if (curenv != NULL)
		iter_envid = (curenv->env_id & MASK_SET_ENVINDEX) + 1;

#ifdef SCHED_ROUND_ROBIN

	// Search through 'envs' inf circular fashion for a runnable env
	for (int i = 0; i < NENV; i++, iter_envid++) {
		if (iter_envid == NENV)
			iter_envid = 0;

		if (envs[iter_envid].env_status == ENV_RUNNABLE)
			env_run(&envs[iter_envid]);
	}

#endif

#ifdef SCHED_PRIORITY

	if (cycles >= CYCLES_PER_PRIORITY) {
		boost();
	}

	int priority = MAX_PRIORITY;
	while (priority <= MIN_PRIORITY) {
		for (int i = 0; i < NENV; i++, iter_envid++) {
			if (iter_envid == NENV)
				iter_envid = 0;

			if (envs[iter_envid].env_status == ENV_RUNNABLE &&
			    envs[iter_envid].priority <= priority) {
				stats.total_cycles++;
				stats.priorities_runs[priority]++;

				if (envs[iter_envid].priority < MIN_PRIORITY)
					envs[iter_envid].priority++;

				if (cycles < CYCLES_PER_PRIORITY) {
					cycles++;
				} else {
					boost();
				}

				env_run(&envs[iter_envid]);
			}
		}
		priority++;
	}

#endif

	// if we got to this point, there were no runnable environments.
	// Run current environment again if it was running
	if (curenv != NULL && curenv->env_status == ENV_RUNNING)
		env_run(curenv);

	sched_halt();
}

// Halt this CPU when there is nothing to do. Wait until the
// timer interrupt wakes it up. This function never returns.
//
void
sched_halt(void)
{
	stats.sched_halt_runs++;
	int i;

	// For debugging and testing purposes, if there are no runnable
	// environments in the system, then drop into the kernel monitor.
	for (i = 0; i < NENV; i++) {
		if ((envs[i].env_status == ENV_RUNNABLE ||
		     envs[i].env_status == ENV_RUNNING ||
		     envs[i].env_status == ENV_DYING))
			break;
	}
	if (i == NENV) {
		cprintf("No runnable environments in the system!\n");
		show_sched_stats();
		while (1)
			monitor(NULL);
	}

	// Mark that no environment is running on this CPU
	curenv = NULL;
	lcr3(PADDR(kern_pgdir));

	// Mark that this CPU is in the HALT state, so that when
	// timer interupts come in, we know we should re-acquire the
	// big kernel lock
	xchg(&thiscpu->cpu_status, CPU_HALTED);

	// Release the big kernel lock as if we were "leaving" the kernel
	unlock_kernel();

	// Once the scheduler has finishied it's work, print statistics on
	// performance. Your code here

	// Reset stack pointer, enable interrupts and then halt.
	asm volatile("movl $0, %%ebp\n"
	             "movl %0, %%esp\n"
	             "pushl $0\n"
	             "pushl $0\n"
	             "sti\n"
	             "1:\n"
	             "hlt\n"
	             "jmp 1b\n"
	             :
	             : "a"(thiscpu->cpu_ts.ts_esp0));
}

void
show_sched_stats(void)
{
	cprintf("-------  SCHEDULER STATS  -------\n");
	cprintf(" ### Sched runs: %d\n", stats.sched_runs);
	cprintf(" ### Sched halt runs: %d\n", stats.sched_halt_runs);

	cprintf(" ### Number of times each process was executed:\n");
	int total_executions = 0;
	int process_history[NENV];
	for (int j = 0; j < NENV; j++) {
		if (envs[j].env_runs == 0)
			continue;
		cprintf("	-EnvID %d: %d times\n",
		        envs[j].env_id,
		        envs[j].env_runs);
		process_history[j] = envs[j].env_id;
		total_executions = total_executions + envs[j].env_runs;
	}
	cprintf(" ### Total exectued: %d\n", total_executions);

	cprintf(" ### History of processes executed:\n");
	for (int i = 0; i < NENV; i++) {
		if (envs[i].env_runs == 0)
			continue;

		if (i == 0) {
			cprintf("	%d", process_history[i]);
		} else {
			cprintf(" - %d", process_history[i]);
		}
	}
	cprintf("\n");

#ifdef SCHED_PRIORITY
	cprintf(" ### Total cycles: %d\n", stats.total_cycles);
	for (int i = MAX_PRIORITY; i < MIN_PRIORITY; i++) {
		cprintf(" ### Priority %d runs: %d\n", i, stats.priorities_runs[i]);
		cprintf(" ### Priority %d boosted: %d\n",
		        i,
		        stats.priorities_boosted[i]);
	}
#endif
	cprintf("-------  END SCHEDULER STATS  -------\n");
}
