#include <stdio.h>

#include "bus.h"
#include "cpu.h"

enum {
	MASK_CARRY = 0x01,
	MASK_ZERO = 0x02,
	MASK_INTERRUPT_DISABLE = 0x04,
	MASK_DECIMAL = 0x08,
	MASK_BREAK = 0x10,
	MASK_UNUSED = 0x20,
	MASK_OVERFLOW = 0x40,
	MASK_NEGATIVE = 0x80
};

enum {
	STACK_OFFSET = 0x0100,
};

enum {
	VECTOR_IRQ = 0xFFFE,
	VECTOR_RESET = 0xFFFC
};

enum { TCPF = 29781 }; /* total cycles per frame */

typedef void (*opcode_func)(r2A03 *);
typedef void (*addr_mode)(r2A03 *);

typedef struct {
	uint8_t idx;
	const char *name;
	opcode_func func;
	uint8_t size;
	uint8_t cycles;
	addr_mode mode;
} instruction;

static uint8_t is_acc_mode(r2A03 *);
static void disable_acc_mode(r2A03 *);

static void disassemble(r2A03 *);

static void write8_addr(r2A03 *, uint16_t, uint8_t);
static void write8(r2A03 *, uint8_t);
static void push8(r2A03 *, uint8_t);
static void push16(r2A03 *, uint16_t);
static uint8_t get8_addr(r2A03 *, uint16_t);
static uint8_t get8(r2A03 *);
static uint8_t read8(r2A03 *);
static uint8_t read8_indirect(r2A03 *, uint16_t);
static uint8_t pop8(r2A03 *);
static uint16_t get16_addr(r2A03 *, uint16_t);
static uint16_t pop16(r2A03 *);
static uint16_t read16(r2A03 *);
static uint16_t read16_indirect(r2A03 *, uint16_t);

static uint8_t getflag(r2A03 *, uint8_t);
static uint8_t get_c(r2A03 *);
static uint8_t get_z(r2A03 *);
static uint8_t get_i(r2A03 *);
static uint8_t get_b(r2A03 *);
static uint8_t get_v(r2A03 *);
static uint8_t get_n(r2A03 *);

static void setflag(r2A03 *, uint8_t);
static void set_c(r2A03 *);
static void set_z(r2A03 *);
static void set_i(r2A03 *);
static void set_d(r2A03 *);
static void set_b(r2A03 *);
static void set_v(r2A03 *);
static void set_n(r2A03 *);

static void unsetflag(r2A03 *, uint8_t);
static void unset_c(r2A03 *);
static void unset_z(r2A03 *);
static void unset_i(r2A03 *);
static void unset_d(r2A03 *);
static void unset_v(r2A03 *);
static void unset_n(r2A03 *);

static void upd_flag(r2A03 *, uint8_t, uint8_t);
static void upd_c(r2A03 *, uint8_t);
static void upd_z(r2A03 *, uint8_t);
static void upd_v(r2A03 *, uint8_t);
static void upd_n(r2A03 *, uint8_t);
static void upd_zn(r2A03 *, uint8_t);

static int overflowed_sum(uint8_t, uint8_t, uint8_t);
static int overflowed_sub(uint8_t, uint8_t, uint8_t);

static void setirq(r2A03 *, uint8_t);
static void setnmi(r2A03 *, uint8_t);

static void ADDR_ABS(r2A03 *); /* absolute */
static void ADDR_ACC(r2A03 *); /* accumulator */
static void ADDR_IAX(r2A03 *); /* indexed absolute x */
static void ADDR_IAY(r2A03 *); /* indexed absolute y */
static void ADDR_IMM(r2A03 *); /* immediate */
static void ADDR_IMP(r2A03 *); /* implied */
static void ADDR_IND(r2A03 *); /* indirect */
static void ADDR_INX(r2A03 *); /* indexed indirect x */
static void ADDR_INY(r2A03 *); /* indirect indexed y */
static void ADDR_IZX(r2A03 *); /* indexed zero page x */
static void ADDR_IZY(r2A03 *); /* indexed zero page y */
static void ADDR_REL(r2A03 *); /* relative */
static void ADDR_ZPG(r2A03 *); /* zero page */

static void ADDR_ILL(r2A03 *); /* illegal */

static void OP_ADC(r2A03 *);
static void OP_AND(r2A03 *);
static void OP_ASL(r2A03 *);
static void OP_BCC(r2A03 *);
static void OP_BCS(r2A03 *);
static void OP_BEQ(r2A03 *);
static void OP_BIT(r2A03 *);
static void OP_BMI(r2A03 *);
static void OP_BNE(r2A03 *);
static void OP_BPL(r2A03 *);
static void OP_BRK(r2A03 *);
static void OP_BVC(r2A03 *);
static void OP_BVS(r2A03 *);
static void OP_CLC(r2A03 *);
static void OP_CLD(r2A03 *);
static void OP_CLI(r2A03 *);
static void OP_CLV(r2A03 *);
static void OP_CMP(r2A03 *);
static void OP_CPX(r2A03 *);
static void OP_CPY(r2A03 *);
static void OP_DEC(r2A03 *);
static void OP_DEX(r2A03 *);
static void OP_DEY(r2A03 *);
static void OP_EOR(r2A03 *);
static void OP_INC(r2A03 *);
static void OP_INX(r2A03 *);
static void OP_INY(r2A03 *);
static void OP_JMP(r2A03 *);
static void OP_JSR(r2A03 *);
static void OP_LDA(r2A03 *);
static void OP_LDX(r2A03 *);
static void OP_LDY(r2A03 *);
static void OP_LSR(r2A03 *);
static void OP_NOP(r2A03 *);
static void OP_ORA(r2A03 *);
static void OP_PHA(r2A03 *);
static void OP_PHP(r2A03 *);
static void OP_PLA(r2A03 *);
static void OP_PLP(r2A03 *);
static void OP_ROL(r2A03 *);
static void OP_ROR(r2A03 *);
static void OP_RTI(r2A03 *);
static void OP_RTS(r2A03 *);
static void OP_SBC(r2A03 *);
static void OP_SEC(r2A03 *);
static void OP_SED(r2A03 *);
static void OP_SEI(r2A03 *);
static void OP_STA(r2A03 *);
static void OP_STX(r2A03 *);
static void OP_STY(r2A03 *);
static void OP_TAX(r2A03 *);
static void OP_TAY(r2A03 *);
static void OP_TSX(r2A03 *);
static void OP_TXA(r2A03 *);
static void OP_TXS(r2A03 *);
static void OP_TYA(r2A03 *);

static void OP_ALR(r2A03 *);
static void OP_ANE(r2A03 *);
static void OP_ARR(r2A03 *);
static void OP_ANC(r2A03 *);
static void OP_DCP(r2A03 *);
static void OP_ISC(r2A03 *);
static void OP_LAS(r2A03 *);
static void OP_LAX(r2A03 *);
static void OP_LXA(r2A03 *);
static void OP_RLA(r2A03 *);
static void OP_RRA(r2A03 *);
static void OP_SAX(r2A03 *);
static void OP_SBX(r2A03 *);
static void OP_SHA(r2A03 *);
static void OP_SHX(r2A03 *);
static void OP_SHY(r2A03 *);
static void OP_SLO(r2A03 *);
static void OP_SRE(r2A03 *);
static void OP_TAS(r2A03 *);

static void OP_ILL(r2A03 *); /* illegal */

