#include "Arduino.h"
#include "math.h"

#define PIN 2
#define COMM_TIMEOUT 500

#define START_ANIMATION_CMD 'A'
#define STOP_ANIMATION_CMD 'B'
#define WRITE_FRAME_CMD 'C'
#define SET_BACKGROUND_INTENSITY_CMD 'D'

#define STRAND_LENGTH  10*144

struct rgbw
{
	byte g;
	byte r;

	byte b;
	byte w;
};

struct orb_info
{
	bool enabled;
	bool requires_init;
	uint8_t init_stage;
	int init_gen_ctr;

	int size;
	ulong cycle_start_time;

	double speed;
	double location;

	double core_hue;
	rgbw core_color;

	bool buffer_set;
	rgbw * render_buffer;
};

