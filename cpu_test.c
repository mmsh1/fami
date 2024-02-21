#include <criterion/criterion.h>
#include <criterion/new/assert.h>

#include "cpu.c"

static uint8_t dummy_ram[RAM_SIZE];

/* mock real bus_ram_write */
void
bus_ram_write(struct bus *bus, uint16_t addr, uint8_t val)
{
	dummy_ram[addr] = val;
}

/* mock real bus_ram_read */
uint8_t
bus_ram_read(struct bus *bus, uint16_t addr)
{
	return dummy_ram[addr];
}


static void
load_dummy_rom(struct bus *bus, uint8_t *rom, int romsize)
{
	int i;
	for (i = 0; i < romsize; i++) {
		bus_ram_write(bus, 0x8000 + i, rom[i]);
	}
}

static void
write_dummy_reset(struct bus *bus, uint16_t addr)
{
	bus_ram_write(bus, VECTOR_RESET, addr & 0x00FF);
	bus_ram_write(bus, VECTOR_RESET + 1, addr >> 8);
}

Test(cpu, reset) {
	r2A03 cpu = {0};
	struct bus bus = {0};

	write_dummy_reset(&bus, 0xFEFC);

	cpu_reset(&cpu, &bus);
	cr_assert(eq(ptr, cpu.bus, &bus));
	cr_assert(eq(u16, cpu.PC, 0xFEFC));
	cr_assert(eq(u8, cpu.SP, 0xFD));
	cr_assert(eq(u8, cpu.P, 0));
	cr_assert(eq(u8, cpu.A, 0));
	cr_assert(eq(u8, cpu.X, 0));
	cr_assert(eq(u8, cpu.Y, 0));
}

Test(cpu, push8) {
}

Test(cpu, push16) {
}

Test(cpu, pop8) {
}

Test(cpu, pop16) {
}

Test(cpu, jsr) {
	r2A03 cpu = {0};
	uint8_t dummy_rom[] = {0x20, 0xF0, 0xFF}; /* 0x20 - JSR ABS */

	load_dummy_rom(cpu.bus, dummy_rom, 3);
	bus_ram_write(cpu.bus, 0xF0, 0x55);
	write_dummy_reset(cpu.bus, 0x8000);

	cpu_reset(&cpu, cpu.bus);
	cpu_tick(&cpu);
	cr_assert(eq(u16, cpu.PC, 0xFFF0));
	cr_assert(eq(u16, pop16(&cpu), 0x8002)); /* 0x8000 + 1 (when read8) + 1 (when pushing in JSR) */
}

Test(cpu, lda_zpg) {
	r2A03 cpu = {0};
	uint8_t dummy_rom[] = {0xA5, 0x10, 0x00}; /* 0xA5 - LDA ZPG */

	load_dummy_rom(cpu.bus, dummy_rom, 3);
	bus_ram_write(cpu.bus, 0x10, 0x55);
	write_dummy_reset(cpu.bus, 0x8000);

	cpu_reset(&cpu, cpu.bus);
	cpu_tick(&cpu);
	cr_assert(eq(u8, cpu.A, 0x55));
}

Test(cpu, sty_zpg) {
	r2A03 cpu = {0};
	uint8_t dummy_rom[] = {0x84, 0x10, 0x00}; /* 0x84 - STY ZPG */

	load_dummy_rom(cpu.bus, dummy_rom, 3);
	write_dummy_reset(cpu.bus, 0x8000);
	
	cpu_reset(&cpu, cpu.bus);
	cpu.Y = 42;
	cpu_tick(&cpu);
	cr_assert(eq(u8, bus_ram_read(cpu.bus, 0x10), cpu.Y));
}

Test(cpu, sta_zpg) {
	r2A03 cpu = {0};
	uint8_t dummy_rom[] = {0x85, 0x10, 0x00}; /* 0x85 - STA ZPG */

	load_dummy_rom(cpu.bus, dummy_rom, 3);
	write_dummy_reset(cpu.bus, 0x8000);

	cpu_reset(&cpu, cpu.bus);
	cpu.A = 42;
	cpu_tick(&cpu);
	cr_assert(eq(u8, bus_ram_read(cpu.bus, 0x10), cpu.A));
}

Test(cpu, stx_zpg) {
	r2A03 cpu = {0};
	uint8_t dummy_rom[] = {0x86, 0x10, 0x00}; /* 0x86 - STX ZPG */

	load_dummy_rom(cpu.bus, dummy_rom, 3);
	write_dummy_reset(cpu.bus, 0x8000);
	
	cpu_reset(&cpu, cpu.bus);
	cpu.X = 42;
	cpu_tick(&cpu);
	cr_assert(eq(u8, bus_ram_read(cpu.bus, 0x10), cpu.X));
}
