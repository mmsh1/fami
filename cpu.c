#include <stdio.h>

#include "bus.h"
#include "cpu.h"

#define MASK_CARRY              0x01
#define MASK_ZERO               0x02
#define MASK_INTERRUPT_DISABLE  0x04
#define MASK_DECIMAL            0x08
#define MASK_BFLAG              0x10
#define MASK_UNUSED             0x20
#define MASK_OVERFLOW           0x40
#define MASK_NEGATIVE           0x80

#define TCPF 29781 /* total cycles per frame */

typedef uint8_t (*opcode_func)(r2A03 *);
typedef uint16_t (*addr_mode)(void);

typedef struct {
	uint8_t idx;
	const char *name;
	opcode_func func;
	uint8_t cycles;
	addr_mode mode;
} instruction;

static uint8_t read8(r2A03 *, uint16_t);
static uint16_t read16(r2A03 *, uint16_t);

static uint16_t ADDR_ACC(void); /* accumulator */
static uint16_t ADDR_IMM(void); /* immediate */
static uint16_t ADDR_ABS(void); /* absolute */
static uint16_t ADDR_ZP(void);  /* zero page */
static uint16_t ADDR_IZX(void); /* indexed zero page x */
static uint16_t ADDR_IZY(void); /* indexed zero page y */
static uint16_t ADDR_IAX(void); /* indexed absolute x */
static uint16_t ADDR_IAY(void); /* indexed absolute y */
static uint16_t ADDR_IMP(void); /* implied */
static uint16_t ADDR_REL(void); /* relative */
static uint16_t ADDR_INX(void); /* indexed indirect x */
static uint16_t ADDR_INY(void); /* indirect indexed y */
static uint16_t ADDR_IND(void); /* indirect */
static uint16_t ADDR_ILL(void); /* illegal */

static uint8_t OP_ADC(r2A03 *);
static uint8_t OP_AND(r2A03 *);
static uint8_t OP_ASL(r2A03 *);
static uint8_t OP_BCC(r2A03 *);
static uint8_t OP_BCS(r2A03 *);
static uint8_t OP_BEQ(r2A03 *);
static uint8_t OP_BIT(r2A03 *);
static uint8_t OP_BMI(r2A03 *);
static uint8_t OP_BNE(r2A03 *);
static uint8_t OP_BPL(r2A03 *);
static uint8_t OP_BRK(r2A03 *);
static uint8_t OP_BVC(r2A03 *);
static uint8_t OP_BVS(r2A03 *);
static uint8_t OP_CLC(r2A03 *);
static uint8_t OP_CLD(r2A03 *);
static uint8_t OP_CLI(r2A03 *);
static uint8_t OP_CLV(r2A03 *);
static uint8_t OP_CMP(r2A03 *);
static uint8_t OP_CPX(r2A03 *);
static uint8_t OP_CPY(r2A03 *);
static uint8_t OP_DEC(r2A03 *);
static uint8_t OP_DEX(r2A03 *);
static uint8_t OP_DEY(r2A03 *);
static uint8_t OP_EOR(r2A03 *);
static uint8_t OP_INC(r2A03 *);
static uint8_t OP_INX(r2A03 *);
static uint8_t OP_INY(r2A03 *);
static uint8_t OP_JMP(r2A03 *);
static uint8_t OP_JSR(r2A03 *);
static uint8_t OP_LDA(r2A03 *);
static uint8_t OP_LDX(r2A03 *);
static uint8_t OP_LDY(r2A03 *);
static uint8_t OP_LSR(r2A03 *);
static uint8_t OP_NOP(r2A03 *);
static uint8_t OP_ORA(r2A03 *);
static uint8_t OP_PHA(r2A03 *);
static uint8_t OP_PHP(r2A03 *);
static uint8_t OP_PLA(r2A03 *);
static uint8_t OP_PLP(r2A03 *);
static uint8_t OP_ROL(r2A03 *);
static uint8_t OP_ROR(r2A03 *);
static uint8_t OP_RTI(r2A03 *);
static uint8_t OP_RTS(r2A03 *);
static uint8_t OP_SBC(r2A03 *);
static uint8_t OP_SEC(r2A03 *);
static uint8_t OP_SED(r2A03 *);
static uint8_t OP_SEI(r2A03 *);
static uint8_t OP_STA(r2A03 *);
static uint8_t OP_STX(r2A03 *);
static uint8_t OP_STY(r2A03 *);
static uint8_t OP_TAX(r2A03 *);
static uint8_t OP_TAY(r2A03 *);
static uint8_t OP_TSX(r2A03 *);
static uint8_t OP_TXA(r2A03 *);
static uint8_t OP_TXS(r2A03 *);
static uint8_t OP_TYA(r2A03 *);

static uint8_t OP_ILL(r2A03 *); /* illegal */