static instruction
optable[0xFF + 1] = {
	{ .idx = 0x00, .name = "BRK", .func = OP_BRK, .cycles = 7, .mode = ADDR_IMP },
	{ .idx = 0x01, .name = "ORA", .func = OP_ORA, .cycles = 6, .mode = ADDR_INX },
	{ .idx = 0x02, .name = "ILL", .func = OP_ILL, .cycles = 0, .mode = ADDR_ILL },
	{ .idx = 0x03, .name = "SLO", .func = OP_SLO, .cycles = 0, .mode = ADDR_INX },
	{ .idx = 0x04, .name = "NOP", .func = OP_NOP, .cycles = 0, .mode = ADDR_ZPG },
	{ .idx = 0x05, .name = "ORA", .func = OP_ORA, .cycles = 3, .mode = ADDR_ZPG },
	{ .idx = 0x06, .name = "ASL", .func = OP_ASL, .cycles = 5, .mode = ADDR_ZPG },
	{ .idx = 0x07, .name = "SLO", .func = OP_SLO, .cycles = 0, .mode = ADDR_ZPG },
	{ .idx = 0x08, .name = "PHP", .func = OP_PHP, .cycles = 3, .mode = ADDR_IMP },
	{ .idx = 0x09, .name = "ORA", .func = OP_ORA, .cycles = 2, .mode = ADDR_IMM },
	{ .idx = 0x0A, .name = "ASL", .func = OP_ASL, .cycles = 2, .mode = ADDR_ACC },
	{ .idx = 0x0B, .name = "ANC", .func = OP_ANC, .cycles = 0, .mode = ADDR_IMM },
	{ .idx = 0x0C, .name = "NOP", .func = OP_NOP, .cycles = 0, .mode = ADDR_ABS },
	{ .idx = 0x0D, .name = "ORA", .func = OP_ORA, .cycles = 4, .mode = ADDR_ABS },
	{ .idx = 0x0E, .name = "ASL", .func = OP_ASL, .cycles = 6, .mode = ADDR_ABS },
	{ .idx = 0x0F, .name = "SLO", .func = OP_SLO, .cycles = 0, .mode = ADDR_ABS },

	{ .idx = 0x10, .name = "BPL", .func = OP_BPL, .cycles = 2, .mode = ADDR_REL },
	{ .idx = 0x11, .name = "ORA", .func = OP_ORA, .cycles = 5, .mode = ADDR_INY },
	{ .idx = 0x12, .name = "ILL", .func = OP_ILL, .cycles = 0, .mode = ADDR_ILL },
	{ .idx = 0x13, .name = "SLO", .func = OP_SLO, .cycles = 0, .mode = ADDR_INY },
	{ .idx = 0x14, .name = "NOP", .func = OP_NOP, .cycles = 0, .mode = ADDR_IZX },
	{ .idx = 0x15, .name = "ORA", .func = OP_ORA, .cycles = 4, .mode = ADDR_IZX },
	{ .idx = 0x16, .name = "ASL", .func = OP_ASL, .cycles = 6, .mode = ADDR_IZX },
	{ .idx = 0x17, .name = "SLO", .func = OP_SLO, .cycles = 0, .mode = ADDR_IZX },
	{ .idx = 0x18, .name = "CLC", .func = OP_CLC, .cycles = 2, .mode = ADDR_IMP },
	{ .idx = 0x19, .name = "ORA", .func = OP_ORA, .cycles = 4, .mode = ADDR_IAY },
	{ .idx = 0x1A, .name = "NOP", .func = OP_NOP, .cycles = 0, .mode = ADDR_IMP },
	{ .idx = 0x1B, .name = "SLO", .func = OP_SLO, .cycles = 0, .mode = ADDR_IAY },
	{ .idx = 0x1C, .name = "NOP", .func = OP_NOP, .cycles = 0, .mode = ADDR_IAX },
	{ .idx = 0x1D, .name = "ORA", .func = OP_ORA, .cycles = 4, .mode = ADDR_IAX },
	{ .idx = 0x1E, .name = "ASL", .func = OP_ASL, .cycles = 7, .mode = ADDR_IAX },
	{ .idx = 0x1F, .name = "SLO", .func = OP_SLO, .cycles = 0, .mode = ADDR_IAX },

	{ .idx = 0x20, .name = "JSR", .func = OP_JSR, .cycles = 6, .mode = ADDR_ABS },
	{ .idx = 0x21, .name = "AND", .func = OP_AND, .cycles = 6, .mode = ADDR_INX },
	{ .idx = 0x22, .name = "ILL", .func = OP_ILL, .cycles = 0, .mode = ADDR_ILL },
	{ .idx = 0x23, .name = "RLA", .func = OP_RLA, .cycles = 0, .mode = ADDR_INX },
	{ .idx = 0x24, .name = "BIT", .func = OP_BIT, .cycles = 3, .mode = ADDR_ZPG },
	{ .idx = 0x25, .name = "AND", .func = OP_AND, .cycles = 3, .mode = ADDR_ZPG },
	{ .idx = 0x26, .name = "ROL", .func = OP_ROL, .cycles = 5, .mode = ADDR_ZPG },
	{ .idx = 0x27, .name = "RLA", .func = OP_RLA, .cycles = 0, .mode = ADDR_IZX },
	{ .idx = 0x28, .name = "PLP", .func = OP_PLP, .cycles = 4, .mode = ADDR_IMP },
	{ .idx = 0x29, .name = "AND", .func = OP_AND, .cycles = 2, .mode = ADDR_IMM },
	{ .idx = 0x2A, .name = "ROL", .func = OP_ROL, .cycles = 2, .mode = ADDR_ACC },
	{ .idx = 0x2B, .name = "ANC", .func = OP_ANC, .cycles = 0, .mode = ADDR_IMM },
	{ .idx = 0x2C, .name = "BIT", .func = OP_BIT, .cycles = 4, .mode = ADDR_ABS },
	{ .idx = 0x2D, .name = "AND", .func = OP_AND, .cycles = 4, .mode = ADDR_ABS },
	{ .idx = 0x2E, .name = "ROL", .func = OP_ROL, .cycles = 6, .mode = ADDR_ABS },
	{ .idx = 0x2F, .name = "RLA", .func = OP_RLA, .cycles = 0, .mode = ADDR_ABS },

	{ .idx = 0x30, .name = "BMI", .func = OP_BMI, .cycles = 2, .mode = ADDR_REL },
	{ .idx = 0x31, .name = "AND", .func = OP_AND, .cycles = 5, .mode = ADDR_INY },
	{ .idx = 0x32, .name = "ILL", .func = OP_ILL, .cycles = 0, .mode = ADDR_ILL },
	{ .idx = 0x33, .name = "RLA", .func = OP_RLA, .cycles = 0, .mode = ADDR_INY },
	{ .idx = 0x34, .name = "NOP", .func = OP_NOP, .cycles = 0, .mode = ADDR_IZX },
	{ .idx = 0x35, .name = "AND", .func = OP_AND, .cycles = 4, .mode = ADDR_IZX },
	{ .idx = 0x36, .name = "ROL", .func = OP_ROL, .cycles = 6, .mode = ADDR_IZX },
	{ .idx = 0x37, .name = "RLA", .func = OP_RLA, .cycles = 0, .mode = ADDR_IZX },
	{ .idx = 0x38, .name = "SEC", .func = OP_SEC, .cycles = 2, .mode = ADDR_IMP },
	{ .idx = 0x39, .name = "AND", .func = OP_AND, .cycles = 4, .mode = ADDR_IAY },
	{ .idx = 0x3A, .name = "NOP", .func = OP_NOP, .cycles = 0, .mode = ADDR_IMP },
	{ .idx = 0x3B, .name = "RLA", .func = OP_RLA, .cycles = 0, .mode = ADDR_IAY },
	{ .idx = 0x3C, .name = "NOP", .func = OP_NOP, .cycles = 0, .mode = ADDR_IAX },
	{ .idx = 0x3D, .name = "AND", .func = OP_AND, .cycles = 4, .mode = ADDR_IAX },
	{ .idx = 0x3E, .name = "ROL", .func = OP_ROL, .cycles = 7, .mode = ADDR_IAX },
	{ .idx = 0x3F, .name = "RLA", .func = OP_RLA, .cycles = 0, .mode = ADDR_IAX },

	{ .idx = 0x40, .name = "RTI", .func = OP_RTI, .cycles = 6, .mode = ADDR_IMP },
	{ .idx = 0x41, .name = "EOR", .func = OP_EOR, .cycles = 6, .mode = ADDR_INX },
	{ .idx = 0x42, .name = "ILL", .func = OP_ILL, .cycles = 0, .mode = ADDR_ILL },
	{ .idx = 0x43, .name = "SRE", .func = OP_SRE, .cycles = 0, .mode = ADDR_INX },
	{ .idx = 0x44, .name = "NOP", .func = OP_NOP, .cycles = 0, .mode = ADDR_ZPG },
	{ .idx = 0x45, .name = "EOR", .func = OP_EOR, .cycles = 3, .mode = ADDR_ZPG },
	{ .idx = 0x46, .name = "LSR", .func = OP_LSR, .cycles = 5, .mode = ADDR_ZPG },
	{ .idx = 0x47, .name = "SRE", .func = OP_SRE, .cycles = 0, .mode = ADDR_ZPG },
	{ .idx = 0x48, .name = "PHA", .func = OP_PHA, .cycles = 3, .mode = ADDR_IMP },
	{ .idx = 0x49, .name = "EOR", .func = OP_EOR, .cycles = 2, .mode = ADDR_IMM },
	{ .idx = 0x4A, .name = "LSR", .func = OP_LSR, .cycles = 2, .mode = ADDR_ACC },
	{ .idx = 0x4B, .name = "ALR", .func = OP_ALR, .cycles = 0, .mode = ADDR_IMM },
	{ .idx = 0x4C, .name = "JMP", .func = OP_JMP, .cycles = 3, .mode = ADDR_ABS },
	{ .idx = 0x4D, .name = "EOR", .func = OP_EOR, .cycles = 4, .mode = ADDR_ABS },
	{ .idx = 0x4E, .name = "LSR", .func = OP_LSR, .cycles = 6, .mode = ADDR_ABS },
	{ .idx = 0x4F, .name = "SRE", .func = OP_SRE, .cycles = 0, .mode = ADDR_ABS },

	{ .idx = 0x50, .name = "BVC", .func = OP_BVC, .cycles = 2, .mode = ADDR_REL },
	{ .idx = 0x51, .name = "EOR", .func = OP_EOR, .cycles = 5, .mode = ADDR_INY },
	{ .idx = 0x52, .name = "ILL", .func = OP_ILL, .cycles = 0, .mode = ADDR_ILL },
	{ .idx = 0x53, .name = "SRE", .func = OP_SRE, .cycles = 0, .mode = ADDR_INY },
	{ .idx = 0x54, .name = "NOP", .func = OP_NOP, .cycles = 0, .mode = ADDR_IZX },
	{ .idx = 0x55, .name = "EOR", .func = OP_EOR, .cycles = 4, .mode = ADDR_IZX },
	{ .idx = 0x56, .name = "LSR", .func = OP_LSR, .cycles = 6, .mode = ADDR_IZX },
	{ .idx = 0x57, .name = "SRE", .func = OP_SRE, .cycles = 0, .mode = ADDR_IZX },
	{ .idx = 0x58, .name = "CLI", .func = OP_CLI, .cycles = 2, .mode = ADDR_IMP },
	{ .idx = 0x59, .name = "EOR", .func = OP_EOR, .cycles = 4, .mode = ADDR_IAY },
	{ .idx = 0x5A, .name = "NOP", .func = OP_NOP, .cycles = 0, .mode = ADDR_IMP },
	{ .idx = 0x5B, .name = "SRE", .func = OP_SRE, .cycles = 0, .mode = ADDR_IAX },
	{ .idx = 0x5C, .name = "NOP", .func = OP_NOP, .cycles = 0, .mode = ADDR_IAX },
	{ .idx = 0x5D, .name = "EOR", .func = OP_EOR, .cycles = 4, .mode = ADDR_IAX },
	{ .idx = 0x5E, .name = "LSR", .func = OP_LSR, .cycles = 7, .mode = ADDR_IAX },
	{ .idx = 0x5F, .name = "SRE", .func = OP_SRE, .cycles = 0, .mode = ADDR_IAX },

	{ .idx = 0x60, .name = "RTS", .func = OP_RTS, .cycles = 6, .mode = ADDR_IMP },
	{ .idx = 0x61, .name = "ADC", .func = OP_ADC, .cycles = 6, .mode = ADDR_INX },
	{ .idx = 0x62, .name = "ILL", .func = OP_ILL, .cycles = 0, .mode = ADDR_ILL },
	{ .idx = 0x63, .name = "RRA", .func = OP_RRA, .cycles = 0, .mode = ADDR_INX },
	{ .idx = 0x64, .name = "NOP", .func = OP_NOP, .cycles = 0, .mode = ADDR_ZPG },
	{ .idx = 0x65, .name = "ADC", .func = OP_ADC, .cycles = 3, .mode = ADDR_ZPG },
	{ .idx = 0x66, .name = "ROR", .func = OP_ROR, .cycles = 5, .mode = ADDR_ZPG },
	{ .idx = 0x67, .name = "RRA", .func = OP_RRA, .cycles = 0, .mode = ADDR_ZPG },
	{ .idx = 0x68, .name = "PLA", .func = OP_PLA, .cycles = 4, .mode = ADDR_IMP },
	{ .idx = 0x69, .name = "ADC", .func = OP_ADC, .cycles = 2, .mode = ADDR_IMM },
	{ .idx = 0x6A, .name = "ROR", .func = OP_ROR, .cycles = 2, .mode = ADDR_ACC },
	{ .idx = 0x6B, .name = "ARR", .func = OP_ARR, .cycles = 0, .mode = ADDR_IMM },
	{ .idx = 0x6C, .name = "JMP", .func = OP_JMP, .cycles = 5, .mode = ADDR_IND },
	{ .idx = 0x6D, .name = "ADC", .func = OP_ADC, .cycles = 4, .mode = ADDR_ABS },
	{ .idx = 0x6E, .name = "ROR", .func = OP_ROR, .cycles = 6, .mode = ADDR_ABS },
	{ .idx = 0x6F, .name = "RRA", .func = OP_RRA, .cycles = 0, .mode = ADDR_ABS },

	{ .idx = 0x70, .name = "BVS", .func = OP_BVS, .cycles = 2, .mode = ADDR_REL },
	{ .idx = 0x71, .name = "ADC", .func = OP_ADC, .cycles = 5, .mode = ADDR_INY },
	{ .idx = 0x72, .name = "ILL", .func = OP_ILL, .cycles = 0, .mode = ADDR_ILL },
	{ .idx = 0x73, .name = "RRA", .func = OP_RRA, .cycles = 0, .mode = ADDR_INY },
	{ .idx = 0x74, .name = "NOP", .func = OP_NOP, .cycles = 0, .mode = ADDR_IZX },
	{ .idx = 0x75, .name = "ADC", .func = OP_ADC, .cycles = 4, .mode = ADDR_IZX },
	{ .idx = 0x76, .name = "ROR", .func = OP_ROR, .cycles = 6, .mode = ADDR_IZX },
	{ .idx = 0x77, .name = "RRA", .func = OP_RRA, .cycles = 0, .mode = ADDR_IZX },
	{ .idx = 0x78, .name = "SEI", .func = OP_SEI, .cycles = 2, .mode = ADDR_IMP },
	{ .idx = 0x79, .name = "ADC", .func = OP_ADC, .cycles = 4, .mode = ADDR_IAY },
	{ .idx = 0x7A, .name = "NOP", .func = OP_NOP, .cycles = 0, .mode = ADDR_IMP },
	{ .idx = 0x7B, .name = "RRA", .func = OP_RRA, .cycles = 0, .mode = ADDR_IAY },
	{ .idx = 0x7C, .name = "NOP", .func = OP_NOP, .cycles = 0, .mode = ADDR_IAX },
	{ .idx = 0x7D, .name = "ADC", .func = OP_ADC, .cycles = 4, .mode = ADDR_IAX },
	{ .idx = 0x7E, .name = "ROR", .func = OP_ROR, .cycles = 7, .mode = ADDR_IAX },
	{ .idx = 0x7F, .name = "RRA", .func = OP_RRA, .cycles = 0, .mode = ADDR_IAX },

	{ .idx = 0x80, .name = "NOP", .func = OP_NOP, .cycles = 0, .mode = ADDR_IMM },
	{ .idx = 0x81, .name = "STA", .func = OP_STA, .cycles = 6, .mode = ADDR_INX },
	{ .idx = 0x82, .name = "NOP", .func = OP_NOP, .cycles = 0, .mode = ADDR_IMM },
	{ .idx = 0x83, .name = "SAX", .func = OP_SAX, .cycles = 0, .mode = ADDR_INX },
	{ .idx = 0x84, .name = "STY", .func = OP_STY, .cycles = 3, .mode = ADDR_ZPG },
	{ .idx = 0x85, .name = "STA", .func = OP_STA, .cycles = 3, .mode = ADDR_ZPG },
	{ .idx = 0x86, .name = "STX", .func = OP_STX, .cycles = 3, .mode = ADDR_ZPG },
	{ .idx = 0x87, .name = "SAX", .func = OP_SAX, .cycles = 0, .mode = ADDR_ZPG },
	{ .idx = 0x88, .name = "DEY", .func = OP_DEY, .cycles = 2, .mode = ADDR_IMP },
	{ .idx = 0x89, .name = "NOP", .func = OP_NOP, .cycles = 0, .mode = ADDR_IMM },
	{ .idx = 0x8A, .name = "TXA", .func = OP_TXA, .cycles = 2, .mode = ADDR_IMP },
	{ .idx = 0x8B, .name = "ANE", .func = OP_ANE, .cycles = 0, .mode = ADDR_IMM },
	{ .idx = 0x8C, .name = "STY", .func = OP_STY, .cycles = 4, .mode = ADDR_ABS },
	{ .idx = 0x8D, .name = "STA", .func = OP_STA, .cycles = 4, .mode = ADDR_ABS },
	{ .idx = 0x8E, .name = "STX", .func = OP_STX, .cycles = 4, .mode = ADDR_ABS },
	{ .idx = 0x8F, .name = "SAX", .func = OP_SAX, .cycles = 0, .mode = ADDR_ABS },

	{ .idx = 0x90, .name = "BCC", .func = OP_BCC, .cycles = 2, .mode = ADDR_REL },
	{ .idx = 0x91, .name = "STA", .func = OP_STA, .cycles = 6, .mode = ADDR_INY },
	{ .idx = 0x92, .name = "ILL", .func = OP_ILL, .cycles = 0, .mode = ADDR_ILL },
	{ .idx = 0x93, .name = "SHA", .func = OP_SHA, .cycles = 0, .mode = ADDR_INY },
	{ .idx = 0x94, .name = "STY", .func = OP_STY, .cycles = 4, .mode = ADDR_IZX },
	{ .idx = 0x95, .name = "STA", .func = OP_STA, .cycles = 4, .mode = ADDR_IZX },
	{ .idx = 0x96, .name = "STX", .func = OP_STX, .cycles = 4, .mode = ADDR_IZY },
	{ .idx = 0x97, .name = "SAX", .func = OP_SAX, .cycles = 0, .mode = ADDR_IZY },
	{ .idx = 0x98, .name = "TYA", .func = OP_TYA, .cycles = 2, .mode = ADDR_IMP },
	{ .idx = 0x99, .name = "STA", .func = OP_STA, .cycles = 5, .mode = ADDR_IAY },
	{ .idx = 0x9A, .name = "TXS", .func = OP_TXS, .cycles = 2, .mode = ADDR_IMP },
	{ .idx = 0x9B, .name = "TAS", .func = OP_TAS, .cycles = 0, .mode = ADDR_IAY },
	{ .idx = 0x9C, .name = "SHY", .func = OP_SHY, .cycles = 0, .mode = ADDR_IAX },
	{ .idx = 0x9D, .name = "STA", .func = OP_STA, .cycles = 5, .mode = ADDR_IAX },
	{ .idx = 0x9E, .name = "SHX", .func = OP_SHX, .cycles = 0, .mode = ADDR_IAY },
	{ .idx = 0x9F, .name = "SHA", .func = OP_SHA, .cycles = 0, .mode = ADDR_IAY },

	{ .idx = 0xA0, .name = "LDY", .func = OP_LDY, .cycles = 2, .mode = ADDR_IMM },
	{ .idx = 0xA1, .name = "LDA", .func = OP_LDA, .cycles = 6, .mode = ADDR_INX },
	{ .idx = 0xA2, .name = "LDX", .func = OP_LDX, .cycles = 2, .mode = ADDR_IMM },
	{ .idx = 0xA3, .name = "LAX", .func = OP_LAX, .cycles = 0, .mode = ADDR_INX },
	{ .idx = 0xA4, .name = "LDY", .func = OP_LDY, .cycles = 3, .mode = ADDR_ZPG },
	{ .idx = 0xA5, .name = "LDA", .func = OP_LDA, .cycles = 3, .mode = ADDR_ZPG },
	{ .idx = 0xA6, .name = "LDX", .func = OP_LDX, .cycles = 3, .mode = ADDR_ZPG },
	{ .idx = 0xA7, .name = "LAX", .func = OP_LAX, .cycles = 0, .mode = ADDR_ZPG },
	{ .idx = 0xA8, .name = "TAY", .func = OP_TAY, .cycles = 2, .mode = ADDR_IMP },
	{ .idx = 0xA9, .name = "LDA", .func = OP_LDA, .cycles = 2, .mode = ADDR_IMM },
	{ .idx = 0xAA, .name = "TAX", .func = OP_TAX, .cycles = 2, .mode = ADDR_IMP },
	{ .idx = 0xAB, .name = "LXA", .func = OP_LXA, .cycles = 0, .mode = ADDR_IMM },
	{ .idx = 0xAC, .name = "LDY", .func = OP_LDY, .cycles = 4, .mode = ADDR_ABS },
	{ .idx = 0xAD, .name = "LDA", .func = OP_LDA, .cycles = 4, .mode = ADDR_ABS },
	{ .idx = 0xAE, .name = "LDX", .func = OP_LDX, .cycles = 4, .mode = ADDR_ABS },
	{ .idx = 0xAF, .name = "LAX", .func = OP_LAX, .cycles = 0, .mode = ADDR_ABS },

	{ .idx = 0xB0, .name = "BCS", .func = OP_BCS, .cycles = 2, .mode = ADDR_REL },
	{ .idx = 0xB1, .name = "LDA", .func = OP_LDA, .cycles = 5, .mode = ADDR_INY },
	{ .idx = 0xB2, .name = "ILL", .func = OP_ILL, .cycles = 0, .mode = ADDR_ILL },
	{ .idx = 0xB3, .name = "LAX", .func = OP_LAX, .cycles = 0, .mode = ADDR_INY },
	{ .idx = 0xB4, .name = "LDY", .func = OP_LDY, .cycles = 4, .mode = ADDR_IZX },
	{ .idx = 0xB5, .name = "LDA", .func = OP_LDA, .cycles = 4, .mode = ADDR_IZX },
	{ .idx = 0xB6, .name = "LDX", .func = OP_LDX, .cycles = 4, .mode = ADDR_IZY },
	{ .idx = 0xB7, .name = "LAX", .func = OP_LAX, .cycles = 0, .mode = ADDR_IZY },
	{ .idx = 0xB8, .name = "CLV", .func = OP_CLV, .cycles = 2, .mode = ADDR_IMP },
	{ .idx = 0xB9, .name = "LDA", .func = OP_LDA, .cycles = 4, .mode = ADDR_IAY },
	{ .idx = 0xBA, .name = "TSX", .func = OP_TSX, .cycles = 2, .mode = ADDR_IMP },
	{ .idx = 0xBB, .name = "LAS", .func = OP_LAS, .cycles = 0, .mode = ADDR_IAY },
	{ .idx = 0xBC, .name = "LDY", .func = OP_LDY, .cycles = 4, .mode = ADDR_IAX },
	{ .idx = 0xBD, .name = "LDA", .func = OP_LDA, .cycles = 4, .mode = ADDR_IAX },
	{ .idx = 0xBE, .name = "LDX", .func = OP_LDX, .cycles = 4, .mode = ADDR_IAY },
	{ .idx = 0xBF, .name = "LAX", .func = OP_LAX, .cycles = 0, .mode = ADDR_IAY },

	{ .idx = 0xC0, .name = "CPY", .func = OP_CPY, .cycles = 2, .mode = ADDR_IMM },
	{ .idx = 0xC1, .name = "CMP", .func = OP_CMP, .cycles = 6, .mode = ADDR_INX },
	{ .idx = 0xC2, .name = "NOP", .func = OP_NOP, .cycles = 0, .mode = ADDR_IMM },
	{ .idx = 0xC3, .name = "DCP", .func = OP_DCP, .cycles = 0, .mode = ADDR_INX },
	{ .idx = 0xC4, .name = "CPY", .func = OP_CPY, .cycles = 3, .mode = ADDR_ZPG },
	{ .idx = 0xC5, .name = "CMP", .func = OP_CMP, .cycles = 3, .mode = ADDR_ZPG },
	{ .idx = 0xC6, .name = "DEC", .func = OP_DEC, .cycles = 5, .mode = ADDR_ZPG },
	{ .idx = 0xC7, .name = "DCP", .func = OP_DCP, .cycles = 0, .mode = ADDR_ZPG },
	{ .idx = 0xC8, .name = "INY", .func = OP_INY, .cycles = 2, .mode = ADDR_IMP },
	{ .idx = 0xC9, .name = "CMP", .func = OP_CMP, .cycles = 2, .mode = ADDR_IMM },
	{ .idx = 0xCA, .name = "DEX", .func = OP_DEX, .cycles = 2, .mode = ADDR_IMP },
	{ .idx = 0xCB, .name = "SBX", .func = OP_SBX, .cycles = 0, .mode = ADDR_IMM },
	{ .idx = 0xCC, .name = "CPY", .func = OP_CPY, .cycles = 4, .mode = ADDR_ABS },
	{ .idx = 0xCD, .name = "CMP", .func = OP_CMP, .cycles = 4, .mode = ADDR_ABS },
	{ .idx = 0xCE, .name = "DEC", .func = OP_DEC, .cycles = 6, .mode = ADDR_ABS },
	{ .idx = 0xCF, .name = "DCP", .func = OP_DCP, .cycles = 0, .mode = ADDR_ABS },

	{ .idx = 0xD0, .name = "BNE", .func = OP_BNE, .cycles = 2, .mode = ADDR_REL },
	{ .idx = 0xD1, .name = "CMP", .func = OP_CMP, .cycles = 5, .mode = ADDR_INY },
	{ .idx = 0xD2, .name = "ILL", .func = OP_ILL, .cycles = 0, .mode = ADDR_ILL },
	{ .idx = 0xD3, .name = "DCP", .func = OP_DCP, .cycles = 0, .mode = ADDR_INY },
	{ .idx = 0xD4, .name = "NOP", .func = OP_NOP, .cycles = 0, .mode = ADDR_IZX },
	{ .idx = 0xD5, .name = "CMP", .func = OP_CMP, .cycles = 4, .mode = ADDR_IZX },
	{ .idx = 0xD6, .name = "DEC", .func = OP_DEC, .cycles = 6, .mode = ADDR_IZX },
	{ .idx = 0xD7, .name = "DCP", .func = OP_DCP, .cycles = 0, .mode = ADDR_IZX },
	{ .idx = 0xD8, .name = "CLD", .func = OP_CLD, .cycles = 2, .mode = ADDR_IMP },
	{ .idx = 0xD9, .name = "CMP", .func = OP_CMP, .cycles = 4, .mode = ADDR_IAY },
	{ .idx = 0xDA, .name = "NOP", .func = OP_NOP, .cycles = 0, .mode = ADDR_IMP },
	{ .idx = 0xDB, .name = "DCP", .func = OP_DCP, .cycles = 0, .mode = ADDR_IAY },
	{ .idx = 0xDC, .name = "NOP", .func = OP_NOP, .cycles = 0, .mode = ADDR_IAX },
	{ .idx = 0xDD, .name = "CMP", .func = OP_CMP, .cycles = 4, .mode = ADDR_IAX },
	{ .idx = 0xDE, .name = "DEC", .func = OP_DEC, .cycles = 7, .mode = ADDR_IAX },
	{ .idx = 0xDF, .name = "ISC", .func = OP_ISC, .cycles = 0, .mode = ADDR_IAX },

	{ .idx = 0xE0, .name = "CPX", .func = OP_CPX, .cycles = 2, .mode = ADDR_IMM },
	{ .idx = 0xE1, .name = "SBC", .func = OP_SBC, .cycles = 6, .mode = ADDR_INX },
	{ .idx = 0xE2, .name = "NOP", .func = OP_NOP, .cycles = 0, .mode = ADDR_IMM },
	{ .idx = 0xE3, .name = "ISC", .func = OP_ISC, .cycles = 0, .mode = ADDR_INX },
	{ .idx = 0xE4, .name = "CPX", .func = OP_CPX, .cycles = 3, .mode = ADDR_ZPG },
	{ .idx = 0xE5, .name = "SBC", .func = OP_SBC, .cycles = 3, .mode = ADDR_ZPG },
	{ .idx = 0xE6, .name = "INC", .func = OP_INC, .cycles = 5, .mode = ADDR_ZPG },
	{ .idx = 0xE7, .name = "ISC", .func = OP_ISC, .cycles = 0, .mode = ADDR_ZPG },
	{ .idx = 0xE8, .name = "INX", .func = OP_INX, .cycles = 2, .mode = ADDR_IMP },
	{ .idx = 0xE9, .name = "SBC", .func = OP_SBC, .cycles = 2, .mode = ADDR_IMM },
	{ .idx = 0xEA, .name = "NOP", .func = OP_NOP, .cycles = 2, .mode = ADDR_IMP },
	{ .idx = 0xEB, .name = "SBC", .func = OP_SBC, .cycles = 0, .mode = ADDR_IMM },
	{ .idx = 0xEC, .name = "CPX", .func = OP_CPX, .cycles = 4, .mode = ADDR_ABS },
	{ .idx = 0xED, .name = "SBC", .func = OP_SBC, .cycles = 4, .mode = ADDR_ABS },
	{ .idx = 0xEE, .name = "INC", .func = OP_INC, .cycles = 6, .mode = ADDR_ABS },
	{ .idx = 0xEF, .name = "ISC", .func = OP_ISC, .cycles = 0, .mode = ADDR_ABS },

	{ .idx = 0xF0, .name = "BEQ", .func = OP_BEQ, .cycles = 2, .mode = ADDR_REL },
	{ .idx = 0xF1, .name = "SBC", .func = OP_SBC, .cycles = 5, .mode = ADDR_INY },
	{ .idx = 0xF2, .name = "ILL", .func = OP_ILL, .cycles = 0, .mode = ADDR_ILL },
	{ .idx = 0xF3, .name = "ISC", .func = OP_ISC, .cycles = 0, .mode = ADDR_INY },
	{ .idx = 0xF4, .name = "NOP", .func = OP_NOP, .cycles = 0, .mode = ADDR_IZX },
	{ .idx = 0xF5, .name = "SBC", .func = OP_SBC, .cycles = 4, .mode = ADDR_IZX },
	{ .idx = 0xF6, .name = "INC", .func = OP_INC, .cycles = 6, .mode = ADDR_IZX },
	{ .idx = 0xF7, .name = "ISC", .func = OP_ISC, .cycles = 0, .mode = ADDR_IZX },
	{ .idx = 0xF8, .name = "SED", .func = OP_SED, .cycles = 2, .mode = ADDR_IMP },
	{ .idx = 0xF9, .name = "SBC", .func = OP_SBC, .cycles = 4, .mode = ADDR_IAY },
	{ .idx = 0xFA, .name = "NOP", .func = OP_NOP, .cycles = 0, .mode = ADDR_IMP },
	{ .idx = 0xFB, .name = "ISC", .func = OP_ISC, .cycles = 0, .mode = ADDR_IAY },
	{ .idx = 0xFC, .name = "NOP", .func = OP_NOP, .cycles = 0, .mode = ADDR_IAX },
	{ .idx = 0xFD, .name = "SBC", .func = OP_SBC, .cycles = 4, .mode = ADDR_IAX },
	{ .idx = 0xFE, .name = "INC", .func = OP_INC, .cycles = 7, .mode = ADDR_IAX },
	{ .idx = 0xFF, .name = "ISC", .func = OP_ISC, .cycles = 0, .mode = ADDR_IAX }
};

