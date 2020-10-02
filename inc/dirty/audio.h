// wengwengweng

#ifndef D_AUDIO_H
#define D_AUDIO_H

// a static sound
typedef struct {
	int channels;
	int sample_rate;
	short *samples;
	int num_samples;
} d_sound;

// sound playback control handle
typedef struct {
	const d_sound *src;
	int pos;
	bool loop;
	float volume;
	bool paused;
	bool done;
} d_sound_pb;

typedef struct {
	float attack;
	float decay;
	float sustain;
	float release;
} d_envelope;

// user provided stream
void d_stream(float (*f)());

// SOUND
d_sound d_parse_sound(const unsigned char *bytes, int size);
d_sound d_load_sound(const char *path);
void d_free_sound(d_sound *sound);
// play a sound, returning a handle for control
d_sound_pb *d_play(const d_sound *sound);
void d_sound_pb_seek(d_sound_pb *pb, float time);

// SYNTH
void d_synth_play(int note);
void d_synth_release(int note);
d_envelope *d_synth_envelope();
void d_synth_wav(float (*func)(float freq, float t));
float d_synth_bufn(int n);

// built in wave forms
float d_wav_sin(float freq, float t);
float d_wav_square(float freq, float t);
float d_wav_tri(float freq, float t);
float d_wav_saw(float freq, float t);
float d_wav_noise(float freq, float t);

#endif

