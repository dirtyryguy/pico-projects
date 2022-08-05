#include "pico/stdlib.h"
#include "pico/multicore.h"
#include <stdio.h>
#include <stdlib.h>
#define BUFFER_LENGTH 256
#define DELAY 1250

const uint32_t half_seq[] = {1, 3, 2, 6, 4, 12, 8, 9};

typedef struct
{
	uint32_t pins[5]; // fifth pin is the zeroing pin
	int32_t curr_step;
	int32_t pos;
}t_stepper;

t_stepper *init_stepper(uint32_t *pins)
{
	t_stepper *s = malloc(sizeof(t_stepper));
	s->curr_step = 0;
	s->pos = 0;
	for (int i = 0; i < 5; i++)
	{
		s->pins[i] = pins[i];
		gpio_init(s->pins[i]);
		gpio_set_dir(s->pins[i], GPIO_OUT);
	}
	s->pins[4] = pins[4];
	gpio_init(s->pins[4]);
	gpio_set_dir(s->pins[4], GPIO_IN);
	gpio_pull_down(s->pins[4]);
	return s;
}

void half_step(t_stepper *s, int dir)
{
	s->curr_step += dir;
	s->pos += dir;
	if (s->curr_step > 7) s->curr_step = 0;
	else if (s->curr_step < 0) s->curr_step = 7;
	for (int i = 0; i < 4; i++)
		gpio_put(s->pins[i], (half_seq[s->curr_step] >> (3-i)) & 1);
}

void zero(t_stepper *s, int dir)
{
	if (dir) // if dir is 0, the program will choose the shortest path
	{
		while (!gpio_get(s->pins[4]))
		{
			half_step(s, dir);
			sleep_us(1750);
		}
	}
	else
	{
		if (s->pos > 0)
			dir = -1;
		else
			dir = 1;

		while (gpio_get(s->pins[4]))
		{
			half_step(s, dir);
			sleep_us(1750);
		}
	}
	s->pos = 0;
}

int sign(int x)
{
	if (x > 0)
		return 1;
	else if (x < 0)
		return -1;
	return 0;
}

int32_t pos_p = 0, pos_t = 0;

// core 1 main
void core1_entry(void)
{
	float p, t;
	while (1)
	{
		scanf("%f,%f", &p, &t);
		pos_p = (int)(2624./360.)*p;
		pos_t = (int)(1472./360.)*t;
	}
}

// core 0 main
int main(void)
{
	stdio_init_all();

	while (!stdio_usb_connected())
		sleep_ms(50);
	
	multicore_launch_core1(core1_entry);
	
	uint32_t pins1[] = {8, 9, 10, 11, 16};
	uint32_t pins2[] = {12, 13, 14, 15, 17};
	t_stepper *s_pan = init_stepper(pins1);
	t_stepper *s_tilt = init_stepper(pins2);

	while (1)
	{
		if (s_pan->pos != pos_p)
			half_step(s_pan, sign(pos_p - s_pan->pos));
		if (s_tilt->pos != pos_t)
			half_step(s_tilt, sign(pos_t - s_tilt->pos));
		sleep_us(1250);
	}
	
	return 0;
}