instruction inst_table[0xFF + 1] = {
	{ .idx = 0x00, .name = "BRK", .func = OP_BRK, .cycles = 7, .mode = ADDR_IMP },
	{ .idx = 0x01, .name = "ORA", .func = OP_ORA, .cycles = 6, .mode = ADDR_INX },
	{ .idx = 0x02, .name = "ILL", .func = OP_ILL, .cycles = 0, .mode = ADDR_ILL },
	{ .idx = 0x03, .name = "ILL", .func = OP_ILL, .cycles = 0, .mode = ADDR_ILL },
	{ .idx = 0x04, .name = "ILL", .func = OP_ILL, .cycles = 0, .mode = ADDR_ILL },
	{ .idx = 0x05, .name = "ORA", .func = OP_ORA, .cycles = 3, .mode = ADDR_ZP },
	{ .idx = 0x06, .name = "ASL", .func = OP_ASL, .cycles = 5, .mode = ADDR_ZP },
	{ .idx = 0x07, .name = "ILL", .func = OP_ILL, .cycles = 0, .mode = ADDR_ILL },
	{ .idx = 0x08, .name = "PHP", .func = OP_PHP, .cycles = 3, .mode = ADDR_IMP },
	{ .idx = 0x09, .name = "ORA", .func = OP_ORA, .cycles = 2, .mode = ADDR_IMM },
	{ .idx = 0x0A, .name = "ASL", .func = OP_ASL, .cycles = 2, .mode = ADDR_ACC },
	{ .idx = 0x0B, .name = "ILL", .func = OP_ILL, .cycles = 0, .mode = ADDR_ILL },
	{ .idx = 0x0C, .name = "ILL", .func = OP_ILL, .cycles = 0, .mode = ADDR_ILL },
	{ .idx = 0x0D, .name = "ORA", .func = OP_ORA, .cycles = 4, .mode = ADDR_ABS },
	{ .idx = 0x0E, .name = "ASL", .func = OP_ASL, .cycles = 6, .mode = ADDR_ABS },
	{ .idx = 0x0F, .name = "ILL", .func = OP_ILL, .cycles = 0, .mode = ADDR_ILL },

	{ .idx = 0x10, .name = "BPL", .func = OP_BPL, .cycles = 2, .mode = ADDR_REL },
	{ .idx = 0x11, .name = "ORA", .func = OP_ORA, .cycles = 5, .mode = ADDR_INY },
	{ .idx = 0x12, .name = "ILL", .func = OP_ILL, .cycles = 0, .mode = ADDR_ILL },
	{ .idx = 0x13, .name = "ILL", .func = OP_ILL, .cycles = 0, .mode = ADDR_ILL },
	{ .idx = 0x14, .name = "ILL", .func = OP_ILL, .cycles = 0, .mode = ADDR_ILL },
	{ .idx = 0x15, .name = "ORA", .func = OP_ORA, .cycles = 4, .mode = ADDR_IZX },
	{ .idx = 0x16, .name = "ASL", .func = OP_ASL, .cycles = 6, .mode = ADDR_IZX },
	{ .idx = 0x17, .name = "ILL", .func = OP_ILL, .cycles = 0, .mode = ADDR_ILL },
	{ .idx = 0x18, .name = "CLC", .func = OP_CLC, .cycles = 2, .mode = ADDR_IMP },
	{ .idx = 0x19, .name = "ORA", .func = OP_ORA, .cycles = 4, .mode = ADDR_IAY },
	{ .idx = 0x1A, .name = "ILL", .func = OP_ILL, .cycles = 0, .mode = ADDR_ILL },
	{ .idx = 0x1B, .name = "ILL", .func = OP_ILL, .cycles = 0, .mode = ADDR_ILL },
	{ .idx = 0x1C, .name = "ILL", .func = OP_ILL, .cycles = 0, .mode = ADDR_ILL },
	{ .idx = 0x1D, .name = "ORA", .func = OP_ORA, .cycles = 4, .mode = ADDR_IAX },
	{ .idx = 0x1E, .name = "ASL", .func = OP_ASL, .cycles = 7, .mode = ADDR_IAX },
	{ .idx = 0x1F, .name = "ILL", .func = OP_ILL, .cycles = 0, .mode = ADDR_ILL },

	{ .idx = 0x20, .name = "JSR", .func = OP_JSR, .cycles = 6, .mode = ADDR_ABS },
	{ .idx = 0x21, .name = "AND", .func = OP_AND, .cycles = 6, .mode = ADDR_INX },
	{ .idx = 0x22, .name = "ILL", .func = OP_ILL, .cycles = 0, .mode = ADDR_ILL },
	{ .idx = 0x23, .name = "ILL", .func = OP_ILL, .cycles = 0, .mode = ADDR_ILL },
	{ .idx = 0x24, .name = "BIT", .func = OP_BIT, .cycles = 3, .mode = ADDR_ZP },
	{ .idx = 0x25, .name = "AND", .func = OP_AND, .cycles = 3, .mode = ADDR_ZP },
	{ .idx = 0x26, .name = "ROL", .func = OP_ROL, .cycles = 5, .mode = ADDR_ZP },
	{ .idx = 0x27, .name = "ILL", .func = OP_ILL, .cycles = 0, .mode = ADDR_ILL },
	{ .idx = 0x28, .name = "PLP", .func = OP_PLP, .cycles = 4, .mode = ADDR_IMP },
	{ .idx = 0x29, .name = "AND", .func = OP_AND, .cycles = 2, .mode = ADDR_IMM },
	{ .idx = 0x2A, .name = "ROL", .func = OP_ROL, .cycles = 2, .mode = ADDR_ACC },
	{ .idx = 0x2B, .name = "ILL", .func = OP_ILL, .cycles = 0, .mode = ADDR_ILL },
	{ .idx = 0x2C, .name = "BIT", .func = OP_BIT, .cycles = 4, .mode = ADDR_ABS },
	{ .idx = 0x2D, .name = "AND", .func = OP_AND, .cycles = 4, .mode = ADDR_ABS },
	{ .idx = 0x2E, .name = "ROL", .func = OP_ROL, .cycles = 6, .mode = ADDR_ABS },
	{ .idx = 0x2F, .name = "ILL", .func = OP_ILL, .cycles = 0, .mode = ADDR_ILL },

	{ .idx = 0x30, .name = "BMI", .func = OP_BMI, .cycles = 2, .mode = ADDR_REL },
	{ .idx = 0x31, .name = "AND", .func = OP_AND, .cycles = 5, .mode = ADDR_INY },
	{ .idx = 0x32, .name = "ILL", .func = OP_ILL, .cycles = 0, .mode = ADDR_ILL },
	{ .idx = 0x33, .name = "ILL", .func = OP_ILL, .cycles = 0, .mode = ADDR_ILL },
	{ .idx = 0x34, .name = "ILL", .func = OP_ILL, .cycles = 0, .mode = ADDR_ILL },
	{ .idx = 0x35, .name = "AND", .func = OP_AND, .cycles = 4, .mode = ADDR_IZX },
	{ .idx = 0x36, .name = "ROL", .func = OP_ROL, .cycles = 6, .mode = ADDR_IZX },
	{ .idx = 0x37, .name = "ILL", .func = OP_ILL, .cycles = 0, .mode = ADDR_ILL },
	{ .idx = 0x38, .name = "SEC", .func = OP_SEC, .cycles = 2, .mode = ADDR_IMP },
	{ .idx = 0x39, .name = "AND", .func = OP_AND, .cycles = 4, .mode = ADDR_IAY },
	{ .idx = 0x3A, .name = "ILL", .func = OP_ILL, .cycles = 0, .mode = ADDR_ILL },
	{ .idx = 0x3B, .name = "ILL", .func = OP_ILL, .cycles = 0, .mode = ADDR_ILL },
	{ .idx = 0x3C, .name = "ILL", .func = OP_ILL, .cycles = 0, .mode = ADDR_ILL },
	{ .idx = 0x3D, .name = "AND", .func = OP_AND, .cycles = 4, .mode = ADDR_IAX },
	{ .idx = 0x3E, .name = "ROL", .func = OP_ROL, .cycles = 7, .mode = ADDR_IAX },
	{ .idx = 0x3F, .name = "ILL", .func = OP_ILL, .cycles = 0, .mode = ADDR_ILL },

	{ .idx = 0x40, .name = "RTI", .func = OP_RTI, .cycles = 6, .mode = ADDR_IMP },
	{ .idx = 0x41, .name = "EOR", .func = OP_EOR, .cycles = 6, .mode = ADDR_INX },
	{ .idx = 0x42, .name = "ILL", .func = OP_ILL, .cycles = 0, .mode = ADDR_ILL },
	{ .idx = 0x43, .name = "ILL", .func = OP_ILL, .cycles = 0, .mode = ADDR_ILL },
	{ .idx = 0x44, .name = "ILL", .func = OP_ILL, .cycles = 0, .mode = ADDR_ILL },
	{ .idx = 0x45, .name = "EOR", .func = OP_EOR, .cycles = 3, .mode = ADDR_ZP },
	{ .idx = 0x46, .name = "LSR", .func = OP_LSR, .cycles = 5, .mode = ADDR_ZP },
	{ .idx = 0x47, .name = "ILL", .func = OP_ILL, .cycles = 0, .mode = ADDR_ILL },
	{ .idx = 0x48, .name = "PHA", .func = OP_PHA, .cycles = 3, .mode = ADDR_IMP },
	{ .idx = 0x49, .name = "EOR", .func = OP_EOR, .cycles = 2, .mode = ADDR_IMM },
	{ .idx = 0x4A, .name = "LSR", .func = OP_LSR, .cycles = 2, .mode = ADDR_ACC },
	{ .idx = 0x4B, .name = "ILL", .func = OP_ILL, .cycles = 0, .mode = ADDR_ILL },
	{ .idx = 0x4C, .name = "JMP", .func = OP_JMP, .cycles = 3, .mode = ADDR_ABS },
	{ .idx = 0x4D, .name = "EOR", .func = OP_EOR, .cycles = 4, .mode = ADDR_ABS },
	{ .idx = 0x4E, .name = "LSR", .func = OP_LSR, .cycles = 6, .mode = ADDR_ABS },
	{ .idx = 0x4F, .name = "ILL", .func = OP_ILL, .cycles = 0, .mode = ADDR_ILL },

	{ .idx = 0x50, .name = "BVC", .func = OP_BVC, .cycles = 2, .mode = ADDR_REL },
	{ .idx = 0x51, .name = "EOR", .func = OP_EOR, .cycles = 5, .mode = ADDR_INY },
	{ .idx = 0x52, .name = "ILL", .func = OP_ILL, .cycles = 0, .mode = ADDR_ILL },
	{ .idx = 0x53, .name = "ILL", .func = OP_ILL, .cycles = 0, .mode = ADDR_ILL },
	{ .idx = 0x54, .name = "ILL", .func = OP_ILL, .cycles = 0, .mode = ADDR_ILL },
	{ .idx = 0x55, .name = "EOR", .func = OP_EOR, .cycles = 4, .mode = ADDR_IZX },
	{ .idx = 0x56, .name = "LSR", .func = OP_LSR, .cycles = 6, .mode = ADDR_IZX },
	{ .idx = 0x57, .name = "ILL", .func = OP_ILL, .cycles = 0, .mode = ADDR_ILL },
	{ .idx = 0x58, .name = "CLI", .func = OP_CLI, .cycles = 2, .mode = ADDR_IMP },
	{ .idx = 0x59, .name = "EOR", .func = OP_EOR, .cycles = 4, .mode = ADDR_IAY },
	{ .idx = 0x5A, .name = "ILL", .func = OP_ILL, .cycles = 0, .mode = ADDR_ILL },
	{ .idx = 0x5B, .name = "ILL", .func = OP_ILL, .cycles = 0, .mode = ADDR_ILL },
	{ .idx = 0x5C, .name = "ILL", .func = OP_ILL, .cycles = 0, .mode = ADDR_ILL },
	{ .idx = 0x5D, .name = "EOR", .func = OP_EOR, .cycles = 4, .mode = ADDR_IAX },
	{ .idx = 0x5E, .name = "LSR", .func = OP_LSR, .cycles = 7, .mode = ADDR_IAX },
	{ .idx = 0x5F, .name = "ILL", .func = OP_ILL, .cycles = 0, .mode = ADDR_ILL },

	{ .idx = 0x60, .name = "RTS", .func = OP_RTS, .cycles = 6, .mode = ADDR_IMP },
	{ .idx = 0x61, .name = "ADC", .func = OP_ADC, .cycles = 6, .mode = ADDR_INX },
	{ .idx = 0x62, .name = "ILL", .func = OP_ILL, .cycles = 0, .mode = ADDR_ILL },
	{ .idx = 0x63, .name = "ILL", .func = OP_ILL, .cycles = 0, .mode = ADDR_ILL },
	{ .idx = 0x64, .name = "ILL", .func = OP_ILL, .cycles = 0, .mode = ADDR_ILL },
	{ .idx = 0x65, .name = "ADC", .func = OP_ADC, .cycles = 3, .mode = ADDR_ZP },
	{ .idx = 0x66, .name = "ROR", .func = OP_ROR, .cycles = 5, .mode = ADDR_ZP },
	{ .idx = 0x67, .name = "ILL", .func = OP_ILL, .cycles = 0, .mode = ADDR_ILL },
	{ .idx = 0x68, .name = "PLA", .func = OP_PLA, .cycles = 4, .mode = ADDR_IMP },
	{ .idx = 0x69, .name = "ADC", .func = OP_ADC, .cycles = 2, .mode = ADDR_IMM },
	{ .idx = 0x6A, .name = "ROR", .func = OP_ROR, .cycles = 2, .mode = ADDR_ACC },
	{ .idx = 0x6B, .name = "ILL", .func = OP_ILL, .cycles = 0, .mode = ADDR_ILL },
	{ .idx = 0x6C, .name = "JMP", .func = OP_JMP, .cycles = 5, .mode = ADDR_IND },
	{ .idx = 0x6D, .name = "ADC", .func = OP_ADC, .cycles = 4, .mode = ADDR_ABS },
	{ .idx = 0x6E, .name = "ROR", .func = OP_ROR, .cycles = 6, .mode = ADDR_ABS },
	{ .idx = 0x6F, .name = "ILL", .func = OP_ILL, .cycles = 0, .mode = ADDR_ILL },

	{ .idx = 0x70, .name = "BVS", .func = OP_BVS, .cycles = 2, .mode = ADDR_REL },
	{ .idx = 0x71, .name = "ADC", .func = OP_ADC, .cycles = 5, .mode = ADDR_INY },
	{ .idx = 0x72, .name = "ILL", .func = OP_ILL, .cycles = 0, .mode = ADDR_ILL },
	{ .idx = 0x73, .name = "ILL", .func = OP_ILL, .cycles = 0, .mode = ADDR_ILL },
	{ .idx = 0x74, .name = "ILL", .func = OP_ILL, .cycles = 0, .mode = ADDR_ILL },
	{ .idx = 0x75, .name = "ADC", .func = OP_ADC, .cycles = 4, .mode = ADDR_IZX },
	{ .idx = 0x76, .name = "ROR", .func = OP_ROR, .cycles = 6, .mode = ADDR_IZX },
	{ .idx = 0x77, .name = "ILL", .func = OP_ILL, .cycles = 0, .mode = ADDR_ILL },
	{ .idx = 0x78, .name = "SEI", .func = OP_SEI, .cycles = 2, .mode = ADDR_IMP },
	{ .idx = 0x79, .name = "ADC", .func = OP_ADC, .cycles = 4, .mode = ADDR_IAY },
	{ .idx = 0x7A, .name = "ILL", .func = OP_ILL, .cycles = 0, .mode = ADDR_ILL },
	{ .idx = 0x7B, .name = "ILL", .func = OP_ILL, .cycles = 0, .mode = ADDR_ILL },
	{ .idx = 0x7C, .name = "ILL", .func = OP_ILL, .cycles = 0, .mode = ADDR_ILL },
	{ .idx = 0x7D, .name = "ADC", .func = OP_ADC, .cycles = 4, .mode = ADDR_IAX },
	{ .idx = 0x7E, .name = "ROR", .func = OP_ROR, .cycles = 7, .mode = ADDR_IAX },
	{ .idx = 0x7F, .name = "ILL", .func = OP_ILL, .cycles = 0, .mode = ADDR_ILL },

	{ .idx = 0x80, .name = "ILL", .func = OP_ILL, .cycles = 0, .mode = ADDR_ILL },
	{ .idx = 0x81, .name = "STA", .func = OP_STA, .cycles = 6, .mode = ADDR_INX },
	{ .idx = 0x82, .name = "ILL", .func = OP_ILL, .cycles = 0, .mode = ADDR_ILL },
	{ .idx = 0x83, .name = "ILL", .func = OP_ILL, .cycles = 0, .mode = ADDR_ILL },
	{ .idx = 0x84, .name = "STY", .func = OP_STY, .cycles = 3, .mode = ADDR_ZP },
	{ .idx = 0x85, .name = "STA", .func = OP_STA, .cycles = 3, .mode = ADDR_ZP },
	{ .idx = 0x86, .name = "STX", .func = OP_STX, .cycles = 3, .mode = ADDR_ZP },
	{ .idx = 0x87, .name = "ILL", .func = OP_ILL, .cycles = 0, .mode = ADDR_ILL },
	{ .idx = 0x88, .name = "DEY", .func = OP_DEY, .cycles = 2, .mode = ADDR_IMP },
	{ .idx = 0x89, .name = "ILL", .func = OP_ILL, .cycles = 0, .mode = ADDR_ILL },
	{ .idx = 0x8A, .name = "TXA", .func = OP_TXA, .cycles = 2, .mode = ADDR_IMP },
	{ .idx = 0x8B, .name = "ILL", .func = OP_ILL, .cycles = 0, .mode = ADDR_ILL },
	{ .idx = 0x8C, .name = "STY", .func = OP_STY, .cycles = 4, .mode = ADDR_ABS },
	{ .idx = 0x8D, .name = "STA", .func = OP_STA, .cycles = 4, .mode = ADDR_ABS },
	{ .idx = 0x8E, .name = "STX", .func = OP_STX, .cycles = 4, .mode = ADDR_ABS },
	{ .idx = 0x8F, .name = "ILL", .func = OP_ILL, .cycles = 0, .mode = ADDR_ILL },

	{ .idx = 0x90, .name = "BCC", .func = OP_BCC, .cycles = 2, .mode = ADDR_REL },
	{ .idx = 0x91, .name = "STA", .func = OP_STA, .cycles = 6, .mode = ADDR_INY },
	{ .idx = 0x92, .name = "ILL", .func = OP_ILL, .cycles = 0, .mode = ADDR_ILL },
	{ .idx = 0x93, .name = "ILL", .func = OP_ILL, .cycles = 0, .mode = ADDR_ILL },
	{ .idx = 0x94, .name = "STY", .func = OP_STY, .cycles = 4, .mode = ADDR_IZX },
	{ .idx = 0x95, .name = "STA", .func = OP_STA, .cycles = 4, .mode = ADDR_IZX },
	{ .idx = 0x96, .name = "STX", .func = OP_STX, .cycles = 4, .mode = ADDR_IZY },
	{ .idx = 0x97, .name = "ILL", .func = OP_ILL, .cycles = 0, .mode = ADDR_ILL },
	{ .idx = 0x98, .name = "TYA", .func = OP_TYA, .cycles = 2, .mode = ADDR_IMP },
	{ .idx = 0x99, .name = "STA", .func = OP_STA, .cycles = 5, .mode = ADDR_IAY },
	{ .idx = 0x9A, .name = "TXS", .func = OP_TXS, .cycles = 2, .mode = ADDR_IMP },
	{ .idx = 0x9B, .name = "ILL", .func = OP_ILL, .cycles = 0, .mode = ADDR_ILL },
	{ .idx = 0x9C, .name = "ILL", .func = OP_ILL, .cycles = 0, .mode = ADDR_ILL },
	{ .idx = 0x9D, .name = "STA", .func = OP_STA, .cycles = 5, .mode = ADDR_IAX },
	{ .idx = 0x9E, .name = "ILL", .func = OP_ILL, .cycles = 0, .mode = ADDR_ILL },
	{ .idx = 0x9F, .name = "ILL", .func = OP_ILL, .cycles = 0, .mode = ADDR_ILL },

	{ .idx = 0xA0, .name = "LDY", .func = OP_LDY, .cycles = 2, .mode = ADDR_REL },
	{ .idx = 0xA1, .name = "LDA", .func = OP_LDA, .cycles = 6, .mode = ADDR_INX },
	{ .idx = 0xA2, .name = "LDX", .func = OP_LDX, .cycles = 2, .mode = ADDR_IMM },
	{ .idx = 0xA3, .name = "ILL", .func = OP_ILL, .cycles = 0, .mode = ADDR_ILL },
	{ .idx = 0xA4, .name = "LDY", .func = OP_LDY, .cycles = 3, .mode = ADDR_ZP },
	{ .idx = 0xA5, .name = "LDA", .func = OP_LDA, .cycles = 3, .mode = ADDR_ZP },
	{ .idx = 0xA6, .name = "LDX", .func = OP_LDX, .cycles = 3, .mode = ADDR_ZP },
	{ .idx = 0xA7, .name = "ILL", .func = OP_ILL, .cycles = 0, .mode = ADDR_ILL },
	{ .idx = 0xA8, .name = "TAY", .func = OP_TAY, .cycles = 2, .mode = ADDR_IMP },
	{ .idx = 0xA9, .name = "LDA", .func = OP_LDA, .cycles = 2, .mode = ADDR_IMM },
	{ .idx = 0xAA, .name = "TAX", .func = OP_TAX, .cycles = 2, .mode = ADDR_IMP },
	{ .idx = 0xAB, .name = "ILL", .func = OP_ILL, .cycles = 0, .mode = ADDR_ILL },
	{ .idx = 0xAC, .name = "LDY", .func = OP_LDY, .cycles = 4, .mode = ADDR_ABS },
	{ .idx = 0xAD, .name = "LDA", .func = OP_LDA, .cycles = 4, .mode = ADDR_ABS },
	{ .idx = 0xAE, .name = "LDX", .func = OP_LDX, .cycles = 4, .mode = ADDR_ABS },
	{ .idx = 0xAF, .name = "ILL", .func = OP_ILL, .cycles = 0, .mode = ADDR_ILL },

	{ .idx = 0xB0, .name = "BCS", .func = OP_BCS, .cycles = 2, .mode = ADDR_REL },
	{ .idx = 0xB1, .name = "LDA", .func = OP_LDA, .cycles = 5, .mode = ADDR_INY },
	{ .idx = 0xB2, .name = "ILL", .func = OP_ILL, .cycles = 0, .mode = ADDR_ILL },
	{ .idx = 0xB3, .name = "ILL", .func = OP_ILL, .cycles = 0, .mode = ADDR_ILL },
	{ .idx = 0xB4, .name = "LDY", .func = OP_LDY, .cycles = 4, .mode = ADDR_IZX },
	{ .idx = 0xB5, .name = "LDA", .func = OP_LDA, .cycles = 4, .mode = ADDR_IZX },
	{ .idx = 0xB6, .name = "LDX", .func = OP_LDX, .cycles = 4, .mode = ADDR_IZY },
	{ .idx = 0xB7, .name = "ILL", .func = OP_ILL, .cycles = 0, .mode = ADDR_ILL },
	{ .idx = 0xB8, .name = "CLV", .func = OP_CLV, .cycles = 2, .mode = ADDR_IMP },
	{ .idx = 0xB9, .name = "LDA", .func = OP_LDA, .cycles = 4, .mode = ADDR_IAY },
	{ .idx = 0xBA, .name = "TSX", .func = OP_TSX, .cycles = 2, .mode = ADDR_IMP },
	{ .idx = 0xBB, .name = "ILL", .func = OP_ILL, .cycles = 0, .mode = ADDR_ILL },
	{ .idx = 0xBC, .name = "LDY", .func = OP_LDY, .cycles = 4, .mode = ADDR_IAX },
	{ .idx = 0xBD, .name = "LDA", .func = OP_LDA, .cycles = 4, .mode = ADDR_IAX },
	{ .idx = 0xBE, .name = "LDX", .func = OP_LDX, .cycles = 4, .mode = ADDR_IAY },
	{ .idx = 0xBF, .name = "ILL", .func = OP_ILL, .cycles = 0, .mode = ADDR_ILL },

	{ .idx = 0xC0, .name = "CPY", .func = OP_CPY, .cycles = 2, .mode = ADDR_IMM },
	{ .idx = 0xC1, .name = "CMP", .func = OP_CMP, .cycles = 6, .mode = ADDR_INX },
	{ .idx = 0xC2, .name = "ILL", .func = OP_ILL, .cycles = 0, .mode = ADDR_ILL },
	{ .idx = 0xC3, .name = "ILL", .func = OP_ILL, .cycles = 0, .mode = ADDR_ILL },
	{ .idx = 0xC4, .name = "CPY", .func = OP_CPY, .cycles = 3, .mode = ADDR_ZP },
	{ .idx = 0xC5, .name = "CMP", .func = OP_CMP, .cycles = 3, .mode = ADDR_ZP },
	{ .idx = 0xC6, .name = "DEC", .func = OP_DEC, .cycles = 5, .mode = ADDR_ZP },
	{ .idx = 0xC7, .name = "ILL", .func = OP_ILL, .cycles = 0, .mode = ADDR_ILL },
	{ .idx = 0xC8, .name = "INY", .func = OP_INY, .cycles = 2, .mode = ADDR_IMP },
	{ .idx = 0xC9, .name = "CMP", .func = OP_CMP, .cycles = 2, .mode = ADDR_IMM },
	{ .idx = 0xCA, .name = "DEX", .func = OP_DEX, .cycles = 2, .mode = ADDR_IMP },
	{ .idx = 0xCB, .name = "ILL", .func = OP_ILL, .cycles = 0, .mode = ADDR_ILL },
	{ .idx = 0xCC, .name = "CPY", .func = OP_CPY, .cycles = 4, .mode = ADDR_ABS },
	{ .idx = 0xCD, .name = "CMP", .func = OP_CMP, .cycles = 4, .mode = ADDR_ABS },
	{ .idx = 0xCE, .name = "DEC", .func = OP_DEC, .cycles = 6, .mode = ADDR_ABS },
	{ .idx = 0xCF, .name = "ILL", .func = OP_ILL, .cycles = 0, .mode = ADDR_ILL },

	{ .idx = 0xD0, .name = "BNE", .func = OP_BNE, .cycles = 2, .mode = ADDR_REL },
	{ .idx = 0xD1, .name = "CMP", .func = OP_CMP, .cycles = 5, .mode = ADDR_INY },
	{ .idx = 0xD2, .name = "ILL", .func = OP_ILL, .cycles = 0, .mode = ADDR_ILL },
	{ .idx = 0xD3, .name = "ILL", .func = OP_ILL, .cycles = 0, .mode = ADDR_ILL },
	{ .idx = 0xD4, .name = "ILL", .func = OP_ILL, .cycles = 0, .mode = ADDR_ILL },
	{ .idx = 0xD5, .name = "CMP", .func = OP_CMP, .cycles = 4, .mode = ADDR_IZX },
	{ .idx = 0xD6, .name = "DEC", .func = OP_DEC, .cycles = 6, .mode = ADDR_IZX },
	{ .idx = 0xD7, .name = "ILL", .func = OP_ILL, .cycles = 0, .mode = ADDR_ILL },
	{ .idx = 0xD8, .name = "CLD", .func = OP_CLD, .cycles = 2, .mode = ADDR_IMP },
	{ .idx = 0xD9, .name = "CMP", .func = OP_CMP, .cycles = 4, .mode = ADDR_IAY },
	{ .idx = 0xDA, .name = "ILL", .func = OP_ILL, .cycles = 0, .mode = ADDR_ILL },
	{ .idx = 0xDB, .name = "ILL", .func = OP_ILL, .cycles = 0, .mode = ADDR_ILL },
	{ .idx = 0xDC, .name = "ILL", .func = OP_ILL, .cycles = 0, .mode = ADDR_ILL },
	{ .idx = 0xDD, .name = "CMP", .func = OP_CMP, .cycles = 4, .mode = ADDR_IAX },
	{ .idx = 0xDE, .name = "DEC", .func = OP_DEC, .cycles = 7, .mode = ADDR_IAX },
	{ .idx = 0xDF, .name = "ILL", .func = OP_ILL, .cycles = 0, .mode = ADDR_ILL },

	{ .idx = 0xE0, .name = "CPX", .func = OP_CPX, .cycles = 2, .mode = ADDR_IMM },
	{ .idx = 0xE1, .name = "SBC", .func = OP_SBC, .cycles = 6, .mode = ADDR_INX },
	{ .idx = 0xE2, .name = "ILL", .func = OP_ILL, .cycles = 0, .mode = ADDR_ILL },
	{ .idx = 0xE3, .name = "ILL", .func = OP_ILL, .cycles = 0, .mode = ADDR_ILL },
	{ .idx = 0xE4, .name = "CPX", .func = OP_CPX, .cycles = 3, .mode = ADDR_ZP },
	{ .idx = 0xE5, .name = "SBC", .func = OP_SBC, .cycles = 3, .mode = ADDR_ZP },
	{ .idx = 0xE6, .name = "INC", .func = OP_INC, .cycles = 5, .mode = ADDR_ZP },
	{ .idx = 0xE7, .name = "ILL", .func = OP_ILL, .cycles = 0, .mode = ADDR_ILL  },
	{ .idx = 0xE8, .name = "INX", .func = OP_INX, .cycles = 2, .mode = ADDR_IMP },
	{ .idx = 0xE9, .name = "SBC", .func = OP_SBC, .cycles = 2, .mode = ADDR_IMM },
	{ .idx = 0xEA, .name = "NOP", .func = OP_NOP, .cycles = 2, .mode = ADDR_IMP },
	{ .idx = 0xEB, .name = "ILL", .func = OP_ILL, .cycles = 0, .mode = ADDR_ILL },
	{ .idx = 0xEC, .name = "CPX", .func = OP_CPX, .cycles = 4, .mode = ADDR_ABS },
	{ .idx = 0xED, .name = "SBC", .func = OP_SBC, .cycles = 4, .mode = ADDR_ABS },
	{ .idx = 0xEE, .name = "INC", .func = OP_INC, .cycles = 6, .mode = ADDR_ABS },
	{ .idx = 0xEF, .name = "ILL", .func = OP_ILL, .cycles = 0, .mode = ADDR_ILL },

	{ .idx = 0xF0, .name = "BEQ", .func = OP_BEQ, .cycles = 2, .mode = ADDR_REL },
	{ .idx = 0xF1, .name = "SBC", .func = OP_SBC, .cycles = 5, .mode = ADDR_INY },
	{ .idx = 0xF2, .name = "ILL", .func = OP_ILL, .cycles = 0, .mode = ADDR_ILL },
	{ .idx = 0xF3, .name = "ILL", .func = OP_ILL, .cycles = 0, .mode = ADDR_ILL },
	{ .idx = 0xF4, .name = "ILL", .func = OP_ILL, .cycles = 0, .mode = ADDR_ILL },
	{ .idx = 0xF5, .name = "SBC", .func = OP_SBC, .cycles = 4, .mode = ADDR_IZX },
	{ .idx = 0xF6, .name = "INC", .func = OP_INC, .cycles = 6, .mode = ADDR_IZX },
	{ .idx = 0xF7, .name = "ILL", .func = OP_ILL, .cycles = 0, .mode = ADDR_ILL },
	{ .idx = 0xF8, .name = "SED", .func = OP_SED, .cycles = 2, .mode = ADDR_IMP },
	{ .idx = 0xF9, .name = "SBC", .func = OP_SBC, .cycles = 4, .mode = ADDR_IAY },
	{ .idx = 0xFA, .name = "ILL", .func = OP_ILL, .cycles = 0, .mode = ADDR_ILL },
	{ .idx = 0xFB, .name = "ILL", .func = OP_ILL, .cycles = 0, .mode = ADDR_ILL },
	{ .idx = 0xFC, .name = "ILL", .func = OP_ILL, .cycles = 0, .mode = ADDR_ILL },
	{ .idx = 0xFD, .name = "SBC", .func = OP_SBC, .cycles = 4, .mode = ADDR_IAX },
	{ .idx = 0xFE, .name = "INC", .func = OP_INC, .cycles = 7, .mode = ADDR_IAX },
	{ .idx = 0xFF, .name = "ILL", .func = OP_ILL, .cycles = 0, .mode = ADDR_ILL }
};

