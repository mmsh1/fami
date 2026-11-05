#include <stdint.h>
#include <stdio.h>

#include "bus.h"
#include "cpu.h"

enum {
	MASK_CARRY = 0x01,              /* 0000 0001 -> (1 << 0) */
	MASK_ZERO = 0x02,               /* 0000 0010 -> (1 << 1) */
	MASK_INTERRUPT_DISABLE = 0x04,  /* 0000 0100 -> (1 << 2) */
	MASK_DECIMAL = 0x08,            /* 0000 1000 -> (1 << 3) */
	MASK_BREAK = 0x10,              /* 0001 0000 -> (1 << 4) */
	MASK_UNUSED = 0x20,             /* 0010 0000 -> (1 << 5) */
	MASK_OVERFLOW = 0x40,           /* 0100 0000 -> (1 << 6) */
	MASK_NEGATIVE = 0x80            /* 1000 0000 -> (1 << 7) */
};

enum {
	STACK_OFFSET = 0x0100,
};

enum {
	VECTOR_RESET = 0xFFFC,
	VECTOR_NMI = 0xFFFA,
	VECTOR_IRQ = 0xFFFE
};

typedef void (*opcode_func)(cpu *);
typedef void (*addr_mode)(cpu *);

typedef struct {
	uint8_t idx;
	const char *name;
	opcode_func func;
	uint8_t size;
	uint8_t cycles;
	addr_mode mode;
} instruction;

typedef union {
	struct {
		uint8_t lo;
		uint8_t hi;
	} part;
	uint16_t whole;
} disassemble_arg;

static inline uint8_t getflag(cpu *, uint8_t);
static inline uint8_t get_c(cpu *);
static inline uint8_t get_z(cpu *);
static inline uint8_t get_v(cpu *);
static inline uint8_t get_n(cpu *);

static inline void setflag(cpu *, uint8_t);
static inline void set_c(cpu *);
static inline void set_i(cpu *);
static inline void set_d(cpu *);
static inline void set_b(cpu *);

static inline void unsetflag(cpu *, uint8_t);
static inline void unset_b(cpu *);
static inline void unset_c(cpu *);
static inline void unset_d(cpu *);
static inline void unset_v(cpu *);

static inline void upd_flag(cpu *, uint8_t, uint8_t);
static inline void upd_c(cpu *, uint8_t);
static inline void upd_z(cpu *, uint8_t);
static inline void upd_v(cpu *, uint8_t);
static inline void upd_n(cpu *, uint8_t);
static inline void upd_zn(cpu *, uint8_t);

static inline addr_mode get_addr_mode(cpu *);
static inline int is_acc_mode(cpu *);
static inline int is_indexed_absolute(cpu *);
static inline int has_memory_operand(cpu *);

static int is_page_crossed(cpu *);

static disassemble_arg disassemble_get_arg(cpu *, addr_mode);
static void disassemble_compose_asm_str(cpu *, uint8_t, char[]);
static void disassemble(cpu *);

static void dummy_get8(cpu *, uint16_t);
static void write8_addr(cpu *, uint16_t, uint8_t);
static void write8(cpu *, uint8_t);
static void push8(cpu *, uint8_t);
static void push16(cpu *, uint16_t);
static uint8_t get8_addr(cpu *, uint16_t);
static uint8_t get8(cpu *);
static uint8_t read8(cpu *);
static uint8_t pop8(cpu *);
static uint16_t get16_addr(cpu *, uint16_t);
static uint16_t pop16(cpu *);
static uint16_t read16(cpu *);

static inline uint8_t overflowed_sum(uint8_t, uint8_t, uint8_t);
static inline uint8_t overflowed_sub(uint8_t, uint8_t, uint8_t);

static void poll_interrupts(cpu *);
static void handle_irq(cpu *);
static void handle_nmi(cpu *);

static void ADDR_ABS(cpu *); /* absolute */
static void ADDR_ACC(cpu *); /* accumulator */
static void ADDR_IAX(cpu *); /* indexed absolute x */
static void ADDR_IAY(cpu *); /* indexed absolute y */
static void ADDR_IMM(cpu *); /* immediate */
static void ADDR_IMP(cpu *); /* implied */
static void ADDR_IND(cpu *); /* indirect */
static void ADDR_INX(cpu *); /* indexed indirect x */
static void ADDR_INY(cpu *); /* indirect indexed y */
static void ADDR_IZX(cpu *); /* indexed zero page x */
static void ADDR_IZY(cpu *); /* indexed zero page y */
static void ADDR_REL(cpu *); /* relative */
static void ADDR_ZPG(cpu *); /* zero page */

static void ADDR_ILL(cpu *); /* illegal */

static void OP_ADC(cpu *);
static void OP_AND(cpu *);
static void OP_ASL(cpu *);
static void OP_BCC(cpu *);
static void OP_BCS(cpu *);
static void OP_BEQ(cpu *);
static void OP_BIT(cpu *);
static void OP_BMI(cpu *);
static void OP_BNE(cpu *);
static void OP_BPL(cpu *);
static void OP_BRK(cpu *);
static void OP_BVC(cpu *);
static void OP_BVS(cpu *);
static void OP_CLC(cpu *);
static void OP_CLD(cpu *);
static void OP_CLI(cpu *);
static void OP_CLV(cpu *);
static void OP_CMP(cpu *);
static void OP_CPX(cpu *);
static void OP_CPY(cpu *);
static void OP_DEC(cpu *);
static void OP_DEX(cpu *);
static void OP_DEY(cpu *);
static void OP_EOR(cpu *);
static void OP_INC(cpu *);
static void OP_INX(cpu *);
static void OP_INY(cpu *);
static void OP_JMP(cpu *);
static void OP_JSR(cpu *);
static void OP_LDA(cpu *);
static void OP_LDX(cpu *);
static void OP_LDY(cpu *);
static void OP_LSR(cpu *);
static void OP_NOP(cpu *);
static void OP_ORA(cpu *);
static void OP_PHA(cpu *);
static void OP_PHP(cpu *);
static void OP_PLA(cpu *);
static void OP_PLP(cpu *);
static void OP_ROL(cpu *);
static void OP_ROR(cpu *);
static void OP_RTI(cpu *);
static void OP_RTS(cpu *);
static void OP_SBC(cpu *);
static void OP_SEC(cpu *);
static void OP_SED(cpu *);
static void OP_SEI(cpu *);
static void OP_STA(cpu *);
static void OP_STX(cpu *);
static void OP_STY(cpu *);
static void OP_TAX(cpu *);
static void OP_TAY(cpu *);
static void OP_TSX(cpu *);
static void OP_TXA(cpu *);
static void OP_TXS(cpu *);
static void OP_TYA(cpu *);

static void OP_ALR(cpu *);
static void OP_ANE(cpu *);
static void OP_ARR(cpu *);
static void OP_ANC(cpu *);
static void OP_DCP(cpu *);
static void OP_ISC(cpu *);
static void OP_LAS(cpu *);
static void OP_LAX(cpu *);
static void OP_LXA(cpu *);
static void OP_RLA(cpu *);
static void OP_RRA(cpu *);
static void OP_SAX(cpu *);
static void OP_SBX(cpu *);
static void OP_SHA(cpu *);
static void OP_SHX(cpu *);
static void OP_SHY(cpu *);
static void OP_SLO(cpu *);
static void OP_SRE(cpu *);
static void OP_TAS(cpu *);

static void OP_ILL(cpu *); /* illegal */

