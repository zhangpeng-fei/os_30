/* グラフィック処理関係 */

#include "bootpack.h"

void init_palette(void)
{
	static unsigned char table_rgb[16 * 3] = {
		0x00, 0x00, 0x00,	/*  0:黒 */
		0xff, 0x00, 0x00,	/*  1:明るい赤 */
		0x00, 0xff, 0x00,	/*  2:明るい緑 */
		0xff, 0xff, 0x00,	/*  3:明るい黄色 */
		0x00, 0x00, 0xff,	/*  4:明るい青 */
		0xff, 0x00, 0xff,	/*  5:明るい紫 */
		0x00, 0xff, 0xff,	/*  6:明るい水色 */
		0xff, 0xff, 0xff,	/*  7:白 */
		0xc6, 0xc6, 0xc6,	/*  8:明るい灰色 */
		0x84, 0x00, 0x00,	/*  9:暗い赤 */
		0x00, 0x84, 0x00,	/* 10:暗い緑 */
		0x84, 0x84, 0x00,	/* 11:暗い黄色 */
		0x00, 0x00, 0x84,	/* 12:暗い青 */
		0x84, 0x00, 0x84,	/* 13:暗い紫 */
		0x00, 0x84, 0x84,	/* 14:暗い水色 */
		0x84, 0x84, 0x84	/* 15:暗い灰色 */
	};
	unsigned char table2[216 * 3];
	int r, g, b;
	set_palette(0, 15, table_rgb);
	for (b = 0; b < 6; b++) {
		for (g = 0; g < 6; g++) {
			for (r = 0; r < 6; r++) {
				table2[(r + g * 6 + b * 36) * 3 + 0] = r * 51;
				table2[(r + g * 6 + b * 36) * 3 + 1] = g * 51;
				table2[(r + g * 6 + b * 36) * 3 + 2] = b * 51;
			}
		}
	}
	set_palette(16, 231, table2);
	return;
}

void set_palette(int start, int end, unsigned char *rgb)
{
	int i, eflags;
	eflags = io_load_eflags();	/* 割り込み許可フラグの値を記録する */
	io_cli(); 					/* 許可フラグを0にして割り込み禁止にする */
	io_out8(0x03c8, start);
	for (i = start; i <= end; i++) {
		io_out8(0x03c9, rgb[0] / 4);
		io_out8(0x03c9, rgb[1] / 4);
		io_out8(0x03c9, rgb[2] / 4);
		rgb += 3;
	}
	io_store_eflags(eflags);	/* 割り込み許可フラグを元に戻す */
	return;
}

void boxfill8(unsigned char *vram, int xsize, unsigned char c, int x0, int y0, int x1, int y1)
{
	int x, y;
	for (y = y0; y <= y1; y++) {
		for (x = x0; x <= x1; x++)
			vram[y * xsize + x] = c;
	}
	return;
}

void init_screen8(char *vram, int x, int y)
{
	boxfill8(vram, x, COL8_008484,  0,     0,      x -  1, y - 29);

	set_picture(vram, x, y);

	boxfill8(vram, x, COL8_C6C6C6,  0,     y - 28, x -  1, y - 28);
	boxfill8(vram, x, COL8_FFFFFF,  0,     y - 27, x -  1, y - 27);
	boxfill8(vram, x, COL8_C6C6C6,  0,     y - 26, x -  1, y -  1);
	
	boxfill8(vram, x, COL8_FFFFFF,  3,     y - 24, 59,     y - 24);
	boxfill8(vram, x, COL8_FFFFFF,  2,     y - 24,  2,     y -  4);
	boxfill8(vram, x, COL8_848484,  3,     y -  4, 59,     y -  4);
	boxfill8(vram, x, COL8_848484, 59,     y - 23, 59,     y -  5);
	boxfill8(vram, x, COL8_000000,  2,     y -  3, 59,     y -  3);
	boxfill8(vram, x, COL8_000000, 60,     y - 24, 60,     y -  3);

	
	putfonts8_asc(vram, x, 11, y - 22 , COL8_FF0000, "ｲﾋｵ･");
	return;
}

void putfont8(char *vram, int xsize, int x, int y, char c, char *font)
{
	int i;
	char *p, d /* data */;
	for (i = 0; i < 16; i++) {
		p = vram + (y + i) * xsize + x;
		d = font[i];
		if ((d & 0x80) != 0) { p[0] = c; }
		if ((d & 0x40) != 0) { p[1] = c; }
		if ((d & 0x20) != 0) { p[2] = c; }
		if ((d & 0x10) != 0) { p[3] = c; }
		if ((d & 0x08) != 0) { p[4] = c; }
		if ((d & 0x04) != 0) { p[5] = c; }
		if ((d & 0x02) != 0) { p[6] = c; }
		if ((d & 0x01) != 0) { p[7] = c; }
	}
	return;
}