static uint16_t
ADDR_ACC(void)
{
}

static uint16_t
ADDR_IMM(void)
{
}

static uint16_t
ADDR_ABS(void)
{
}

static uint16_t
ADDR_ZP(void)
{
}

static uint16_t
ADDR_IZX(void)
{
}

static uint16_t
ADDR_IZY(void)
{
}

static uint16_t
ADDR_IAX(void)
{
}

static uint16_t
ADDR_IAY(void)
{
}

static uint16_t
ADDR_IMP(void)
{
}

static uint16_t
ADDR_REL(void)
{
}

static uint16_t
ADDR_INX(void)
{
}

static uint16_t
ADDR_INY(void)
{
}

static uint16_t
ADDR_IND(void)
{
}

static uint16_t
ADDR_ILL(void)
{
}

static uint8_t
OP_ADC(r2A03 *cpu)
{
}

static uint8_t
OP_AND(r2A03 *cpu)
{
}

static uint8_t
OP_ASL(r2A03 *cpu)
{
}

static uint8_t
OP_BCC(r2A03 *cpu)
{
}

static uint8_t
OP_BCS(r2A03 *cpu)
{
}

static uint8_t
OP_BEQ(r2A03 *cpu)
{
}

static uint8_t
OP_BIT(r2A03 *cpu)
{
}