static instruction
optable[0xFF + 1] = {
	{ .idx = 0x00, .name = "BRK", .func = OP_BRK, .cycles = 7, .mode = ADDR_IMP },
	{ .idx = 0x01, .name = "ORA", .func = OP_ORA, .cycles = 6, .mode = ADDR_INX },
	{ .idx = 0x02, .name = "ILL", .func = OP_ILL, .cycles = 0, .mode = ADDR_ILL },
	{ .idx = 0x03, .name = "SLO", .func = OP_SLO, .cycles = 8, .mode = ADDR_INX },
	{ .idx = 0x04, .name = "NOP", .func = OP_NOP, .cycles = 3, .mode = ADDR_ZPG },
	{ .idx = 0x05, .name = "ORA", .func = OP_ORA, .cycles = 3, .mode = ADDR_ZPG },
	{ .idx = 0x06, .name = "ASL", .func = OP_ASL, .cycles = 5, .mode = ADDR_ZPG },
	{ .idx = 0x07, .name = "SLO", .func = OP_SLO, .cycles = 5, .mode = ADDR_ZPG },
	{ .idx = 0x08, .name = "PHP", .func = OP_PHP, .cycles = 3, .mode = ADDR_IMP },
	{ .idx = 0x09, .name = "ORA", .func = OP_ORA, .cycles = 2, .mode = ADDR_IMM },
	{ .idx = 0x0A, .name = "ASL", .func = OP_ASL, .cycles = 2, .mode = ADDR_ACC },
	{ .idx = 0x0B, .name = "ANC", .func = OP_ANC, .cycles = 2, .mode = ADDR_IMM },
	{ .idx = 0x0C, .name = "NOP", .func = OP_NOP, .cycles = 4, .mode = ADDR_ABS },
	{ .idx = 0x0D, .name = "ORA", .func = OP_ORA, .cycles = 4, .mode = ADDR_ABS },
	{ .idx = 0x0E, .name = "ASL", .func = OP_ASL, .cycles = 6, .mode = ADDR_ABS },
	{ .idx = 0x0F, .name = "SLO", .func = OP_SLO, .cycles = 6, .mode = ADDR_ABS },

	{ .idx = 0x10, .name = "BPL", .func = OP_BPL, .cycles = 2, .mode = ADDR_REL },
	{ .idx = 0x11, .name = "ORA", .func = OP_ORA, .cycles = 5, .mode = ADDR_INY },
	{ .idx = 0x12, .name = "ILL", .func = OP_ILL, .cycles = 0, .mode = ADDR_ILL },
	{ .idx = 0x13, .name = "SLO", .func = OP_SLO, .cycles = 8, .mode = ADDR_INY },
	{ .idx = 0x14, .name = "NOP", .func = OP_NOP, .cycles = 4, .mode = ADDR_IZX },
	{ .idx = 0x15, .name = "ORA", .func = OP_ORA, .cycles = 4, .mode = ADDR_IZX },
	{ .idx = 0x16, .name = "ASL", .func = OP_ASL, .cycles = 6, .mode = ADDR_IZX },
	{ .idx = 0x17, .name = "SLO", .func = OP_SLO, .cycles = 6, .mode = ADDR_IZX },
	{ .idx = 0x18, .name = "CLC", .func = OP_CLC, .cycles = 2, .mode = ADDR_IMP },
	{ .idx = 0x19, .name = "ORA", .func = OP_ORA, .cycles = 4, .mode = ADDR_IAY },
	{ .idx = 0x1A, .name = "NOP", .func = OP_NOP, .cycles = 2, .mode = ADDR_IMP },
	{ .idx = 0x1B, .name = "SLO", .func = OP_SLO, .cycles = 7, .mode = ADDR_IAY },
	{ .idx = 0x1C, .name = "NOP", .func = OP_NOP, .cycles = 4, .mode = ADDR_IAX },
	{ .idx = 0x1D, .name = "ORA", .func = OP_ORA, .cycles = 4, .mode = ADDR_IAX },
	{ .idx = 0x1E, .name = "ASL", .func = OP_ASL, .cycles = 7, .mode = ADDR_IAX },
	{ .idx = 0x1F, .name = "SLO", .func = OP_SLO, .cycles = 7, .mode = ADDR_IAX },

	{ .idx = 0x20, .name = "JSR", .func = OP_JSR, .cycles = 6, .mode = ADDR_ABS },
	{ .idx = 0x21, .name = "AND", .func = OP_AND, .cycles = 6, .mode = ADDR_INX },
	{ .idx = 0x22, .name = "ILL", .func = OP_ILL, .cycles = 0, .mode = ADDR_ILL },
	{ .idx = 0x23, .name = "RLA", .func = OP_RLA, .cycles = 8, .mode = ADDR_INX },
	{ .idx = 0x24, .name = "BIT", .func = OP_BIT, .cycles = 3, .mode = ADDR_ZPG },
	{ .idx = 0x25, .name = "AND", .func = OP_AND, .cycles = 3, .mode = ADDR_ZPG },
	{ .idx = 0x26, .name = "ROL", .func = OP_ROL, .cycles = 5, .mode = ADDR_ZPG },
	{ .idx = 0x27, .name = "RLA", .func = OP_RLA, .cycles = 5, .mode = ADDR_ZPG },
	{ .idx = 0x28, .name = "PLP", .func = OP_PLP, .cycles = 4, .mode = ADDR_IMP },
	{ .idx = 0x29, .name = "AND", .func = OP_AND, .cycles = 2, .mode = ADDR_IMM },
	{ .idx = 0x2A, .name = "ROL", .func = OP_ROL, .cycles = 2, .mode = ADDR_ACC },
	{ .idx = 0x2B, .name = "ANC", .func = OP_ANC, .cycles = 2, .mode = ADDR_IMM },
	{ .idx = 0x2C, .name = "BIT", .func = OP_BIT, .cycles = 4, .mode = ADDR_ABS },
	{ .idx = 0x2D, .name = "AND", .func = OP_AND, .cycles = 4, .mode = ADDR_ABS },
	{ .idx = 0x2E, .name = "ROL", .func = OP_ROL, .cycles = 6, .mode = ADDR_ABS },
	{ .idx = 0x2F, .name = "RLA", .func = OP_RLA, .cycles = 6, .mode = ADDR_ABS },

	{ .idx = 0x30, .name = "BMI", .func = OP_BMI, .cycles = 2, .mode = ADDR_REL },
	{ .idx = 0x31, .name = "AND", .func = OP_AND, .cycles = 5, .mode = ADDR_INY },
	{ .idx = 0x32, .name = "ILL", .func = OP_ILL, .cycles = 0, .mode = ADDR_ILL },
	{ .idx = 0x33, .name = "RLA", .func = OP_RLA, .cycles = 8, .mode = ADDR_INY },
	{ .idx = 0x34, .name = "NOP", .func = OP_NOP, .cycles = 4, .mode = ADDR_IZX },
	{ .idx = 0x35, .name = "AND", .func = OP_AND, .cycles = 4, .mode = ADDR_IZX },
	{ .idx = 0x36, .name = "ROL", .func = OP_ROL, .cycles = 6, .mode = ADDR_IZX },
	{ .idx = 0x37, .name = "RLA", .func = OP_RLA, .cycles = 6, .mode = ADDR_IZX },
	{ .idx = 0x38, .name = "SEC", .func = OP_SEC, .cycles = 2, .mode = ADDR_IMP },
	{ .idx = 0x39, .name = "AND", .func = OP_AND, .cycles = 4, .mode = ADDR_IAY },
	{ .idx = 0x3A, .name = "NOP", .func = OP_NOP, .cycles = 2, .mode = ADDR_IMP },
	{ .idx = 0x3B, .name = "RLA", .func = OP_RLA, .cycles = 7, .mode = ADDR_IAY },
	{ .idx = 0x3C, .name = "NOP", .func = OP_NOP, .cycles = 4, .mode = ADDR_IAX },
	{ .idx = 0x3D, .name = "AND", .func = OP_AND, .cycles = 4, .mode = ADDR_IAX },
	{ .idx = 0x3E, .name = "ROL", .func = OP_ROL, .cycles = 7, .mode = ADDR_IAX },
	{ .idx = 0x3F, .name = "RLA", .func = OP_RLA, .cycles = 7, .mode = ADDR_IAX },

	{ .idx = 0x40, .name = "RTI", .func = OP_RTI, .cycles = 6, .mode = ADDR_IMP },
	{ .idx = 0x41, .name = "EOR", .func = OP_EOR, .cycles = 6, .mode = ADDR_INX },
	{ .idx = 0x42, .name = "ILL", .func = OP_ILL, .cycles = 0, .mode = ADDR_ILL },
	{ .idx = 0x43, .name = "SRE", .func = OP_SRE, .cycles = 8, .mode = ADDR_INX },
	{ .idx = 0x44, .name = "NOP", .func = OP_NOP, .cycles = 3, .mode = ADDR_ZPG },
	{ .idx = 0x45, .name = "EOR", .func = OP_EOR, .cycles = 3, .mode = ADDR_ZPG },
	{ .idx = 0x46, .name = "LSR", .func = OP_LSR, .cycles = 5, .mode = ADDR_ZPG },
	{ .idx = 0x47, .name = "SRE", .func = OP_SRE, .cycles = 5, .mode = ADDR_ZPG },
	{ .idx = 0x48, .name = "PHA", .func = OP_PHA, .cycles = 3, .mode = ADDR_IMP },
	{ .idx = 0x49, .name = "EOR", .func = OP_EOR, .cycles = 2, .mode = ADDR_IMM },
	{ .idx = 0x4A, .name = "LSR", .func = OP_LSR, .cycles = 2, .mode = ADDR_ACC },
	{ .idx = 0x4B, .name = "ALR", .func = OP_ALR, .cycles = 2, .mode = ADDR_IMM },
	{ .idx = 0x4C, .name = "JMP", .func = OP_JMP, .cycles = 3, .mode = ADDR_ABS },
	{ .idx = 0x4D, .name = "EOR", .func = OP_EOR, .cycles = 4, .mode = ADDR_ABS },
	{ .idx = 0x4E, .name = "LSR", .func = OP_LSR, .cycles = 6, .mode = ADDR_ABS },
	{ .idx = 0x4F, .name = "SRE", .func = OP_SRE, .cycles = 6, .mode = ADDR_ABS },

	{ .idx = 0x50, .name = "BVC", .func = OP_BVC, .cycles = 2, .mode = ADDR_REL },
	{ .idx = 0x51, .name = "EOR", .func = OP_EOR, .cycles = 5, .mode = ADDR_INY },
	{ .idx = 0x52, .name = "ILL", .func = OP_ILL, .cycles = 0, .mode = ADDR_ILL },
	{ .idx = 0x53, .name = "SRE", .func = OP_SRE, .cycles = 8, .mode = ADDR_INY },
	{ .idx = 0x54, .name = "NOP", .func = OP_NOP, .cycles = 4, .mode = ADDR_IZX },
	{ .idx = 0x55, .name = "EOR", .func = OP_EOR, .cycles = 4, .mode = ADDR_IZX },
	{ .idx = 0x56, .name = "LSR", .func = OP_LSR, .cycles = 6, .mode = ADDR_IZX },
	{ .idx = 0x57, .name = "SRE", .func = OP_SRE, .cycles = 6, .mode = ADDR_IZX },
	{ .idx = 0x58, .name = "CLI", .func = OP_CLI, .cycles = 2, .mode = ADDR_IMP },
	{ .idx = 0x59, .name = "EOR", .func = OP_EOR, .cycles = 4, .mode = ADDR_IAY },
	{ .idx = 0x5A, .name = "NOP", .func = OP_NOP, .cycles = 2, .mode = ADDR_IMP },
	{ .idx = 0x5B, .name = "SRE", .func = OP_SRE, .cycles = 7, .mode = ADDR_IAY },
	{ .idx = 0x5C, .name = "NOP", .func = OP_NOP, .cycles = 4, .mode = ADDR_IAX },
	{ .idx = 0x5D, .name = "EOR", .func = OP_EOR, .cycles = 4, .mode = ADDR_IAX },
	{ .idx = 0x5E, .name = "LSR", .func = OP_LSR, .cycles = 7, .mode = ADDR_IAX },
	{ .idx = 0x5F, .name = "SRE", .func = OP_SRE, .cycles = 7, .mode = ADDR_IAX },

	{ .idx = 0x60, .name = "RTS", .func = OP_RTS, .cycles = 6, .mode = ADDR_IMP },
	{ .idx = 0x61, .name = "ADC", .func = OP_ADC, .cycles = 6, .mode = ADDR_INX },
	{ .idx = 0x62, .name = "ILL", .func = OP_ILL, .cycles = 0, .mode = ADDR_ILL },
	{ .idx = 0x63, .name = "RRA", .func = OP_RRA, .cycles = 8, .mode = ADDR_INX },
	{ .idx = 0x64, .name = "NOP", .func = OP_NOP, .cycles = 3, .mode = ADDR_ZPG },
	{ .idx = 0x65, .name = "ADC", .func = OP_ADC, .cycles = 3, .mode = ADDR_ZPG },
	{ .idx = 0x66, .name = "ROR", .func = OP_ROR, .cycles = 5, .mode = ADDR_ZPG },
	{ .idx = 0x67, .name = "RRA", .func = OP_RRA, .cycles = 5, .mode = ADDR_ZPG },
	{ .idx = 0x68, .name = "PLA", .func = OP_PLA, .cycles = 4, .mode = ADDR_IMP },
	{ .idx = 0x69, .name = "ADC", .func = OP_ADC, .cycles = 2, .mode = ADDR_IMM },
	{ .idx = 0x6A, .name = "ROR", .func = OP_ROR, .cycles = 2, .mode = ADDR_ACC },
	{ .idx = 0x6B, .name = "ARR", .func = OP_ARR, .cycles = 2, .mode = ADDR_IMM },
	{ .idx = 0x6C, .name = "JMP", .func = OP_JMP, .cycles = 5, .mode = ADDR_IND },
	{ .idx = 0x6D, .name = "ADC", .func = OP_ADC, .cycles = 4, .mode = ADDR_ABS },
	{ .idx = 0x6E, .name = "ROR", .func = OP_ROR, .cycles = 6, .mode = ADDR_ABS },
	{ .idx = 0x6F, .name = "RRA", .func = OP_RRA, .cycles = 6, .mode = ADDR_ABS },

	{ .idx = 0x70, .name = "BVS", .func = OP_BVS, .cycles = 2, .mode = ADDR_REL },
	{ .idx = 0x71, .name = "ADC", .func = OP_ADC, .cycles = 5, .mode = ADDR_INY },
	{ .idx = 0x72, .name = "ILL", .func = OP_ILL, .cycles = 0, .mode = ADDR_ILL },
	{ .idx = 0x73, .name = "RRA", .func = OP_RRA, .cycles = 8, .mode = ADDR_INY },
	{ .idx = 0x74, .name = "NOP", .func = OP_NOP, .cycles = 4, .mode = ADDR_IZX },
	{ .idx = 0x75, .name = "ADC", .func = OP_ADC, .cycles = 4, .mode = ADDR_IZX },
	{ .idx = 0x76, .name = "ROR", .func = OP_ROR, .cycles = 6, .mode = ADDR_IZX },
	{ .idx = 0x77, .name = "RRA", .func = OP_RRA, .cycles = 6, .mode = ADDR_IZX },
	{ .idx = 0x78, .name = "SEI", .func = OP_SEI, .cycles = 2, .mode = ADDR_IMP },
	{ .idx = 0x79, .name = "ADC", .func = OP_ADC, .cycles = 4, .mode = ADDR_IAY },
	{ .idx = 0x7A, .name = "NOP", .func = OP_NOP, .cycles = 2, .mode = ADDR_IMP },
	{ .idx = 0x7B, .name = "RRA", .func = OP_RRA, .cycles = 7, .mode = ADDR_IAY },
	{ .idx = 0x7C, .name = "NOP", .func = OP_NOP, .cycles = 4, .mode = ADDR_IAX },
	{ .idx = 0x7D, .name = "ADC", .func = OP_ADC, .cycles = 4, .mode = ADDR_IAX },
	{ .idx = 0x7E, .name = "ROR", .func = OP_ROR, .cycles = 7, .mode = ADDR_IAX },
	{ .idx = 0x7F, .name = "RRA", .func = OP_RRA, .cycles = 7, .mode = ADDR_IAX },

	{ .idx = 0x80, .name = "NOP", .func = OP_NOP, .cycles = 2, .mode = ADDR_IMM },
	{ .idx = 0x81, .name = "STA", .func = OP_STA, .cycles = 6, .mode = ADDR_INX },
	{ .idx = 0x82, .name = "NOP", .func = OP_NOP, .cycles = 2, .mode = ADDR_IMM },
	{ .idx = 0x83, .name = "SAX", .func = OP_SAX, .cycles = 6, .mode = ADDR_INX },
	{ .idx = 0x84, .name = "STY", .func = OP_STY, .cycles = 3, .mode = ADDR_ZPG },
	{ .idx = 0x85, .name = "STA", .func = OP_STA, .cycles = 3, .mode = ADDR_ZPG },
	{ .idx = 0x86, .name = "STX", .func = OP_STX, .cycles = 3, .mode = ADDR_ZPG },
	{ .idx = 0x87, .name = "SAX", .func = OP_SAX, .cycles = 3, .mode = ADDR_ZPG },
	{ .idx = 0x88, .name = "DEY", .func = OP_DEY, .cycles = 2, .mode = ADDR_IMP },
	{ .idx = 0x89, .name = "NOP", .func = OP_NOP, .cycles = 2, .mode = ADDR_IMM },
	{ .idx = 0x8A, .name = "TXA", .func = OP_TXA, .cycles = 2, .mode = ADDR_IMP },
	{ .idx = 0x8B, .name = "ANE", .func = OP_ANE, .cycles = 2, .mode = ADDR_IMM },
	{ .idx = 0x8C, .name = "STY", .func = OP_STY, .cycles = 4, .mode = ADDR_ABS },
	{ .idx = 0x8D, .name = "STA", .func = OP_STA, .cycles = 4, .mode = ADDR_ABS },
	{ .idx = 0x8E, .name = "STX", .func = OP_STX, .cycles = 4, .mode = ADDR_ABS },
	{ .idx = 0x8F, .name = "SAX", .func = OP_SAX, .cycles = 4, .mode = ADDR_ABS },

	{ .idx = 0x90, .name = "BCC", .func = OP_BCC, .cycles = 2, .mode = ADDR_REL },
	{ .idx = 0x91, .name = "STA", .func = OP_STA, .cycles = 6, .mode = ADDR_INY },
	{ .idx = 0x92, .name = "ILL", .func = OP_ILL, .cycles = 0, .mode = ADDR_ILL },
	{ .idx = 0x93, .name = "SHA", .func = OP_SHA, .cycles = 6, .mode = ADDR_INY },
	{ .idx = 0x94, .name = "STY", .func = OP_STY, .cycles = 4, .mode = ADDR_IZX },
	{ .idx = 0x95, .name = "STA", .func = OP_STA, .cycles = 4, .mode = ADDR_IZX },
	{ .idx = 0x96, .name = "STX", .func = OP_STX, .cycles = 4, .mode = ADDR_IZY },
	{ .idx = 0x97, .name = "SAX", .func = OP_SAX, .cycles = 4, .mode = ADDR_IZY },
	{ .idx = 0x98, .name = "TYA", .func = OP_TYA, .cycles = 2, .mode = ADDR_IMP },
	{ .idx = 0x99, .name = "STA", .func = OP_STA, .cycles = 5, .mode = ADDR_IAY },
	{ .idx = 0x9A, .name = "TXS", .func = OP_TXS, .cycles = 2, .mode = ADDR_IMP },
	{ .idx = 0x9B, .name = "TAS", .func = OP_TAS, .cycles = 2, .mode = ADDR_IAY },
	{ .idx = 0x9C, .name = "SHY", .func = OP_SHY, .cycles = 5, .mode = ADDR_IAX },
	{ .idx = 0x9D, .name = "STA", .func = OP_STA, .cycles = 5, .mode = ADDR_IAX },
	{ .idx = 0x9E, .name = "SHX", .func = OP_SHX, .cycles = 5, .mode = ADDR_IAY },
	{ .idx = 0x9F, .name = "SHA", .func = OP_SHA, .cycles = 5, .mode = ADDR_IAY },

	{ .idx = 0xA0, .name = "LDY", .func = OP_LDY, .cycles = 2, .mode = ADDR_IMM },
	{ .idx = 0xA1, .name = "LDA", .func = OP_LDA, .cycles = 6, .mode = ADDR_INX },
	{ .idx = 0xA2, .name = "LDX", .func = OP_LDX, .cycles = 2, .mode = ADDR_IMM },
	{ .idx = 0xA3, .name = "LAX", .func = OP_LAX, .cycles = 6, .mode = ADDR_INX },
	{ .idx = 0xA4, .name = "LDY", .func = OP_LDY, .cycles = 3, .mode = ADDR_ZPG },
	{ .idx = 0xA5, .name = "LDA", .func = OP_LDA, .cycles = 3, .mode = ADDR_ZPG },
	{ .idx = 0xA6, .name = "LDX", .func = OP_LDX, .cycles = 3, .mode = ADDR_ZPG },
	{ .idx = 0xA7, .name = "LAX", .func = OP_LAX, .cycles = 3, .mode = ADDR_ZPG },
	{ .idx = 0xA8, .name = "TAY", .func = OP_TAY, .cycles = 2, .mode = ADDR_IMP },
	{ .idx = 0xA9, .name = "LDA", .func = OP_LDA, .cycles = 2, .mode = ADDR_IMM },
	{ .idx = 0xAA, .name = "TAX", .func = OP_TAX, .cycles = 2, .mode = ADDR_IMP },
	{ .idx = 0xAB, .name = "LXA", .func = OP_LXA, .cycles = 2, .mode = ADDR_IMM },
	{ .idx = 0xAC, .name = "LDY", .func = OP_LDY, .cycles = 4, .mode = ADDR_ABS },
	{ .idx = 0xAD, .name = "LDA", .func = OP_LDA, .cycles = 4, .mode = ADDR_ABS },
	{ .idx = 0xAE, .name = "LDX", .func = OP_LDX, .cycles = 4, .mode = ADDR_ABS },
	{ .idx = 0xAF, .name = "LAX", .func = OP_LAX, .cycles = 4, .mode = ADDR_ABS },

	{ .idx = 0xB0, .name = "BCS", .func = OP_BCS, .cycles = 2, .mode = ADDR_REL },
	{ .idx = 0xB1, .name = "LDA", .func = OP_LDA, .cycles = 5, .mode = ADDR_INY },
	{ .idx = 0xB2, .name = "ILL", .func = OP_ILL, .cycles = 0, .mode = ADDR_ILL },
	{ .idx = 0xB3, .name = "LAX", .func = OP_LAX, .cycles = 5, .mode = ADDR_INY },
	{ .idx = 0xB4, .name = "LDY", .func = OP_LDY, .cycles = 4, .mode = ADDR_IZX },
	{ .idx = 0xB5, .name = "LDA", .func = OP_LDA, .cycles = 4, .mode = ADDR_IZX },
	{ .idx = 0xB6, .name = "LDX", .func = OP_LDX, .cycles = 4, .mode = ADDR_IZY },
	{ .idx = 0xB7, .name = "LAX", .func = OP_LAX, .cycles = 4, .mode = ADDR_IZY },
	{ .idx = 0xB8, .name = "CLV", .func = OP_CLV, .cycles = 2, .mode = ADDR_IMP },
	{ .idx = 0xB9, .name = "LDA", .func = OP_LDA, .cycles = 4, .mode = ADDR_IAY },
	{ .idx = 0xBA, .name = "TSX", .func = OP_TSX, .cycles = 2, .mode = ADDR_IMP },
	{ .idx = 0xBB, .name = "LAS", .func = OP_LAS, .cycles = 2, .mode = ADDR_IAY },
	{ .idx = 0xBC, .name = "LDY", .func = OP_LDY, .cycles = 4, .mode = ADDR_IAX },
	{ .idx = 0xBD, .name = "LDA", .func = OP_LDA, .cycles = 4, .mode = ADDR_IAX },
	{ .idx = 0xBE, .name = "LDX", .func = OP_LDX, .cycles = 4, .mode = ADDR_IAY },
	{ .idx = 0xBF, .name = "LAX", .func = OP_LAX, .cycles = 4, .mode = ADDR_IAY },

	{ .idx = 0xC0, .name = "CPY", .func = OP_CPY, .cycles = 2, .mode = ADDR_IMM },
	{ .idx = 0xC1, .name = "CMP", .func = OP_CMP, .cycles = 6, .mode = ADDR_INX },
	{ .idx = 0xC2, .name = "NOP", .func = OP_NOP, .cycles = 2, .mode = ADDR_IMM },
	{ .idx = 0xC3, .name = "DCP", .func = OP_DCP, .cycles = 8, .mode = ADDR_INX },
	{ .idx = 0xC4, .name = "CPY", .func = OP_CPY, .cycles = 3, .mode = ADDR_ZPG },
	{ .idx = 0xC5, .name = "CMP", .func = OP_CMP, .cycles = 3, .mode = ADDR_ZPG },
	{ .idx = 0xC6, .name = "DEC", .func = OP_DEC, .cycles = 5, .mode = ADDR_ZPG },
	{ .idx = 0xC7, .name = "DCP", .func = OP_DCP, .cycles = 5, .mode = ADDR_ZPG },
	{ .idx = 0xC8, .name = "INY", .func = OP_INY, .cycles = 2, .mode = ADDR_IMP },
	{ .idx = 0xC9, .name = "CMP", .func = OP_CMP, .cycles = 2, .mode = ADDR_IMM },
	{ .idx = 0xCA, .name = "DEX", .func = OP_DEX, .cycles = 2, .mode = ADDR_IMP },
	{ .idx = 0xCB, .name = "SBX", .func = OP_SBX, .cycles = 2, .mode = ADDR_IMM },
	{ .idx = 0xCC, .name = "CPY", .func = OP_CPY, .cycles = 4, .mode = ADDR_ABS },
	{ .idx = 0xCD, .name = "CMP", .func = OP_CMP, .cycles = 4, .mode = ADDR_ABS },
	{ .idx = 0xCE, .name = "DEC", .func = OP_DEC, .cycles = 6, .mode = ADDR_ABS },
	{ .idx = 0xCF, .name = "DCP", .func = OP_DCP, .cycles = 6, .mode = ADDR_ABS },

	{ .idx = 0xD0, .name = "BNE", .func = OP_BNE, .cycles = 2, .mode = ADDR_REL },
	{ .idx = 0xD1, .name = "CMP", .func = OP_CMP, .cycles = 5, .mode = ADDR_INY },
	{ .idx = 0xD2, .name = "ILL", .func = OP_ILL, .cycles = 0, .mode = ADDR_ILL },
	{ .idx = 0xD3, .name = "DCP", .func = OP_DCP, .cycles = 8, .mode = ADDR_INY },
	{ .idx = 0xD4, .name = "NOP", .func = OP_NOP, .cycles = 4, .mode = ADDR_IZX },
	{ .idx = 0xD5, .name = "CMP", .func = OP_CMP, .cycles = 4, .mode = ADDR_IZX },
	{ .idx = 0xD6, .name = "DEC", .func = OP_DEC, .cycles = 6, .mode = ADDR_IZX },
	{ .idx = 0xD7, .name = "DCP", .func = OP_DCP, .cycles = 6, .mode = ADDR_IZX },
	{ .idx = 0xD8, .name = "CLD", .func = OP_CLD, .cycles = 2, .mode = ADDR_IMP },
	{ .idx = 0xD9, .name = "CMP", .func = OP_CMP, .cycles = 4, .mode = ADDR_IAY },
	{ .idx = 0xDA, .name = "NOP", .func = OP_NOP, .cycles = 2, .mode = ADDR_IMP },
	{ .idx = 0xDB, .name = "DCP", .func = OP_DCP, .cycles = 7, .mode = ADDR_IAY },
	{ .idx = 0xDC, .name = "NOP", .func = OP_NOP, .cycles = 4, .mode = ADDR_IAX },
	{ .idx = 0xDD, .name = "CMP", .func = OP_CMP, .cycles = 4, .mode = ADDR_IAX },
	{ .idx = 0xDE, .name = "DEC", .func = OP_DEC, .cycles = 7, .mode = ADDR_IAX },
	{ .idx = 0xDF, .name = "DCP", .func = OP_DCP, .cycles = 7, .mode = ADDR_IAX },

	{ .idx = 0xE0, .name = "CPX", .func = OP_CPX, .cycles = 2, .mode = ADDR_IMM },
	{ .idx = 0xE1, .name = "SBC", .func = OP_SBC, .cycles = 6, .mode = ADDR_INX },
	{ .idx = 0xE2, .name = "NOP", .func = OP_NOP, .cycles = 2, .mode = ADDR_IMM },
	{ .idx = 0xE3, .name = "ISC", .func = OP_ISC, .cycles = 8, .mode = ADDR_INX },
	{ .idx = 0xE4, .name = "CPX", .func = OP_CPX, .cycles = 3, .mode = ADDR_ZPG },
	{ .idx = 0xE5, .name = "SBC", .func = OP_SBC, .cycles = 3, .mode = ADDR_ZPG },
	{ .idx = 0xE6, .name = "INC", .func = OP_INC, .cycles = 5, .mode = ADDR_ZPG },
	{ .idx = 0xE7, .name = "ISC", .func = OP_ISC, .cycles = 5, .mode = ADDR_ZPG },
	{ .idx = 0xE8, .name = "INX", .func = OP_INX, .cycles = 2, .mode = ADDR_IMP },
	{ .idx = 0xE9, .name = "SBC", .func = OP_SBC, .cycles = 2, .mode = ADDR_IMM },
	{ .idx = 0xEA, .name = "NOP", .func = OP_NOP, .cycles = 2, .mode = ADDR_IMP },
	{ .idx = 0xEB, .name = "SBC", .func = OP_SBC, .cycles = 2, .mode = ADDR_IMM },
	{ .idx = 0xEC, .name = "CPX", .func = OP_CPX, .cycles = 4, .mode = ADDR_ABS },
	{ .idx = 0xED, .name = "SBC", .func = OP_SBC, .cycles = 4, .mode = ADDR_ABS },
	{ .idx = 0xEE, .name = "INC", .func = OP_INC, .cycles = 6, .mode = ADDR_ABS },
	{ .idx = 0xEF, .name = "ISC", .func = OP_ISC, .cycles = 6, .mode = ADDR_ABS },

	{ .idx = 0xF0, .name = "BEQ", .func = OP_BEQ, .cycles = 2, .mode = ADDR_REL },
	{ .idx = 0xF1, .name = "SBC", .func = OP_SBC, .cycles = 5, .mode = ADDR_INY },
	{ .idx = 0xF2, .name = "ILL", .func = OP_ILL, .cycles = 0, .mode = ADDR_ILL },
	{ .idx = 0xF3, .name = "ISC", .func = OP_ISC, .cycles = 8, .mode = ADDR_INY },
	{ .idx = 0xF4, .name = "NOP", .func = OP_NOP, .cycles = 4, .mode = ADDR_IZX },
	{ .idx = 0xF5, .name = "SBC", .func = OP_SBC, .cycles = 4, .mode = ADDR_IZX },
	{ .idx = 0xF6, .name = "INC", .func = OP_INC, .cycles = 6, .mode = ADDR_IZX },
	{ .idx = 0xF7, .name = "ISC", .func = OP_ISC, .cycles = 6, .mode = ADDR_IZX },
	{ .idx = 0xF8, .name = "SED", .func = OP_SED, .cycles = 2, .mode = ADDR_IMP },
	{ .idx = 0xF9, .name = "SBC", .func = OP_SBC, .cycles = 4, .mode = ADDR_IAY },
	{ .idx = 0xFA, .name = "NOP", .func = OP_NOP, .cycles = 2, .mode = ADDR_IMP },
	{ .idx = 0xFB, .name = "ISC", .func = OP_ISC, .cycles = 7, .mode = ADDR_IAY },
	{ .idx = 0xFC, .name = "NOP", .func = OP_NOP, .cycles = 4, .mode = ADDR_IAX },
	{ .idx = 0xFD, .name = "SBC", .func = OP_SBC, .cycles = 4, .mode = ADDR_IAX },
	{ .idx = 0xFE, .name = "INC", .func = OP_INC, .cycles = 7, .mode = ADDR_IAX },
	{ .idx = 0xFF, .name = "ISC", .func = OP_ISC, .cycles = 7, .mode = ADDR_IAX }
};