static void
write_acc(r2A03 *cpu, uint8_t data)
{
	cpu->A = data;
}

static uint8_t
get_acc(r2A03 *cpu)
{
	return cpu->A;
}

static uint8_t
is_acc_mode(r2A03 *cpu)
{
	return cpu->acc_mode;
}

static void
disable_acc_mode(r2A03 *cpu)
{
	cpu->acc_mode = 0;
}

static void
write8_addr(r2A03 *cpu, uint16_t addr, uint8_t data)
{
	bus_ram_write(cpu->bus, addr, data);
}

static void
write8(r2A03 *cpu, uint8_t data)
{
	write8_addr(cpu, cpu->addr, data);
}

static void
push8(r2A03 *cpu, uint8_t data)
{
	uint16_t addr = STACK_OFFSET + cpu->SP;
	cpu->SP--;
	write8_addr(cpu, addr, data);
}

static void
push16(r2A03 *cpu, uint16_t data)
{
	push8(cpu, data >> 8);
	push8(cpu, data & 0x00FF);
}

static uint8_t
get8_addr(r2A03 *cpu, uint16_t addr)
{
	return bus_ram_read(cpu->bus, addr);
}

static uint8_t
get8(r2A03 *cpu)
{
	return get8_addr(cpu, cpu->addr);
}

