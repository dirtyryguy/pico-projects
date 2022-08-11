#include "pico/stdlib.h"
#include "pico/multicore.h"
#include <stdio.h>
#include <stdlib.h>

/**
 *
 * global variables and functions
 *
**/

uint32_t half_seq[] = {1, 3, 2, 6, 4, 12, 8, 9};

static int sign(int x)
{
	if (x > 0)
		return 1;
	else if (x < 0)
		return -1;
	return 0;
}

/**
 *
 * shift register code
 *
**/

typedef struct
{
	uint8_t serial_pin;
	uint8_t clock_pin;
	uint8_t latch_pin;
}shift_register;

shift_register *init_shift_register(uint8_t serial_pin, uint8_t clock_pin, uint8_t latch_pin)
{
	shift_register *reg = malloc(sizeof(shift_register));
	
	reg->serial_pin = serial_pin;
	reg->clock_pin = clock_pin;
	reg->latch_pin = latch_pin;

	gpio_init(reg->serial_pin);
	gpio_set_dir(reg->serial_pin, GPIO_OUT);

	gpio_init(reg->clock_pin);
	gpio_set_dir(reg->clock_pin, GPIO_OUT);

	gpio_init(reg->latch_pin);
	gpio_set_dir(reg->latch_pin, GPIO_OUT);

	return reg;
}

void write_bit(shift_register *reg, uint8_t bit)
{
	gpio_put(reg->serial_pin, bit);
	gpio_put(reg->clock_pin, 1);
	sleep_us(0);
	gpio_put(reg->clock_pin, 0);
}

void write_byte(shift_register *reg, uint8_t byte)
{
	uint8_t i;

	for (i = 0; i < 8; i++)
		write_bit(reg, (byte >> (7 - i)) & 1);
}

uint8_t write_block_data(shift_register *reg, uint32_t buf, uint8_t length)
{
	uint8_t i;
	
	for (i = 0; i < length; i++)
		write_bit(reg, (buf >> ((length - 1) - i)) & 1);
	
	return i;
}

void clear_register(shift_register *reg)
{
	write_byte(reg, 0x00);
}

/**
 *
 * gimbal code
 *
**/

typedef struct
{
	uint8_t ord;		// position on the shift register chain
	uint8_t pan_pin;	// zeroing pin for pan
	uint8_t tilt_pin;	// zeroing pin for tilt

	int32_t pan_pos;	// position in steps for the pan
	int32_t tilt_pos;	// and tilt stages

	int8_t pan_seq;		// current sequence for the pan
	int8_t tilt_seq;	// and tilt stages. signed int to avoid overflow
}gimbal;

gimbal *init_gimbal(uint8_t ord, uint8_t pan_pin, uint8_t tilt_pin)
{
	gimbal *g = malloc(sizeof(gimbal));

	g->ord = ord;
	g->pan_pin = pan_pin;
	g->tilt_pin = tilt_pin;

	g->pan_pos = 0;
	g->tilt_pos = 0;
	g->pan_seq = 0;
	g->tilt_seq = 0;

	gpio_init(g->pan_pin);
	gpio_init(g->tilt_pin);
	gpio_set_dir(g->pan_pin, GPIO_IN);
	gpio_set_dir(g->tilt_pin, GPIO_IN);
	gpio_pull_down(g->pan_pin);
	gpio_pull_down(g->tilt_pin);

	return g;
}

/**
 *
 * main program entry
 *
**/

int32_t pos_t[8];
uint8_t _zer0;

// core 1 main, used for io
void core1_entry(void)
{
	while (1)
	{
		scanf("%i,%i,%i,%i,%i,%i,%i,%i,%i", pos_t, pos_t+1, pos_t+2, pos_t+3,
											pos_t+4, pos_t+5, pos_t+6, pos_t+7, &_zer0);

		for (uint8_t i = 0; i < 8; i++)
			printf("%i,", *(pos_t+i));
		printf("%i\n", _zer0);
	}
}

// core 0 main, used for control of the gimbals
int main(void)
{
	// obligatory startup sequence
	// program will not begin until serial connection
	// is established
	stdio_init_all();

	while (!stdio_usb_connected())
		sleep_ms(50);
	
	multicore_launch_core1(core1_entry);

	// initialize shift register
	shift_register *reg = init_shift_register(18, 16, 17);

	gimbal *gimbals[4];

	for (uint8_t i = 0; i < 4; i++)
		*(gimbals+i) = init_gimbal(i, 2*i+6, 2*i+7);
	
	uint32_t buf = 0;

	while (1)
	{
		for (uint8_t i = 0; i < 4; i++)
		{
			int8_t pan_dir = sign(pos_t[i] - gimbals[i]->pan_pos);
			int8_t tilt_dir = sign(pos_t[i+1] - gimbals[i]->tilt_pos);

			if (pan_dir)
			{
				gimbals[i]->pan_pos += pan_dir;
				gimbals[i]->pan_seq += pan_dir;
				
				if (gimbals[i]->pan_seq > 7) gimbals[i]->pan_seq = 0;
				else if (gimbals[i]->pan_seq < 0) gimbals[i]->pan_seq = 7;

				uint32_t mask = 0xFFFFFFFF ^ (0xF << (i*8));
				buf &= mask;
				buf += half_seq[gimbals[i]->pan_seq] << (i*8);
			}

			if (tilt_dir)
			{
				gimbals[i]->tilt_pos += tilt_dir;
				gimbals[i]->tilt_seq += tilt_dir;

				if (gimbals[i]->tilt_seq > 7) gimbals[i]->tilt_seq = 0;
				else if (gimbals[i]->tilt_seq < 0) gimbals[i]->tilt_seq = 7;

				uint32_t mask = 0xFFFFFFFF ^ (0xF << (i*8 + 4));
				buf &= mask;
				buf += half_seq[gimbals[i]->tilt_seq] << (i*8 + 4);
			}
		}

		write_block_data(reg, buf, 32);
		gpio_put(reg->latch_pin, 1);
		sleep_us(0);
		gpio_put(reg->latch_pin, 0);
		sleep_us(1250);

		/**
		for (int i = 0; i < 8; i++)
		{
			uint32_t buf = half_seq[i] + (half_seq[i] << 12);
			write_block_data(reg, buf, 16);
			gpio_put(reg->latch_pin, 1);
			sleep_us(0);
			gpio_put(reg->latch_pin, 0);
			sleep_us(1750);	
		}
		**/
	}
}
