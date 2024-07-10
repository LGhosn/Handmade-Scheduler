#include <inc/lib.h>

void
umain(int argc, char **argv)
{
	// Un nuevo proceso siempre tiene la prioridad máxima
	cprintf("#### TESTS SCHEDULER WITH PRIORITIES ####\n");
	cprintf("A new process has priority (higest):\n");
	cprintf("EnvId: %d has priority: %d\n", thisenv->env_id, thisenv->priority);

	// Si se usa la syscall "sys_decrease_priority" la prioridad del mismo proceso baja
	sys_decrease_priority(thisenv->env_id);
	cprintf("After calling 'sys_decrease_priority' the new priority of %d is %d\n", thisenv->env_id, thisenv->priority);

	// Si se usa la syscall "sys_increase_priority" la prioridad del mismo proceso subirá
	sys_increase_priority(thisenv->env_id);
	cprintf("After calling 'sys_increase_priority' the new priority of %d is %d\n", thisenv->env_id, thisenv->priority);

	// Si un proceso está corriendo por mucho tiempo su prioridad va bajando hasta que esporadicamente recibe un boost
	cprintf("If a process runs for a while its priority will be lower\n");
	int max = thisenv->priority;
	int pid = fork();
	if (pid == 0) {
		for (int i = 0; i < 10000; i++) {
			sys_yield();
		}
		return;
	} else if (pid > 0) {
		while (1) {
			if (max < thisenv->priority) {
				cprintf("Process's (%d) priority is getting lower. New priority: %d\n", thisenv->env_id, thisenv->priority);
				max = thisenv->priority;
				if (max == MIN_PRIORITY) {
					return;
				}
			}
			sys_yield();
		}
	} else {
		panic("fork");
	}
}
