//collab led stuff

#include "LEDAnimator2.h"
#include <Adafruit_NeoPixel.h>
#include "math.h"




Adafruit_NeoPixel strip = Adafruit_NeoPixel(STRAND_LENGTH, PIN, NEO_RGBW + NEO_KHZ800);

int MAX_ORB_SIZE = 144 * 2;

double MAX_ORB_SPEED = 144.0 * 10.0 * 2;
double MIN_ORB_SPEED = 144.0 * 2.0;

int max_framerate = 600; //fps 340
int alt_framerate = 30;
double drop_framerate = 10.0;
double collision_detect_fps = 60;

//pixel buffer
rgbw * strip_buffer;

const int num_orbs = 20;
orb_info orbs[num_orbs];

rgbw background = { 0, 0, 0 , 0 };

bool drop_dispatch_pending = false;
int last_drop = 0;

int active_orbs = 0;

bool drop_init = true;

bool render_order_needs_update = false;

double average_cycle_time = ((1.0 / max_framerate) * 1000 * 1000);
double average_cycle_time_b = ((1.0 / max_framerate) * 1000 * 1000);
double pending_average_cycle_time = 0;
int cycle_time_ctr = 0;
double cycle_correction = 0;

double speed_to_size = MAX_ORB_SIZE / MAX_ORB_SPEED;

bool drop_init_active = false;

ulong target_cycle_time;
ulong start_time;
ulong cycle_time;
ulong alt_cycle_time;
ulong drop_cycle_time;
ulong background_cycle_timer;
ulong debug_writer_timer;
ulong drop_cycle_timer;
int orb_processing_ctr = 0;

float background_hue = 0;
float background_intensity = 0.125;//0.375;//0.125;

bool animation_running = true;

void setup()
{
	for(int i = 0; i < num_orbs; i++)
		orbs[i] = { false, true,0,0, MAX_ORB_SIZE, 0, 0, 0,0, {0, 0, 0, 0} ,false, NULL };
	
	Serial.begin(9600);
	Serial.println("Starting Application");
	
	randomSeed(analogRead(A0) * analogRead(A1) * analogRead(A2) * analogRead(A3));


	background_hue = ((float)rand() / (float)RAND_MAX) * 360.0;

	strip.begin();

	strip_buffer = (rgbw*)&strip.pixels[0];

	strip.setBrightness(0);

	fillBackground();

	strip.show();
}

void drop_manager()
{	
	if ((ulong)((long)micros() - drop_cycle_timer) >= drop_cycle_time)
	{

		if (drop_init)
		{
			drop_init = false;
		}
		else
		{
			//if there's an orb in the way then wait before addding a new one
			//if ((orbs[last_drop].location - orbs[last_drop].size/2.0) < orbs[last_drop].size/2.0)
			//	return;
		}

		bool found_available_drop = false;

		//find the next available drop
		for (int l = 0; l < num_orbs; l++)
			if (orbs[l].enabled == false)
			{
				active_orbs++;
				last_drop = l;
				found_available_drop = true;
				orbs[l].enabled = true;
				drop_init_active = true;
				orb_processing_ctr = l;
				break;
			}

		drop_cycle_timer = micros();
	}
}

void setOrbSize(int orb_index, const int size)
{
	if (orbs[orb_index].buffer_set == true)
	{
		
	}
	
	if (orbs[orb_index].buffer_set == false)
	{
		orbs[orb_index].render_buffer = (rgbw*)malloc((size + 1) * 4);
		orbs[orb_index].buffer_set = true;
	}

	orbs[orb_index].size = size;
}

void fillBackground()
{
	for (int bfillctr = 0; bfillctr < STRAND_LENGTH; bfillctr++)
		if (!(strip_buffer[bfillctr].r > background.r || strip_buffer[bfillctr].g > background.g || strip_buffer[bfillctr].b > background.b || strip_buffer[bfillctr].w > background.w))
			memcpy(&strip_buffer[bfillctr], &background, 4);
}

ulong cycle_start_timer;
ulong idle_time = 10000;
ulong edge_etect_cycle_time;

