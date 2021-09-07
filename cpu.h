#ifndef NES_CPU_H
#define NES_CPU_H

#include <stdint.h>

struct bus;

typedef struct {
	uint8_t A;      /* accumulator */
	uint8_t X, Y;   /* index */
	uint8_t SP;     /* stack pointer */
	uint8_t P;      /* flag register */
	uint8_t nmi;    /* non-maskable interrupt */
	uint8_t irq;    /* interrupt request */
	uint16_t PC;    /* program counter */

	uint32_t remaining_cycles;
	struct bus *bus;
} r2A03;

void cpu_exec(r2A03 *, uint8_t);
void cpu_reset(r2A03 *);
void cpu_tick(r2A03 *);

uint8_t cpu_getflag(r2A03 *, uint8_t);

/* TODO do we need make theese function public? */
void cpu_setflag(r2A03 *, uint8_t, uint8_t);
void cpu_setirq(r2A03 *, uint8_t);
void cpu_setnmi(r2A03 *, uint8_t);

#endif /* NES_CPU_H */