static inline uint8_t getflag(cpu *cpu, uint8_t mask) { return cpu->P & mask; }
static inline uint8_t get_c(cpu *cpu)                 { return getflag(cpu, MASK_CARRY); }
static inline uint8_t get_n(cpu *cpu)                 { return getflag(cpu, MASK_NEGATIVE); }
static inline uint8_t get_v(cpu *cpu)                 { return getflag(cpu, MASK_OVERFLOW); }
static inline uint8_t get_z(cpu *cpu)                 { return getflag(cpu, MASK_ZERO); }

static inline void setflag(cpu *cpu, uint8_t mask) { cpu->P |= mask; }
static inline void set_b(cpu *cpu)                 { setflag(cpu, MASK_BREAK); }
static inline void set_c(cpu *cpu)                 { setflag(cpu, MASK_CARRY); }
static inline void set_d(cpu *cpu)                 { setflag(cpu, MASK_DECIMAL); }
static inline void set_i(cpu *cpu)                 { setflag(cpu, MASK_INTERRUPT_DISABLE); }

static inline void unsetflag(cpu *cpu, uint8_t mask) { cpu->P &= ~mask; }
static inline void unset_b(cpu *cpu)                 { unsetflag(cpu, MASK_BREAK); }
static inline void unset_c(cpu *cpu)                 { unsetflag(cpu, MASK_CARRY); }
static inline void unset_d(cpu *cpu)                 { unsetflag(cpu, MASK_DECIMAL); }
static inline void unset_i(cpu *cpu)                 { unsetflag(cpu, MASK_INTERRUPT_DISABLE); }
static inline void unset_v(cpu *cpu)                 { unsetflag(cpu, MASK_OVERFLOW); }