static uint16_t
get16_addr(r2A03 *cpu, uint16_t addr)
{
	uint8_t lo = get8_addr(cpu, addr);
	uint8_t hi = get8_addr(cpu, addr + 1); /* TODO: check address somehow */
	return (hi << 8) | lo;
}

static uint8_t
read8(r2A03 *cpu)
{
	return get8_addr(cpu, cpu->PC++);
}

static uint8_t
read8_indirect(r2A03 *cpu, uint16_t location)
{
	return get8_addr(cpu, location);
}

static uint8_t
pop8(r2A03 *cpu)
{
	cpu->SP++;
	uint16_t addr = STACK_OFFSET + cpu->SP;
	return get8_addr(cpu, addr);
}

static uint16_t
pop16(r2A03 *cpu)
{
	uint8_t lo = pop8(cpu);
	uint8_t hi = pop8(cpu);
	return (hi << 8) | lo;
}

static uint16_t
read16(r2A03 *cpu)
{
	uint8_t lo = read8(cpu);
	uint8_t hi = read8(cpu);
	return (hi << 8) | lo;
}

static uint16_t
read16_indirect(r2A03 *cpu, uint16_t addr)
{
	uint8_t lo = read8_indirect(cpu, addr);
	uint8_t hi = read8_indirect(cpu, addr + 1);
	return (hi << 8) | lo;
}

