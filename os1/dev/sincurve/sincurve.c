#include "apilib.h"
#include <math.h>

void HariMain(void)
{
	short buf[200 * 200];
	int win, i, j;
	double rd;
	win = api_openwin(buf, 200, 200, -1, "sincurve");
	api_boxfilwin(win, 5, 25, 193, 193, 0);
	for (i = 0; i < 360; i++) {
		rd = 3.14159 * i / 180;
		for (j = 1; j < 8; j++) {
			api_point(win, cos(rd) * cos(6 * rd) * (j + 1) * 10 + 100,
						  -sin(rd) * cos(6 * rd) * (j + 1) * 10 + 110, j);
		}
	}
	api_getkey(1); /* ‰½‚©ƒL[‚ð‰Ÿ‚¹‚ÎI—¹ */
	api_end();
}
