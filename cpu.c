#include <stdio.h>

#include "bus.h"
#include "cpu.h"

typedef enum {
	ADDR_ACC,
	ADDR_IMM,
	ADDR_ABS,
	ADDR_ZP,
	ADDR_IZX,
	ADDR_IZY,
	ADDR_IAX,
	ADDR_IAY,
	ADDR_IMP,
	ADDR_REL,
	ADDR_INX,
	ADDR_INY,
	ADDR_IND,
	ADDR_XXX
} addr_mode;

typedef uint8_t (*opcode_func)(mos6502 *);

typedef struct {
	const char *name;
	opcode_func *opcode;
	uint8_t cycles;
	addr_mode mode;
} instruction;

instruction inst_table[0xFF + 1] = {
	{ .name = "BRK", .opcode = NULL, .cycles = 7, .mode = ADDR_IMP }, /* 0x00 */
	{ .name = "ORA", .opcode = NULL, .cycles = 6, .mode = ADDR_INX },
	{ .name = "XXX", .opcode = NULL, .cycles = 0, .mode = ADDR_XXX },
	{ .name = "XXX", .opcode = NULL, .cycles = 0, .mode = ADDR_XXX },
	{ .name = "XXX", .opcode = NULL, .cycles = 0, .mode = ADDR_XXX },
	{ .name = "ORA", .opcode = NULL, .cycles = 3, .mode = ADDR_ZP },
	{ .name = "ASL", .opcode = NULL, .cycles = 5, .mode = ADDR_ZP },
	{ .name = "XXX", .opcode = NULL, .cycles = 0, .mode = ADDR_XXX },
	{ .name = "PHP", .opcode = NULL, .cycles = 3, .mode = ADDR_IMP },
	{ .name = "ORA", .opcode = NULL, .cycles = 2, .mode = ADDR_IMM },
	{ .name = "ASL", .opcode = NULL, .cycles = 2, .mode = ADDR_ACC },
	{ .name = "XXX", .opcode = NULL, .cycles = 0, .mode = ADDR_XXX },
	{ .name = "XXX", .opcode = NULL, .cycles = 0, .mode = ADDR_XXX },
	{ .name = "ORA", .opcode = NULL, .cycles = 4, .mode = ADDR_ABS },
	{ .name = "ASL", .opcode = NULL, .cycles = 6, .mode = ADDR_ABS },
	{ .name = "XXX", .opcode = NULL, .cycles = 0, .mode = ADDR_XXX },

	{ .name = "BPL", .opcode = NULL, .cycles = 2, .mode = ADDR_REL }, /* 0x10 */
	{ .name = "ORA", .opcode = NULL, .cycles = 5, .mode = ADDR_INY },
	{ .name = "XXX", .opcode = NULL, .cycles = 0, .mode = ADDR_XXX },
	{ .name = "XXX", .opcode = NULL, .cycles = 0, .mode = ADDR_XXX },
	{ .name = "XXX", .opcode = NULL, .cycles = 0, .mode = ADDR_XXX },
	{ .name = "ORA", .opcode = NULL, .cycles = 4, .mode = ADDR_IZX },
	{ .name = "ASL", .opcode = NULL, .cycles = 6, .mode = ADDR_IZX },
	{ .name = "XXX", .opcode = NULL, .cycles = 0, .mode = ADDR_XXX },
	{ .name = "CLC", .opcode = NULL, .cycles = 2, .mode = ADDR_IMP },
	{ .name = "ORA", .opcode = NULL, .cycles = 4, .mode = ADDR_IAY },
	{ .name = "XXX", .opcode = NULL, .cycles = 0, .mode = ADDR_XXX },
	{ .name = "XXX", .opcode = NULL, .cycles = 0, .mode = ADDR_XXX },
	{ .name = "XXX", .opcode = NULL, .cycles = 0, .mode = ADDR_XXX },
	{ .name = "ORA", .opcode = NULL, .cycles = 4, .mode = ADDR_IAX },
	{ .name = "ASL", .opcode = NULL, .cycles = 7, .mode = ADDR_IAX },
	{ .name = "XXX", .opcode = NULL, .cycles = 0, .mode = ADDR_XXX },

	{ .name = "JSR", .opcode = NULL, .cycles = 6, .mode = ABSOLUTE }, /* 0x20 */
	{ .name = "AND", .opcode = NULL, .cycles = 6, .mode = INDEXED },
	{ .name = "XXX", .opcode = NULL, .cycles = 0, .mode = UNKNOWN },
	{ .name = "XXX", .opcode = NULL, .cycles = 0, .mode = UNKNOWN },
	{ .name = "BIT", .opcode = NULL, .cycles = 3, .mode = ZERO_PAGE },
	{ .name = "AND", .opcode = NULL, .cycles = 3, .mode = ZERO_PAGE },
	{ .name = "ROL", .opcode = NULL, .cycles = 5, .mode = ZERO_PAGE },
	{ .name = "XXX", .opcode = NULL, .cycles = 0, .mode = UNKNOWN },
	{ .name = "PLP", .opcode = NULL, .cycles = 4, .mode = IMPLIED },
	{ .name = "AND", .opcode = NULL, .cycles = 2, .mode = IMMEDIATE },
	{ .name = "ROL", .opcode = NULL, .cycles = 2, .mode = ACCUMULATOR },
	{ .name = "XXX", .opcode = NULL, .cycles = 0, .mode = UNKNOWN },
	{ .name = "BIT", .opcode = NULL, .cycles = 4, .mode = ABSOLUTE },
	{ .name = "AND", .opcode = NULL, .cycles = 4, .mode = ABSOLUTE },
	{ .name = "ROL", .opcode = NULL, .cycles = 6, .mode = ABSOLUTE },
	{ .name = "XXX", .opcode = NULL, .cycles = 0, .mode = UNKNOWN },

	{.name = "ADC", .opcode = NULL, .cycles = 0, .mode = NULL}, /* 0x30 */
	{.name = "AND", .opcode = NULL, .cycles = 0, .mode = NULL},
	{.name = "ASL", .opcode = NULL, .cycles = 0, .mode = NULL},
	{.name = "BCC", .opcode = NULL, .cycles = 0, .mode = NULL},
	{.name = "BCS", .opcode = NULL, .cycles = 0, .mode = NULL},
	{.name = "BEQ", .opcode = NULL, .cycles = 0, .mode = NULL},
	{.name = "BIT", .opcode = NULL, .cycles = 0, .mode = NULL},
	{.name = "BMI", .opcode = NULL, .cycles = 0, .mode = NULL},
	{.name = "BNE", .opcode = NULL, .cycles = 0, .mode = NULL},
	{.name = "BPL", .opcode = NULL, .cycles = 0, .mode = NULL},
	{.name = "BRK", .opcode = NULL, .cycles = 0, .mode = NULL},
	{.name = "BVC", .opcode = NULL, .cycles = 0, .mode = NULL},
	{.name = "BVS", .opcode = NULL, .cycles = 0, .mode = NULL},
	{.name = "CLC", .opcode = NULL, .cycles = 0, .mode = NULL},
	{.name = "CLD", .opcode = NULL, .cycles = 0, .mode = NULL},
	{.name = "CLI", .opcode = NULL, .cycles = 0, .mode = NULL},
	{.name = "CLV", .opcode = NULL, .cycles = 0, .mode = NULL},
	{.name = "CMP", .opcode = NULL, .cycles = 0, .mode = NULL},
	{.name = "CPX", .opcode = NULL, .cycles = 0, .mode = NULL},
	{.name = "CPY", .opcode = NULL, .cycles = 0, .mode = NULL},
	{.name = "DEC", .opcode = NULL, .cycles = 0, .mode = NULL},
	{.name = "DEX", .opcode = NULL, .cycles = 0, .mode = NULL},
	{.name = "DEY", .opcode = NULL, .cycles = 0, .mode = NULL},
	{.name = "EOR", .opcode = NULL, .cycles = 0, .mode = NULL},
	{.name = "INC", .opcode = NULL, .cycles = 0, .mode = NULL},
	{.name = "INX", .opcode = NULL, .cycles = 0, .mode = NULL},
	{.name = "INY", .opcode = NULL, .cycles = 0, .mode = NULL},
	{.name = "JMP", .opcode = NULL, .cycles = 0, .mode = NULL},
	{.name = "JSR", .opcode = NULL, .cycles = 0, .mode = NULL},
	{.name = "LDA", .opcode = NULL, .cycles = 0, .mode = NULL},
	{.name = "LDX", .opcode = NULL, .cycles = 0, .mode = NULL},
	{.name = "LDY", .opcode = NULL, .cycles = 0, .mode = NULL},
	{.name = "LSR", .opcode = NULL, .cycles = 0, .mode = NULL},
	{.name = "NOP", .opcode = NULL, .cycles = 0, .mode = NULL},
	{.name = "ORA", .opcode = NULL, .cycles = 0, .mode = NULL},
	{.name = "PHA", .opcode = NULL, .cycles = 0, .mode = NULL},
	{.name = "PHP", .opcode = NULL, .cycles = 0, .mode = NULL},
	{.name = "PLA", .opcode = NULL, .cycles = 0, .mode = NULL},
	{.name = "PLP", .opcode = NULL, .cycles = 0, .mode = NULL},
	{.name = "ROL", .opcode = NULL, .cycles = 0, .mode = NULL},
	{.name = "ROR", .opcode = NULL, .cycles = 0, .mode = NULL},
	{.name = "RTI", .opcode = NULL, .cycles = 0, .mode = NULL},
	{.name = "RTS", .opcode = NULL, .cycles = 0, .mode = NULL},
	{.name = "SBC", .opcode = NULL, .cycles = 0, .mode = NULL},
	{.name = "SEC", .opcode = NULL, .cycles = 0, .mode = NULL},
	{.name = "SED", .opcode = NULL, .cycles = 0, .mode = NULL},
	{.name = "SEI", .opcode = NULL, .cycles = 0, .mode = NULL},
	{.name = "STA", .opcode = NULL, .cycles = 0, .mode = NULL},
	{.name = "STX", .opcode = NULL, .cycles = 0, .mode = NULL},
	{.name = "STY", .opcode = NULL, .cycles = 0, .mode = NULL},
	{.name = "TAX", .opcode = NULL, .cycles = 0, .mode = NULL},
	{.name = "TAY", .opcode = NULL, .cycles = 0, .mode = NULL},
	{.name = "TSX", .opcode = NULL, .cycles = 0, .mode = NULL},
	{.name = "TXA", .opcode = NULL, .cycles = 0, .mode = NULL},
	{.name = "TXS", .opcode = NULL, .cycles = 0, .mode = NULL},
	{.name = "TYA", .opcode = NULL, .cycles = 0, .mode = NULL}
};


