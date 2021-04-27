#include <stdio.h>

#include "cpu.h"

typedef enum {
	IMMEDIATE,
	ZERO_PAGE,
	ABSOLUTE,
	IMPLIED,
	ACCUMULATOR,
	INDEXED,
	ZERO_PAGE_INDEXED,
	INDIRECT,
	PRE_INDEXED_INDIRECT,
	POST_INDEXED_INDIRECT,
	RELATIVE
} addr_mode;

typedef uint8_t (*opcode_func)(mos6502 *, uint16_t, uint8_t, addr_mode);

/* opcodes */
/* http://obelisk.me.uk/6502/reference.html */
static uint8_t OP_ADC(mos6502 *, uint16_t, uint8_t, addr_mode);
static uint8_t OP_AND(mos6502 *, uint16_t, uint8_t, addr_mode);
static uint8_t OP_ASL(mos6502 *, uint16_t, uint8_t, addr_mode);
static uint8_t OP_BCC(mos6502 *, uint16_t, uint8_t, addr_mode);
static uint8_t OP_BCS(mos6502 *, uint16_t, uint8_t, addr_mode);
static uint8_t OP_BEQ(mos6502 *, uint16_t, uint8_t, addr_mode);
static uint8_t OP_BIT(mos6502 *, uint16_t, uint8_t, addr_mode);
static uint8_t OP_BMI(mos6502 *, uint16_t, uint8_t, addr_mode);
static uint8_t OP_BNE(mos6502 *, uint16_t, uint8_t, addr_mode);
static uint8_t OP_BPL(mos6502 *, uint16_t, uint8_t, addr_mode);
static uint8_t OP_BRK(mos6502 *, uint16_t, uint8_t, addr_mode);
static uint8_t OP_BVC(mos6502 *, uint16_t, uint8_t, addr_mode);
static uint8_t OP_BVS(mos6502 *, uint16_t, uint8_t, addr_mode);
static uint8_t OP_CLC(mos6502 *, uint16_t, uint8_t, addr_mode);
static uint8_t OP_CLD(mos6502 *, uint16_t, uint8_t, addr_mode);
static uint8_t OP_CLI(mos6502 *, uint16_t, uint8_t, addr_mode);
static uint8_t OP_CLV(mos6502 *, uint16_t, uint8_t, addr_mode);
static uint8_t OP_CMP(mos6502 *, uint16_t, uint8_t, addr_mode);
static uint8_t OP_CPX(mos6502 *, uint16_t, uint8_t, addr_mode);
static uint8_t OP_CPY(mos6502 *, uint16_t, uint8_t, addr_mode);
static uint8_t OP_DEC(mos6502 *, uint16_t, uint8_t, addr_mode);
static uint8_t OP_DEX(mos6502 *, uint16_t, uint8_t, addr_mode);
static uint8_t OP_DEY(mos6502 *, uint16_t, uint8_t, addr_mode);
static uint8_t OP_EOR(mos6502 *, uint16_t, uint8_t, addr_mode);
static uint8_t OP_INC(mos6502 *, uint16_t, uint8_t, addr_mode);
static uint8_t OP_INX(mos6502 *, uint16_t, uint8_t, addr_mode);
static uint8_t OP_INY(mos6502 *, uint16_t, uint8_t, addr_mode);
static uint8_t OP_JMP(mos6502 *, uint16_t, uint8_t, addr_mode);
static uint8_t OP_JSR(mos6502 *, uint16_t, uint8_t, addr_mode);
static uint8_t OP_LDA(mos6502 *, uint16_t, uint8_t, addr_mode);
static uint8_t OP_LDX(mos6502 *, uint16_t, uint8_t, addr_mode);
static uint8_t OP_LDY(mos6502 *, uint16_t, uint8_t, addr_mode);
static uint8_t OP_LSR(mos6502 *, uint16_t, uint8_t, addr_mode);
static uint8_t OP_NOP(mos6502 *, uint16_t, uint8_t, addr_mode);
static uint8_t OP_ORA(mos6502 *, uint16_t, uint8_t, addr_mode);
static uint8_t OP_PHA(mos6502 *, uint16_t, uint8_t, addr_mode);
static uint8_t OP_PHP(mos6502 *, uint16_t, uint8_t, addr_mode);
static uint8_t OP_PLA(mos6502 *, uint16_t, uint8_t, addr_mode);
static uint8_t OP_PLP(mos6502 *, uint16_t, uint8_t, addr_mode);
static uint8_t OP_ROL(mos6502 *, uint16_t, uint8_t, addr_mode);
static uint8_t OP_ROR(mos6502 *, uint16_t, uint8_t, addr_mode);
static uint8_t OP_RTI(mos6502 *, uint16_t, uint8_t, addr_mode);
static uint8_t OP_RTS(mos6502 *, uint16_t, uint8_t, addr_mode);
static uint8_t OP_SBC(mos6502 *, uint16_t, uint8_t, addr_mode);
static uint8_t OP_SEC(mos6502 *, uint16_t, uint8_t, addr_mode);
static uint8_t OP_SED(mos6502 *, uint16_t, uint8_t, addr_mode);
static uint8_t OP_SEI(mos6502 *, uint16_t, uint8_t, addr_mode);
static uint8_t OP_STA(mos6502 *, uint16_t, uint8_t, addr_mode);
static uint8_t OP_STX(mos6502 *, uint16_t, uint8_t, addr_mode);
static uint8_t OP_STY(mos6502 *, uint16_t, uint8_t, addr_mode);
static uint8_t OP_TAX(mos6502 *, uint16_t, uint8_t, addr_mode);
static uint8_t OP_TAY(mos6502 *, uint16_t, uint8_t, addr_mode);
static uint8_t OP_TSX(mos6502 *, uint16_t, uint8_t, addr_mode);
static uint8_t OP_TXA(mos6502 *, uint16_t, uint8_t, addr_mode);
static uint8_t OP_TXS(mos6502 *, uint16_t, uint8_t, addr_mode);
static uint8_t OP_TYA(mos6502 *, uint16_t, uint8_t, addr_mode);