static uint8_t
OP_BMI(r2A03 *cpu)
{
}

static uint8_t
OP_BNE(r2A03 *cpu)
{
}

static uint8_t
OP_BPL(r2A03 *cpu)
{
}

static uint8_t
OP_BRK(r2A03 *cpu)
{
}

static uint8_t
OP_BVC(r2A03 *cpu)
{
}

static uint8_t
OP_BVS(r2A03 *cpu)
{
}

static uint8_t
OP_CLC(r2A03 *cpu)
{
}

static uint8_t
OP_CLD(r2A03 *cpu)
{
}

static uint8_t
OP_CLI(r2A03 *cpu)
{
}

static uint8_t
OP_CLV(r2A03 *cpu)
{
}

static uint8_t
OP_CMP(r2A03 *cpu)
{
}

static uint8_t
OP_CPX(r2A03 *cpu)
{
}

static uint8_t
OP_CPY(r2A03 *cpu)
{
}

static uint8_t
OP_DEC(r2A03 *cpu)
{
}

static uint8_t
OP_DEX(r2A03 *cpu)
{
}

static uint8_t
OP_DEY(r2A03 *cpu)
{
}

static uint8_t
OP_EOR(r2A03 *cpu)
{
}

static uint8_t
OP_INC(r2A03 *cpu)
{
}

