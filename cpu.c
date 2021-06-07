#include <stdio.h>

#include "bus.h"
#include "cpu.h"

typedef enum {
	ADDR_ACC,	/* accumulator */
	ADDR_IMM,	/* immediate */
	ADDR_ABS,	/* absolute */
	ADDR_ZP,	/* zero page */
	ADDR_IZX,	/* indexed zero page x */
	ADDR_IZY,	/* indexed zero page y */
	ADDR_IAX,	/* indexed absolute x */
	ADDR_IAY,	/* indexed absolute y */
	ADDR_IMP,	/* implied */
	ADDR_REL,	/* relative */
	ADDR_INX,	/* indexed indirect x */
	ADDR_INY,	/* indirect indexed y */
	ADDR_IND,	/* indirect */
	ADDR_XXX	/* unknown */
} addr_mode;

typedef uint8_t (*opcode_func)(mos6502 *);

typedef struct {
	const char *opcode;
	opcode_func *func;
	uint8_t cycles;
	addr_mode mode;
} instruction;

instruction inst_table[0xFF + 1] = {
	{ .opcode = "BRK", .func = NULL, .cycles = 7, .mode = ADDR_IMP }, /* 0x00 */
	{ .opcode = "ORA", .func = NULL, .cycles = 6, .mode = ADDR_INX },
	{ .opcode = "XXX", .func = NULL, .cycles = 0, .mode = ADDR_XXX },
	{ .opcode = "XXX", .func = NULL, .cycles = 0, .mode = ADDR_XXX },
	{ .opcode = "XXX", .func = NULL, .cycles = 0, .mode = ADDR_XXX },
	{ .opcode = "ORA", .func = NULL, .cycles = 3, .mode = ADDR_ZP },
	{ .opcode = "ASL", .func = NULL, .cycles = 5, .mode = ADDR_ZP },
	{ .opcode = "XXX", .func = NULL, .cycles = 0, .mode = ADDR_XXX },
	{ .opcode = "PHP", .func = NULL, .cycles = 3, .mode = ADDR_IMP },
	{ .opcode = "ORA", .func = NULL, .cycles = 2, .mode = ADDR_IMM },
	{ .opcode = "ASL", .func = NULL, .cycles = 2, .mode = ADDR_ACC },
	{ .opcode = "XXX", .func = NULL, .cycles = 0, .mode = ADDR_XXX },
	{ .opcode = "XXX", .func = NULL, .cycles = 0, .mode = ADDR_XXX },
	{ .opcode = "ORA", .func = NULL, .cycles = 4, .mode = ADDR_ABS },
	{ .opcode = "ASL", .func = NULL, .cycles = 6, .mode = ADDR_ABS },
	{ .opcode = "XXX", .func = NULL, .cycles = 0, .mode = ADDR_XXX },

	{ .opcode = "BPL", .func = NULL, .cycles = 2, .mode = ADDR_REL }, /* 0x10 */
	{ .opcode = "ORA", .func = NULL, .cycles = 5, .mode = ADDR_INY },
	{ .opcode = "XXX", .func = NULL, .cycles = 0, .mode = ADDR_XXX },
	{ .opcode = "XXX", .func = NULL, .cycles = 0, .mode = ADDR_XXX },
	{ .opcode = "XXX", .func = NULL, .cycles = 0, .mode = ADDR_XXX },
	{ .opcode = "ORA", .func = NULL, .cycles = 4, .mode = ADDR_IZX },
	{ .opcode = "ASL", .func = NULL, .cycles = 6, .mode = ADDR_IZX },
	{ .opcode = "XXX", .func = NULL, .cycles = 0, .mode = ADDR_XXX },
	{ .opcode = "CLC", .func = NULL, .cycles = 2, .mode = ADDR_IMP },
	{ .opcode = "ORA", .func = NULL, .cycles = 4, .mode = ADDR_IAY },
	{ .opcode = "XXX", .func = NULL, .cycles = 0, .mode = ADDR_XXX },
	{ .opcode = "XXX", .func = NULL, .cycles = 0, .mode = ADDR_XXX },
	{ .opcode = "XXX", .func = NULL, .cycles = 0, .mode = ADDR_XXX },
	{ .opcode = "ORA", .func = NULL, .cycles = 4, .mode = ADDR_IAX },
	{ .opcode = "ASL", .func = NULL, .cycles = 7, .mode = ADDR_IAX },
	{ .opcode = "XXX", .func = NULL, .cycles = 0, .mode = ADDR_XXX },

	{ .opcode = "JSR", .func = NULL, .cycles = 6, .mode = ADDR_ABS }, /* 0x20 */
	{ .opcode = "AND", .func = NULL, .cycles = 6, .mode = ADDR_INX },
	{ .opcode = "XXX", .func = NULL, .cycles = 0, .mode = ADDR_XXX },
	{ .opcode = "XXX", .func = NULL, .cycles = 0, .mode = ADDR_XXX },
	{ .opcode = "BIT", .func = NULL, .cycles = 3, .mode = ADDR_ZP },
	{ .opcode = "AND", .func = NULL, .cycles = 3, .mode = ADDR_ZP },
	{ .opcode = "ROL", .func = NULL, .cycles = 5, .mode = ADDR_ZP },
	{ .opcode = "XXX", .func = NULL, .cycles = 0, .mode = ADDR_XXX },
	{ .opcode = "PLP", .func = NULL, .cycles = 4, .mode = ADDR_IMP },
	{ .opcode = "AND", .func = NULL, .cycles = 2, .mode = ADDR_IMM },
	{ .opcode = "ROL", .func = NULL, .cycles = 2, .mode = ADDR_ACC },
	{ .opcode = "XXX", .func = NULL, .cycles = 0, .mode = ADDR_XXX },
	{ .opcode = "BIT", .func = NULL, .cycles = 4, .mode = ADDR_ABS },
	{ .opcode = "AND", .func = NULL, .cycles = 4, .mode = ADDR_ABS },
	{ .opcode = "ROL", .func = NULL, .cycles = 6, .mode = ADDR_ABS },
	{ .opcode = "XXX", .func = NULL, .cycles = 0, .mode = ADDR_XXX },

	{ .opcode = "BMI", .func = NULL, .cycles = 2, .mode = ADDR_REL }, /* 0x30 */
	{ .opcode = "AND", .func = NULL, .cycles = 5, .mode = ADDR_INY },
	{ .opcode = "XXX", .func = NULL, .cycles = 0, .mode = ADDR_XXX },
	{ .opcode = "XXX", .func = NULL, .cycles = 0, .mode = ADDR_XXX },
	{ .opcode = "XXX", .func = NULL, .cycles = 0, .mode = ADDR_XXX },
	{ .opcode = "AND", .func = NULL, .cycles = 4, .mode = ADDR_IZX },
	{ .opcode = "ROL", .func = NULL, .cycles = 6, .mode = ADDR_IZX },
	{ .opcode = "XXX", .func = NULL, .cycles = 0, .mode = ADDR_XXX },
	{ .opcode = "SEC", .func = NULL, .cycles = 2, .mode = ADDR_IMP },
	{ .opcode = "AND", .func = NULL, .cycles = 4, .mode = ADDR_IAY },
	{ .opcode = "XXX", .func = NULL, .cycles = 0, .mode = ADDR_XXX },
	{ .opcode = "XXX", .func = NULL, .cycles = 0, .mode = ADDR_XXX },
	{ .opcode = "XXX", .func = NULL, .cycles = 0, .mode = ADDR_XXX },
	{ .opcode = "AND", .func = NULL, .cycles = 4, .mode = ADDR_IAX },
	{ .opcode = "ROL", .func = NULL, .cycles = 7, .mode = ADDR_IAX },
	{ .opcode = "XXX", .func = NULL, .cycles = 0, .mode = ADDR_XXX },

	{ .opcode = "RTI", .func = NULL, .cycles = 6, .mode = ADDR_IMP }, /* 0x40 */
	{ .opcode = "EOR", .func = NULL, .cycles = 6, .mode = ADDR_INX },
	{ .opcode = "XXX", .func = NULL, .cycles = 0, .mode = ADDR_XXX },
	{ .opcode = "XXX", .func = NULL, .cycles = 0, .mode = ADDR_XXX },
	{ .opcode = "XXX", .func = NULL, .cycles = 0, .mode = ADDR_XXX },
	{ .opcode = "EOR", .func = NULL, .cycles = 3, .mode = ADDR_ZP },
	{ .opcode = "LSR", .func = NULL, .cycles = 5, .mode = ADDR_ZP },
	{ .opcode = "XXX", .func = NULL, .cycles = 0, .mode = ADDR_XXX },
	{ .opcode = "PHA", .func = NULL, .cycles = 3, .mode = ADDR_IMP },
	{ .opcode = "EOR", .func = NULL, .cycles = 2, .mode = ADDR_IMM },
	{ .opcode = "LSR", .func = NULL, .cycles = 2, .mode = ADDR_ACC },
	{ .opcode = "XXX", .func = NULL, .cycles = 0, .mode = ADDR_XXX },
	{ .opcode = "JMP", .func = NULL, .cycles = 3, .mode = ADDR_ABS },
	{ .opcode = "EOR", .func = NULL, .cycles = 4, .mode = ADDR_ABS },
	{ .opcode = "LSR", .func = NULL, .cycles = 6, .mode = ADDR_ABS },
	{ .opcode = "XXX", .func = NULL, .cycles = 0, .mode = ADDR_XXX },

	{ .opcode = "BVC", .func = NULL, .cycles = 2, .mode = ADDR_REL }, /* 0x50 */
	{ .opcode = "EOR", .func = NULL, .cycles = 5, .mode = ADDR_INY },
	{ .opcode = "XXX", .func = NULL, .cycles = 0, .mode = ADDR_XXX },
	{ .opcode = "XXX", .func = NULL, .cycles = 0, .mode = ADDR_XXX },
	{ .opcode = "XXX", .func = NULL, .cycles = 0, .mode = ADDR_XXX },
	{ .opcode = "EOR", .func = NULL, .cycles = 4, .mode = ADDR_IZX },
	{ .opcode = "LSR", .func = NULL, .cycles = 6, .mode = ADDR_IZX },
	{ .opcode = "XXX", .func = NULL, .cycles = 0, .mode = ADDR_XXX },
	{ .opcode = "CLI", .func = NULL, .cycles = 2, .mode = ADDR_IMP },
	{ .opcode = "EOR", .func = NULL, .cycles = 4, .mode = ADDR_IAY },
	{ .opcode = "XXX", .func = NULL, .cycles = 0, .mode = ADDR_XXX },
	{ .opcode = "XXX", .func = NULL, .cycles = 0, .mode = ADDR_XXX },
	{ .opcode = "XXX", .func = NULL, .cycles = 0, .mode = ADDR_XXX },
	{ .opcode = "EOR", .func = NULL, .cycles = 4, .mode = ADDR_IAX },
	{ .opcode = "LSR", .func = NULL, .cycles = 7, .mode = ADDR_IAX },
	{ .opcode = "XXX", .func = NULL, .cycles = 0, .mode = ADDR_XXX },

	{ .opcode = "RTS", .func = NULL, .cycles = 6, .mode = ADDR_IMP }, /* 0x60 */
	{ .opcode = "ADC", .func = NULL, .cycles = 6, .mode = ADDR_INX },
	{ .opcode = "XXX", .func = NULL, .cycles = 0, .mode = ADDR_XXX },
	{ .opcode = "XXX", .func = NULL, .cycles = 0, .mode = ADDR_XXX },
	{ .opcode = "XXX", .func = NULL, .cycles = 0, .mode = ADDR_XXX },
	{ .opcode = "ADC", .func = NULL, .cycles = 3, .mode = ADDR_ZP },
	{ .opcode = "ROR", .func = NULL, .cycles = 5, .mode = ADDR_ZP },
	{ .opcode = "XXX", .func = NULL, .cycles = 0, .mode = ADDR_XXX },
	{ .opcode = "PLA", .func = NULL, .cycles = 4, .mode = ADDR_IMP },
	{ .opcode = "ADC", .func = NULL, .cycles = 2, .mode = ADDR_IMM },
	{ .opcode = "ROR", .func = NULL, .cycles = 2, .mode = ADDR_ACC },
	{ .opcode = "XXX", .func = NULL, .cycles = 0, .mode = ADDR_XXX },
	{ .opcode = "JMP", .func = NULL, .cycles = 5, .mode = ADDR_IND },
	{ .opcode = "ADC", .func = NULL, .cycles = 4, .mode = ADDR_ABS },
	{ .opcode = "ROR", .func = NULL, .cycles = 6, .mode = ADDR_ABS },
	{ .opcode = "XXX", .func = NULL, .cycles = 0, .mode = ADDR_XXX },

	{ .opcode = "BVS", .func = NULL, .cycles = 2, .mode = ADDR_REL }, /* 0x70 */
	{ .opcode = "ADC", .func = NULL, .cycles = 5, .mode = ADDR_INY },
	{ .opcode = "XXX", .func = NULL, .cycles = 0, .mode = ADDR_XXX },
	{ .opcode = "XXX", .func = NULL, .cycles = 0, .mode = ADDR_XXX },
	{ .opcode = "XXX", .func = NULL, .cycles = 0, .mode = ADDR_XXX },
	{ .opcode = "ADC", .func = NULL, .cycles = 4, .mode = ADDR_IZX },
	{ .opcode = "ROR", .func = NULL, .cycles = 6, .mode = ADDR_IZX },
	{ .opcode = "XXX", .func = NULL, .cycles = 0, .mode = ADDR_XXX },
	{ .opcode = "SEI", .func = NULL, .cycles = 2, .mode = ADDR_IMP },
	{ .opcode = "ADC", .func = NULL, .cycles = 4, .mode = ADDR_IAY },
	{ .opcode = "XXX", .func = NULL, .cycles = 0, .mode = ADDR_XXX },
	{ .opcode = "XXX", .func = NULL, .cycles = 0, .mode = ADDR_XXX },
	{ .opcode = "XXX", .func = NULL, .cycles = 0, .mode = ADDR_XXX },
	{ .opcode = "ADC", .func = NULL, .cycles = 4, .mode = ADDR_IAX },
	{ .opcode = "ROR", .func = NULL, .cycles = 7, .mode = ADDR_IAX },
	{ .opcode = "XXX", .func = NULL, .cycles = 0, .mode = ADDR_XXX },

	{ .opcode = "XXX", .func = NULL, .cycles = 0, .mode = ADDR_XXX }, /* 0x80 */
	{ .opcode = "STA", .func = NULL, .cycles = 6, .mode = ADDR_INX },
	{ .opcode = "XXX", .func = NULL, .cycles = 0, .mode = ADDR_XXX },
	{ .opcode = "XXX", .func = NULL, .cycles = 0, .mode = ADDR_XXX },
	{ .opcode = "STY", .func = NULL, .cycles = 3, .mode = ADDR_ZP },
	{ .opcode = "STA", .func = NULL, .cycles = 3, .mode = ADDR_ZP },
	{ .opcode = "STX", .func = NULL, .cycles = 3, .mode = ADDR_ZP },
	{ .opcode = "XXX", .func = NULL, .cycles = 0, .mode = ADDR_XXX },
	{ .opcode = "DEY", .func = NULL, .cycles = 2, .mode = ADDR_IMP },
	{ .opcode = "XXX", .func = NULL, .cycles = 0, .mode = ADDR_XXX },
	{ .opcode = "TXA", .func = NULL, .cycles = 2, .mode = ADDR_IMP },
	{ .opcode = "XXX", .func = NULL, .cycles = 0, .mode = ADDR_XXX },
	{ .opcode = "STY", .func = NULL, .cycles = 4, .mode = ADDR_ABS },
	{ .opcode = "STA", .func = NULL, .cycles = 4, .mode = ADDR_ABS },
	{ .opcode = "STX", .func = NULL, .cycles = 4, .mode = ADDR_ABS },
	{ .opcode = "XXX", .func = NULL, .cycles = 0, .mode = ADDR_XXX },

	{ .opcode = "BCC", .func = NULL, .cycles = 2, .mode = ADDR_REL }, /* 0x90 */
	{ .opcode = "STA", .func = NULL, .cycles = 6, .mode = ADDR_INY },
	{ .opcode = "XXX", .func = NULL, .cycles = 0, .mode = ADDR_XXX },
	{ .opcode = "XXX", .func = NULL, .cycles = 0, .mode = ADDR_XXX },
	{ .opcode = "STY", .func = NULL, .cycles = 4, .mode = ADDR_IZX },
	{ .opcode = "STA", .func = NULL, .cycles = 4, .mode = ADDR_IZX },
	{ .opcode = "STX", .func = NULL, .cycles = 4, .mode = ADDR_IZY },
	{ .opcode = "XXX", .func = NULL, .cycles = 0, .mode = ADDR_XXX },
	{ .opcode = "TYA", .func = NULL, .cycles = 2, .mode = ADDR_IMP },
	{ .opcode = "STA", .func = NULL, .cycles = 5, .mode = ADDR_IAY },
	{ .opcode = "TXS", .func = NULL, .cycles = 2, .mode = ADDR_IMP },
	{ .opcode = "XXX", .func = NULL, .cycles = 0, .mode = ADDR_XXX },
	{ .opcode = "XXX", .func = NULL, .cycles = 0, .mode = ADDR_XXX },
	{ .opcode = "STA", .func = NULL, .cycles = 5, .mode = ADDR_IAX },
	{ .opcode = "XXX", .func = NULL, .cycles = 0, .mode = ADDR_XXX },
	{ .opcode = "XXX", .func = NULL, .cycles = 0, .mode = ADDR_XXX },

	{ .opcode = "LDY", .func = NULL, .cycles = 2, .mode = ADDR_REL }, /* 0xA0 */
	{ .opcode = "LDA", .func = NULL, .cycles = 6, .mode = ADDR_INX },
	{ .opcode = "LDX", .func = NULL, .cycles = 2, .mode = ADDR_IMM },
	{ .opcode = "XXX", .func = NULL, .cycles = 0, .mode = ADDR_XXX },
	{ .opcode = "LDY", .func = NULL, .cycles = 3, .mode = ADDR_ZP },
	{ .opcode = "LDA", .func = NULL, .cycles = 3, .mode = ADDR_ZP },
	{ .opcode = "LDX", .func = NULL, .cycles = 3, .mode = ADDR_ZP },
	{ .opcode = "XXX", .func = NULL, .cycles = 0, .mode = ADDR_XXX },
	{ .opcode = "TAY", .func = NULL, .cycles = 2, .mode = ADDR_IMP },
	{ .opcode = "LDA", .func = NULL, .cycles = 2, .mode = ADDR_IMM },
	{ .opcode = "TAX", .func = NULL, .cycles = 2, .mode = ADDR_IMP },
	{ .opcode = "XXX", .func = NULL, .cycles = 0, .mode = ADDR_XXX },
	{ .opcode = "LDY", .func = NULL, .cycles = 4, .mode = ADDR_ABS },
	{ .opcode = "LDA", .func = NULL, .cycles = 4, .mode = ADDR_ABS },
	{ .opcode = "LDX", .func = NULL, .cycles = 4, .mode = ADDR_ABS },
	{ .opcode = "XXX", .func = NULL, .cycles = 0, .mode = ADDR_XXX },

	{ .opcode = "BSC", .func = NULL, .cycles = 2, .mode = ADDR_REL }, /* 0xB0 */
	{ .opcode = "LDA", .func = NULL, .cycles = 5, .mode = ADDR_INY },
	{ .opcode = "XXX", .func = NULL, .cycles = 0, .mode = ADDR_XXX },
	{ .opcode = "XXX", .func = NULL, .cycles = 0, .mode = ADDR_XXX },
	{ .opcode = "LDY", .func = NULL, .cycles = 4, .mode = ADDR_IZX },
	{ .opcode = "LDA", .func = NULL, .cycles = 4, .mode = ADDR_IZX },
	{ .opcode = "LDX", .func = NULL, .cycles = 4, .mode = ADDR_IZY },
	{ .opcode = "XXX", .func = NULL, .cycles = 0, .mode = ADDR_XXX },
	{ .opcode = "CLV", .func = NULL, .cycles = 2, .mode = ADDR_IMP },
	{ .opcode = "LDA", .func = NULL, .cycles = 4, .mode = ADDR_IAY },
	{ .opcode = "TSX", .func = NULL, .cycles = 2, .mode = ADDR_IMP },
	{ .opcode = "XXX", .func = NULL, .cycles = 0, .mode = ADDR_XXX },
	{ .opcode = "LDY", .func = NULL, .cycles = 4, .mode = ADDR_IAX },
	{ .opcode = "LDA", .func = NULL, .cycles = 4, .mode = ADDR_IAX },
	{ .opcode = "LDX", .func = NULL, .cycles = 4, .mode = ADDR_IAY },
	{ .opcode = "XXX", .func = NULL, .cycles = 0, .mode = ADDR_XXX },

	{ .opcode = "CPY", .func = NULL, .cycles = 2, .mode = ADDR_IMM }, /* 0xC0 */
	{ .opcode = "CMP", .func = NULL, .cycles = 6, .mode = ADDR_INX },
	{ .opcode = "XXX", .func = NULL, .cycles = 0, .mode = ADDR_XXX },
	{ .opcode = "XXX", .func = NULL, .cycles = 0, .mode = ADDR_XXX },
	{ .opcode = "CPY", .func = NULL, .cycles = 3, .mode = ADDR_ZP },
	{ .opcode = "CMP", .func = NULL, .cycles = 3, .mode = ADDR_ZP },
	{ .opcode = "DEC", .func = NULL, .cycles = 5, .mode = ADDR_ZP },
	{ .opcode = "XXX", .func = NULL, .cycles = 0, .mode = ADDR_XXX },
	{ .opcode = "INY", .func = NULL, .cycles = 2, .mode = ADDR_IMP },
	{ .opcode = "CMP", .func = NULL, .cycles = 2, .mode = ADDR_IMM },
	{ .opcode = "DEX", .func = NULL, .cycles = 2, .mode = ADDR_IMP },
	{ .opcode = "XXX", .func = NULL, .cycles = 0, .mode = ADDR_XXX },
	{ .opcode = "CPY", .func = NULL, .cycles = 4, .mode = ADDR_ABS },
	{ .opcode = "CMP", .func = NULL, .cycles = 4, .mode = ADDR_ABS },
	{ .opcode = "DEC", .func = NULL, .cycles = 6, .mode = ADDR_ABS },
	{ .opcode = "XXX", .func = NULL, .cycles = 0, .mode = ADDR_XXX },

	{ .opcode = "BNE", .func = NULL, .cycles = 2, .mode = ADDR_REL }, /* 0xD0 */
	{ .opcode = "CMP", .func = NULL, .cycles = 5, .mode = ADDR_INY },
	{ .opcode = "XXX", .func = NULL, .cycles = 0, .mode = ADDR_XXX },
	{ .opcode = "XXX", .func = NULL, .cycles = 0, .mode = ADDR_XXX },
	{ .opcode = "XXX", .func = NULL, .cycles = 0, .mode = ADDR_XXX },
	{ .opcode = "CMP", .func = NULL, .cycles = 4, .mode = ADDR_IZX },
	{ .opcode = "DEC", .func = NULL, .cycles = 6, .mode = ADDR_IZX },
	{ .opcode = "XXX", .func = NULL, .cycles = 0, .mode = ADDR_XXX },
	{ .opcode = "CLD", .func = NULL, .cycles = 2, .mode = ADDR_IMP },
	{ .opcode = "CMP", .func = NULL, .cycles = 4, .mode = ADDR_IAY },
	{ .opcode = "XXX", .func = NULL, .cycles = 0, .mode = ADDR_XXX },
	{ .opcode = "XXX", .func = NULL, .cycles = 0, .mode = ADDR_XXX },
	{ .opcode = "XXX", .func = NULL, .cycles = 0, .mode = ADDR_XXX },
	{ .opcode = "CMP", .func = NULL, .cycles = 4, .mode = ADDR_IAX },
	{ .opcode = "DEC", .func = NULL, .cycles = 7, .mode = ADDR_IAX },
	{ .opcode = "XXX", .func = NULL, .cycles = 0, .mode = ADDR_XXX },

	{ .opcode = "CPX", .func = NULL, .cycles = 2, .mode = ADDR_IMM }, /* 0xE0 */
	{ .opcode = "SBC", .func = NULL, .cycles = 6, .mode = ADDR_INX },
	{ .opcode = "XXX", .func = NULL, .cycles = 0, .mode = ADDR_XXX },
	{ .opcode = "XXX", .func = NULL, .cycles = 0, .mode = ADDR_XXX },
	{ .opcode = "CPX", .func = NULL, .cycles = 3, .mode = ADDR_ZP },
	{ .opcode = "SBC", .func = NULL, .cycles = 3, .mode = ADDR_ZP },
	{ .opcode = "INC", .func = NULL, .cycles = 5, .mode = ADDR_ZP },
	{ .opcode = "XXX", .func = NULL, .cycles = 0, .mode = ADDR_XXX  },
	{ .opcode = "INX", .func = NULL, .cycles = 2, .mode = ADDR_IMP },
	{ .opcode = "SBC", .func = NULL, .cycles = 2, .mode = ADDR_IMM },
	{ .opcode = "NOP", .func = NULL, .cycles = 2, .mode = ADDR_IMP },
	{ .opcode = "XXX", .func = NULL, .cycles = 0, .mode = ADDR_XXX },
	{ .opcode = "CPX", .func = NULL, .cycles = 4, .mode = ADDR_ABS },
	{ .opcode = "SBC", .func = NULL, .cycles = 4, .mode = ADDR_ABS },
	{ .opcode = "INC", .func = NULL, .cycles = 6, .mode = ADDR_ABS },
	{ .opcode = "XXX", .func = NULL, .cycles = 0, .mode = ADDR_XXX },

	{ .opcode = "BEO", .func = NULL, .cycles = 2, .mode = ADDR_REL }, /* 0xF0 */
	{ .opcode = "SBC", .func = NULL, .cycles = 5, .mode = ADDR_INY },
	{ .opcode = "XXX", .func = NULL, .cycles = 0, .mode = ADDR_XXX },
	{ .opcode = "XXX", .func = NULL, .cycles = 0, .mode = ADDR_XXX },
	{ .opcode = "XXX", .func = NULL, .cycles = 0, .mode = ADDR_XXX },
	{ .opcode = "SBC", .func = NULL, .cycles = 4, .mode = ADDR_IZX },
	{ .opcode = "INC", .func = NULL, .cycles = 6, .mode = ADDR_IZX },
	{ .opcode = "XXX", .func = NULL, .cycles = 0, .mode = ADDR_XXX },
	{ .opcode = "SED", .func = NULL, .cycles = 2, .mode = ADDR_IMP },
	{ .opcode = "SBC", .func = NULL, .cycles = 4, .mode = ADDR_IAY },
	{ .opcode = "XXX", .func = NULL, .cycles = 0, .mode = ADDR_XXX },
	{ .opcode = "XXX", .func = NULL, .cycles = 0, .mode = ADDR_XXX },
	{ .opcode = "XXX", .func = NULL, .cycles = 0, .mode = ADDR_XXX },
	{ .opcode = "SBC", .func = NULL, .cycles = 4, .mode = ADDR_IAX },
	{ .opcode = "INC", .func = NULL, .cycles = 7, .mode = ADDR_IAX },
	{ .opcode = "XXX", .func = NULL, .cycles = 0, .mode = ADDR_XXX }
};


static uint8_t fetch_opcode(mos6502 *, uint8_t *, uint8_t);
static uint8_t read_byte(uint8_t *, uint8_t, uint8_t);
static void OP_LDA(mos6502 *, uint8_t *, uint8_t, addr_mode);

static void
OP_LDA(mos6502 *cpu, uint8_t *mem, uint8_t cycles, addr_mode mode)
{
	uint8_t zp_addr;
	fprintf(stderr, "OP_LDA_%d\n", mode);

	if (mode == ADDR_IMM) {
		cpu->A = fetch_opcode(cpu, mem, cycles);
	}

	if (mode == ADDR_ZP) {
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
				OP_LDA(cpu, mem, cycles, ADDR_IMM);
				break;
			case 0xA5: /* INS_LDA_ZP */
				OP_LDA(cpu, mem, cycles, ADDR_ZP);
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