void putfont8_ch(char *vram, int xsize, int x, int y, char c, char *font) {
	int i;
	char *p, d;
	for (i = 0; i < 16; i++) {
		p = vram + (y + i) * xsize + x;
		d = font[i * 2];
		if ((d & 0x80) != 0)
			p[0] = c;
		if ((d & 0x40) != 0)
			p[1] = c;
		if ((d & 0x20) != 0)
			p[2] = c;
		if ((d & 0x10) != 0)
			p[3] = c;
		if ((d & 0x08) != 0)
			p[4] = c;
		if ((d & 0x04) != 0)
			p[5] = c;
		if ((d & 0x02) != 0)
			p[6] = c;
		if ((d & 0x01) != 0)
			p[7] = c;
	}
	return;
}

void putfonts8_asc(char *vram, int xsize, int x, int y, char c, unsigned char *s)
{
	extern char hankaku[4096];
	struct TASK *task = task_now();
	char *nihongo = (char *) *((int *) 0x0fe8), *font;
	int k, t;

	if (task->langmode == 0) {
		for (; *s != 0x00; s++) {
			putfont8(vram, xsize, x, y, c, hankaku + *s * 16);
			x += 8;
		}
	}
	if (task->langmode == 1) {
		for (; *s != 0x00; s++) {
			if (task->langbyte1 == 0) {
				if ((0x81 <= *s && *s <= 0x9f) || (0xe0 <= *s && *s <= 0xfc)) {
					task->langbyte1 = *s;
				} else {
					putfont8(vram, xsize, x, y, c, nihongo + *s * 16);
				}
			} else {
				if (0x81 <= task->langbyte1 && task->langbyte1 <= 0x9f) {
					k = (task->langbyte1 - 0x81) * 2;
				} else {
					k = (task->langbyte1 - 0xe0) * 2 + 62;
				}
				if (0x40 <= *s && *s <= 0x7e) {
					t = *s - 0x40;
				} else if (0x80 <= *s && *s <= 0x9e) {
					t = *s - 0x80 + 63;
				} else {
					t = *s - 0x9f;
					k++;
				}
				task->langbyte1 = 0;
				font = nihongo + 256 * 16 + (k * 94 + t) * 32;
				putfont8(vram, xsize, x - 8, y, c, font     );	/* 左半分 */
				putfont8(vram, xsize, x    , y, c, font + 16);	/* 右半分 */
			}
			x += 8;
		}
	}
	if (task->langmode == 2) {
		for (; *s != 0x00; s++) {
			if (task->langbyte1 == 0) {
				if (0x81 <= *s && *s <= 0xfe) {
					task->langbyte1 = *s;
				} else {
					putfont8(vram, xsize, x, y, c, nihongo + *s * 16);
				}
			} else {
				k = task->langbyte1 - 0xa1;
				t = *s - 0xa1;
				task->langbyte1 = 0;
				font = nihongo + 256 * 16 + (k * 94 + t) * 32;
				putfont8(vram, xsize, x - 8, y, c, font     );	/* 左半分 */
				putfont8(vram, xsize, x    , y, c, font + 16);	/* 右半分 */
			}
			x += 8;
		}
	}
	if (task->langmode == 3) {
		for (; *s != 0x00; s++) {
			if (task->langbyte1 == 0) {
				if (*s >= 0xa0) {
					task->langbyte1 = *s;
				} else {
					putfont8(vram, xsize, x, y, c, hankaku + *s * 16);
				}
			} else {
				k = task->langbyte1 - 0xa0;
				t = *s - 0xa0;
				task->langbyte1 = 0;
				font = nihongo + ((k - 1) * 94 + (t - 1)) * 32;
				putfont8_ch(vram, xsize, x - 8, y, c, font);
				putfont8_ch(vram, xsize, x, y, c, font + 1);
			}
			x += 8;
		}
	}
	return;
}

