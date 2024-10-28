#ifndef NES_BUS_H
#define NES_BUS_H

#include <stdint.h>

#include "cartrige.h"
#include "cpu.h"
#include "mem.h"
#include "ppu.h"

typedef struct bus {
	r2A03 *cpu;
	r2C02 *ppu;
	uint8_t *ram;
	cartrige rom; /* TODO use pointer? */
} bus;

void bus_init(bus *, r2A03 *, uint8_t *, cartrige);

void bus_apu_reset(bus *);
void bus_apu_tick(bus *);

void bus_cpu_reset(bus *);
void bus_cpu_tick(bus *);

void bus_ppu_reset(bus *);
void bus_ppu_tick(bus *);

void bus_ram_reset(bus *);

uint8_t bus_read(bus *, uint16_t);
void bus_write(bus *, uint16_t, uint8_t);

#endif /* NES_BUS_H */