void loop()
{
	target_cycle_time = round(((1.0 / max_framerate) * 1000.0 * 1000.0));
	alt_cycle_time = round(((1.0 / alt_framerate) * 1000.0 * 1000.0));
	edge_etect_cycle_time = round(((1.0 / alt_framerate) * 1000.0 * 1000.0));
	drop_cycle_time = round(((1.0 / drop_framerate) * 1000.0 * 1000.0));
	cycle_time = round(target_cycle_time);
	
	start_time = micros();

	while (true)
	{		
		//if (Serial.available())
			serial_processor();

		if (animation_running)
		{
			if (active_orbs > 0)
				cycle_time = target_cycle_time;
			else
				cycle_time = alt_cycle_time;

			do
			{
				//timed
				update_background();

				//timed
				if (!drop_init_active)
					drop_manager();
				
				//collision_detect();

				if (!orbs[orb_processing_ctr].enabled)
					orb_processing_ctr++;
				else
					if (!orbs[orb_processing_ctr].requires_init)
						orb_processing_ctr++;
					else
						if (process_orb(orb_processing_ctr))
							orb_processing_ctr++;

				if (!(orb_processing_ctr < num_orbs))
					orb_processing_ctr = 0;

				

			} while (((ulong)((long)micros() - start_time) < cycle_time));
			
			frame_refresh();
			strip.show();
			start_time = micros();
			


			/*
			if ((ulong)((long)micros() - debug_writer_timer) >= drop_cycle_time)
			{
			Serial.print("fps uncor ");
			Serial.println(1.0 / (average_cycle_time / 1000.0 / 1000.0));
			Serial.print("fps cor ");
			Serial.println(1.0 / (average_cycle_time_b / 1000.0 / 1000.0));
			Serial.print("cor ");
			Serial.println(cycle_correction);
			Serial.print("norbs ");
			Serial.println(active_orbs);

			debug_writer_timer = micros();
			}
			*/

			//while ((ulong)((long)micros() - start_time) < cycle_time);
		}
		else
			if ((ulong)((long)millis() - cycle_start_timer) >= idle_time)
			{
				animation_running = true;
				cycle_start_timer = millis();
			}
	}
}

ulong comm_timer;
bool comm_flag;
int buffer_pos;
uint8_t current_command;

void serial_processor()
{
	if ((comm_flag && ((unsigned long)((long)millis() - comm_timer) >= COMM_TIMEOUT)))
	{
		while (Serial.read() > -1);
		comm_flag = false;
		buffer_pos = 0;
		return;
	}

	if (Serial.available() == 0)
		return;
	
	if (!comm_flag)
	{
		comm_flag = true;
		comm_timer = millis();
	}

		if (buffer_pos == 0)
		{
			current_command = Serial.read();
			buffer_pos++;
			return;
		}

		//do something
		if (current_command == START_ANIMATION_CMD)
		{
			animation_running = true;

			Serial.write(0x1); //ack
		}
		else
			if (current_command == STOP_ANIMATION_CMD)
			{
				animation_running = false;

				Serial.write(0x1); //ack
			}
			else
				if (current_command == WRITE_FRAME_CMD)
				{
					while (Serial.available())
					{
						if (buffer_pos == 1)
						{
							cycle_start_timer = millis();
							animation_running = false;
						}

						byte * b_ptr = (byte*)&strip_buffer[0];

						b_ptr[buffer_pos - 1] = Serial.read();
						buffer_pos++;

						if (buffer_pos < STRAND_LENGTH)
							continue;
						else
						{
							strip.show();

							Serial.write(0x1); //ack
							Serial.flush();
							buffer_pos = 0;
							comm_flag = false;
						}
					}

					return;
				}
				else
				{
					//todo: clear buffered data
					
					Serial.write(0x0); //nack
				}

		Serial.flush();
		buffer_pos = 0;
		comm_flag = false;
}

void post_cycle()
{
	average_cycle_time_b = (((ulong)((long)micros() - start_time)));

	pending_average_cycle_time += ((((ulong)((long)micros() - start_time)))) - round(cycle_correction);

	cycle_time_ctr++;

	if (pending_average_cycle_time > 1000000.0)
	{
		average_cycle_time = pending_average_cycle_time / cycle_time_ctr;
		pending_average_cycle_time = 0;
		cycle_time_ctr = 0;
	}

	cycle_correction = (average_cycle_time - target_cycle_time);

	double temp = target_cycle_time - cycle_correction;
	if (temp < 0)
		cycle_time = 0;
	else
		cycle_time = round(temp);
	
	start_time = micros();
}