static uint8_t fetch_opcode(mos6502 *, uint8_t *, uint8_t);
static uint8_t read_byte(uint8_t *, uint8_t, uint8_t);
static void OP_LDA(mos6502 *, uint8_t *, uint8_t, addr_mode);

static void
OP_LDA(mos6502 *cpu, uint8_t *mem, uint8_t cycles, addr_mode mode)
{
	uint8_t zp_addr;
	fprintf(stderr, "OP_LDA_%d\n", mode);

	if (mode == IMMEDIATE) {
		cpu->A = fetch_opcode(cpu, mem, cycles);
	}

	if (mode == ZERO_PAGE) {
		zp_addr = fetch_opcode(cpu, mem, cycles);
		cpu->A = read_byte(mem, zp_addr, cycles);
	}

	set_flag(cpu, MASK_ZERO, cpu->A == 0); /* TODO rewrite!! */
	set_flag(cpu, MASK_NEGATIVE, cpu->A & 0x80); /* TODO rewrite!! */
}

/* Unofficial instructions */
/*
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
*/

/* for unsupported or illegal opcodes */
/* static uint8_t OP_UNS(mos6502 *, uint16_t, uint8_t, addr_mode); */

/*static void clock(void);
static void reset(void);
static void set_irq(void);
static void set_nmi(void);*/

static uint8_t
fetch_opcode(mos6502 *cpu, uint8_t *mem, uint8_t cycles)
{
	uint8_t data = mem[cpu->PC];
	(cpu->PC)++;
	cycles--;
	return data;
}

static uint8_t
read_byte(uint8_t *mem, uint8_t address, uint8_t cycles)
{
	uint8_t data = mem[address];
	cycles--;
	return data;
}

void
execute(mos6502 *cpu, uint8_t *mem, uint8_t cycles)
{
	uint8_t opcode;

	while (cycles > 0) {
		opcode = fetch_opcode(cpu, mem, cycles);

		switch (opcode) {
			/* TODO arrange opcodes somehow */
			case 0xA9: /* INS_LDA_IM */
				OP_LDA(cpu, mem, cycles, IMMEDIATE);
				break;
			case 0xA5: /* INS_LDA_ZP */
				OP_LDA(cpu, mem, cycles, ZERO_PAGE);
				break;
			default:
				fprintf(stderr, "ERROR: UNKNOWN OPCODE!\n");
				break;
		}
	}
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