static uint8_t
OP_INX(r2A03 *cpu)
{
}

static uint8_t
OP_INY(r2A03 *cpu)
{
}

static uint8_t
OP_JMP(r2A03 *cpu)
{
}

static uint8_t
OP_JSR(r2A03 *cpu)
{
}

static uint8_t
OP_LDA(r2A03 *cpu)
{
}

static uint8_t
OP_LDX(r2A03 *cpu)
{
}

static uint8_t
OP_LDY(r2A03 *cpu)
{
}

static uint8_t
OP_LSR(r2A03 *cpu)
{
}

static uint8_t
OP_NOP(r2A03 *cpu)
{
}

static uint8_t
OP_ORA(r2A03 *cpu)
{
}

static uint8_t
OP_PHA(r2A03 *cpu)
{
}

static uint8_t
OP_PHP(r2A03 *cpu)
{
}

static uint8_t
OP_PLA(r2A03 *cpu)
{
}

static uint8_t
OP_PLP(r2A03 *cpu)
{
}

static uint8_t
OP_ROL(r2A03 *cpu)
{
}

static uint8_t
OP_ROR(r2A03 *cpu)
{
}

static uint8_t
OP_RTI(r2A03 *cpu)
{
}

static uint8_t
OP_RTS(r2A03 *cpu)
{
}

