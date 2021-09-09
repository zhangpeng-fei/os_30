#include "apilib.h"

void HariMain(void)
{
	int win;
	short buf[150 * 50];
	win = api_openwin(buf, 150, 50, -1, "hello’≈≈Ù∑…");
	for (;;) {
		if (api_getkey(1) == 0x0a) {
			break; /* EnterÇ»ÇÁbreak; */
		}
	}
	api_end();
}