/*
void sort(orb_info * aa, int sizez)
{
	for (int i = 0; i < (sizez - 1); i++)
	{
		for (int o = 0; o < (sizez - (i + 1)); o++)
		{
			if (aa[o].speed > aa[o + 1].speed)
			{
				orb_info * t = &aa[o];
				aa[o] = aa[o + 1];
				aa[o + 1] = *(orb_info*)t;
			}
		}
	}
}
*/

void frame_refresh()
{
	//fillBackground();
	process_orbs();
}

int rgbw[4] = { 0, 0, 0, 0 };

bool init_orb(int orb_index)
{
	//orb init stage 0 - property and buffer creation
	if (orbs[orb_index].init_stage == 0)
	{
		orbs[orb_index].location = -MAX_ORB_SIZE;

		randomSeed(analogRead(A0) * analogRead(A1) * analogRead(A2) * analogRead(A3));


		orbs[orb_index].core_hue = (double)rand() / ((double)RAND_MAX )  * 360.0;

		int rgbw[4] = { 0, 0, 0, 0 };

		hsi2rgbw(orbs[orb_index].core_hue,
		1,
		1,
		rgbw);

		orbs[orb_index].core_color.r = rgbw[0];
		orbs[orb_index].core_color.g = rgbw[1];
		orbs[orb_index].core_color.b = rgbw[2];
		orbs[orb_index].core_color.w = rgbw[3];

		orbs[orb_index].speed = random(MIN_ORB_SPEED, MAX_ORB_SPEED);

		setOrbSize(orb_index, round(orbs[orb_index].speed * speed_to_size));

		orbs[orb_index].init_stage++;
	}

	//orb init stage 1 - render orb

	if (orbs[orb_index].init_stage == 1)
	{
		int orb_pos = orbs[orb_index].init_gen_ctr;

		//apply the transform
		double orb_center = orbs[orb_index].size / 2.0;
		double max_dist = orbs[orb_index].size - orb_center;

		for (; orb_pos <= orbs[orb_index].size; orb_pos++)
		{			
			//distance from center
			int dist_center = round(orb_pos - orb_center);

			//percentage of distance from center
			double perdist = abs(dist_center) / max_dist;

			if ((orb_pos >= orb_center && orb_pos <= orb_center) || orb_pos == orb_center)
				//if(dist_center == 0)
			{
				//this is the center. apply the maximum values

				hsi2rgbw(orbs[orb_index].core_hue,
					1,
					1,
					rgbw);

				//memcpy((void*)(&orbs[orb_index].render_buffer[orb_pos]), (void*)&(rgbw), 4);

				//orbs[orb_index].render_buffer[orb_pos].s = 1;
				orbs[orb_index].render_buffer[orb_pos].r = rgbw[0];
				orbs[orb_index].render_buffer[orb_pos].g = rgbw[1];
				orbs[orb_index].render_buffer[orb_pos].b = rgbw[2];
				orbs[orb_index].render_buffer[orb_pos].w = rgbw[3];
			}
			else
			{				
				if (dist_center < 0)
				{
					float res = pow(1.0 - perdist, 2);
					
					if (res <= background_intensity)
					{
						orbs[orb_index].render_buffer[orb_pos].r = 0;
						orbs[orb_index].render_buffer[orb_pos].g = 0;
						orbs[orb_index].render_buffer[orb_pos].b = 0;
						orbs[orb_index].render_buffer[orb_pos].w = 0;

						continue;
					}
						
					
					//this is the left side from the center
					hsi2rgbw(orbs[orb_index].core_hue,
						1,
						res,
						rgbw);

					//memcpy(&(orbs[orb_index].render_buffer[orb_pos]), &rgbw, 4);

					orbs[orb_index].render_buffer[orb_pos].r = rgbw[0];
					orbs[orb_index].render_buffer[orb_pos].g = rgbw[1];
					orbs[orb_index].render_buffer[orb_pos].b = rgbw[2];
					orbs[orb_index].render_buffer[orb_pos].w = rgbw[3];

				}
				else
					if (dist_center > 0)
					{
						float res = pow(1.0 - perdist, 60);
						
						
						//if (res <= background_intensity)
						//{
						//	orbs[orb_index].render_buffer[orb_pos].r = 0;
						//	orbs[orb_index].render_buffer[orb_pos].g = 0;
						//	orbs[orb_index].render_buffer[orb_pos].b = 0;
						//	orbs[orb_index].render_buffer[orb_pos].w = 0;

						//	continue;
						//}
						
						
						//this is the right side
						hsi2rgbw(orbs[orb_index].core_hue,
							.99,
							res,
							rgbw);

						orbs[orb_index].render_buffer[orb_pos].r = rgbw[0];
						orbs[orb_index].render_buffer[orb_pos].g = rgbw[1];
						orbs[orb_index].render_buffer[orb_pos].b = rgbw[2];
						orbs[orb_index].render_buffer[orb_pos].w = rgbw[3];
					}
			}

			//if ((ulong)((long)micros() - start_time) < cycle_time)
			//	continue;
			//else
			//{
				orb_pos++;
				break;
			//}
		}

		if (orb_pos <= orbs[orb_index].size)
		{
			orbs[orb_index].init_gen_ctr = orb_pos;
			return false;
		}

		orbs[orb_index].init_stage = 0;
		orbs[orb_index].init_gen_ctr = 0;
		orbs[orb_index].requires_init = false;
		drop_init_active = false;

		

		orbs[orb_index].cycle_start_time = micros();

		return true;
	}

	return false;
}

