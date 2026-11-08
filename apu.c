#include "apu.h"

static uint8_t
length_table[32] = {
	10, 254, 20, 2, 40, 4, 80, 6,
	160, 8, 60, 10, 14, 12, 26, 14,
	12, 16, 24, 18, 48, 20, 96, 22,
	192, 24, 72, 26, 16, 28, 32, 30
};

static void
length_clock(apu_length_counter *length)
{
	if (!length->halt && length->value > 0) {
		length->value--;
	}
}

static void
length_load(apu_length_counter *length, uint8_t value)
{
	if (length->enabled) {
		length->value = length_table[value >> 3];
	}
}

static void
quarter_frame_clock(apu *apu)
{
	(void)apu;
}

static void
half_frame_clock(apu *apu)
{
	length_clock(&apu->pulse[0]);
	length_clock(&apu->pulse[1]);
	length_clock(&apu->triangle);
	length_clock(&apu->noise);
}

static void
frame_counter_tick(apu *apu)
{
	apu->frame_cycle++;

	if (apu->five_step_mode) {
		switch (apu->frame_cycle) {
			case 7457:
			case 22371:
				quarter_frame_clock(apu);
				break;
			case 14913:
			case 37281:
				quarter_frame_clock(apu);
				half_frame_clock(apu);
				break;
			case 37282:
				apu->frame_cycle = 0;
				break;
			default:
				break;
		}
		return;
	}
	
	switch (apu->frame_cycle) {
		case 7457:
		case 22371:
			quarter_frame_clock(apu);
			break;
		case 14913:
			quarter_frame_clock(apu);
			half_frame_clock(apu);
			break;
		case 29828:
			if (!apu->frame_irq_inhibit) {
				apu->frame_irq = 1;
			}
			break;
		case 29829:
			quarter_frame_clock(apu);
			half_frame_clock(apu);
			if (!apu->frame_irq_inhibit) {
				apu->frame_irq = 1;
			}
			break;
		case 29830:
			if (!apu->frame_irq_inhibit) {
				apu->frame_irq = 1;
			}
			apu->frame_cycle = 0;
			break;
		default:
			break;
	}
}

void
apu_reset(apu *apu, struct bus *bus)
{
	apu->bus = bus;
}

void
apu_tick(apu *apu)
{
	frame_counter_tick(apu);
}

uint8_t
apu_read(apu *apu, uint16_t addr)
{
	uint8_t status = 0;
	
	if (addr != 0x4015) { return 0; }
	if (apu->pulse[0].value > 0) { status |= 0x01; }
	if (apu->pulse[1].value > 0) { status |= 0x02; }
	if (apu->triangle.value > 0) { status |= 0x04; }
	if (apu->noise.value > 0) { status |= 0x08; }
	if (apu->frame_irq) { status |= 0x40; }
	
	apu->frame_irq = 0;
	return status;
}

void
apu_write(apu *apu, uint16_t addr, uint8_t val)
{
	uint8_t enabled;
	unsigned int i;
	
	switch (addr) {
		case 0x4003:
			length_load(&apu->pulse[0], val);
			break;
		case 0x4007:
			length_load(&apu->pulse[1], val);
			break;
		case 0x400B:
			length_load(&apu->triangle, val);
			break;
		case 0x400F:
			length_load(&apu->noise, val);
			break;
		case 0x4015:
			for (i = 0; i < 2; i++) {
				enabled = (val >> i) & 1;
				apu->pulse[i].enabled = enabled;
				if (!enabled) {
					apu->pulse[i].value = 0;
				}
			}
			apu->triangle.enabled = (val & 0x04) != 0;
			if (!apu->triangle.enabled) {
				apu->triangle.value = 0;
			}
			apu->noise.enabled = (val & 0x08) != 0;
			if (!apu->noise.enabled) {
				apu->noise.value = 0;
			}
			break;
		case 0x4017:
			apu->five_step_mode = (val & 0x80) != 0;
			apu->frame_irq_inhibit = (val & 0x40) != 0;
			if (apu->frame_irq_inhibit) {
				apu->frame_irq = 0;
			}
			apu->frame_cycle = 0;
			if (apu->five_step_mode) {
				quarter_frame_clock(apu);
				half_frame_clock(apu);
			}
			break;
		default:
			break;
	}
}