static inline void
upd_flag(cpu *cpu, uint8_t mask, uint8_t val)
{
	if (val) {
		setflag(cpu, mask);
	} else {
		unsetflag(cpu, mask);
	}
}

static inline void upd_c(cpu *cpu, uint8_t val) { upd_flag(cpu, MASK_CARRY, val); }
static inline void upd_z(cpu *cpu, uint8_t val) { upd_flag(cpu, MASK_ZERO, val); }
static inline void upd_v(cpu *cpu, uint8_t val) { upd_flag(cpu, MASK_OVERFLOW, val); }
static inline void upd_n(cpu *cpu, uint8_t val) { upd_flag(cpu, MASK_NEGATIVE, val); }

static inline void
upd_zn(cpu *cpu, uint8_t val)
{
	upd_z(cpu, val == 0);
	upd_n(cpu, val & MASK_NEGATIVE); /* check if bit 7 of val is set */
}

static inline addr_mode
get_addr_mode(cpu *cpu)
{
	return optable[cpu->opcode].mode;
}

static inline int
is_acc_mode(cpu *cpu)
{
	addr_mode mode = get_addr_mode(cpu);
	return mode == ADDR_ACC;
}

static inline int
has_memory_operand(cpu *cpu)
{
	addr_mode mode = get_addr_mode(cpu);
	return mode != ADDR_IMP && mode != ADDR_IMM;
}