//simple blend
void process_orbs()
{
	double hue_avg = 0;
	int num_hues = 0;

	//clear the buffer
	for (int i = 0; i < STRAND_LENGTH; i++)
		((uint32_t*)&strip_buffer[0])[i] = 0;
	
	//process orbs
	for (int j = 0; j < num_orbs; j++)
	{
		if (!orbs[j].enabled)
			continue;

		if (!orbs[j].requires_init)
			if (!process_orb(j))
				continue;
		
		//blend orbs into buffer//

		num_hues++;
		hue_avg += orbs[j].core_hue;

		int orb_location = round(orbs[j].location);

		//copy orb buffer to display buffer
		
		for (int k = 0; k < orbs[j].size/2+2; k++)
		{
			int opk = orb_location + k;
			
			if (opk >= 0 && opk < STRAND_LENGTH)
			{
				if (orbs[j].render_buffer[k].r > background.r || orbs[j].render_buffer[k].g > background.g || orbs[j].render_buffer[k].b > background.b || orbs[j].render_buffer[k].w > background.w)
				{
					if (strip_buffer[opk].r > 0 || strip_buffer[opk].g > 0 || strip_buffer[opk].b > 0 || strip_buffer[opk].w > 0)
					{
						
						
						//strip_buffer[opk].w = 255;
							//strip_buffer[opk].r = ((int)orbs[j].render_buffer[k].r + strip_buffer[opk].r) >> 1;

							//strip_buffer[opk].g = ((int)orbs[j].render_buffer[k].g + strip_buffer[opk].g) >> 1;

							//strip_buffer[opk].b = ((int)orbs[j].render_buffer[k].b + strip_buffer[opk].b) >> 1;

							//strip_buffer[opk].w = ((int)orbs[j].render_buffer[k].w + strip_buffer[opk].w) >> 1;

						strip_buffer[opk].r = ((int)orbs[j].core_color.r + strip_buffer[opk].r) >> 1;

						strip_buffer[opk].g = ((int)orbs[j].core_color.g + strip_buffer[opk].g) >> 1;

						strip_buffer[opk].b = ((int)orbs[j].core_color.b + strip_buffer[opk].b) >> 1;

						strip_buffer[opk].w = ((int)orbs[j].core_color.w + strip_buffer[opk].w) >> 1;
					}
					else
						memcpy(&strip_buffer[opk], &(orbs[j].render_buffer[k]), 4);
				}
			}			
		}
	}

	fillBackground();

	if (num_hues > 0)
	{
		hue_avg /= num_hues;
		background_hue = background_hue * (1 - 2.7777777777777777777777777777778e-4) + hue_avg * (2.7777777777777777777777777777778e-4);
	}
}

double b_sat_max = 1.0;
double b_sat_min = 0.9;
double b_sat = 1;

double b_int_max = 0.2;