void init_mouse_cursor8(char *mouse, char bc)
/* マウスカーソルを準備（16x16） */
{
	static char cursor[16][16] = {
		"**************..",
		"*OOOOOOOOOOO*...",
		"*OOOOOOOOOO*....",
		"*OOOOOOOOO*.....",
		"*OOOOOOOO*......",
		"*OOOOOOO*.......",
		"*OOOOOOO*.......",
		"*OOOOOOOO*......",
		"*OOOO**OOO*.....",
		"*OOO*..*OOO*....",
		"*OO*....*OOO*...",
		"*O*......*OOO*..",
		"**........*OOO*.",
		"*..........*OOO*",
		"............*OO*",
		".............***"
	};

	int x, y;

	for (y = 0; y < 16; y++) {
		for (x = 0; x < 16; x++) {
			if (cursor[y][x] == '*') {
				mouse[y * 16 + x] = COL8_000000;
			}
			if (cursor[y][x] == 'O') {
				mouse[y * 16 + x] = COL8_FFFFFF;
			}
			if (cursor[y][x] == '.') {
				mouse[y * 16 + x] = bc;
			}
		}
	}
	return;
}

void putblock8_8(char *vram, int vxsize, int pxsize,
	int pysize, int px0, int py0, char *buf, int bxsize)
{
	int x, y;
	for (y = 0; y < pysize; y++) {
		for (x = 0; x < pxsize; x++) {
			vram[(py0 + y) * vxsize + (px0 + x)] = buf[y * bxsize + x];
		}
	}
	return;
}

//start_status_switch()ｺｯﾊ?ﾓﾃﾓﾚﾇﾐｻｻｱｳｾｰﾍｼｲ聊ﾐstartｰｴﾅ･ｵﾄﾏﾔﾊｾﾗｴﾌｬ
void start_status_switch(unsigned int status,struct SHEET *sht){
	unsigned char *vram;
	int xsize, ysize;
	vram = sht->buf, xsize = sht->bxsize, ysize = sht->bysize;
	int color=COL8_FF0000;
	if(status == 0){			//startｰｴﾅ･ﾎｴｱｻｰｴﾏﾂ
			boxfill8(vram, xsize, COL8_FFFFFF, 3, ysize - 24, 59, ysize - 24);
			boxfill8(vram, xsize, COL8_FFFFFF, 2, ysize - 24, 2, ysize - 4);
			boxfill8(vram, xsize, COL8_848484, 3, ysize - 4, 59, ysize - 4);
			boxfill8(vram, xsize, COL8_848484, 59, ysize - 23, 59, ysize - 5);
			boxfill8(vram, xsize, COL8_000000, 2, ysize - 3, 59, ysize - 3);
			boxfill8(vram, xsize, COL8_000000, 60, ysize - 24, 60, ysize - 3);
			color = COL8_FF0000;
	}
	else{						//startｰｴﾅ･ｱｻｰｴﾏﾂ
			boxfill8(vram, xsize, COL8_000000, 3, ysize - 24, 59, ysize - 24);
			boxfill8(vram, xsize, COL8_000000, 2, ysize - 24, 2, ysize - 4);
			boxfill8(vram, xsize, COL8_848484, 3, ysize - 23, 59, ysize - 23);
			boxfill8(vram, xsize, COL8_848484, 3, ysize - 23, 3, ysize - 5);
			boxfill8(vram, xsize, COL8_FFFFFF, 2, ysize - 3, 59, ysize - 3);
			boxfill8(vram, xsize, COL8_FFFFFF, 60, ysize - 24, 60, ysize - 3);
			color = COL8_000000;
	}
	putfonts8_asc(vram, xsize, 11, ysize - 22, color,	"ｲﾋｵ･");
	sheet_refresh(sht, 1, ysize - 25, 61,	ysize - 2);
}