static inline int
is_indexed_absolute(cpu *cpu)
{
	addr_mode mode = get_addr_mode(cpu);
	return mode == ADDR_IAX || mode == ADDR_IAY;
}

static int
is_page_crossed(cpu *cpu)
{
	uint16_t address = cpu->addr;
	uint16_t base;
	addr_mode mode = get_addr_mode(cpu);

	if (mode == ADDR_IAX) {
		base = address - cpu->X;
	} else if (mode == ADDR_IAY || mode == ADDR_INY) {
		base = address - cpu->Y;
	} else {
		return 0;
	}

	return (address & 0xFF00) != (base & 0xFF00);
}

static void
dummy_get8(cpu *cpu, uint16_t addr)
{
	(void)get8_addr(cpu, addr);
}

static void
write8_addr(cpu *cpu, uint16_t addr, uint8_t data)
{
	bus_write(cpu->bus, addr, data);
	cpu->open_bus = data;
}

static void
write8(cpu *cpu, uint8_t data)
{
	if (is_indexed_absolute(cpu)) {
		dummy_get8(cpu, cpu->addr);
	}
	if (is_page_crossed(cpu)) {
		dummy_get8(cpu, cpu->addr - 0x100);
	}
	write8_addr(cpu, cpu->addr, data);
}