void update_background()
{	
	if ((ulong)((long)micros() - background_cycle_timer) >= alt_cycle_time)
	{
	    randomSeed(analogRead(A0) * analogRead(A1) * analogRead(A2) * analogRead(A3));
			
		if(random(0, 2) == 0)
			background_hue += ((float)rand() / (float)RAND_MAX);
		else
			background_hue -= ((float)rand() / (float)RAND_MAX);

		if(random(0, 2) == 0)
			b_sat += ((float)rand() / (float)RAND_MAX) * 0.005;
		else
			b_sat -= ((float)rand() / (float)RAND_MAX) * 0.005;

		if (random(0, 2) == 0)
			background_intensity += ((float)rand() / (float)RAND_MAX) * 0.005;
		else
			background_intensity -= ((float)rand() / (float)RAND_MAX) * 0.005;

		if (background_hue > 360.0)
			background_hue = 360.0 - background_hue;
		else
			if (background_hue < 0)
				background_hue = 360.0 - abs(background_hue);

		if (b_sat > 1.0)
			b_sat = 1.0;
		else
			if (b_sat < 0.9)
				b_sat = 0.9;

		if (background_intensity > b_int_max)
			background_intensity = b_int_max;
		else
			if (background_intensity < 0)
				background_intensity = 0;

		int rgbw[4] = { 0, 0, 0, 0 };

		hsi2rgbw(background_hue,
			b_sat,
			background_intensity,
			rgbw);


		background.r = rgbw[0];
		background.g = rgbw[1];
		background.b = rgbw[2];
		background.w = rgbw[3];

		//fillBackground();

		background_cycle_timer = micros();
	}
}

bool process_orb(int orb_index)
{
	if (orbs[orb_index].requires_init)
		if (!init_orb(orb_index))
			return false;
		else
			return true;

	ulong current_time = micros();

	double time_passed = ((ulong)((long)current_time - orbs[orb_index].cycle_start_time));
	time_passed /= 1000000.0;

	orbs[orb_index].location += time_passed*orbs[orb_index].speed;
	orbs[orb_index].cycle_start_time = current_time;

	if (orbs[orb_index].location >= STRAND_LENGTH)
	{
		free(orbs[orb_index].render_buffer);
		orbs[orb_index].buffer_set = false;

		orbs[orb_index].requires_init = true;
		orbs[orb_index].enabled = false;
		active_orbs--;
		//return;
	}

	return true;
}

const float r_scale = 0.65;
const float g_scale = 0.675;
const float b_scale = 1.0;
const float w_scale = 1.0;

void hsi2rgbw(float H, float S, float I, int* rgbw)
{
	float r, g, b, w;
	float cos_h, cos_1047_h;
	H = fmod(H, 360); // cycle H around to 0-360 degrees
	H = 3.14159*H / (float)180; // Convert to radians.
	S = S>0 ? (S<1 ? S : 1) : 0; // clamp S and I to interval [0,1]
	I = I>0 ? (I<1 ? I : 1) : 0;

	if (H < 2.09439) {
		cos_h = cos(H);
		cos_1047_h = cos(1.047196667 - H);
		r = S*I / 3 * (1 + cos_h / cos_1047_h);
		g = S*I / 3 * (1 + (1 - cos_h / cos_1047_h));
		b = 0;
		w = (1 - S)*I;
	}
	else if (H < 4.188787) {
		H = H - 2.09439;
		cos_h = cos(H);
		cos_1047_h = cos(1.047196667 - H);
		g = S*I / 3 * (1 + cos_h / cos_1047_h);
		b = S*I / 3 * (1 + (1 - cos_h / cos_1047_h));
		r = 0;
		w = (1 - S)*I;
	}
	else {
		H = H - 4.188787;
		cos_h = cos(H);
		cos_1047_h = cos(1.047196667 - H);
		b = S*I / 3 * (1 + cos_h / cos_1047_h);
		r = S*I / 3 * (1 + (1 - cos_h / cos_1047_h));
		g = 0;
		w = (1 - S)*I;
	}

	rgbw[0] = round(((r ) * 255.0) );
	rgbw[1] = round(((g ) * 255.0) );
	rgbw[2] = round(((b ) * 255.0) );
	rgbw[3] = round(((w ) * 255.0) );
}
/*
//bool detected[num_orbs];
int ctrg;
ulong collision_detect_timer;
ulong collision_detect_time = 0;
int pind;
int orbind;
void collision_detect()
{
	if ((ulong)((long)micros() - collision_detect_timer) >= collision_detect_time)
	{
		if(pind >= numPixels)
			pind = 0;

		for (; pind < numPixels; pind++)
		{
			ctrg = 0;
			orbind = 0;
			for (; orbind < num_orbs; orbind++)
			{
				if (orbs[orbind].enabled && !orbs[orbind].requires_init)
				{
					if (pind >= orbs[orbind].location && pind <= (orbs[orbind].location + orbs[orbind].size/2))
						ctrg++;
					else
						continue;

					if (ctrg > 1)
						strip_buffer[pind].w = 255;
				}
				else
					continue;
			}

		}
	}
}
*/