static uint8_t
OP_SBC(r2A03 *cpu)
{
}

static uint8_t
OP_SEC(r2A03 *cpu)
{
}

static uint8_t
OP_SED(r2A03 *cpu)
{
}

static uint8_t
OP_SEI(r2A03 *cpu)
{
}

static uint8_t
OP_STA(r2A03 *cpu)
{
}

static uint8_t
OP_STX(r2A03 *cpu)
{
}

static uint8_t
OP_STY(r2A03 *cpu)
{
}

static uint8_t
OP_TAX(r2A03 *cpu)
{
}

static uint8_t
OP_TAY(r2A03 *cpu)
{
}

static uint8_t
OP_TSX(r2A03 *cpu)
{
}

static uint8_t
OP_TXA(r2A03 *cpu)
{
}

static uint8_t
OP_TXS(r2A03 *cpu)
{
}

static uint8_t
OP_TYA(r2A03 *cpu)
{
}

static uint8_t
OP_ILL(r2A03 *cpu)
{
}

static uint8_t
read8(r2A03 *cpu, uint16_t address)
{
	cpu_tick(cpu);
	return bus_ram_read(cpu->bus, address);
}

static uint16_t
read16(r2A03 *cpu, uint16_t address)
{
	return read8(cpu, address) | (read8(cpu, address + 1) << 8);
}

void
cpu_exec(r2A03 *cpu, uint8_t cycles)
{
	uint8_t opcode;

	cpu->total_cycles += TCPF;

	while (cycles > 0) {

		opcode = read8(cpu, cpu->PC);
		fprintf(stderr, "idx: %d, ", inst_table[opcode].idx);	/* TODO remove later */
		fprintf(stderr, "name: %s\n", inst_table[opcode].name);	/* TODO remove later */
		cpu->PC++;

		inst_table[opcode].mode();
		inst_table[opcode].func(cpu);
	}
}

uint8_t
cpu_getflag(r2A03 *cpu, uint8_t bit)
{
	return (cpu->P) & bit;
}

void
cpu_setflag(r2A03 *cpu, uint8_t bit, uint8_t val)
{
	(cpu->P) ^= (-val ^ cpu->P) & bit;
}

void
cpu_reset(r2A03 *cpu)
{
	cpu->PC = 0xFFFC;
	cpu->SP = 0x00;
	cpu->P = 0;
	cpu->A = 0;
	cpu->X = 0;
	cpu->Y = 0;
}

void
cpu_tick(r2A03 *cpu)
{
	cpu->total_cycles--;
}
