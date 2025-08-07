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
	uint8_t acc_mode; /* accumulator addr mode flag */
	uint8_t opcode;   /* loaded opcode */

	uint16_t PC;      /* program counter */
	uint16_t addr;    /* loaded address */

	uint64_t total;
	uint64_t stall;
	struct bus *bus;
} r2A03;

void cpu_reset(r2A03 *, struct bus *);
void cpu_tick(r2A03 *);
void cpu_trigger_nmi(r2A03 *);

#endif /* NES_CPU_H */