static void
push8(cpu *cpu, uint8_t data)
{
	uint16_t addr = STACK_OFFSET + cpu->SP;
	cpu->SP--;
	write8_addr(cpu, addr, data);
}

static void
push16(cpu *cpu, uint16_t data)
{
	push8(cpu, (uint8_t)(data >> 8));
	push8(cpu, (uint8_t)(data & 0x00FF));
}

static uint8_t
get8_addr(cpu *cpu, uint16_t addr)
{
	uint8_t val = bus_read(cpu->bus, addr);
	if (addr != 0x4015) { /* TODO: rewrite */
		cpu->open_bus = val;
	}
	return val;
}

static uint8_t
get8(cpu *cpu)
{
	if (is_page_crossed(cpu)) {
		cpu->stall++;
		dummy_get8(cpu, cpu->addr - 0x100);
	}
	return get8_addr(cpu, cpu->addr);
}

static uint8_t
read8(cpu *cpu)
{
	return get8_addr(cpu, cpu->PC++);
}

static uint8_t
pop8(cpu *cpu)
{
	cpu->SP++;
	uint16_t addr = STACK_OFFSET + cpu->SP;
	return get8_addr(cpu, addr);
}

static uint16_t
get16_addr(cpu *cpu, uint16_t addr)
{
	uint8_t lo = get8_addr(cpu, addr);
	uint8_t hi = get8_addr(cpu, addr + 1); /* TODO: check address somehow */
	return (hi << 8) | lo;
}

static uint16_t
pop16(cpu *cpu)
{
	uint8_t lo = pop8(cpu);
	uint8_t hi = pop8(cpu);
	return (hi << 8) | lo;
}

static uint16_t
read16(cpu *cpu)
{
	uint8_t lo = read8(cpu);
	uint8_t hi = read8(cpu);
	return (hi << 8) | lo;
}

static inline uint8_t
overflowed_sum(uint8_t a, uint8_t b, uint8_t c)
{
	/*
	 * returns 1 if A and B have the same sign
	 * but A and C have different signs
	 */
	return ((a ^ b) & 0x80) == 0 && ((a ^ c) & 0x80) != 0;
}

static inline uint8_t
overflowed_sub(uint8_t a, uint8_t b, uint8_t c)
{
	/*
	 * like overflowed_sum but for subtraction
	 */
	return ((a ^ b) & 0x80) != 0 && ((a ^ c) & 0x80) != 0;
}

static void
poll_interrupts(cpu *cpu)
{
	if (cpu->nmi) {
		handle_nmi(cpu);
		cpu->nmi = 0;
		return;
	}

	if (cpu->irq) {
		handle_irq(cpu);
		cpu->irq = 0;
		return;
	}
}

static void
handle_irq(cpu *cpu)
{
	set_i(cpu);
	push16(cpu, cpu->PC);
	push8(cpu, cpu->P | MASK_UNUSED);
	cpu->stall += 7;
	cpu->PC = get16_addr(cpu, VECTOR_IRQ);
}

static void
handle_nmi(cpu *cpu)
{
	push16(cpu, cpu->PC);
	push8(cpu, cpu->P | MASK_UNUSED);
	cpu->stall += 7;
	cpu->PC = get16_addr(cpu, VECTOR_NMI);
}

static void
ADDR_ABS(cpu *cpu)
{
	cpu->addr = read16(cpu);
}

static void
ADDR_ACC(cpu *cpu)
{
	(void)cpu;
}

static void
ADDR_IAX(cpu *cpu)
{
	cpu->addr = read16(cpu) + cpu->X;
}

static void
ADDR_IAY(cpu *cpu)
{
	cpu->addr = read16(cpu) + cpu->Y;
}

static void
ADDR_IMM(cpu *cpu)
{
	cpu->addr = cpu->PC++;
}

static void
ADDR_IMP(cpu *cpu)
{
	(void)cpu;
}

static void
ADDR_IND(cpu *cpu)
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
	uint8_t addr_lo = get8_addr(cpu, location_hi << 8 | location_lo);
	uint8_t addr_hi = get8_addr(cpu, location_hi << 8 | (uint8_t)(location_lo + 1));
	cpu->addr = addr_hi << 8 | addr_lo;
}

static void
ADDR_INX(cpu *cpu)
{
	uint8_t location = read8(cpu);
	uint8_t addr_lo = get8_addr(cpu, (location + cpu->X) & 0x00FF);
	uint8_t addr_hi = get8_addr(cpu, (location + cpu->X + 1) & 0x00FF);
	cpu->addr = addr_hi << 8 | addr_lo;
}

static void
ADDR_INY(cpu *cpu)
{
	uint8_t location = read8(cpu);
	uint8_t addr_lo = get8_addr(cpu, location);
	uint8_t addr_hi = get8_addr(cpu, (location + 1) & 0x00FF);
	cpu->addr = (uint16_t)((addr_hi << 8 | addr_lo) + cpu->Y);
}

static void
ADDR_IZX(cpu *cpu)
{
	cpu->addr = (read8(cpu) + cpu->X) & 0x00FF;
}

static void
ADDR_IZY(cpu *cpu)
{
	cpu->addr = (read8(cpu) + cpu->Y) & 0x00FF;
}