static uint8_t
getflag(r2A03 *cpu, uint8_t mask)
{
	return cpu->P & mask;
}

static uint8_t
get_c(r2A03 *cpu)
{
	return getflag(cpu, MASK_CARRY);
}

static uint8_t
get_n(r2A03 *cpu)
{
	return getflag(cpu, MASK_NEGATIVE);
}

static uint8_t
get_i(r2A03 *cpu)
{
	return getflag(cpu, MASK_INTERRUPT_DISABLE);
}

static uint8_t
get_b(r2A03 *cpu)
{
	return getflag(cpu, MASK_BREAK);
}

static uint8_t
get_v(r2A03 *cpu)
{
	return getflag(cpu, MASK_OVERFLOW);
}

static uint8_t
get_z(r2A03 *cpu)
{
	return getflag(cpu, MASK_ZERO);
}

static void
setflag(r2A03 *cpu, uint8_t mask)
{
	cpu->P |= mask;
}

static void
unsetflag(r2A03 *cpu, uint8_t mask)
{
	cpu->P &= ~mask;
}

static void
set_c(r2A03 *cpu)
{
	setflag(cpu, MASK_CARRY);
}

static void
set_z(r2A03 *cpu)
{
	setflag(cpu, MASK_ZERO);
}

static void
set_i(r2A03 *cpu)
{
	setflag(cpu, MASK_INTERRUPT_DISABLE);
}