void init_menu(struct SHEET *sht){
	unsigned char *vram;
	int xsize, ysize, yskip;

	vram = sht->buf;
	xsize = sht->bxsize, ysize = sht->bysize;
	yskip = ysize / 6;

	boxfill8(vram, xsize, COL8_C6C6C6, 0, 0, xsize - 1, ysize - 1);	//ｱｳｾｰﾉｫｴｦﾀ・
	//boxfill8(vram, xsize, COL8_848484, 1, 1, xsize - 2, yskip-1);

	//ｱﾟｿ?ｴｦﾀ・
	boxfill8(vram, xsize, COL8_000000, 0, 0, xsize - 1, 0);
	boxfill8(vram, xsize, COL8_000000, 0, 0, 0, ysize - 1);
	boxfill8(vram, xsize, COL8_000000, 0, ysize - 1, xsize - 1, ysize - 1);
	boxfill8(vram, xsize, COL8_000000, xsize-1, 0, xsize - 1, ysize - 1);


	//ｲﾋｵ･ｸ?ﾑ｡ﾏ?ﾘ荳?ﾏﾟｴｦﾀ・
	boxfill8(vram, xsize, COL8_000000, 0, 1 * yskip, xsize - 1, 1 * yskip);
	boxfill8(vram, xsize, COL8_000000, 0, 2 * yskip, xsize - 1, 2 * yskip);
	boxfill8(vram, xsize, COL8_000000, 0, 3 * yskip, xsize - 1, 3 * yskip);
	boxfill8(vram, xsize, COL8_000000, 0, 4 * yskip, xsize - 1, 4 * yskip);
	boxfill8(vram, xsize, COL8_000000, 0, 5 * yskip, xsize - 1, 5 * yskip);

	putfonts8_asc(vram, xsize, 8, 7, COL8_000000,	"ｸ?ﾈﾋﾐﾅﾏ｢");
	putfonts8_asc(vram, xsize, 8, 7 + yskip * 1, COL8_000000,	"ｿﾘﾖﾆﾌｨ");
	putfonts8_asc(vram, xsize, 8, 7 + yskip * 2, COL8_000000,	"ｳﾌﾐ?   >");
	putfonts8_asc(vram, xsize, 8, 7 + yskip * 3, COL8_000000,	"ﾎﾄｼ?ｹﾜﾀ?");
	putfonts8_asc(vram, xsize, 8, 7 + yskip * 4, COL8_000000,	"ﾈﾎﾎ?ｹﾜﾀ?");
	putfonts8_asc(vram, xsize, 8, 7 + yskip * 5, COL8_000000,	"ｹﾘｻ?");

	sheet_refresh(sht, 0, 0, xsize - 1,	ysize - 1);
}

//struct SHEET *init_infomation(struct SHTCTL *shtctl){   //ｳ?ﾊｼｻｯ  ｸ?ﾈﾋﾐﾅﾏ｢  ﾍｼｲ?
//	unsigned char *buf_info;
//	struct SHEET *sht_info = sheet_alloc(shtctl);
//	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
//	buf_info = (unsigned char *) memman_alloc(memman, 300 * 200);
//	sheet_setbuf(sht_info, buf_info, 300, 200, -1);
//	sht_info->flags |= 0x40;				//0x40 ｱ?ﾊｾｸﾃﾍｼｲ?  ﾎｪ   ｲﾋｵ･  ﾑ｡ﾏ?ﾖｮﾒｻ
//	make_window8(sht_info->buf, sht_info->bxsize, sht_info->bysize, "ｸ?ﾈﾋﾐﾅﾏ｢", 0);
//	make_textbox8(sht_info, 8, 28, sht_info->bxsize - 16, sht_info->bysize - 36, COL8_FFFFFF);
//	putfonts8_asc(sht_info->buf, sht_info->bxsize, 8, 28, COL8_000000,	"ｰ狆ｶ:  ｼﾆｿﾆ1301");
//	putfonts8_asc(sht_info->buf, sht_info->bxsize, 8, 28 + 16, COL8_000000,	"ﾐﾕﾃ?:  ﾅ?ﾃ?ｷｪ ");
//	putfonts8_asc(sht_info->buf, sht_info->bxsize, 8, 28 + 32, COL8_000000,	"ﾑｧｺﾅ:  201308010116");
//	return sht_info;
//}

struct SHEET *init_fileManager(struct SHTCTL *shtctl){
	struct SHEET *sht_fm = sheet_alloc(shtctl);
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	unsigned char *buf_fm = (unsigned char *) memman_alloc_4k(memman, 780 * 500);
	struct FILEINFO *finfo = (struct FILEINFO *) (ADR_DISKIMG + 0x002600);
	int i, j, line = 0;
	char s[50];
	unsigned short year = 0, month = 0, day = 0, hour = 0, minute = 0, second = 0;

	sheet_setbuf(sht_fm, buf_fm, 780, 500, -1); /* ﾎｪﾍｼｲ翹靹ﾃｻｺｳ衂? */
	sht_fm->flags |= 0x40;				//ｲﾋｵ･ﾑ｡ﾏ?

	make_window8(sht_fm->buf, sht_fm->bxsize, sht_fm->bysize, "ﾎﾄｼ?ｹﾜﾀ?", 1);
	boxfill8(buf_fm, sht_fm->bxsize, COL8_FFFFFF, 6, 27, sht_fm->bxsize - 6, sht_fm->bysize - 6);