static void
ADDR_REL(cpu *cpu)
{
	int8_t offset = (int8_t)read8(cpu);
	cpu->addr = (uint16_t)(cpu->PC + offset);
}

static void
ADDR_ZPG(cpu *cpu) {
	cpu->addr = read8(cpu);
}

static void
ADDR_ILL(cpu *cpu)
{
	(void)cpu; /* to remove compiler warning */
	fprintf(stderr, "illegal address mode!\n");
}

static void
OP_ADC(cpu *cpu)
{
	uint8_t acc = cpu->A;
	uint8_t val = get8(cpu);
	uint8_t carry = get_c(cpu);

	cpu->A = (uint8_t)(acc + val + carry);

	upd_c(cpu, (int)acc + (int)val + (int)carry > 0xFF); /* TODO: create func detect_carry() */
	upd_v(cpu, overflowed_sum(acc, val, cpu->A));
	upd_zn(cpu, cpu->A);
}

static void
OP_AND(cpu *cpu)
{
	cpu->A &= get8(cpu);
	upd_zn(cpu, cpu->A);
}

static void
OP_ASL(cpu *cpu)
{
	uint8_t val;

	if (is_acc_mode(cpu)) {
		upd_c(cpu, cpu->A & 0x80);
		cpu->A <<= 1;
		upd_zn(cpu, cpu->A);
	} else {
		val = get8(cpu);
		upd_c(cpu, val & 0x80);
		write8(cpu, val);
		val <<= 1;
		write8(cpu, val);
		upd_zn(cpu, val);
	}
}

static void
OP_BCC(cpu *cpu)
{
	if (!get_c(cpu)) {
		cpu->PC = cpu->addr;
	}
}

static void
OP_BCS(cpu *cpu)
{
	if (get_c(cpu)) {
		cpu->PC = cpu->addr;
	}
}

static void
OP_BEQ(cpu *cpu)
{
	if (get_z(cpu)) {
		cpu->PC = cpu->addr;
	}
}

static void
OP_BIT(cpu *cpu)
{
	uint8_t val = get8(cpu);
	upd_z(cpu, (cpu->A & val) == 0);
	upd_n(cpu, val & MASK_NEGATIVE);
	upd_v(cpu, val & MASK_OVERFLOW);
}

static void
OP_BMI(cpu *cpu)
{
	if (get_n(cpu)) {
		cpu->PC = cpu->addr;
	}
}

static void
OP_BNE(cpu *cpu)
{
	if (!get_z(cpu)) {
		cpu->PC = cpu->addr;
	}
}

static void
OP_BPL(cpu *cpu)
{
	if (!get_n(cpu)) {
		cpu->PC = cpu->addr;
	}
}

static void
OP_BRK(cpu *cpu)
{
	push16(cpu, cpu->PC + 1);
	set_i(cpu);
	push8(cpu, cpu->P | MASK_BREAK | MASK_UNUSED);
	cpu->PC = get16_addr(cpu, VECTOR_IRQ);
}

static void
OP_BVC(cpu *cpu)
{
	if (!get_v(cpu)) {
		cpu->PC = cpu->addr;
	}
}

static void
OP_BVS(cpu *cpu)
{
	if (get_v(cpu)) {
		cpu->PC = cpu->addr;
	}
}

static void
OP_CLC(cpu *cpu)
{
	unset_c(cpu);
}

static void
OP_CLD(cpu *cpu)
{
	unset_d(cpu);
}

static void
OP_CLI(cpu *cpu)
{
	unset_i(cpu);
}

static void
OP_CLV(cpu *cpu)
{
	unset_v(cpu);
}

static void
OP_CMP(cpu *cpu)
{
	uint8_t val = get8(cpu);
	upd_c(cpu, cpu->A >= val);
	upd_z(cpu, cpu->A == val);
	upd_n(cpu, (cpu->A - val) & MASK_NEGATIVE);
}

static void
OP_CPX(cpu *cpu)
{
	uint8_t val = get8(cpu);
	upd_c(cpu, cpu->X >= val);
	upd_z(cpu, cpu->X == val);
	upd_n(cpu, (cpu->X - val) & MASK_NEGATIVE);
}

static void
OP_CPY(cpu *cpu)
{
	uint8_t val = get8(cpu);
	upd_c(cpu, cpu->Y >= val);
	upd_z(cpu, cpu->Y == val);
	upd_n(cpu, (cpu->Y - val) & MASK_NEGATIVE);
}

static void
OP_DEC(cpu *cpu)
{
	uint8_t val = get8(cpu);
	write8(cpu, val);
	write8(cpu, val - 1);
	upd_z(cpu, (val - 1) == 0);
	upd_n(cpu, (val - 1) & MASK_NEGATIVE);
}

static void
OP_DEX(cpu *cpu)
{
	cpu->X--;
	upd_zn(cpu, cpu->X);
}

static void
OP_DEY(cpu *cpu)
{
	cpu->Y--;
	upd_zn(cpu, cpu->Y);
}

static void
OP_EOR(cpu *cpu)
{
	cpu->A ^= get8(cpu);
	upd_zn(cpu, cpu->A);
}

static void
OP_INC(cpu *cpu)
{
	uint8_t val = get8(cpu);
	write8(cpu, val);
	val++;
	write8(cpu, val);
	upd_zn(cpu, val);
}

static void
OP_INX(cpu *cpu)
{
	cpu->X++;
	upd_zn(cpu, cpu->X);
}

static void
OP_INY(cpu *cpu)
{
	cpu->Y++;
	upd_zn(cpu, cpu->Y);
}

static void
OP_JMP(cpu *cpu)
{
	cpu->PC = cpu->addr;
}

static void
OP_JSR(cpu *cpu)
{
	push16(cpu, cpu->PC - 1);
	cpu->PC = cpu->addr;
	cpu->open_bus = cpu->addr >> 8;
}

static void
OP_LDA(cpu *cpu)
{
	cpu->A = get8(cpu);
	upd_zn(cpu, cpu->A);
}

static void
OP_LDX(cpu *cpu)
{
	cpu->X = get8(cpu);
	upd_zn(cpu, cpu->X);
}

static void
OP_LDY(cpu *cpu)
{
	cpu->Y = get8(cpu);
	upd_zn(cpu, cpu->Y);
}

static void
OP_LSR(cpu *cpu)
{
	uint8_t val;

	if (is_acc_mode(cpu)) {
		upd_c(cpu, cpu->A & MASK_CARRY);
		cpu->A >>= 1;
		upd_zn(cpu, cpu->A);
	} else {
		val = get8(cpu);
		upd_c(cpu, val & MASK_CARRY);
		write8(cpu, val);
		val >>= 1;
		write8(cpu, val);
		upd_zn(cpu, val);
	}
}

static void
OP_NOP(cpu *cpu)
{
	/*
	 * The NOP instruction causes no changes to the processor
	 * other than the normal incrementing of the program counter
	 * to the next instruction. However, a NOP with a memory
	 * operand must perform a dummy read of that operand
	 */
	if (has_memory_operand(cpu)) {
		(void)get8(cpu);
	}
}

static void
OP_ORA(cpu *cpu)
{
	cpu->A |= get8(cpu);
	upd_zn(cpu, cpu->A);
}

static void
OP_PHA(cpu *cpu)
{
	push8(cpu, cpu->A);
}

static void
OP_PHP(cpu *cpu)
{
	push8(cpu, cpu->P | MASK_BREAK | MASK_UNUSED);
}

static void
OP_PLA(cpu *cpu)
{
	cpu->A = pop8(cpu);
	upd_zn(cpu, cpu->A);
}

static void
OP_PLP(cpu *cpu)
{
	cpu->P = pop8(cpu);
	unset_b(cpu);
}

static void
OP_ROL(cpu *cpu)
{
	uint8_t carry = get_c(cpu);
	uint8_t val;

	if (is_acc_mode(cpu)) {
		upd_c(cpu, cpu->A & 0x80);
		cpu->A = (cpu->A << 1) | carry;
		upd_zn(cpu, cpu->A);
	} else {
		val = get8(cpu);
		upd_c(cpu, val & 0x80);
		write8(cpu, val);
		val = (val << 1) | carry;
		write8(cpu, val);
		upd_zn(cpu, val);
	}
}