static void
set_d(r2A03 *cpu)
{
	setflag(cpu, MASK_DECIMAL);
}

static void
set_b(r2A03 *cpu)
{
	setflag(cpu, MASK_BREAK);
}

static void
set_v(r2A03 *cpu)
{
	setflag(cpu, MASK_OVERFLOW);
}

static void
set_n(r2A03 *cpu)
{
	setflag(cpu, MASK_NEGATIVE);
}

static void
unset_c(r2A03 *cpu)
{
	unsetflag(cpu, MASK_CARRY);
}

static void
unset_z(r2A03 *cpu)
{
	unsetflag(cpu, MASK_ZERO);
}

static void
unset_i(r2A03 *cpu)
{
	unsetflag(cpu, MASK_INTERRUPT_DISABLE);
}

static void
unset_d(r2A03 *cpu)
{
	unsetflag(cpu, MASK_DECIMAL);
}

static void
unset_v(r2A03 *cpu)
{
	unsetflag(cpu, MASK_OVERFLOW);
}

static void
unset_n(r2A03 *cpu)
{
	unsetflag(cpu, MASK_NEGATIVE);
}

static void
upd_flag(r2A03 *cpu, uint8_t mask, uint8_t val)
{
	if (val) {
		setflag(cpu, mask);
	} else {
		unsetflag(cpu, mask);
	}
}

static void
upd_c(r2A03 *cpu, uint8_t val)
{
	upd_flag(cpu, MASK_CARRY, val);
}

static void
upd_z(r2A03 *cpu, uint8_t val)
{
	upd_flag(cpu, MASK_ZERO, val);
}

static void
upd_v(r2A03 *cpu, uint8_t val)
{
	upd_flag(cpu, MASK_OVERFLOW, val);
}

static void
upd_n(r2A03 *cpu, uint8_t val)
{
	upd_flag(cpu, MASK_NEGATIVE, val);
}

static void
upd_zn(r2A03 *cpu, uint8_t val)
{
	upd_z(cpu, val == 0);
	upd_n(cpu, val & MASK_NEGATIVE); /* check if bit 7 of val is set */
}

static int
overflowed_sum(uint8_t a, uint8_t b, uint8_t c)
{
	/*
	 * returns 1 if A and B have the same sign
	 * but A and C have different signs
	 */
	return (((a ^ b) & 0x80) == 0 && ((a ^ c) & 0x80) != 0);
}

static int
overflowed_sub(uint8_t a, uint8_t b, uint8_t c)
{
	/*
	 * like overflowed_sum but for subtraction
	 */
	return (((a ^ b) & 0x80) != 0 && ((a ^ c) & 0x80) != 0);
}

static void
setirq(r2A03 *cpu, uint8_t val)
{
	/* maskable */
	if (get_i(cpu)) {
		return;
	}
}

static void
setnmi(r2A03 *cpu, uint8_t val)
{
	/* non maskable */
}

static void
ADDR_ABS(r2A03 *cpu)
{
	cpu->addr = read16(cpu);
}

static void
ADDR_ACC(r2A03 *cpu)
{
	cpu->acc_mode = 1;
}

static void
ADDR_IAX(r2A03 *cpu)
{
	cpu->addr = read16(cpu) + cpu->X;
}

static void
ADDR_IAY(r2A03 *cpu)
{
	cpu->addr = read16(cpu) + cpu->Y;
}

static void
ADDR_IMM(r2A03 *cpu)
{
	cpu->addr = cpu->PC++;
}

static void
ADDR_IMP(r2A03 *cpu)
{
	return;
}

static void
ADDR_IND(r2A03 *cpu)
{
	/* NOTE: here we implement 6502's indirect addressing bug.
	 * See: https://www.reddit.com/r/EmuDev/comments/fi29ah/6502_jump_indirect_error
	 *
	 * TL;DR: if JMP instruction uses the last byte of the page (e.g JMP($30FF)),
	 * then the resulting two-byte address will be composed of one byte from $30FF
	 * and one byte from $3000, rather than $30FF and $3100.
	 */

	uint8_t location_lo = read8(cpu);
	uint8_t location_hi = read8(cpu);
	// TODO: DRY this part?
	uint8_t addr_lo = get8_addr(cpu, location_hi << 8 | location_lo);
	uint8_t addr_hi = get8_addr(cpu, location_hi << 8 | (uint8_t)(location_lo + 1));
	cpu->addr = addr_hi << 8 | addr_lo;
}

static void
ADDR_INX(r2A03 *cpu)
{
	uint8_t location = read8(cpu);
	// TODO: DRY this part?
	uint8_t addr_lo = get8_addr(cpu, (location + cpu->X) & 0x00FF);
	uint8_t addr_hi = get8_addr(cpu, (location + cpu->X + 1) & 0x00FF);
	cpu->addr = addr_hi << 8 | addr_lo;
}

static void
ADDR_INY(r2A03 *cpu)
{
	uint8_t location = read8(cpu);
	// TODO: DRY this part?
    uint8_t addr_lo = get8_addr(cpu, location);
    uint8_t addr_hi = get8_addr(cpu, (location + 1) & 0x00FF);
    cpu->addr = (addr_hi << 8 | addr_lo) + cpu->Y;
}

static void
ADDR_IZX(r2A03 *cpu)
{
	cpu->addr = (read8(cpu) + cpu->X) & 0x00FF;
}

static void
ADDR_IZY(r2A03 *cpu)
{
	cpu->addr = (read8(cpu) + cpu->Y) & 0x00FF;
}

static void
ADDR_REL(r2A03 *cpu)
{
	int8_t offset = (int8_t)read8(cpu);
	cpu->addr = cpu->PC + offset;
}

static void
ADDR_ZPG(r2A03 *cpu) {
	cpu->addr = read8(cpu) | 0x0000;
}

