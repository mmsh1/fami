#include <pthread.h>
#include <stdint.h>

#include "audio.h"
#include "raylib.h"

enum {
	AUDIO_RING_SIZE = 48000
};

typedef struct {
	AudioStream stream;
	uint8_t initialized;
	uint8_t ready;

	int16_t ring[AUDIO_RING_SIZE];
	uint64_t read_pos;
	uint64_t write_pos;
	uint64_t ring_length;

	pthread_mutex_t ring_mutex;
} audio_state;

static audio_state audio = {
	.ring_mutex = PTHREAD_MUTEX_INITIALIZER
};

static void
audio_callback(void *output, unsigned int frames)
{
	int16_t *samples = output;
	unsigned int i;

	pthread_mutex_lock(&audio.ring_mutex);

	for (i = 0; i < frames; i++) {
		if (audio.ring_length > 0) {
			samples[i] = audio.ring[audio.read_pos];
			audio.read_pos = (audio.read_pos + 1) % AUDIO_RING_SIZE;
			audio.ring_length--;
		} else {
			samples[i] = 0;
		}
	}

	pthread_mutex_unlock(&audio.ring_mutex);
}

void
audio_init(uint32_t sample_rate)
{
	audio.read_pos = 0;
	audio.write_pos = 0;
	audio.ring_length = 0;

	InitAudioDevice();
	audio.initialized = 1;

	if (!IsAudioDeviceReady()) {
		return;
	}

	audio.stream = LoadAudioStream((unsigned int)sample_rate, 16, 1);

	if (!IsAudioStreamValid(audio.stream)) {
		return;
	}

	SetAudioStreamCallback(audio.stream, audio_callback);
	PlayAudioStream(audio.stream);
	audio.ready = 1;
}

void
audio_write(const int16_t *samples, uint64_t count)
{
	uint64_t i;

	if (!audio.ready) {
		return;
	}

	pthread_mutex_lock(&audio.ring_mutex);

	for (i = 0; i < count; i++) {
		if (audio.ring_length == AUDIO_RING_SIZE) {
			break;
		}

		audio.ring[audio.write_pos] = samples[i];
		audio.write_pos = (audio.write_pos + 1) % AUDIO_RING_SIZE;
		audio.ring_length++;
	}

	pthread_mutex_unlock(&audio.ring_mutex);
}

void
audio_destroy(void)
{
	if (audio.ready) {
		StopAudioStream(audio.stream);
		UnloadAudioStream(audio.stream);
	}

	if (audio.initialized) {
		CloseAudioDevice();
	}

	audio.initialized = 0;
	audio.ready = 0;
	audio.read_pos = 0;
	audio.write_pos = 0;
	audio.ring_length = 0;
}
