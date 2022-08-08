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

int sign(int x)
{
	if (x > 0)
		return 1;
	else if (x < 0)
		return -1;
	return 0;
}

void zero(t_stepper *s)
{
	int i = s->pos;
	if (i == 0)
		return;
	while (1)
	{
		half_step(s, sign(0 - i));
		sleep_us(1750);
		if (gpio_get(s->pins[4]))
		{
			printf("%i\n", gpio_get(s->pins[4]));
			break;
		}
	}
	s->pos = 0;
}

int32_t pos_t[4], _zer0;

// core 1 main
void core1_entry(void)
{
	while (1)
	{
		scanf("%i,%i,%i,%i,%i", pos_t, pos_t+1, pos_t+2, pos_t+3, &_zer0);
		printf("%i,%i,%i,%i,%i\n", *pos_t, *(pos_t+1), *(pos_t+2), *(pos_t+3), _zer0);
	}
}

// core 0 main
int main(void)
{
	stdio_init_all();

	while (!stdio_usb_connected())
		sleep_ms(50);
	
	multicore_launch_core1(core1_entry);
	
	uint32_t pins1[] = {0, 1, 2, 3, 16};
	uint32_t pins2[] = {4, 5, 6, 7, 17};
	uint32_t pins3[] = {8, 9, 10, 11, 18};
	uint32_t pins4[] = {12, 13, 14, 15, 19};
	t_stepper *s_pan0 = init_stepper(pins1);
	t_stepper *s_tilt0 = init_stepper(pins2);
	t_stepper *s_pan1 = init_stepper(pins3);
	t_stepper *s_tilt1 = init_stepper(pins4);

	while (1)
	{
		if (_zer0)
		{
			zero(s_pan0);
			zero(s_tilt0);
			zero(s_pan1);
			zero(s_tilt1);
			_zer0 = 0;
		}

		if (s_pan0->pos != pos_t[0])
			half_step(s_pan0, sign(pos_t[0] - s_pan0->pos));

		if (s_tilt0->pos != pos_t[1])
			half_step(s_tilt0, sign(pos_t[2] - s_tilt0->pos));

		if (s_pan1->pos != pos_t[2])
			half_step(s_pan1, sign(pos_t[3] - s_pan1->pos));

		if (s_tilt1->pos != pos_t[3])
			half_step(s_tilt1, sign(pos_t[3] - s_tilt1->pos));

		sleep_us(1250);
	}
	
	return 0;
}