static void
ADDR_ILL(r2A03 *cpu)
{
	fprintf(stderr, "illegal address mode!\n");
}

static void
OP_ADC(r2A03 *cpu)
{
	uint8_t acc = cpu->A;
	uint8_t val = get8(cpu);
	uint8_t carry = get_c(cpu);

	cpu->A = acc + val + carry;

	upd_c(cpu, (int)acc + (int)val + (int)carry > 0xFF); /* TODO: create func detect_carry() */
	upd_v(cpu, overflowed_sum(acc, val, cpu->A));
	upd_zn(cpu, cpu->A);
}

static void
OP_AND(r2A03 *cpu)
{
	cpu->A &= get8(cpu);
	upd_zn(cpu, cpu->A);
}

static void
OP_ASL(r2A03 *cpu)
{
	if (is_acc_mode(cpu)) {
		upd_c(cpu, cpu->A & 0x80);
		cpu->A <<= 1;
		upd_zn(cpu, cpu->A);
		disable_acc_mode(cpu);
	} else {
		uint8_t val = get8(cpu);
		upd_c(cpu, val & 0x80);
		val <<= 1;
		write8(cpu, val);
		upd_zn(cpu, val);
	}
}

static void
OP_BCC(r2A03 *cpu)
{
	if (!get_c(cpu)) {
		cpu->PC = cpu->addr;
	}
}

static void
OP_BCS(r2A03 *cpu)
{
	if (get_c(cpu)) {
		cpu->PC = cpu->addr;
	}
}

static void
OP_BEQ(r2A03 *cpu)
{
	if (get_z(cpu)) {
		cpu->PC = cpu->addr;
	}
}

static void
OP_BIT(r2A03 *cpu)
{
	uint8_t val = get8(cpu);
	upd_z(cpu, (cpu->A & val) == 0);
	upd_n(cpu, val & MASK_NEGATIVE);
	upd_v(cpu, val & MASK_OVERFLOW);
}

static void
OP_BMI(r2A03 *cpu)
{
	if (get_n(cpu)) {
		cpu->PC = cpu->addr;
	}
}

static void
OP_BNE(r2A03 *cpu)
{
	if (!get_z(cpu)) {
		cpu->PC = cpu->addr;
	}
}

static void
OP_BPL(r2A03 *cpu)
{
	if (!get_n(cpu)) {
		cpu->PC = cpu->addr;
	}
}

static void
OP_BRK(r2A03 *cpu)
{
	push16(cpu, cpu->PC);
	set_b(cpu);
	push8(cpu, cpu->P);
	cpu->PC = get16_addr(cpu, VECTOR_IRQ);
}

static void
OP_BVC(r2A03 *cpu)
{
	if (!get_v(cpu)) {
		cpu->PC = cpu->addr;
	}
}

static void
OP_BVS(r2A03 *cpu)
{
	if (get_v(cpu)) {
		cpu->PC = cpu->addr;
	}
}

static void
OP_CLC(r2A03 *cpu)
{
	unset_c(cpu);
}

static void
OP_CLD(r2A03 *cpu)
{
	unset_d(cpu);
}

static void
OP_CLI(r2A03 *cpu)
{
	unset_i(cpu);
}

static void
OP_CLV(r2A03 *cpu)
{
	unset_v(cpu);
}

static void
OP_CMP(r2A03 *cpu)
{
	uint8_t val = get8(cpu);
	upd_c(cpu, cpu->A >= val);
	upd_z(cpu, cpu->A == val);
	upd_n(cpu, (cpu->A - val) & MASK_NEGATIVE);
}

static void
OP_CPX(r2A03 *cpu)
{
	uint8_t val = get8(cpu);
	upd_c(cpu, cpu->X >= val);
	upd_z(cpu, cpu->X == val);
	upd_n(cpu, (cpu->X - val) & MASK_NEGATIVE);
}

static void
OP_CPY(r2A03 *cpu)
{
	uint8_t val = get8(cpu);
	upd_c(cpu, cpu->Y >= val);
	upd_z(cpu, cpu->Y == val);
	upd_n(cpu, (cpu->Y - val) & MASK_NEGATIVE);
}

static void
OP_DEC(r2A03 *cpu)
{
	uint8_t val = get8(cpu);
	write8(cpu, val - 1);
	upd_z(cpu, (val - 1) == 0);
	upd_n(cpu, (val - 1) & MASK_NEGATIVE);
}

static void
OP_DEX(r2A03 *cpu)
{
	cpu->X--;
	upd_zn(cpu, cpu->X);
}

static void
OP_DEY(r2A03 *cpu)
{
	cpu->Y--;
	upd_zn(cpu, cpu->Y);
}

static void
OP_EOR(r2A03 *cpu)
{
	cpu->A ^= get8(cpu);
	upd_zn(cpu, cpu->A);
}

static void
OP_INC(r2A03 *cpu)
{
	uint8_t val = get8(cpu);
	val++;
	write8(cpu, val);
	upd_zn(cpu, val);
}

static void
OP_INX(r2A03 *cpu)
{
	cpu->X++;
	upd_zn(cpu, cpu->X);
}

static void
OP_INY(r2A03 *cpu)
{
	cpu->Y++;
	upd_zn(cpu, cpu->Y);
}

static void
OP_JMP(r2A03 *cpu)
{
	cpu->PC = cpu->addr;
}

static void
OP_JSR(r2A03 *cpu)
{
	push16(cpu, cpu->PC - 1);
	cpu->PC = cpu->addr;
}

static void
OP_LDA(r2A03 *cpu)
{
	cpu->A = get8(cpu);
	upd_zn(cpu, cpu->A);
}

static void
OP_LDX(r2A03 *cpu)
{
	cpu->X = get8(cpu);
	upd_zn(cpu, cpu->X);
}

static void
OP_LDY(r2A03 *cpu)
{
	cpu->Y = get8(cpu);
	upd_zn(cpu, cpu->Y);
}

static void
OP_LSR(r2A03 *cpu)
{
	if (is_acc_mode(cpu)) {
		upd_c(cpu, cpu->A & MASK_CARRY);
		cpu->A >>= 1;
		upd_zn(cpu, cpu->A);
		disable_acc_mode(cpu);
	} else {
		uint8_t val = get8(cpu);
		upd_c(cpu, val & MASK_CARRY);
		val >>= 1;
		write8(cpu, val);
		upd_zn(cpu, val);
	}
}

static void
OP_NOP(r2A03 *cpu)
{
	/*
	 * The NOP instruction causes no changes to the processor
	 * other than the normal incrementing of the program counter
	 * to the next instruction.
	 */
}

static void
OP_ORA(r2A03 *cpu)
{
	cpu->A |= get8(cpu);
	upd_zn(cpu, cpu->A);
}

static void
OP_PHA(r2A03 *cpu)
{
	push8(cpu, cpu->A);
}

static void
OP_PHP(r2A03 *cpu)
{
	push8(cpu, cpu->P);
}

static void
OP_PLA(r2A03 *cpu)
{
	cpu->A = pop8(cpu);
	upd_zn(cpu, cpu->A);
}

static void
OP_PLP(r2A03 *cpu)
{
	cpu->P = pop8(cpu);
	unsetflag(cpu, MASK_BREAK); // TODO: ?
}

static void
OP_ROL(r2A03 *cpu)
{
	uint8_t carry = get_c(cpu);
	if (is_acc_mode(cpu)) {
		upd_c(cpu, cpu->A & 0x80);
		cpu->A = (cpu->A << 1) | carry;
		upd_zn(cpu, cpu->A);
		disable_acc_mode(cpu);
	} else {
		uint8_t val = get8(cpu);
		upd_c(cpu, val & 0x80);
		val = (val << 1) | carry;
		write8(cpu, val);
		upd_zn(cpu, val);
	}
}

static void
OP_ROR(r2A03 *cpu)
{
	uint8_t carry = get_c(cpu);
	if (is_acc_mode(cpu)) {
		upd_c(cpu, cpu->A & 0x01);
		cpu->A = (cpu->A >> 1) | (carry << 7);
		upd_zn(cpu, cpu->A);
		disable_acc_mode(cpu);
	} else {
		uint8_t val = get8(cpu);
		upd_c(cpu, val & 0x01);
		val = (val >> 1) | (carry << 7);
		write8(cpu, val);
		upd_zn(cpu, val);
	}
}

