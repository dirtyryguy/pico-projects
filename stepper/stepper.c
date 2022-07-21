#include "pico/stdlib.h"
#include <stdlib.h>
#include <stdio.h>

const int half_seq[] = {1, 3, 2, 6, 4, 12, 8, 9};
const int full_seq[] = {3, 6, 12, 9};

typedef struct stepper
{
    int pins[4];
    int curr_step;
} t_stepper;

t_stepper *init_stepper(int *pins)
{
    t_stepper *s = malloc(sizeof(t_stepper));
    s->curr_step = 0;
    for (int i = 0; i < 4; i++)
    {
        s->pins[i] = pins[i];
        gpio_init(s->pins[i]);
        gpio_set_dir(s->pins[i], GPIO_OUT);
    }
    return s;
}

void half_step(t_stepper *s, int dir)
{
    s->curr_step += dir;
    if (s->curr_step > 7) s->curr_step = 0;
    else if (s->curr_step < 0) s->curr_step = 7;
    for (int i = 0; i < 4; i++)
        gpio_put(s->pins[i], (half_seq[s->curr_step] >> (3-i)) & 1);
}

void full_step(t_stepper *s, int dir)
{
    s->curr_step += dir;
    if (s->curr_step > 3) s->curr_step = 0;
    else if (s->curr_step < 0) s->curr_step = 3;
    for (int i = 0; i < 4; i++)
        gpio_put(s->pins[i], (full_seq[s->curr_step] >> (3-i)) & 1);
}

int main()
{
	stdio_init_all();
    int pins1[] = {12, 13, 14, 15};
    int pins2[] = {8, 9, 10, 11};
    t_stepper *s1 = init_stepper(pins1);
    t_stepper *s2 = init_stepper(pins2);
    
    int ctrl_pins[] = {16, 17, 18, 19};
    for (int i = 0; i < 4; i++)
    {
        gpio_init(ctrl_pins[i]);
        gpio_set_dir(ctrl_pins[i], GPIO_IN);
        gpio_pull_down(ctrl_pins[i]);
    }
    
	int d1 = 0;
	int d2 = 0;
    while (1)
    {
		d1 += gpio_get(ctrl_pins[2]) - gpio_get(ctrl_pins[3]);
		d2 += gpio_get(ctrl_pins[0]) - gpio_get(ctrl_pins[1]);
        half_step(s1, gpio_get(ctrl_pins[2]) - gpio_get(ctrl_pins[3]));
        half_step(s2, gpio_get(ctrl_pins[0]) - gpio_get(ctrl_pins[1]));
		printf("s1: %d, s2: %d\n", d1, d2);
        sleep_us(1250);
    }

    /**
    int i;
    while (1)
    {
        for (i = 0; i < 2500; i++)
        {
            full_step(s1, 1);
            sleep_us(1250);
        }
        for (i = 0; i < 2500; i++)
        {
            full_step(s1, -1);
            sleep_us(1250);
        }
    }
    **/

    return 0;
}