/* Unofficial instructions */
static uint8_t OP_ALR(mos6502 *, uint16_t, uint8_t, addr_mode);
static uint8_t OP_ANC(mos6502 *, uint16_t, uint8_t, addr_mode);
static uint8_t OP_ARR(mos6502 *, uint16_t, uint8_t, addr_mode);
static uint8_t OP_AXS(mos6502 *, uint16_t, uint8_t, addr_mode);
static uint8_t OP_LAX(mos6502 *, uint16_t, uint8_t, addr_mode);
static uint8_t OP_SAX(mos6502 *, uint16_t, uint8_t, addr_mode);
static uint8_t OP_DCP(mos6502 *, uint16_t, uint8_t, addr_mode);
static uint8_t OP_ISC(mos6502 *, uint16_t, uint8_t, addr_mode);
static uint8_t OP_RLA(mos6502 *, uint16_t, uint8_t, addr_mode);
static uint8_t OP_RRA(mos6502 *, uint16_t, uint8_t, addr_mode);
static uint8_t OP_SLO(mos6502 *, uint16_t, uint8_t, addr_mode);
static uint8_t OP_SRE(mos6502 *, uint16_t, uint8_t, addr_mode);
static uint8_t OP_XAA(mos6502 *, uint16_t, uint8_t, addr_mode);
static uint8_t OP_AHX(mos6502 *, uint16_t, uint8_t, addr_mode);
static uint8_t OP_TAS(mos6502 *, uint16_t, uint8_t, addr_mode);
static uint8_t OP_LAS(mos6502 *, uint16_t, uint8_t, addr_mode);

/* for unsupported or illegal opcodes */
static uint8_t OP_UNS(mos6502 *, uint16_t, uint8_t, addr_mode);