static void
OP_RTI(r2A03 *cpu)
{
	cpu->P = pop8(cpu);
	cpu->PC = pop16(cpu);
}

static void
OP_RTS(r2A03 *cpu)
{
	cpu->PC = pop16(cpu) + 1;
}

static void
OP_SBC(r2A03 *cpu)
{
	uint8_t acc = cpu->A;
	uint8_t val = get8(cpu);
	uint8_t carry = get_c(cpu);

	cpu->A = acc - val - (1 - carry);

	upd_c(cpu, (int)acc - (int)val - (int)(1 - carry) >= 0x00);
	upd_v(cpu, overflowed_sub(acc, val, cpu->A));
	upd_zn(cpu, cpu->A);
}

static void
OP_SEC(r2A03 *cpu)
{
	set_c(cpu);
}

static void
OP_SED(r2A03 *cpu)
{
	set_d(cpu);
}

static void
OP_SEI(r2A03 *cpu)
{
	set_i(cpu);
}

static void
OP_STA(r2A03 *cpu)
{
	write8(cpu, cpu->A);
}

static void
OP_STX(r2A03 *cpu)
{
	write8(cpu, cpu->X);
}

static void
OP_STY(r2A03 *cpu)
{
	write8(cpu, cpu->Y);
}

static void
OP_TAX(r2A03 *cpu)
{
	cpu->X = cpu->A;
	upd_zn(cpu, cpu->X);
}

static void
OP_TAY(r2A03 *cpu)
{
	cpu->Y = cpu->A;
	upd_zn(cpu, cpu->Y);
}

static void
OP_TSX(r2A03 *cpu)
{
	cpu->X = cpu->SP;
	upd_zn(cpu, cpu->X);
}

static void
OP_TXA(r2A03 *cpu)
{
	cpu->A = cpu->X;
	upd_zn(cpu, cpu->A);
}

static void
OP_TXS(r2A03 *cpu)
{
	cpu->SP = cpu->X;
}

static void
OP_TYA(r2A03 *cpu)
{
	cpu->A = cpu->Y;
	upd_zn(cpu, cpu->A);
}

static void
OP_ALR(r2A03 *cpu)
{
}

static void
OP_ANE(r2A03 *cpu)
{
}

static void
OP_ARR(r2A03 *cpu)
{
}

static void
OP_ANC(r2A03 *cpu)
{
}

static void
OP_DCP(r2A03 *cpu)
{
}

static void
OP_ISC(r2A03 *cpu)
{
}

static void
OP_LAS(r2A03 *cpu)
{
}

static void
OP_LAX(r2A03 *cpu)
{
}

static void
OP_LXA(r2A03 *cpu)
{
}

static void
OP_RLA(r2A03 *cpu)
{
}

static void
OP_RRA(r2A03 *cpu)
{
}

static void
OP_SAX(r2A03 *cpu)
{
}

static void
OP_SBX(r2A03 *cpu)
{
}

static void
OP_SHA(r2A03 *cpu)
{
}

static void
OP_SHX(r2A03 *cpu)
{
}

static void
OP_SHY(r2A03 *cpu)
{
}

static void
OP_SLO(r2A03 *cpu)
{
}

static void
OP_SRE(r2A03 *cpu)
{
}

static void
OP_TAS(r2A03 *cpu)
{
}

static void
OP_USBC(r2A03 *cpu)
{
}

static void
OP_ILL(r2A03 *cpu)
{
	fprintf(stderr, "illegal opcode!\n"); /* TODO: remove */
	return;
}

void
cpu_tick(r2A03 *cpu)
{
	/* poll interrupts here */
	/* if irq -> cpu_setirq */
	/* if nmi -> cpu_setnmi */

	disassemble(cpu);
	cpu->opcode = read8(cpu);
	optable[cpu->opcode].mode(cpu);
	optable[cpu->opcode].func(cpu);
}

void
cpu_reset(r2A03 *cpu, bus *bus)
{
	cpu->bus = bus;
	/* cpu->PC = get16_addr(cpu, VECTOR_RESET); */
	cpu->PC = 0xC000; /* only for nestest! TODO: remove! */
	cpu->SP = 0xFD;   /* 0x00 - 0x3. See https://www.youtube.com/watch?v=fWqBmmPQP40&t=2536s */
	cpu->P = 0x24;
	cpu->A = 0;
	cpu->X = 0;
	cpu->Y = 0;
	cpu->stall = 0; /* TODO: do we need it here? */
}

typedef union {
	uint16_t whole;
	struct {
		uint8_t lo;
		uint8_t hi;
	} part;
} disassemble_arg;

static disassemble_arg
disassemble_get_arg(r2A03 *cpu, addr_mode amode)
{
	disassemble_arg arg = {0};
	uint16_t curr_pc = cpu->PC + 1;

	if (amode == ADDR_ABS) {
		arg.whole = get16_addr(cpu, curr_pc);
	} else if (amode == ADDR_ACC) {
		arg.whole = 0xA;
	} else if (amode == ADDR_IAX) {
		arg.whole = get16_addr(cpu, curr_pc) + cpu->X;
	} else if (amode == ADDR_IAY) {
		arg.whole = get16_addr(cpu, curr_pc) + cpu->Y;
	} else if (amode == ADDR_IMM) {
		arg.part.lo = get8_addr(cpu, curr_pc);
	} else if (amode == ADDR_IMP) {
		arg.whole = 0;
	} else if (amode == ADDR_IND) {
		uint16_t location = get16_addr(cpu, curr_pc);
		arg.whole = get16_addr(cpu, location);
	} else if (amode == ADDR_INX) {
		uint8_t location = get8_addr(cpu, curr_pc);
		arg.whole = get16_addr(cpu, location + cpu->X);
	} else if (amode == ADDR_INY) {
		uint8_t location = get8_addr(cpu, curr_pc);
		arg.whole = get8_addr(cpu, location + (cpu->Y & 0x00FF));
	} else if (amode == ADDR_IZX) {
		arg.whole = get8_addr(cpu, curr_pc) + (cpu->X & 0x00FF);
	} else if (amode == ADDR_IZY) {
		arg.whole = get8_addr(cpu, curr_pc) + (cpu->Y & 0x00FF);
	} else if (amode == ADDR_REL) {
		int8_t offset = (int8_t)get8_addr(cpu, curr_pc++);
		arg.whole = curr_pc + offset;
	} else if (amode == ADDR_ZPG) {
		arg.whole = get8_addr(cpu, curr_pc) | 0x0000;
	} else {
		arg.whole = 0;
	}

	return arg;
}

#define ASM_STR_SIZE 25

static void
disassemble_compose_asm_str(r2A03 *cpu, uint8_t opcode, char strbuf[])
{
	char *curr = strbuf;
	char *end = strbuf + ASM_STR_SIZE;
	disassemble_arg arg = disassemble_get_arg(cpu, optable[opcode].mode);
	
	curr += snprintf(curr, end - curr, "%02X ", optable[opcode].idx);
	if (arg.part.hi != 0) {
		curr += snprintf(curr, end - curr, "%02X %02X ",  arg.part.lo, arg.part.hi);
	} else {
		curr += snprintf(curr, end - curr, "%02X    ",  arg.part.lo);
	}
	curr += snprintf(curr, end - curr, "%s ", optable[opcode].name);
	curr += snprintf(curr, end - curr, "%*X  ", 5, arg.whole);
}

/* Using nestest log format for easy comparision with nestest.log.
 * See:
 * https://github.com/christopherpow/nes-test-roms/blob/master/other/nestest.log
 */
static void
disassemble(r2A03 *cpu)
{
	uint8_t opcode = get8_addr(cpu, cpu->PC);
	char asm_str[ASM_STR_SIZE];

	fprintf(stderr, "%X  ", cpu->PC);
	disassemble_compose_asm_str(cpu, opcode, asm_str);
	fprintf(stderr, "%s ", asm_str);

	fprintf(stderr, "A:%02X ", cpu->A);
	fprintf(stderr, "X:%02X ", cpu->X);
	fprintf(stderr, "Y:%02X ", cpu->Y);
	fprintf(stderr, "P:%02X ", cpu->P);
	fprintf(stderr, "SP:%02X ", cpu->SP);
	fprintf(stderr, "PPU:%*d,%*d ", 3, 3, 0, 0); /* TODO: fix it */
	fprintf(stderr, "CYC:%lu\n", cpu->total);
}
