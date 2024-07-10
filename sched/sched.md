# sched

Lugar para respuestas en prosa, seguimientos con GDB y documentación del TP.

> Tuvimos que cambiar el flag `-gstabs` por `-ggdb3` en `GNUmakefile` para correr grade. Para correr gdb y  qemu tuvimos que volver a poner `-gstabs`

# Utilizar GDB para visualizar el cambio de contexto. Realizar una captura donde se muestre claramente:
* el cambio de contexto
* el estado del stack al inicio de la llamada de context_switch
* cómo cambia el stack instrucción a instrucción
* cómo se modifican los registros luego de ejecutar iret

```s
context_switch:
	add $4, %esp
	mov (%esp), %esp
	popal
	pop %es
	pop %ds	
	add $8, %esp
	iret
spin:
	jmp spin

```

### Estado de los registros pre context_switch.
```
eax            0x0                 0
ecx            0xf01243c0          -267238464
edx            0xef803000          -276811776
ebx            0xf02ed000          -265367552
esp            0xf0122fbc          0xf0122fbc
ebp            0xf0122fd8          0xf0122fd8
esi            0xf02ed000          -265367552
edi            0x2ad000            2805760
eip            0xf01043bb          0xf01043bb <context_switch>
eflags         0x46                [ IOPL=0 ZF PF ]
cs             0x8                 8
ss             0x10                16
ds             0x10                16
es             0x10                16
fs             0x23                35
gs             0x23                35
fs_base        0x0                 0
gs_base        0x0                 0
k_gs_base      0x0                 0
cr0            0x80050033          [ PG AM WP NE ET MP PE ]
cr2            0x0                 0
cr3            0x3bb000            [ PDBR=0 PCID=0 ]
cr4            0x10                [ PSE ]
cr8            0x0                 0
```

###### Se suma 4 al stack pointer para buscar el primer parametro.
* `add $4, %esp`
```
esp            0xf0122fc0          0xf0122fc0
```

###### Se le guarda al registro `%esp` el puntero a Trapframe.
* `mov (%esp), %esp`
```
esp            0xf02ed000          0xf02ed000
```

###### Se popean los registros del `struct Trapframe` a los registros del CPU. El stack en este momento apunta al primer campo del `struct Trapframe`, lo que es equivalente al campo `reg_edi` dentro del `struct PushRegs`. Por lo que en este orden se van a popear y guardar en el CPU.

```C
struct PushRegs {
	uint32_t reg_edi; // <--- %esp
	uint32_t reg_esi;
	uint32_t reg_ebp;
	uint32_t reg_oesp;
	uint32_t reg_ebx;
	uint32_t reg_edx;
	uint32_t reg_ecx;
	uint32_t reg_eax;
} __attribute__((packed));

struct Trapframe {
	struct PushRegs tf_regs;
	uint16_t tf_es;
	uint16_t tf_padding1;
	uint16_t tf_ds;
	uint16_t tf_padding2;
    // ...
} __attribute__((packed));
```

* `popal`
```
eax            0x0                 0
ecx            0x0                 0
edx            0x0                 0
ebx            0x0                 0
esp            0xf02ed020          0xf02ed020
ebp            0x0                 0x0
esi            0x0                 0
edi            0x0                 0
```

###### Se hace pop del campo `tf_es` del `struct Trapframe` y se guarda en su registro en el CPU.

* `pop %es`
```
esp            0xf02ed024          0xf02ed024
es             0x23                35
```

###### Se hace pop del campo `tf_ds` del `struct Trapframe` y se guarda en su registro en el CPU.

* `pop %ds`
```
esp            0xf02ed028          0xf02ed028
ds             0x23                35
```

###### Se corre el stack para saltear los campos del `struct Trapframe` que iret no quiere.
* `add $8, %esp`
```
esp            0xf02ed030          0xf02ed030
```

###### Se ejecuta iret y se vuelve a `user space`.
* `iret`
```
eax            0x0                 0
ecx            0x0                 0
edx            0x0                 0
ebx            0x0                 0
esp            0xeebfe000          0xeebfe000
ebp            0x0                 0x0
esi            0x0                 0
edi            0x0                 0
eip            0x800020            0x800020
eflags         0x202               [ IOPL=0 IF ]
cs             0x1b                27
ss             0x23                35
ds             0x23                35
es             0x23                35
fs             0x23                35
gs             0x23                35
fs_base        0x0                 0
gs_base        0x0                 0
k_gs_base      0x0                 0
cr0            0x80050033          [ PG AM WP NE ET MP PE ]
cr2            0x0                 0
cr3            0x3bb000            [ PDBR=0 PCID=0 ]
cr4            0x10                [ PSE ]
cr8            0x0                 0
```

# Ejecutar el kernel con qemu y validar que las syscalls están funcionando.

* Programa `hello.c`

```C
// hello, world
#include <inc/lib.h>

void
umain(int argc, char **argv)
{
	cprintf("hello, world\n");
	cprintf("i am environment %08x\n", thisenv->env_id);
}
```

* Output de `qemu`

```console
SMP: CPU 0 found 1 CPU(s)
enabled interrupts: 1 2
[00000000] new env 00001000
0xf01243f4
hello, world
i am environment 00001000
[00001000] exiting gracefully
[00001000] free env 00001000
No runnable environments in the system!
```