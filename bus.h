#ifndef NES_BUS_H
#define NES_BUS_H

#include "apu.h"
#include "cartridge.h"
#include "controller.h"
#include "cpu.h"
#include "ppu.h"

typedef struct bus {
	apu *apu;
	cpu *cpu;
	ppu *ppu;
	uint8_t *ram;
	controller *controller;
	cartridge *rom;
} bus;

void bus_init(bus *, apu *, cpu *, ppu *, uint8_t *, controller *, cartridge *);

void bus_apu_reset(bus *);
void bus_apu_tick(bus *);

uint8_t bus_cartridge_get_mirroring(bus *);
uint8_t bus_cartridge_read(bus *, uint16_t );
void bus_cartridge_write(bus *, uint16_t, uint8_t);

uint64_t bus_cpu_get_total_cycles(bus *);
void bus_cpu_reset(bus *);
void bus_cpu_set_irq(bus *, uint8_t);
void bus_cpu_set_stall_cycles(bus *, uint64_t);
void bus_cpu_tick(bus *);
void bus_cpu_trigger_nmi(bus *);

int bus_ppu_get_frame_ready_flag(bus *);
void bus_ppu_unset_frame_ready_flag(bus *);
void bus_ppu_reset(bus *);
void bus_ppu_tick(bus *);

uint8_t bus_read(bus *, uint16_t);
void bus_write(bus *, uint16_t, uint8_t);

#endif /* NES_BUS_H */
