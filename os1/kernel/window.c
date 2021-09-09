/* ウィンドウ関係 */

#include "bootpack.h"

extern unsigned short table_8_565[256];

void make_window8(unsigned short *buf, int xsize, int ysize, char *title, char act)
{
	boxfill8(buf, xsize, COL8_848484, 0,         0,         xsize-1, ysize-1        );
	boxfill8(buf, xsize, COL8_C6C6C6, 1,         1,         xsize-2, ysize-2        );
	
	make_wtitle8(buf, xsize, title, act, ysize);
	return;
}

void make_menu8(unsigned short *buf, int xsize, int ysize, char *title, struct MENU *menu, int num)
{
	int i, yp, col = 16 + 2 + 2 * 6 + 2 * 36;
	struct MENU *pmenu;

	boxfill8(buf, xsize, COL8_848484, 0,         0,         xsize-1, ysize-1        );
	boxfill8(buf, xsize, COL8_C6C6C6, 1,         1,         xsize-2, ysize-2        );

	pmenu = menu;
	for (i = 0; i < num; i++) {
		putfonts8_asc(buf, xsize, 8, ysize - (2 + 26 * (i + 1)) + 6, COL8_000000, 1, pmenu->name);
		pmenu = pmenu->next;
	}
	for (i = 0; i < num - 1; i++) {
		yp = ysize - (2 + 26 * (i + 1));
		boxfill8(buf, xsize, col,         4, yp,     xsize - 6, yp    );
		boxfill8(buf, xsize, COL8_FFFFFF, 5, yp + 1, xsize - 5, yp + 1);
	}

	if (menu->level == 0) {
		make_mtitle8(buf, xsize, title, 1);
	}

	return;
}

void make_wtitle8(unsigned short *buf, int xsize, char *title, char act, int ysize)
{
	static char closebtn[16][16] = {
		"@@@@@@@@@@@@@@@@",
		"@QQQQQQQQQQQQQQ@",
		"@QQQQQQQQQQQQQQ@",
		"@QQQ@@QQQQ@@QQQ@",
		"@QQQQ@@QQ@@QQQQ@",
		"@QQQQQ@@@@QQQQQ@",
		"@QQQQQQ@@QQQQQQ@",
		"@QQQQQ@@@@QQQQQ@",
		"@QQQQ@@QQ@@QQQQ@",
		"@QQQ@@QQQQ@@QQQ@",
		"@QQQQQQQQQQQQQQ@",
		"@QQQQQQQQQQQQQQ@",
		"@@@@@@@@@@@@@@@@",
		"QQQQQQQQQQQQQQQQ"
	};
	int x, y;
	char c, tc, tbc;
	if (act != 0) {
		tc = 0;
	} else {
		tc = 7;
	}
	putfonts8_asc(buf, xsize, 6, 4, tc,1, title);
	for (y = 0; y < 16; y++) {
		for (x = 0; x < 16; x++) {
			c = closebtn[y][x];
			if (c == '@') {
				c = COL8_848484;
			} else {
				c = COL8_C6C6C6;
			}
			buf[(5 + y) * xsize + (xsize - 23 +x)] = table_8_565[c];
		}
	}
	
	return;
}

void make_mtitle8(unsigned short *buf, int xsize, char *title, char act)
{
	
	unsigned char c, tc, tbc;
	if (act != 0) {
		tc = COL8_FFFFFF;
		tbc = COL8_848484;
	} else {
		tc = COL8_FFFFFF;
		tbc = COL8_848484;
	}
	boxfill8(buf, xsize, tbc, 5, 4, xsize - 6, 19);
	putfonts8_asc(buf, xsize, 24, 4, tc, 1, title);
	
	return;
}

void putfouts8_asc_sht(struct SHEET *sht, int x, int y, int c, int b, char *s, int l)
{
	struct TASK *task = task_now();
	boxfill8(sht->buf, sht->bxsize, b, x, y, x + l * 8 - 1, y + 15);
	if (task->langmode != 0 && task->langbyte1 != 0) {
		putfonts8_asc(sht->buf, sht->bxsize, x, y, c, 1, s);
		sheet_refresh(sht, x - 8, y, x + l * 8, y + 16);
	} else {
		putfonts8_asc(sht->buf, sht->bxsize, x, y, c, 1, s);
		sheet_refresh(sht, x, y, x + l * 8, y + 16);
	}
	return;
}

void make_textbox8(struct SHEET *sht, int x0, int y0, int sx, int sy, int c)
{
	boxfill8(sht->buf, sht->bxsize, COL8_848484, x0, y0, sx, sy);
	boxfill8(sht->buf, sht->bxsize, c, x0 +1, y0+1, sx-1, sy-1);
	return;
}

void make_header8(struct SHEET *sht, int x0, int y0, int sx, int sy, int c)
{
	boxfill8(sht->buf, sht->bxsize, COL8_848484, x0, y0, sx, sy);
	boxfill8(sht->buf, sht->bxsize, c, x0 +1, y0+1, sx-1, sy-1);
	return;
}

void change_wtitle8(struct SHEET *sht, char act)
{
	int xsize = sht->bxsize;
	int ysize = sht->bysize;
	char buf  = sht->buf;
	unsigned char  tc;
	if (act != 0) {
		tc = 7;
	} else {
		tc = 0;
	}
	//putfonts8_asc(buf, xsize, 24, 4, tc, 1, title);
	sheet_refresh(sht, 5, 23, xsize- 6, ysize -6);
	return;
}

void change_mtitle8(struct SHEET *sht, int level, int mn_flg, char act)
{
	int x, y, yp, xsize = sht->bxsize;
	int ys, ye;
	unsigned char tc_new, tbc_new, tc_old, tbc_old;
	unsigned short c565, *buf = sht->buf;
	if (act != 0) {
		tc_new  = COL8_FFFFFF;
		tbc_new = COL8_848484;
		tc_old  = COL8_000000;
		tbc_old = COL8_C6C6C6;
	} else {
		tc_new  = COL8_000000;
		tbc_new = COL8_C6C6C6;
		tc_old  = COL8_FFFFFF;
		tbc_old = COL8_848484;
	}
	yp = (mn_flg -1) * 26 + 3;
	if (level == 0) {
		yp += 18;
	}
	for (y = 0; y < 22; y++) {
		for (x = 5; x <= xsize - 6; x++) {
			c565 = buf[(yp + y) * xsize + x];
			if (c565 == table_8_565[tc_old]) {
				c565 = table_8_565[tc_new];
			} else if (c565 == table_8_565[tbc_old]) {
				c565 = table_8_565[tbc_new];
			}
			buf[(yp + y) * xsize + x] = c565;
		}
	}
	sheet_refresh(sht, 5, yp, xsize - 5, yp + 22);
	
	
	return;
}