	for(i = 0; i < 49; i++){
		s[i] = 0;
	}
	for (i = 0; i < 224; i++) {
		if (finfo[i].name[0] == 0x00) {
			break;
		}
		if (finfo[i].name[0] != 0xe5) {
			if ((finfo[i].type & 0x18) == 0) {
				//ﾗｪｻｻ ﾈﾕﾆﾚ｡｢ﾊｱｼ?
				year = (finfo[i].date >> 9) + 1980;
				month = (finfo[i].date >> 5) & 15;
				day = finfo[i].date & 31;
				hour = finfo[i].time >> 11;
				minute = (finfo[i].time >> 5) & 63,
				second = (finfo[i].time & 31) * 2;

				sprintf(s, "filename.ext  %7d byte %4d/%02d/%02d %02d:%02d:%02d",
						finfo[i].size, year, month, day, hour, minute, second);
				for (j = 0; j < 8; j++) {
					s[j] = finfo[i].name[j];
				}
				s[ 9] = finfo[i].ext[0];
				s[10] = finfo[i].ext[1];
				s[11] = finfo[i].ext[2];


				if(line > 26){
					putfonts8_asc_sht(sht_fm, 6 + (sht_fm->bxsize / 2), (line - 27) * 16 + 27, COL8_000000, COL8_FFFFFF, s, 45);
				}
				else{
					putfonts8_asc_sht(sht_fm, 6, line * 16 + 27, COL8_000000, COL8_FFFFFF, s, 45);
				}
				line++;
			}
		}
	}
	return sht_fm;
}

int set_picture(unsigned char *vram, int x, int y)
{
	int i, j, x0, y0, fsize, info[4];
	unsigned char *filebuf, r, g, b;
	struct RGB *picbuf;
	struct MEMMAN *memman  = (struct MEMMAN *) MEMMAN_ADDR;
	struct BOOTINFO *binfo = (struct BOOTINFO *) ADR_BOOTINFO;
	struct FILEINFO *finfo;
	struct DLL_STRPICENV *env;
	int *fat;

	fat = (int *) memman_alloc_4k(memman, 4 * 2880);
	file_readfat(fat, (unsigned char *) (ADR_DISKIMG + 0x000200));
	//finfo = file_search("RAPUWP.jpg", (struct FILEINFO *) (ADR_DISKIMG + 0x002600), 224);
	finfo = file_search("image.jpg", (struct FILEINFO *) (ADR_DISKIMG + 0x002600), 224);
	if (finfo == 0 || (finfo->type & 0x18) != 0) {
		boxfill8(vram, x, COL8_FF0000, 100, 100, 300, 400);
		return -1;
	}
	fsize   = finfo->size;
	filebuf = (unsigned char *) memman_alloc_4k(memman, fsize);
	filebuf = file_loadfile2(finfo->clustno, &fsize, fat);

	env = (struct DLL_STRPICENV *) memman_alloc_4k(memman, sizeof(struct DLL_STRPICENV));
	if(info_JPEG(env, info, fsize, filebuf) == 0){
		return -1;			//		ｲｻﾊﾇjpegﾎﾄｼ?
	}
	picbuf  = (struct RGB *) memman_alloc_4k(memman, info[2] * info[3] * sizeof(struct RGB));
	decode0_JPEG(env, fsize, filebuf, 4, (unsigned char *) picbuf, 0);

	x0 = (int) ((x - info[2]) / 2);
	y0 = (int) ((y - info[3]) / 2);
	for (i = 0; i < info[3]; i++) {
		for (j = 0; j < info[2]; j++) {
			r = picbuf[i * info[2] + j].r;
			g = picbuf[i * info[2] + j].g;
			b = picbuf[i * info[2] + j].b;
			vram[(y0 + i) * x + (x0 + j)] =(unsigned char) rgb2pal(r, g, b, j, i, binfo->vmode);
		}
	}

	memman_free_4k(memman, (int) filebuf, fsize);
	memman_free_4k(memman, (int) picbuf , info[2] * info[3] * sizeof(struct RGB));
	memman_free_4k(memman, (int) env    , sizeof(struct DLL_STRPICENV));
	memman_free_4k(memman, (int) fat, 4 * 2880);
	return 0;
}

unsigned char rgb2pal(int r, int g, int b, int x, int y, int cb)
{
	if (cb == 8) {
		static	int table[4] = { 3, 1, 0, 2 };
		int		i;
		x &= 1;					/* 偶数か奇数か */
		y &= 1;
		i = table[x + y * 2];	/* 中間色を作るための定数 */
		r = (r * 21) / 256;		/* これで 0〜20 になる */
		g = (g * 21) / 256;
		b = (b * 21) / 256;
		r = (r +  i) /   4;		/* これで 0〜5 になる */
		g = (g +  i) /   4;
		b = (b +  i) /   4;
		return((unsigned char) (16 + r + g * 6 + b * 36));
	} else {
		return((unsigned char) (((r << 8) & 0xf800) | ((g << 3) & 0x07e0) | (b >> 3)));
	}
}
