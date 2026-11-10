#include "apu.h"
#include "bus.h"

enum {
	CPU_FREQ = 1789773
};

static uint8_t
length_table[32] = {
	10, 254, 20, 2, 40, 4, 80, 6,
	160, 8, 60, 10, 14, 12, 26, 14,
	12, 16, 24, 18, 48, 20, 96, 22,
	192, 24, 72, 26, 16, 28, 32, 30
};

static const uint16_t
noise_period_table[16] = {
	4, 8, 16, 32, 64, 96, 128, 160,
	202, 254, 380, 508, 762, 1016, 2034, 4068
};

static const uint8_t
pulse_duty_table[4][8] = {
	{ 0, 1, 0, 0, 0, 0, 0, 0 },
	{ 0, 1, 1, 0, 0, 0, 0, 0 },
	{ 0, 1, 1, 1, 1, 0, 0, 0 },
	{ 1, 0, 0, 1, 1, 1, 1, 1 }
};

static int16_t apu_mix(apu *);
static void apu_sample_tick(apu *);
static void apu_update_irq(apu *);
static void apu_write_frame_counter(apu *, uint8_t);
static void apu_write_status(apu *, uint8_t);
static void envelope_clock(apu_envelope *, apu_length_counter *);
static uint8_t envelope_output(const apu_envelope *);
static void envelope_write(apu_envelope *, apu_length_counter *, uint8_t);
static void frame_counter_tick(apu *);
static void frame_write_tick(apu *);
static void half_frame_clock(apu *);
static void length_clock(apu_length_counter *);
static void length_load(apu_length_counter *, uint8_t);
static void length_set_enabled(apu_length_counter *, uint8_t);
static uint8_t noise_output(const apu_noise *);
static void noise_tick(apu_noise *);
static void period_write_hi(uint16_t *, uint8_t);
static void period_write_lo(uint16_t *, uint8_t);
static uint8_t pulse_is_silent(const apu_pulse *, unsigned int);
static uint8_t pulse_output(const apu_pulse *, unsigned int);
static void pulse_sweep_clock(apu_pulse *, unsigned int);
static int32_t pulse_target_period(const apu_pulse *, unsigned int);
static void pulse_tick(apu_pulse *);
static void quarter_frame_clock(apu *);
static int timer_tick(uint16_t *, uint16_t);
static void triangle_linear_clock(apu_triangle *);
static uint8_t triangle_output(const apu_triangle *);
static void triangle_tick(apu_triangle *);

static int16_t
apu_mix(apu *apu)
{
	uint16_t pulse_sum;
	uint8_t triangle;
	uint8_t noise;
	double pulse_level = 0.0;
	double tnd_level = 0.0;

	pulse_sum = (uint16_t)(pulse_output(&apu->pulse[0], 0) + pulse_output(&apu->pulse[1], 1));
	triangle = triangle_output(&apu->triangle);
	noise = noise_output(&apu->noise);

	if (pulse_sum > 0) {
		pulse_level = 95.88 / (8128.0 / (double)pulse_sum + 100.0);
	}

	if (triangle > 0 || noise > 0) {
		tnd_level = 159.79 / (1.0 / ((double)triangle / 8227.0 + (double)noise / 12241.0) + 100.0);
	}

	return (int16_t)((pulse_level + tnd_level) * 32767.0);
}

static void
apu_sample_tick(apu *apu)
{
	apu->sample_fraction += APU_SAMPLE_RATE;
	if (apu->sample_fraction < CPU_FREQ) {
		return;
	}
	apu->sample_fraction -= CPU_FREQ;
	if (apu->audio_buffer_length < APU_AUDIO_BUFFER_SIZE) {
		apu->audio_buffer[apu->audio_buffer_length] = apu_mix(apu);
		apu->audio_buffer_length++;
	}
}

static void
apu_update_irq(apu *apu)
{
	bus_cpu_set_irq(apu->bus, apu->frame_irq);
}

static void
apu_write_frame_counter(apu *apu, uint8_t val)
{
	apu->frame_write_value = val;
	apu->frame_write_delay = (uint8_t)(3 + (apu->cycle & 1));
	apu->frame_write_pending = 1;
	apu->frame_irq_inhibit = (val & 0x40) != 0;

	if (apu->frame_irq_inhibit) {
		apu->frame_irq = 0;
	}

	apu_update_irq(apu);
}

static void
apu_write_status(apu *apu, uint8_t val)
{
	unsigned int i;
	for (i = 0; i < 2; i++) {
		length_set_enabled(&apu->pulse[i].length, (val >> i) & 1);
	}
	length_set_enabled(&apu->triangle.length, (val & 0x04) != 0);
	length_set_enabled(&apu->noise.length, (val & 0x08) != 0);
}

static void
envelope_clock(apu_envelope *envelope, apu_length_counter *length)
{
	if (envelope->start) {
		envelope->start = 0;
		envelope->decay = 15;
		envelope->divider = envelope->volume;
		return;
	}

	if (envelope->divider > 0) {
		envelope->divider--;
		return;
	}

	envelope->divider = envelope->volume;

	if (envelope->decay > 0) {
		envelope->decay--;
	} else if (length->halt) {
		envelope->decay = 15;
	}
}