static void
OP_ROR(cpu *cpu)
{
	uint8_t carry = get_c(cpu);
	uint8_t val;

	if (is_acc_mode(cpu)) {
		upd_c(cpu, cpu->A & 0x01);
		cpu->A = (cpu->A >> 1) | (carry << 7);
		upd_zn(cpu, cpu->A);
	} else {
		val = get8(cpu);
		upd_c(cpu, val & 0x01);
		write8(cpu, val);
		val = (val >> 1) | (carry << 7);
		write8(cpu, val);
		upd_zn(cpu, val);
	}
}

static void
OP_RTI(cpu *cpu)
{
	cpu->P = pop8(cpu);
	cpu->PC = pop16(cpu);
	poll_interrupts(cpu);
}

static void
OP_RTS(cpu *cpu)
{
	cpu->PC = pop16(cpu) + 1;
}

static void
OP_SBC(cpu *cpu)
{
	uint8_t acc = cpu->A;
	uint8_t val = get8(cpu);
	uint8_t carry = get_c(cpu);

	cpu->A = (uint8_t)(acc - val - (1 - carry));

	upd_c(cpu, (int)acc - (int)val - (int)(1 - carry) >= 0x00);
	upd_v(cpu, overflowed_sub(acc, val, cpu->A));
	upd_zn(cpu, cpu->A);
}

static void
OP_SEC(cpu *cpu)
{
	set_c(cpu);
}

static void
OP_SED(cpu *cpu)
{
	set_d(cpu);
}

static void
OP_SEI(cpu *cpu)
{
	set_i(cpu);
}

static void
OP_STA(cpu *cpu)
{
	write8(cpu, cpu->A);
}

static void
OP_STX(cpu *cpu)
{
	write8(cpu, cpu->X);
}

static void
OP_STY(cpu *cpu)
{
	write8(cpu, cpu->Y);
}

static void
OP_TAX(cpu *cpu)
{
	cpu->X = cpu->A;
	upd_zn(cpu, cpu->X);
}

static void
OP_TAY(cpu *cpu)
{
	cpu->Y = cpu->A;
	upd_zn(cpu, cpu->Y);
}

static void
OP_TSX(cpu *cpu)
{
	cpu->X = cpu->SP;
	upd_zn(cpu, cpu->X);
}

static void
OP_TXA(cpu *cpu)
{
	cpu->A = cpu->X;
	upd_zn(cpu, cpu->A);
}

static void
OP_TXS(cpu *cpu)
{
	cpu->SP = cpu->X;
}

static void
OP_TYA(cpu *cpu)
{
	cpu->A = cpu->Y;
	upd_zn(cpu, cpu->A);
}

static void
OP_ALR(cpu *cpu)
{
	uint8_t val = get8(cpu);
	cpu->A &= val;
	upd_c(cpu, cpu->A & MASK_CARRY);
	cpu->A >>= 1;
	upd_zn(cpu, cpu->A);
}

static void
OP_ANE(cpu *cpu)
{
	(void)cpu; /* TODO: OPCODE */
}

static void
OP_ARR(cpu *cpu)
{
	uint8_t val = get8(cpu);
	uint8_t carry = get_c(cpu);

	cpu->A &= val;
	cpu->A = (cpu->A >> 1) | (carry << 7);

	upd_n(cpu, cpu->A & MASK_NEGATIVE);
	upd_c(cpu, cpu->A & 0x40);
	upd_v(cpu, ((cpu->A >> 5) ^ (cpu->A >> 6)) & 0x01);
	upd_z(cpu, cpu->A == 0);
}

static void
OP_ANC(cpu *cpu)
{
	uint8_t val = get8(cpu);
	cpu->A &= val;
	upd_zn(cpu, cpu->A);
	upd_c(cpu, cpu->A & MASK_NEGATIVE);
}

static void
OP_DCP(cpu *cpu)
{
	OP_DEC(cpu);
	OP_CMP(cpu);
}

static void
OP_ISC(cpu *cpu)
{
	OP_INC(cpu);
	OP_SBC(cpu);
}

static void
OP_LAS(cpu *cpu)
{
	cpu->SP &= get8(cpu);
	cpu->A = cpu->SP;
	cpu->X = cpu->SP;
	upd_zn(cpu, cpu->A);
}

static void
OP_LAX(cpu *cpu)
{
	OP_LDA(cpu);
	cpu->X = get8(cpu);
	upd_zn(cpu, cpu->X);
}

static void
OP_LXA(cpu *cpu)
{
	(void)cpu; /* TODO: OPCODE */
}

static void
OP_RLA(cpu *cpu)
{
	OP_ROL(cpu);
	OP_AND(cpu);
}

static void
OP_RRA(cpu *cpu)
{
	OP_ROR(cpu);
	OP_ADC(cpu);
}

static void
OP_SAX(cpu *cpu)
{
	write8(cpu, cpu->A & cpu->X);
}

static void
OP_SBX(cpu *cpu)
{
	uint8_t val = get8(cpu);
	uint8_t acc_with_x = cpu->A & cpu->X;

	cpu->X = acc_with_x - val;

	upd_c(cpu, acc_with_x >= val);
	upd_zn(cpu, cpu->X);
}

static void
OP_SHA(cpu *cpu)
{
	(void)cpu; /* TODO: OPCODE */
}

static void
OP_SHX(cpu *cpu)
{
	(void)cpu; /* TODO: OPCODE */
}

static void
OP_SHY(cpu *cpu)
{
	(void)cpu; /* TODO: OPCODE */
}

static void
OP_SLO(cpu *cpu)
{
	OP_ASL(cpu);
	OP_ORA(cpu);
}

static void
OP_SRE(cpu *cpu)
{
	OP_LSR(cpu);
	OP_EOR(cpu);
}

static void
OP_TAS(cpu *cpu)
{
	(void)cpu; /* TODO: OPCODE */
}

static void
OP_ILL(cpu *cpu)
{
	(void)cpu;
	fprintf(stderr, "illegal opcode!\n"); /* TODO: remove */
	return;
}

void
cpu_reset(cpu *cpu, bus *bus)
{
	cpu->bus = bus;
	cpu->PC = get16_addr(cpu, VECTOR_RESET);
	cpu->SP = 0xFD; /* 0x00 - 0x3. See https://www.youtube.com/watch?v=fWqBmmPQP40&t=2536s */
	cpu->P = 0x24;

	cpu->stall = 7; /* because of interrupt? TODO: check */
	cpu->total = 0;
}

void
cpu_tick(cpu *cpu)
{
	cpu->total++;
	cpu->stall--;

	if (cpu->stall > 0) {
		return;
	}

	poll_interrupts(cpu);

	/* disassemble(cpu); */
	cpu->opcode = read8(cpu);
	cpu->stall += optable[cpu->opcode].cycles;
	optable[cpu->opcode].mode(cpu);
	optable[cpu->opcode].func(cpu);
}

void
cpu_trigger_nmi(cpu *cpu)
{
	cpu->nmi = 1;
}

uint64_t
cpu_get_total_cycles(cpu *cpu)
{
	return cpu->total;
}

void
cpu_set_stall_cycles(cpu *cpu, uint64_t cycles)
{
	cpu->stall = cycles;
}

static disassemble_arg
disassemble_get_arg(cpu *cpu, addr_mode amode)
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
		arg.whole = (uint16_t)(curr_pc + offset);
	} else if (amode == ADDR_ZPG) {
		arg.whole = get8_addr(cpu, curr_pc);
	} else {
		arg.whole = 0;
	}

	return arg;
}

#define ASM_STR_SIZE 25

static void
disassemble_compose_asm_str(cpu *cpu, uint8_t opcode, char strbuf[])
{
	char *curr = strbuf;
	char *end = strbuf + ASM_STR_SIZE;
	size_t buflen = (size_t)(end - curr);
	disassemble_arg arg = disassemble_get_arg(cpu, optable[opcode].mode);
	
	curr += snprintf(curr, buflen, "%02X ", optable[opcode].idx);
	if (arg.part.hi != 0) {
		curr += snprintf(curr, buflen, "%02X %02X ",  arg.part.lo, arg.part.hi);
	} else {
		curr += snprintf(curr, buflen, "%02X    ",  arg.part.lo);
	}
	curr += snprintf(curr, buflen, "%s ", optable[opcode].name);
	curr += snprintf(curr, buflen, "%*X  ", 5, arg.whole);
}

/* Using nestest log format for easy comparision with nestest.log.
 * See:
 * https://github.com/christopherpow/nes-test-roms/blob/master/other/nestest.log
 */
static void
disassemble(cpu *cpu)
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