/* This table was copied from Rockwell R650x manual */
static opcode_func optable[0xFF + 1] = {
	/*OP_BRK
	OP_ORA
	OP_UNS
	OP_UNS
	OP_UNS
	OP_ORA
	OP_ASL
	OP_UNS
	OP_PHP
	OP_ORA
	OP_ASL
	OP_UNS
	OP_UNS
	OP_ORA
	OP_ASL
	OP_UNS

	OP_BPL
	OP_ORA
	OP_UNS
	OP_UNS
	OP_UNS
	OP_ORA
	OP_ASL
	OP_UNS
	OP_CLC
	OP_ORA
	OP_UNS
	OP_UNS
	OP_UNS
	OP_ORA
	OP_ASL
	OP_UNS

	OP_JSR
	OP_AND
	OP_UNS
	OP_UNS
	OP_BIT
	OP_AND
	OP_ROL
	OP_UNS
	OP_PLP
	OP_AND
	OP_ROL
	OP_UNS
	OP_BIT
	OP_AND
	OP_ROL
	OP_UNS

	OP_BMI
	OP_AND
	OP_UNS
	OP_UNS
	OP_UNS
	OP_AND
	OP_ROL
	OP_UNS
	OP_SEC
	OP_AND
	OP_UNS
	OP_UNS
	OP_UNS
	OP_AND
	OP_ROL
	OP_UNS

	OP_RTI
	OP_EOR
	OP_UNS
	OP_UNS
	OP_UNS
	OP_EOR
	OP_LSR
	OP_UNS
	OP_PHA
	OP_EOR
	OP_LSR
	OP_UNS
	OP_JMP
	OP_EOR
	OP_LSR
	OP_UNS

	OP_BVC
	OP_EOR
	OP_UNS
	OP_UNS
	OP_UNS
	OP_EOR
	OP_LSR
	OP_UNS
	OP_CLI
	OP_EOR
	OP_UNS
	OP_UNS
	OP_UNS
	OP_EOR
	OP_LSR
	OP_UNS

	OP_RTS
	OP_ADC
	OP_UNS
	OP_UNS
	OP_UNS
	OP_ADC
	OP_ROR
	OP_UNS
	OP_PLA
	OP_ADC
	OP_ROR
	OP_UNS
	OP_JMP
	OP_ADC
	OP_ROR
	OP_UNS

	OP_BVS
	OP_ADC
	OP_UNS
	OP_UNS
	OP_UNS
	OP_ADC
	OP_ROR
	OP_UNS
	OP_SEI
	OP_ADC
	OP_UNS
	OP_UNS
	OP_UNS
	OP_ADC
	OP_ROR
	OP_UNS

	OP_UNS
	OP_STA
	OP_UNS
	OP_UNS
	OP_STY
	OP_STA
	OP_STX
	OP_UNS
	OP_DEY
	OP_UNS
	OP_TXA
	OP_UNS
	OP_STY
	OP_STA
	OP_STX
	OP_UNS

	OP_BCC
	OP_STA
	OP_UNS
	OP_UNS
	OP_STY
	OP_STA
	OP_STX
	OP_UNS
	OP_TYA
	OP_STA
	OP_TXS
	OP_UNS
	OP_UNS
	OP_STA
	OP_UNS
	OP_UNS

	OP_LDY
	OP_LDA
	OP_LDX
	OP_UNS
	OP_LDY
	OP_LDA
	OP_LDX
	OP_UNS
	OP_TAY
	OP_LDA
	OP_TAX
	OP_UNS
	OP_LDY
	OP_LDA
	OP_LDX
	OP_UNS

	OP_BCS
	OP_LDA
	OP_UNS
	OP_UNS
	OP_LDY
	OP_LDA
	OP_LDX
	OP_UNS
	OP_CLV
	OP_LDA
	OP_TSX
	OP_UNS
	OP_LDY
	OP_LDA
	OP_LDX
	OP_UNS

	OP_CPY
	OP_CMP
	OP_UNS
	OP_UNS
	OP_CPY
	OP_CMP
	OP_DEC
	OP_UNS
	OP_INY
	OP_CMP
	OP_DEX
	OP_UNS
	OP_CPY
	OP_CMP
	OP_DEC
	OP_UNS

	OP_BNE
	OP_CMP
	OP_UNS
	OP_UNS
	OP_UNS
	OP_CMP
	OP_DEC
	OP_UNS
	OP_CLD
	OP_CMP
	OP_UNS
	OP_UNS
	OP_UNS
	OP_CMP
	OP_DEC
	OP_UNS

	OP_CPX
	OP_SBC
	OP_UNS
	OP_UNS
	OP_CPX
	OP_SBC
	OP_INC
	OP_UNS
	OP_INX
	OP_SBC
	OP_NOP
	OP_UNS
	OP_CPX
	OP_SBC
	OP_INC
	OP_UNS

	OP_BEQ
	OP_SBC
	OP_UNS
	OP_UNS
	OP_UNS
	OP_SBC
	OP_INC
	OP_UNS
	OP_SED
	OP_SBC
	OP_UNS
	OP_UNS
	OP_UNS
	OP_SBC
	OP_INC
	OP_UNS*/
};

static void clock(void);
static void reset(void);
static void set_irq(void);
static void set_nmi(void);

static void
clock(void)
{
}

static void
reset(void)
{
}

static void
set_irq(void)
{
}

static void
set_nmi(void)
{
}

uint8_t
get_flag(mos6502 *cpu, uint8_t bit)
{
	return (cpu->P) && bit;
}

void
set_flag(mos6502 *cpu, uint8_t bit, uint8_t val)
{
	(cpu->P) ^= (-val ^ cpu->P) & bit;
}

void
cpu_reset(mos6502 *cpu)
{
	cpu->PC = 0xFFFC;
	cpu->SP = 0x00;
	cpu->P = 0;
	cpu->A = 0;
	cpu->X = 0;
	cpu->Y = 0;
}

int
test(void)
{
	/* mock for building */
	fprintf(stderr, "test\n");
	return 0;
}
