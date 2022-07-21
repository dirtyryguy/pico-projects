#include "pico/stdlib.h"
#include <stdio.h>

int main(void)
{
	stdio_init_all();

	while (!stdio_usb_connected())
		sleep_ms(50);

	int buf[2];
	while (1)
	{
		scanf("%i,%i", buf, buf+1);
		printf("%i,%i\n", *buf, *(buf+1));
	}

	return 0;
}
