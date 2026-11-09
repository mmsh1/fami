#ifndef NES_CPU_H
#define NES_CPU_H

#include <stdint.h>

struct bus;

typedef struct {
	uint8_t A;        /* accumulator */
	uint8_t X, Y;     /* index */
	uint8_t SP;       /* stack pointer */
	uint8_t P;        /* flag register */
	uint8_t nmi;      /* non-maskable interrupt */
	uint8_t irq;      /* interrupt request */
	uint8_t opcode;   /* loaded opcode */
	uint8_t open_bus;

	uint16_t PC;      /* program counter */
	uint16_t addr;    /* loaded address */

	uint64_t total;
	uint64_t stall;
	struct bus *bus;
} cpu;

uint64_t cpu_get_total_cycles(cpu *);
void cpu_reset(cpu *, struct bus *);
void cpu_set_irq(cpu *, uint8_t);
void cpu_set_stall_cycles(cpu *, uint64_t);
void cpu_tick(cpu *);
void cpu_trigger_nmi(cpu *);

#endif /* NES_CPU_H */