static uint8_t
envelope_output(const apu_envelope *envelope)
{
	if (envelope->constant_volume) {
		return envelope->volume;
	}

	return envelope->decay;
}

static void
envelope_write(apu_envelope *envelope, apu_length_counter *length, uint8_t val)
{
	length->halt = (val & 0x20) != 0;
	envelope->constant_volume = (val & 0x10) != 0;
	envelope->volume = val & 0x0F;
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

static void
frame_write_tick(apu *apu)
{
	apu->frame_write_delay--;

	if (apu->frame_write_delay > 0) {
		frame_counter_tick(apu);
		return;
	}

	apu->frame_write_pending = 0;
	apu->frame_cycle = 0;
	apu->five_step_mode = (apu->frame_write_value & 0x80) != 0;

	if (apu->five_step_mode) {
		quarter_frame_clock(apu);
		half_frame_clock(apu);
	}
}

static void
half_frame_clock(apu *apu)
{
	length_clock(&apu->pulse[0].length);
	length_clock(&apu->pulse[1].length);
	length_clock(&apu->triangle.length);
	length_clock(&apu->noise.length);

	pulse_sweep_clock(&apu->pulse[0], 0);
	pulse_sweep_clock(&apu->pulse[1], 1);
}

static void
length_clock(apu_length_counter *length)
{
	length->reload_inhibit = length->value > 0;

	if (!length->halt && length->value > 0) {
		length->value--;
	}
}

static void
length_load(apu_length_counter *length, uint8_t value)
{
	if (length->enabled && !length->reload_inhibit) {
		length->value = length_table[value >> 3];
	}
}

static void
length_set_enabled(apu_length_counter *length, uint8_t enabled)
{
	length->enabled = enabled;
	if (!enabled) {
		length->value = 0;
	}
}

static uint8_t
noise_output(const apu_noise *noise)
{
	if (noise->length.value == 0 || (noise->lfsr & 1)) {
		return 0;
	}

	return envelope_output(&noise->envelope);
}

static void
noise_tick(apu_noise *noise)
{
	uint16_t reload = noise->period > 0 ? noise->period - 1 : 0;

	if (!timer_tick(&noise->timer, reload)) {
		return;
	}
}

static void
period_write_hi(uint16_t *period, uint8_t val)
{
	*period = (uint16_t)((*period & 0x00FF) | ((uint16_t)(val & 0x07) << 8));
}

static void
period_write_lo(uint16_t *period, uint8_t val)
{
	*period = (uint16_t)((*period & 0x0700) | val);
}

static uint8_t
pulse_is_silent(const apu_pulse *pulse, unsigned int channel)
{
	return pulse->period < 8 || pulse_target_period(pulse, channel) > 0x7FF;
}

static uint8_t
pulse_output(const apu_pulse *pulse, unsigned int channel)
{
	/* TODO: create wrapper func */
	if (pulse->length.value == 0
		|| pulse_is_silent(pulse, channel)
		|| !pulse_duty_table[pulse->duty][pulse->step]) {
		return 0;
	}

	return envelope_output(&pulse->envelope);
}

static int32_t
pulse_target_period(const apu_pulse *pulse, unsigned int channel)
{
	int32_t target = pulse->period;
	int32_t change = (int32_t)(pulse->period >> pulse->sweep.shift);

	if (pulse->sweep.negate) {
		target -= change;
		if (channel == 0) {
			target--;
		}
	} else {
		target += change;
	}

	return target;
}

static void
pulse_tick(apu_pulse *pulse)
{
	if (!timer_tick(&pulse->timer, pulse->period)) {
		return;
	}
	pulse->step = (uint8_t)((pulse->step + 1) & 7);
}

static void
pulse_sweep_clock(apu_pulse *pulse, unsigned int channel)
{
	int32_t target = pulse_target_period(pulse, channel);

	if (pulse->sweep.divider == 0
		&& pulse->sweep.enabled
		&& pulse->sweep.shift != 0
		&& !pulse_is_silent(pulse, channel)) {
		pulse->period = (uint16_t)target;
	}

	if (pulse->sweep.divider == 0 || pulse->sweep.reload) {
		pulse->sweep.divider = pulse->sweep.period;
		pulse->sweep.reload = 0;
	} else {
		pulse->sweep.divider--;
	}
}

static void
pulse_write(apu_pulse *pulse, uint16_t addr, uint8_t val)
{
	switch (addr & 0x03) {
		case 0:
			pulse->duty = val >> 6;
			envelope_write(&pulse->envelope, &pulse->length, val);
			break;
		case 1:
			pulse->sweep.enabled = (val & 0x80) != 0;
			pulse->sweep.period = (val >> 4) & 0x07;
			pulse->sweep.negate = (val & 0x08) != 0;
			pulse->sweep.shift = val & 0x07;
			pulse->sweep.reload = 1;
			break;
		case 2:
			period_write_lo(&pulse->period, val);
			break;
		case 3:
			period_write_hi(&pulse->period, val);
			pulse->step = 0;
			pulse->envelope.start = 1;
			length_load(&pulse->length, val);
			break;
	}
}

static void
quarter_frame_clock(apu *apu)
{
	envelope_clock(&apu->pulse[0].envelope, &apu->pulse[0].length);
	envelope_clock(&apu->pulse[1].envelope, &apu->pulse[1].length);
	envelope_clock(&apu->noise.envelope, &apu->noise.length);
	triangle_linear_clock(&apu->triangle);
}

static int
timer_tick(uint16_t *timer, uint16_t reload)
{
	if (*timer > 0) {
		(*timer)--;
		return 0;
	}
	*timer = reload;
	return 1;
}

static void
triangle_linear_clock(apu_triangle *triangle)
{
	if (triangle->linear_reload) {
		triangle->linear_value = triangle->linear_reload_value;
	} else if (triangle->linear_value > 0) {
		triangle->linear_value--;
	}

	if (!triangle->length.halt) {
		triangle->linear_reload = 0;
	}
}

static uint8_t
triangle_output(const apu_triangle *triangle)
{
	if (triangle->step < 16) {
		return (uint8_t)(15 - triangle->step);
	}

	return (uint8_t)(triangle->step - 16);
}

static void
triangle_tick(apu_triangle *triangle)
{
	if (!timer_tick(&triangle->timer, triangle->period)) {
		return;
	}
	if (triangle->length.value > 0 && triangle->linear_value > 0) {
		triangle->step = (uint8_t)((triangle->step + 1) & 31);
	}
}

void
apu_reset(apu *apu, struct bus *bus)
{
	apu->bus = bus;

	apu->pulse[0].length.enabled = 1;
	apu->pulse[1].length.enabled = 1;
	apu->triangle.length.enabled = 1;
	apu->noise.length.enabled = 1;

	apu->noise.period = noise_period_table[0];
	apu->noise.lfsr = 1;
	apu->frame_cycle = 10;

	apu_update_irq(apu);
}

void
apu_tick(apu *apu)
{
	apu->pulse[0].length.reload_inhibit = 0;
	apu->pulse[1].length.reload_inhibit = 0;
	apu->triangle.length.reload_inhibit = 0;
	apu->noise.length.reload_inhibit = 0;

	apu->cycle++;

	triangle_tick(&apu->triangle);
	noise_tick(&apu->noise);

	if ((apu->cycle & 1) == 0) {
		pulse_tick(&apu->pulse[0]);
		pulse_tick(&apu->pulse[1]);
	}

	if (apu->frame_write_pending) {
		frame_write_tick(apu);
	} else {
		frame_counter_tick(apu);
	}

	apu_update_irq(apu);
	apu_sample_tick(apu);
}

uint8_t
apu_read(apu *apu, uint16_t addr)
{
	uint8_t status = 0;
	
	if (addr != 0x4015) { return 0; }
	if (apu->pulse[0].length.value > 0) { status |= 0x01; }
	if (apu->pulse[1].length.value > 0) { status |= 0x02; }
	if (apu->triangle.length.value > 0) { status |= 0x04; }
	if (apu->noise.length.value > 0) { status |= 0x08; }
	if (apu->frame_irq) { status |= 0x40; }
	
	apu->frame_irq = 0;
	apu_update_irq(apu);
	return status;
}

void
apu_write(apu *apu, uint16_t addr, uint8_t val)
{
	uint8_t enabled;
	unsigned int i;
	
	switch (addr) {
		case 0x4000:
		case 0x4001:
		case 0x4002:
		case 0x4003:
			pulse_write(&apu->pulse[0], addr, val);
			break;
		case 0x4004:
		case 0x4005:
		case 0x4006:
		case 0x4007:
			pulse_write(&apu->pulse[1], addr, val);
			break;
		case 0x4008:
			apu->triangle.length.halt = (val & 0x80) != 0;
			apu->triangle.linear_reload_value = val & 0x7F;
			break;
		case 0x400A:
			period_write_lo(&apu->triangle.period, val);
			break;
		case 0x400B:
			period_write_hi(&apu->triangle.period, val);
			apu->triangle.linear_reload = 1;
			length_load(&apu->triangle.length, val);
			break;
		case 0x400C:
			envelope_write(&apu->noise.envelope, &apu->noise.length, val);
			break;
		case 0x400E:
			apu->noise.mode = (val & 0x80) != 0;
			apu->noise.period = noise_period_table[val & 0x0F];
			break;
		case 0x400F:
			apu->noise.envelope.start = 1;
			length_load(&apu->noise.length, val);
			break;
		case 0x4015:
			apu_write_status(apu, val);
			break;
		case 0x4017:
			apu_write_frame_counter(apu, val);
			break;
		default:
			break;
	}
}
