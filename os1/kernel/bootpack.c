/* bootpackのメイン */

#include "bootpack.h"
#include <stdio.h>
#include <string.h>

#define KEYCMD_LED		0xed

int *u_fat;
char cpath[256], epath[1024];
struct TASK *fdc, *inout, *taskmgr, *filetask;

void keywin_off(struct SHEET *key_win);
void close_console(struct SHEET *sht);
void close_constask(struct TASK *task);
void close_taskmgr(void);
void init_menu(struct MNLV *mnlv, struct MENU **menu);
unsigned short *buf_back2;

void task_filetask(unsigned int memtotal);
void open_filetask(unsigned int memtotal);

struct CURSORBAR {
	int x,x1;
	int y[10];
	char *name;
};
struct MOUSEBLACK {
	int x,y;
	char *name;
	int xsize;
};
int mx, my;
struct MOUSE_DEC mdec;


void HariMain(void)
{
	struct BOOTINFO *binfo = (struct BOOTINFO *) ADR_BOOTINFO;
	struct SHTCTL *shtctl;
	char s[40];
	struct FIFO32 fifo, keycmd;
	int fifobuf[128], keycmd_buf[32];
	int i, new_mx = -1, new_my = 0, new_wx = 0x7fffffff, new_wy = 0;
	int pre_key = 0;
	unsigned int memtotal;
	
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	unsigned short *buf_back, buf_mouse[256];
	struct SHEET *sht_back, *sht_mouse;
	struct TASK *task_a, *task;
	static char keytable0[0x80] = {
		0,   0,   '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '^', 0x08, 0,
		'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '@', '[', 0x0a, 0, 'A', 'S',
		'D', 'F', 'G', 'H', 'J', 'K', 'L', ';', ':', 0,   0,   ']', 'Z', 'X', 'C', 'V',
		'B', 'N', 'M', ',', '.', '/', 0,   '*', 0,   ' ', 0,   0,   0,   0,   0,   0,
		0,   0,   0,   0,   0,   0,   0,   '7', '8', '9', '-', '4', '5', '6', '+', '1',
		'2', '3', '0', '.', 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
		0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
		0,   0,   0,   0x5c, 0,  0,   0,   0,   0,   0,   0,   0,   0,   0x5c, 0,  0
	};
	static char keytable1[0x80] = {
		0,   0,   '!', 0x22, '#', '$', '%', '&', 0x27, '(', ')', '~', '=', '~', 0x08, 0,
		'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '`', '{', 0x0a, 0, 'A', 'S',
		'D', 'F', 'G', 'H', 'J', 'K', 'L', '+', '*', 0,   0,   '}', 'Z', 'X', 'C', 'V',
		'B', 'N', 'M', '<', '>', '?', 0,   '*', 0,   ' ', 0,   0,   0,   0,   0,   0,
		0,   0,   0,   0,   0,   0,   0,   '7', '8', '9', '-', '4', '5', '6', '+', '1',
		'2', '3', '0', '.', 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
		0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
		0,   0,   0,   '_', 0,   0,   0,   0,   0,   0,   0,   0,   0,   '|', 0,   0
	};
	int key_shift = 0, key_leds = (binfo->leds >> 4) & 7, keycmd_wait = -1;
	int j, x, y, mmx = -1, mmy = -1, mmx2 = 0, fw_flg = 0, mn_flg = -1, rc;
	struct MENU *menu;
	struct MNLV mnlv[MAX_MNLV];
	struct SHEET *sht = 0, *key_win, *sht2;
	unsigned char *nihongo;
	struct FILEINFO *finfo;
	extern char hankaku[4096];
	
	init_gdtidt();
	init_pic();
	io_sti(); /* IDT/PICの初期化が終わったのでCPUの割り込み禁止を解除 */
	fifo32_init(&fifo, 128, fifobuf, 0);
	*((int *) 0x0fec) = (int) &fifo;
	init_pit();
	init_keyboard(&fifo, 256);
	enable_mouse(&fifo, 512, &mdec);
	io_out8(PIC0_IMR, 0xb8); /* FDCとPITとPIC1とキーボードを許可(10111000) */
	io_out8(PIC1_IMR, 0xef); /* マウスを許可(11101111) */
	fifo32_init(&keycmd, 32, keycmd_buf, 0);

	memtotal = memtest(0x00400000, 0xbfffffff);
	memman_init(memman);
	memman_free(memman, 0x00001000, 0x0009e000); /* 0x00001000 - 0x0009efff */
	memman_free(memman, 0x00400000, memtotal - 0x00400000);

	init_palette();
	shtctl = shtctl_init(memman, binfo->vram, binfo->scrnx, binfo->scrny);
	task_a = task_init(memman);
	fifo.task = task_a;
	task_run(task_a, 1, 2);
	*((int *) 0x0fe4) = (int) shtctl;
	task_a->langmode = 0;

	/* PATHの初期化 */
	strcpy(cpath, "/");
	strcpy(epath, "/");

	/* FATの展開、テーブル初期化 */
	u_fat = (int *) memman_alloc_4k(memman, 4 * 2880);
	file_readfat(u_fat, (unsigned char *) (ADR_DISKIMG + 0x000200));
	file_inittbl();

	/* sht_back */
	sht_back  = sheet_alloc(shtctl);
	buf_back  = (unsigned short *) memman_alloc_4k(memman, binfo->scrnx * binfo->scrny * 2);
	buf_back2  = (unsigned short *) memman_alloc_4k(memman, binfo->scrnx * binfo->scrny * 2);
	sheet_setbuf(sht_back, buf_back, binfo->scrnx, binfo->scrny, -1); /* 透明色なし */
	init_screen8(buf_back, binfo->scrnx, binfo->scrny);
	
	/* sht_cons */
	key_win = open_console(shtctl, memtotal);

	/* sht_mouse */
	sht_mouse = sheet_alloc(shtctl);
	sheet_setbuf(sht_mouse, buf_mouse, 16, 16, 99);
	init_mouse_cursor8(buf_mouse, 99);
	mx = (binfo->scrnx - 16) / 2; /* 画面中央になるように座標計算 */
	my = (binfo->scrny - 28 - 16) / 2;

	sheet_slide(sht_back,  0, 0);
	sheet_slide(key_win,   8, 8);
	sheet_slide(sht_mouse, mx, my);
	sheet_updown(sht_back,  0);
	sheet_updown(key_win,   1);
	sheet_updown(sht_mouse, 2);
	keywin_on(key_win);

	/* 最初にキーボード状態との食い違いがないように、設定しておくことにする */
	io_cli();
	fifo32_put(&keycmd, KEYCMD_LED);
	fifo32_put(&keycmd, key_leds);
	io_sti();

	/* nihongo.fntの読み込み */
	finfo = file_search2("/nihongo.fnt",
			(struct FILEINFO *) (ADR_DISKIMG + 0x002600), 224, u_fat);
	if (finfo != 0 && (finfo->type & 0x18) == 0) {
		i = finfo->size;
		nihongo = file_loadfile2(finfo->clustno, &i, u_fat);
	} else {
		nihongo = (unsigned char *) memman_alloc_4k(memman, 16 * 256 + 32 * 94 * 47);
		for (i = 0; i < 16 * 256; i++) {
			nihongo[i] = hankaku[i]; /* フォントがなかったので半角部分をコピー */
		}
		for (i = 16 * 256; i < 16 * 256 + 32 * 94 * 47; i++) {
			nihongo[i] = 0xff; /* フォントがなかったので全角部分を0xffで埋め尽くす */
		}
	}
	*((int *) 0x0fe8) = (int) nihongo;

	/* menuの初期化 */
	init_menu(mnlv, &menu);

	struct TASK *clock = task_alloc();
	int *clock_fifo = (int *) memman_alloc_4k(memman, 128 * 4);
	clock->tss.esp = memman_alloc_4k(memman, 64 * 1024) + 64 * 1024;
	clock->tss.eip = (int) &sysclock_task;
	clock->tss.es = 1 * 8;
	clock->tss.cs = 2 * 8;
	clock->tss.ss = 1 * 8;
	clock->tss.ds = 1 * 8;
	clock->tss.fs = 1 * 8;
	clock->tss.gs = 1 * 8;
	clock->time = 0;
	strcpy(clock->name, "sysclock");
	task_run(clock, 1, 2); /* level=1, priority=2 */
	fifo32_init(&clock->fifo, 128, clock_fifo, clock);

	struct TASK *scrsvr = task_alloc();
	int *scr_fifo = (int *) memman_alloc_4k(memman, 128 * 4);
	scrsvr->tss.esp = memman_alloc_4k(memman, 64 * 1024) + 64 * 1024 - 12;
	scrsvr->tss.eip = (int) &scrsaver_task;
	scrsvr->tss.es = 1 * 8;
	scrsvr->tss.cs = 2 * 8;
	scrsvr->tss.ss = 1 * 8;
	scrsvr->tss.ds = 1 * 8;
	scrsvr->tss.fs = 1 * 8;
	scrsvr->tss.gs = 1 * 8;
	scrsvr->time = 0;
	strcpy(scrsvr->name, "scrsaver");
	*((int *) (scrsvr->tss.esp + 4)) = (int) sht_mouse;
	*((int *) (scrsvr->tss.esp + 8)) = memtotal;
	task_run(scrsvr, 2, 2); /* level=2, priority=2 */
	fifo32_init(&scrsvr->fifo, 128, scr_fifo, scrsvr);

	fdc = task_alloc();
	int *fdc_fifo = (int *) memman_alloc_4k(memman, 2880 * 4 * 4);
	fdc->tss.esp = memman_alloc_4k(memman, 64 * 1024) + 64 * 1024;
	fdc->tss.eip = (int) &fdc_task;
	fdc->tss.es = 1 * 8;
	fdc->tss.cs = 2 * 8;
	fdc->tss.ss = 1 * 8;
	fdc->tss.ds = 1 * 8;
	fdc->tss.fs = 1 * 8;
	fdc->tss.gs = 1 * 8;
	fdc->time = 0;
	strcpy(fdc->name, "fdc");
	task_run(fdc, 2, 2); /* level=2, priority=2 */
	fifo32_init(&fdc->fifo, 2880 * 4, fdc_fifo, fdc);

	inout = task_alloc();
	int *inout_fifo = (int *) memman_alloc_4k(memman, 128 * 4);
	inout->tss.esp = memman_alloc_4k(memman, 64 * 1024) + 64 * 1024 - 8;
	inout->tss.eip = (int) &inout_task;
	inout->tss.es = 1 * 8;
	inout->tss.cs = 2 * 8;
	inout->tss.ss = 1 * 8;
	inout->tss.ds = 1 * 8;
	inout->tss.fs = 1 * 8;
	inout->tss.gs = 1 * 8;
	inout->time = 0;
	strcpy(inout->name, "ioctrl");
	*((int *) (inout->tss.esp + 4)) = (int) sht_back;
	task_run(inout, 2, 2); /* level=2, priority=2 */
	fifo32_init(&inout->fifo, 128, inout_fifo, inout);

	for (;;) {
		if (fifo32_status(&keycmd) > 0 && keycmd_wait < 0) {
			/* キーボードコントローラに送るデータがあれば、送る */
			keycmd_wait = fifo32_get(&keycmd);
			wait_KBC_sendready();
			io_out8(PORT_KEYDAT, keycmd_wait);
		}
		io_cli();
		if (fifo32_status(&fifo) == 0) {
			/* FIFOがからっぽになったので、保留している描画があれば実行する */
			if (new_mx >= 0) {
				io_sti();
				sheet_slide(sht_mouse, new_mx, new_my);
				new_mx = -1;
			} else if (new_wx != 0x7fffffff) {
				io_sti();
				sheet_slide(sht, new_wx, new_wy);
				new_wx = 0x7fffffff;
			} else {
				task_sleep(task_a);
				io_sti();
			}
		} else {
			i = fifo32_get(&fifo);
			io_sti();
			if (shtctl->key_win != 0 &&
				(shtctl->key_win->flags == 0 || shtctl->key_win->height == -100)) {
				/* ウィンドウが閉じられたまたは最小化された */
				if (shtctl->top == 1) {	/* もうマウスと背景しかない */
					shtctl->key_win = 0;
				} else {
					key_win = shtctl->sheets[shtctl->top - 1];
					keywin_on(key_win);
				}
			}
			if (i == 256 + 0xe0) {	/* pre_key */
				pre_key = i - 256;
				continue;
			} else {
				if (pre_key == 0xe0) {
					i += 4096;
				}
				pre_key = 0;
			}
			if (256 <= i && i <= 511 || 4352 <= i && i <= 4607) { /* キーボードデータ */
				if (i == 4352 + 0x37) {	/* PrintScreen */
					bmp_conv(640, 480, (unsigned short *) shtctl->vram);
					
				}
				fifo32_put_io(&scrsvr->fifo, (i & 0x01ff));
				if (mn_flg != -1) {
					for (; mn_flg > -1;) {
						mn_flg = close_menu(shtctl->sheets[0], &mnlv[mn_flg]);
					}
				}
				if (i < 0x80 + 256) { /* キーコードを文字コードに変換 */
					if (key_shift == 0) {
						s[0] = keytable0[i - 256];
					} else {
						s[0] = keytable1[i - 256];
					}
				} else {
					s[0] = 0;
				}
				if ('A' <= s[0] && s[0] <= 'Z') {	/* 入力文字がアルファベット */
					if (((key_leds & 4) == 0 && key_shift == 0) ||
							((key_leds & 4) != 0 && key_shift != 0)) {
						s[0] += 0x20;	/* 大文字を小文字に変換 */
					}
				}
				if (s[0] != 0 && shtctl->key_win != 0) { /* 通常文字、バックスペース、Enter */
					fifo32_put_io(&shtctl->key_win->task->fifo, s[0] + 256);
				}
				if (i == 256 + 0x0f && shtctl->key_win != 0) {	/* Tab */
					j = shtctl->key_win->height - 1;
					if (j == 0) {
						j = shtctl->top - 1;
					}
					key_win = shtctl->sheets[j];
					keywin_on(key_win);
				}
				if (i == 256 + 0x2a) {	/* 左シフト ON */
					key_shift |= 1;
				}
				if (i == 256 + 0x36) {	/* 右シフト ON */
					key_shift |= 2;
				}
				if (i == 256 + 0xaa) {	/* 左シフト OFF */
					key_shift &= ~1;
				}
				if (i == 256 + 0xb6) {	/* 右シフト OFF */
					key_shift &= ~2;
				}
				if (i == 256 + 0x3a) {	/* CapsLock */
					key_leds ^= 4;
					io_cli();
					fifo32_put(&keycmd, KEYCMD_LED);
					fifo32_put(&keycmd, key_leds);
					io_sti();
				}
				if (i == 256 + 0x45) {	/* NumLock */
					key_leds ^= 2;
					io_cli();
					fifo32_put(&keycmd, KEYCMD_LED);
					fifo32_put(&keycmd, key_leds);
					io_sti();
				}
				if (i == 256 + 0x46) {	/* ScrollLock */
					key_leds ^= 1;
					io_cli();
					fifo32_put(&keycmd, KEYCMD_LED);
					fifo32_put(&keycmd, key_leds);
					io_sti();
				}
				if (i == 256 + 0x3b && key_shift != 0 && shtctl->key_win != 0) {	/* Shift+F1 */
					task = shtctl->key_win->task;
					if (task != 0 && task->tss.ss0 != 0) {
						cons_putstr0(task->cons, "\nBreak(key) :\n");
						io_cli();	/* 強制終了処理中にタスクが変わると困るから */
						task->tss.eax = (int) &(task->tss.esp0);
						task->tss.eip = (int) asm_end_app;
						io_sti();
						task_run(task, -1, 0);	/* 終了処理を確実にやらせるために、寝ていたら起こす */
					}
				}
				if (i == 256 + 0x3c && key_shift != 0) {	/* Shift+F2 */
					/* 新しく作ったコンソールを入力選択状態にする（そのほうが親切だよね？） */
					key_win = open_console(shtctl, memtotal);
					sheet_slide(key_win, 8, 8);
					sheet_updown(key_win, shtctl->top);
					keywin_on(key_win);
				}
				if (i == 256 + 0x57) {	/* F11 */
					if (shtctl->top > 2) {
						sheet_updown(shtctl->sheets[1], shtctl->top - 1);
					}
				}
				if (i == 256 + 0x58) {	/* F12 */
					sht = sheet_view(shtctl);
					if (sht != 0) {
						key_win = sht;
						keywin_on(key_win);
					}
				}
				if (i == 256 + 0xfa) {	/* キーボードがデータを無事に受け取った */
					keycmd_wait = -1;
				}
				if (i == 256 + 0xfe) {	/* キーボードがデータを無事に受け取れなかった */
					wait_KBC_sendready();
					io_out8(PORT_KEYDAT, keycmd_wait);
				}
			} else if (512 <= i && i <= 767) { /* マウスデータ */
				fifo32_put_io(&scrsvr->fifo, i);
				if (mouse_decode(&mdec, i - 512) != 0) {
					/* マウスカーソルの移動 */
					mx += mdec.x;
					my += mdec.y;
					if (mx < 0) {
						mx = 0;
					}
					if (my < 0) {
						my = 0;
					}
					if (mx > binfo->scrnx - 1) {
						mx = binfo->scrnx - 1;
					}
					if (my > binfo->scrny - 1) {
						my = binfo->scrny - 1;
					}
					new_mx = mx;
					new_my = my;
					if ((mdec.btn & 0x01) != 0) {
						/* 左ボタンを押している */
						if (mmx < 0) {
							/* 通常モードの場合 */
							if (mn_flg != -1) {
								rc = exec_menu(&mnlv[mn_flg], memtotal);
								if (rc == -1) {
									for (; mn_flg > -1;) {
										mn_flg = close_menu(shtctl->sheets[0], &mnlv[mn_flg]);
									}
								} else {
									mn_flg = rc;
								}
								continue;
							}
							/* 上の下じきから順番にマウスが指している下じきを探す */
							fw_flg = 0;
							for (j = shtctl->top - 1; j > 0; j--) {
								sht = shtctl->sheets[j];
								x = mx - sht->vx0;
								y = my - sht->vy0;
								if (0 <= x && x < sht->bxsize && 0 <= y && y < sht->bysize) {
									if ((sht->flags & 0x100) == 0 &&
										sht->buf[y * sht->bxsize + x] != sht->col_inv) {
										sheet_updown(sht, shtctl->top - 1);
										if (sht != shtctl->key_win) {
											key_win = sht;
											keywin_on(key_win);
										}
										if ((sht->flags & 0x100) == 0 &&
											3 <= x && x < sht->bxsize - 3 && 3 <= y && y < 21) {
											mmx = mx;	/* ウィンドウ移動モードへ */
											mmy = my;
											mmx2 = sht->vx0;
											new_wy = sht->vy0;
										}
										if ((sht->flags & 0x100) == 0 &&
											sht->bxsize - 23 <= x && x < sht->bxsize - 5 && 5 <= y && y < 19) {
											/* 「×」ボタンクリック */
											if ((sht->flags & 0x10) != 0) {		/* アプリが作ったウィンドウか？ */
												task = sht->task;
												cons_putstr0(task->cons, "\nBreak(mouse) :\n");
												io_cli();	/* 強制終了処理中にタスクが変わると困るから */
												task->tss.eax = (int) &(task->tss.esp0);
												task->tss.eip = (int) asm_end_app;
												io_sti();
												task_run(task, -1, 0);
											} else {	/* コンソール */
												task = sht->task;
												sheet_updown(sht, -1); /* とりあえず非表示にしておく */
												key_win = shtctl->sheets[shtctl->top - 1];
												keywin_on(key_win);
												fifo32_put_io(&task->fifo, 4);
											}
										}
										if ((sht->flags & 0x100) == 0 &&
											sht->bxsize - x - 32 < y && y < sht->bxsize  - x - 17 && 5 < y && y < 16) {
											sheet_updown(sht, -100);
										}
										fw_flg = (int) sht;
										break;
									}
								}
							}
							if (fw_flg == 0 && mn_flg == -1) {
								sht = shtctl->sheets[0];
								if (2 <= mx && mx <= 60 && sht->bysize - 24 <= my && my <= sht->bysize - 3) {
									mn_flg = open_menu(sht, mnlv, menu);
								}
							}
						} else {
							/* ウィンドウ移動モードの場合 */
							x = mx - mmx;	/* マウスの移動量を計算 */
							y = my - mmy;
							new_wx = (mmx2 + x + 2) & ~3;
							new_wy = new_wy + y;
							mmy = my;	/* 移動後の座標に更新 */
						}
					} else {
						/* 左ボタンを押していない */
						mmx = -1;	/* 通常モードへ */
						if (new_wx != 0x7fffffff) {
							sheet_slide(sht, new_wx, new_wy);	/* 一度確定させる */
							new_wx = 0x7fffffff;
						} else if (mn_flg != -1) {
							x = mx - mnlv[mn_flg].sht->vx0;
							y = my - mnlv[mn_flg].sht->vy0;
							select_menu(&mnlv[mn_flg], x, y);
						}
					}
					if ((mdec.btn & 0x02) != 0) {
						/* 右ボタンを押している */
						if (mmx < 0) {
							/* 通常モードの場合 */
							if (mn_flg != -1) {
								
								sht = mnlv[mn_flg].sht;
								x = mx - sht->vx0;
								y = my - sht->vy0;
								if (0 <= x && x < sht->bxsize && 0 <= y && y < sht->bysize) {
									mn_flg = close_menu(shtctl->sheets[0], &mnlv[mn_flg]);
								}
								continue;
							}									
						}
						
					}
				}
			} else if (768 <= i && i <= 1023) {	/* コンソール終了処理 */
				close_console(shtctl->sheets0 + (i - 768));
			} else if (1024 <= i && i <= 2023) {
				close_constask(taskctl->tasks0 + (i - 1024));
			} else if (2024 <= i && i <= 2279) {	/* コンソールだけを閉じる */
				sht2 = shtctl->sheets0 + (i - 2024);
				memman_free_4k(memman, (int) sht2->buf, 256 * 165);
				sheet_free(sht2);
			} else if (i == 2280) {	/* タスクマネージャを閉じる */
				close_taskmgr();
			}
		}
	}
}

void keywin_off(struct SHEET *key_win)
{
	change_wtitle8(key_win, 0);
	if ((key_win->flags & 0x20) != 0) {
		fifo32_put_io(&key_win->task->fifo, 3); /* コンソールのカーソルOFF */
	}
	return;
}

void keywin_on(struct SHEET *key_win)
{
	struct SHTCTL *ctl = (struct SHTCTL *) *((int *) 0x0fe4);

	if (ctl->key_win != 0) {
		keywin_off(ctl->key_win);
	}
	change_wtitle8(key_win, 1);
	if ((key_win->flags & 0x20) != 0) {
		fifo32_put_io(&key_win->task->fifo, 2); /* コンソールのカーソルON */
	}
	ctl->key_win = key_win;

	return;
}

struct TASK *open_constask(struct SHEET *sht, unsigned int memtotal)
{
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	struct TASK *task = task_alloc();
	int *cons_fifo = (int *) memman_alloc_4k(memman, 128 * 4);
	task->cons_stack = memman_alloc_4k(memman, 64 * 1024);
	task->tss.esp = task->cons_stack + 64 * 1024 - 12;
	task->tss.eip = (int) &console_task;
	task->tss.es = 1 * 8;
	task->tss.cs = 2 * 8;
	task->tss.ss = 1 * 8;
	task->tss.ds = 1 * 8;
	task->tss.fs = 1 * 8;
	task->tss.gs = 1 * 8;
	task->time = 0;
	strcpy(task->name, "console");
	*((int *) (task->tss.esp + 4)) = (int) sht;
	*((int *) (task->tss.esp + 8)) = memtotal;
	task_run(task, 2, 2); /* level=2, priority=2 */
	fifo32_init(&task->fifo, 128, cons_fifo, task);
	return task;
}

struct SHEET *open_console(struct SHTCTL *shtctl, unsigned int memtotal)
{
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	struct SHEET *sht = sheet_alloc(shtctl);
	unsigned short *buf = (unsigned short *) memman_alloc_4k(memman, 256 * 165 * 2);
	sheet_setbuf(sht, buf, 256, 165, -1); /* 透明色なし */
	make_window8(buf, 256, 165, "console", 0);
	make_textbox8(sht, 5, 24, 250, 159, COL8_000000);
	sht->task = open_constask(sht, memtotal);
	sht->flags |= 0x20;	/* カーソルあり */
	return sht;
}

void close_constask(struct TASK *task)
{
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	task_sleep(task);
	memman_free_4k(memman, task->cons_stack, 64 * 1024);
	memman_free_4k(memman, (int) task->fifo.buf, 128 * 4);
	task_free(task);
	return;
}

void close_console(struct SHEET *sht)
{
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	struct TASK *task = sht->task;
	memman_free_4k(memman, (int) sht->buf, 256 * 165 * 2);
	sheet_free(sht);
	close_constask(task);
	return;
}

void open_taskmgr(unsigned int memtotal)
{
	
	
	int i;
	struct SHEET *sht;
	struct SHTCTL *ctl = (struct SHTCTL *) *((int *) 0x0fe4);
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	if (taskmgr != 0) {
		for (i = 0; i < MAX_SHEETS; i++) {
			sht = &(ctl->sheets0[i]);
			if (sht->task == taskmgr) {
				keywin_on(sht);
			}
		}
		return;
	}
	taskmgr = task_alloc();
	int *tmgr_fifo = (int *) memman_alloc_4k(memman, 128 * 4);
	taskmgr->tss.esp = memman_alloc_4k(memman, 64 * 1024) + 64 * 1024 - 8;
	taskmgr->tss.eip = (int) &taskmgr_task;
	taskmgr->tss.es = 1 * 8;
	taskmgr->tss.cs = 2 * 8;
	taskmgr->tss.ss = 1 * 8;
	taskmgr->tss.ds = 1 * 8;
	taskmgr->tss.fs = 1 * 8;
	taskmgr->tss.gs = 1 * 8;
	taskmgr->time = 0;
	strcpy(taskmgr->name, "taskmgr");
	*((int *) (taskmgr->tss.esp + 4)) = memtotal;
	task_run(taskmgr, 2, 2); /* level=2, priority=2 */
	fifo32_init(&taskmgr->fifo, 128, tmgr_fifo, taskmgr);
	return;
}
void open_filetask(unsigned int memtotal)
{
	int i;
	struct SHEET *sht;
	struct SHTCTL *ctl = (struct SHTCTL *) *((int *) 0x0fe4);
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	if (filetask != 0) {
		for (i = 0; i < MAX_SHEETS; i++) {
			sht = &(ctl->sheets0[i]);
			if (sht->task == filetask) {
				keywin_on(sht);
			}
		}
		return;
	}
	filetask = task_alloc();
	int *file_fifo = (int *) memman_alloc_4k(memman, 128 * 4);
	filetask->tss.esp = memman_alloc_4k(memman, 64 * 1024) + 64 * 1024 - 8;
	filetask->tss.eip = (int) &task_filetask;
	filetask->tss.es = 1 * 8;
	filetask->tss.cs = 2 * 8;
	filetask->tss.ss = 1 * 8;
	filetask->tss.ds = 1 * 8;
	filetask->tss.fs = 1 * 8;
	filetask->tss.gs = 1 * 8;
	filetask->time = 0;
	strcpy(filetask->name, "filetask");
	*((int *) (filetask->tss.esp + 4)) = memtotal;
	task_run(filetask, 2, 2); /* level=2, priority=2 */
	fifo32_init(&filetask->fifo, 128, file_fifo, filetask);
	return;
}
void close_taskmgr(void)
{
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	task_sleep(taskmgr);
	memman_free_4k(memman, (int) taskmgr->fifo.buf, 128 * 4);
	task_free(taskmgr);
	taskmgr = 0;
	return;
}
void close_filetask(void)
{
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	task_sleep(filetask);
	memman_free_4k(memman, (int) filetask->fifo.buf, 128 * 4);
	task_free(filetask);
	filetask = 0;
	return;
}

int open_menu(struct SHEET *sht, struct MNLV *omlv, struct MENU *menu)
{
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	struct SHTCTL *ctl = (struct SHTCTL *) *((int *) 0x0fe4);
	int xsize = 144, ysize, num_menu, x, y, yp;

	for (num_menu = 0; num_menu < MAX_MENU; num_menu++) {
		if (menu[num_menu].next == 0) {
			break;
		}
	}
	num_menu++;

	if (menu->level == 0) {
		push_menu(sht->buf, sht->bxsize, sht->bysize);
		sheet_refresh(sht, 2, sht->bysize - 24, 61, sht->bysize - 2);
	}

	ysize = num_menu * 24 + (num_menu - 1) * 2 + 4;
	if(menu->level == 0) {
		ysize += 18;
	}
	omlv->sht = sheet_alloc(ctl);
	omlv->sht->flags |= 0x100;
	omlv->buf = (unsigned short *) memman_alloc_4k(memman, xsize * ysize * 2);
	sheet_setbuf(omlv->sht, omlv->buf, xsize, ysize, -1);	/* 透明色なし */
	make_menu8(omlv->buf, xsize, ysize, "Panda menu", menu, num_menu);

	if(menu->level == 0) {
		x = 0;
		y = ctl->ysize - (ysize + 28);
	} else {
		x  = (xsize - 4) * menu->level;
		yp = (omlv - 1)->pos * 26 + 2;
		if (menu->level == 1) {
			yp += 18;
		}
		y  = (omlv - 1)->sht->vy0 + yp - ysize;
	}
	sheet_slide(omlv->sht, x, y);
	sheet_updown(omlv->sht, ctl->top);
	omlv->menu = menu;
	omlv->pos = 0;
	omlv->num = num_menu;

	return menu->level;
}

int close_menu(struct SHEET *sht, struct MNLV *cmlv)
{
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;

	sheet_updown(cmlv->sht, -1);
	memman_free_4k(memman, (int) cmlv->buf, cmlv->sht->bxsize * cmlv->sht->bysize * 2);
	sheet_free(cmlv->sht);

	if (cmlv->menu->level == 0) {
		pull_menu(sht->buf, sht->bxsize, sht->bysize);
		sheet_refresh(sht, 2, sht->bysize - 24, 61, sht->bysize - 2);
	}

	return cmlv->menu->level - 1;
}

void select_menu(struct MNLV *smlv, int x, int y)
{
	int yp = 1;

	if (smlv->pos != 0) {
		change_mtitle8(smlv->sht, smlv->menu->level, smlv->pos, 0);
	}
	if (smlv->menu->level == 0) {
		yp += 18;
	}
	if (2 <= x && x < smlv->sht->bxsize - 2 && yp < y && y < smlv->sht->bysize - 2) {
		smlv->pos = (int) ((y - yp) / 26) + 1;
		change_mtitle8(smlv->sht, smlv->menu->level, smlv->pos, 1);
	} else {
		smlv->pos = 0;
	}

	return;
}

int exec_menu(struct MNLV *emlv, unsigned int memtotal)
{
	struct SHTCTL *ctl = (struct SHTCTL *) *((int *) 0x0fe4);
	struct MENU *menu;
	struct TASK *task;
	struct SHEET *key_win;
	struct FIFO32 *fifo;
	int i, end = emlv->num - emlv->pos;

	if (emlv->pos == 0) {
		return -1;
	}

	menu = emlv->menu;
	for (i = 0; i < end; i++) {
		menu = menu->next;
	}

	if (strcmp(menu->exec, "<submenu>") == 0) {
		return open_menu(0, emlv + 1, menu->sub);
	} else if (strcmp(menu->exec, "<console>") == 0) {
		key_win = open_console(ctl, memtotal);
		sheet_slide(key_win, 8, 8);
		sheet_updown(key_win, ctl->top);
		keywin_on(key_win);
	} else if (strcmp(menu->exec, "<taskmgr>") == 0) {
		open_taskmgr(memtotal);
	} else if (strcmp(menu->exec, "<filetask>") == 0) {
		open_filetask(memtotal);
	} else {
		task = open_constask(0, memtotal);
		fifo = &task->fifo;
		for (i = 0; menu->exec[i] != 0; i++) {
			fifo32_put_io(fifo, menu->exec[i] + 256);
		}
		fifo32_put_io(fifo, 10 + 256);	/* Enter */
	}

	return -1;
}

int set_menu(struct MENU *menu, int level, char *name, char *exec, char *parent, char *prev)
{
	int i, fs_flg = -1;
	struct MENU *pam = 0, *prm = 0;

	for (i = 0; i < MAX_MENU; i++) {
		if (parent != 0) {
			if (strcmp(menu[i].name, parent) == 0) {
				pam = &menu[i];
			}
		} else if (prev != 0) {
			if (strcmp(menu[i].name, prev) == 0) {
				prm = &menu[i];
			}
		}
		if (menu[i].name[0] == 0) {
			if ((parent != 0 && pam == 0) || (prev != 0 && prm == 0)) {
				break;
			}
			if (parent != 0) {
				pam->sub = &menu[i];
			} else if (prev != 0) {
				prm->next = &menu[i];
			}
			menu[i].level = level;
			strcpy(menu[i].name, name);
			strcpy(menu[i].exec, exec);
			fs_flg = i;
			break;
		}
	}

	return fs_flg;
}

void init_menu(struct MNLV *mnlv, struct MENU **menu)
{
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	int i;

	*menu = (struct MENU *) memman_alloc_4k(memman, 64 * MAX_MENU);
	for (i = 0; i < MAX_MENU; i++) {
		(*menu)[i].level   = 0;
		(*menu)[i].name[0] = 0;
		(*menu)[i].exec[0] = 0;
		(*menu)[i].next    = 0;
		(*menu)[i].sub     = 0;
	}
	for (i = 0; i < MAX_MNLV; i++) {
		mnlv[i].menu = 0;
		mnlv[i].sht  = 0;
		mnlv[i].buf  = 0;
		mnlv[i].pos  = 0;
		mnlv[i].num  = 0;
	}

	// 第1階層
	set_menu(*menu, 0, "shutdown"    , "shutdown" , 0, 0             );
	set_menu(*menu, 0, "task manager", "<taskmgr>", 0, "shutdown"    );
	set_menu(*menu, 0, "file manager", "<filetask>", 0, "task manager"    );
	set_menu(*menu, 0, "programs    >", "<submenu>", 0, "file manager");
	set_menu(*menu, 0, "console"     , "<console>", 0, "programs    >"  );

	// 第2階層
	set_menu(*menu, 1, "cpuid"     , "cpuid"    , "programs    >", 0           );
	
	return;
}

void bmp_conv(int xsize, int ysize, unsigned short *vram)
{
	struct BMPFLHDR bfh;
	struct BMPIFHDR bih;
	struct FILEHANDLE fh;
	int i, j, p, q;
	long fwidth = xsize + ((4 - (xsize % 4)) % 4);
	struct SHTCTL *ctl = (struct SHTCTL *) *((int *) 0x0fe4);
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	unsigned char *bmpbuf = (unsigned char *) memman_alloc_4k(memman, fwidth * ysize * 3);

	/* BMPファイルヘッダー */
	bfh.type    = 0x4d42;
	bfh.size    = sizeof(bfh) - 2 + sizeof(bih) + fwidth * ysize * 3;
	bfh.rsv1    = 0;
	bfh.rsv2    = 0;
	bfh.offbits = sizeof(bfh) - 2 + sizeof(bih);

	/* BMP情報ヘッダー */
	bih.size    = sizeof(bih);
	bih.width   = xsize;
	bih.height  = ysize;
	bih.planes  = 1;
	bih.bitcnt  = 24;
	bih.comp    = 0;
	bih.sizeimg = fwidth * ysize * 3;
	bih.xperm   = 0;
	bih.yperm   = 0;
	bih.clused  = 0;
	bih.climpt  = 0;

	/* BMPピクセルデータ */
	for (i = 0; i < bih.height; i++) {
		for (j = 0; j < bih.width; j++) {
			p = ((bih.height - 1 - i) * fwidth + j) * 3;
			q = i * ctl->xsize + j;
			bmpbuf[p + 0] = (char) (((vram[q] << 3) & 0xf8) + ((vram[q] >>  2) & 0x07));
			bmpbuf[p + 1] = (char) (((vram[q] >> 3) & 0xfc) + ((vram[q] >>  9) & 0x03));
			bmpbuf[p + 2] = (char) (((vram[q] >> 8) & 0xf8) + ((vram[q] >> 13) & 0x07));
		}
		for (j = bih.width; j < fwidth; j++) {
			p = ((bih.height - 1 - i) * fwidth + j) * 3;
			bmpbuf[p + 0] = 0;
			bmpbuf[p + 1] = 0;
			bmpbuf[p + 2] = 0;
		}
	}

	if (hrb_api_fopen("/kernel.jpg", 0x10, &fh, u_fat) != 0) {
		hrb_api_fwrite((char *) &bfh.type, sizeof(bfh) - 2, &fh, u_fat);
		hrb_api_fwrite((char *) &bih, sizeof(bih), &fh, u_fat);
		hrb_api_fwrite(bmpbuf, fwidth * ysize * 3, &fh, u_fat);
	}
	hrb_api_fclose(&fh, u_fat);
	memman_free_4k(memman, (int) bmpbuf, fwidth * ysize * 3);

	return;
}

void task_filetask(unsigned int memtotal)
{
	extern int mx,my;
	extern struct CONSOLE cons;
	int i,j,ww,wh;
	struct TASK *task = task_now();
	struct FILEINFO *finfo1 = (struct FILEINFO *) (ADR_DISKIMG + 0x002600);
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	struct TIMER *task_timer = timer_alloc();
	struct FIFO32 *fifo = (struct FIFO32 *) *((int *) 0x0fec);
	struct SHTCTL *ctl = (struct SHTCTL *) *((int *) 0x0fe4);
	struct SHEET *sht = sheet_alloc(ctl);
	struct BOOTINFO *binfo = (struct BOOTINFO *) ADR_BOOTINFO;
	unsigned short *buf = (unsigned short *) memman_alloc_4k(memman, binfo->scrnx* (binfo->scrny -28) * 2);
	struct CURSORBAR cursorbar;
	struct MOUSEBLACK f;
	int fx,fy;
	char s[20],info[100];
	char *name;
	int prev_time, curr_time;
	int flage[224];
	int size[224];
	struct FILEINFO *finfo;
	char c;
	cursorbar.x = 8;
	
	ww = 8;
	wh = 30;
	cursorbar.x1 = 8*16;
	cursorbar.y[0] = 30;
	cursorbar.y[1] = (cursorbar.y[0])+16;
	cursorbar.y[2] = (cursorbar.y[1])+16;
	cursorbar.y[3] = (cursorbar.y[2])+16;
	cursorbar.y[4] = (cursorbar.y[3])+16;
	cursorbar.y[5] = (cursorbar.y[4])+16;
	cursorbar.y[6] = (cursorbar.y[5])+16;
	cursorbar.y[7] = (cursorbar.y[6])+16;
	cursorbar.y[8] = (cursorbar.y[7])+16;
	cursorbar.y[9] = (cursorbar.y[8])+16;
	cursorbar.y[10] = (cursorbar.y[9])+16;
	
	
	
	fx = 8*3;
	fy = 30;
	
	sheet_setbuf(sht, buf, binfo->scrnx, binfo->scrny-28, -1); /* 透明色なし */
	make_window8(buf, binfo->scrnx, binfo->scrny-28, "file contral manager", 0);
	
	sht->task = filetask;
	sheet_slide(sht, 0, 0);
	
	boxfill8(buf, sht->bxsize, 15, 6, 26, 301, binfo->scrny-28-7);
	boxfill8(buf, sht->bxsize, 7, 7, 27, 300, binfo->scrny-28-8);
	
	boxfill8(buf, sht->bxsize, 15, 310, 26, binfo->scrnx - 7, binfo->scrny-28-7);
	boxfill8(buf, sht->bxsize, 7, 311, 27, binfo->scrnx-8, binfo->scrny-28-8);
	
	for (i = 0; i < 224; i++) {
		flage[i] = 0;
	}
	for (i = 0; i < 224; i++) {
		if (finfo1[i].name[0] == 0x00) {
			break;
		}
		if (finfo1[i].name[0] != 0xe5 ) {
			if ((finfo1[i].type & 0x18) == 0) {
				sprintf(s, "filename.ext", finfo1[i].size);
				for (j = 0; j < 8; j++) {
					s[j] = finfo1[i].name[j];
				}
				s[ 9] = finfo1[i].ext[0];
				s[10] = finfo1[i].ext[1];
				s[11] = finfo1[i].ext[2];
				
				putfouts8_asc_sht(sht, fx  , fy, COL8_000000, COL8_FFFFFF, s, 13);		
			}
			fy +=16;
			if(fy >= cursorbar.y[9]){
				fy = 28;
				fx += 17*8;
			}
		}
	}
	
	boxfill8(buf, sht->bxsize, 0, cursorbar.x, cursorbar.y[0], cursorbar.x+7, cursorbar.y[1]);
	
	sheet_updown(sht, ctl->top);
	keywin_on(sht);
	timer_init(task_timer, &task->fifo, 1);
	prev_time = taskctl->tasks0[1].time - 99;
	timer_settime(task_timer, 1);
	
	for (;;) {
		io_cli();
		if (fifo32_status(&task->fifo) == 0) {
			task_sleep(task);
			io_sti();
		} else {
			i = fifo32_get(&task->fifo);
			io_sti();
			f.x = mx;
			f.y = my;
			if (i == 1) {
				curr_time = taskctl->tasks0[1].time;
				timer_settime(task_timer, 10);
				prev_time = curr_time;
			}
			/*if(strcmp(f.name, "a.hrb") ==0){
				cursorbar.y += 16;
			}*/
			
			//if (mouse_decode(&mdec, i - 512) != 0) {
				if ((mdec.btn & 0x01) != 0) {
						/*if(f.x > cursorbar.x && f.x < cursorbar.x+(8*16) && f.y < cursorbar.y+15 && f.y >cursorbar.y && flage == 1){
							boxfill8(buf, sht->bxsize, 7, cursorbar.x, cursorbar.y, cursorbar.x+7, cursorbar.y+15);
							boxfill8(buf, sht->bxsize, 7, cursorbar.x1, cursorbar.y, cursorbar.x1+7, cursorbar.y+15);			
							cursorbar.y += 16;
							boxfill8(buf, sht->bxsize, 0, cursorbar.x, cursorbar.y, cursorbar.x+7, cursorbar.y+15);
							boxfill8(buf, sht->bxsize, 0, cursorbar.x1, cursorbar.y, cursorbar.x1+7, cursorbar.y+15);
							sheet_refresh(sht,cursorbar.x, cursorbar.y-16, cursorbar.x+(8*16), cursorbar.y+16);	
							name = "A       .HRB";	
						}*/
						
					if(f.x > ww && f.x < ww+(8*16) && f.y > cursorbar.y[0] && f.y <cursorbar.y[1] ){	
						
						f.name = "KERNEL  .SYS";
						name = "KERNEL  .SYS";
						if(strcmp(f.name, name) == 0){
							flage[0] = 1;
						}
						if(flage[0] == 1){
							putfouts8_asc_sht(sht, 318  , 30, COL8_000000, COL8_FFFFFF, "NAME    TYPE SIZE ", 20);	
							sprintf(info, "%10s",finfo1[0].name);
							putfouts8_asc_sht(sht, 318  , 46, COL8_000000, COL8_FFFFFF, info, 20);	
							boxfill8(buf, sht->bxsize,15, 318, 80, 350, 100);						
						}
						
						
						
						boxfill8(buf, sht->bxsize, 7, 8, cursorbar.y[0], 8+7, cursorbar.y[0]+15);
						boxfill8(buf, sht->bxsize, 7, 8, cursorbar.y[1], 8+7, cursorbar.y[1]+15);
						boxfill8(buf, sht->bxsize, 7, 8, cursorbar.y[2], 8+7, cursorbar.y[2]+15);
						boxfill8(buf, sht->bxsize, 7, 8, cursorbar.y[3], 8+7, cursorbar.y[3]+15);
						boxfill8(buf, sht->bxsize, 7, 8, cursorbar.y[4], 8+7, cursorbar.y[4]+15);
						boxfill8(buf, sht->bxsize, 7, 8, cursorbar.y[5], 8+7, cursorbar.y[5]+15);
						boxfill8(buf, sht->bxsize, 7, 8, cursorbar.y[6], 8+7, cursorbar.y[6]+15);
						boxfill8(buf, sht->bxsize, 7, 8, cursorbar.y[7], 8+7, cursorbar.y[7]+15);
						boxfill8(buf, sht->bxsize, 7, 8, cursorbar.y[8], 8+7, cursorbar.y[8]+15);
						
						boxfill8(buf, sht->bxsize, 0, 8, cursorbar.y[0], 8+7, cursorbar.y[0]+15);
						sheet_refresh(sht, 8, cursorbar.y[0], 8+8, cursorbar.y[9]);
					}
					if(f.x > ww && f.x < ww+(8*16) && f.y > cursorbar.y[1] && f.y <cursorbar.y[2] ){
						
						f.name = "CUPID   .HRB";
						name = "CUPID   .HRB";
						if(strcmp(f.name, name) == 0){
							flage[1] = 1;
						}
						if(flage[1] == 1){
							putfouts8_asc_sht(sht, 318  , 30, COL8_000000, COL8_FFFFFF, "NAME    TYPE SIZE ", 20);	
							sprintf(info, "%10s",finfo1[1].name);
							putfouts8_asc_sht(sht, 318  , 46, COL8_000000, COL8_FFFFFF, info, 20);	
						}				
						boxfill8(buf, sht->bxsize, 7, 8, cursorbar.y[0], 8+7, cursorbar.y[0]+15);
						boxfill8(buf, sht->bxsize, 7, 8, cursorbar.y[1], 8+7, cursorbar.y[1]+15);
						boxfill8(buf, sht->bxsize, 7, 8, cursorbar.y[2], 8+7, cursorbar.y[2]+15);
						boxfill8(buf, sht->bxsize, 7, 8, cursorbar.y[3], 8+7, cursorbar.y[3]+15);
						boxfill8(buf, sht->bxsize, 7, 8, cursorbar.y[4], 8+7, cursorbar.y[4]+15);
						boxfill8(buf, sht->bxsize, 7, 8, cursorbar.y[5], 8+7, cursorbar.y[5]+15);
						boxfill8(buf, sht->bxsize, 7, 8, cursorbar.y[6], 8+7, cursorbar.y[6]+15);
						boxfill8(buf, sht->bxsize, 7, 8, cursorbar.y[7], 8+7, cursorbar.y[7]+15);
						boxfill8(buf, sht->bxsize, 7, 8, cursorbar.y[8], 8+7, cursorbar.y[8]+15);
						
						boxfill8(buf, sht->bxsize, 0, 8, cursorbar.y[1], 8+7, cursorbar.y[1]+15);						
						sheet_refresh(sht, 8, cursorbar.y[0], 8+8, cursorbar.y[9]);
					}
					if(f.x > ww && f.x < ww+(8*16) && f.y > cursorbar.y[2] && f.y <cursorbar.y[3] ){
						boxfill8(buf, sht->bxsize, 7, 8, cursorbar.y[0], 8+7, cursorbar.y[0]+15);
						boxfill8(buf, sht->bxsize, 7, 8, cursorbar.y[1], 8+7, cursorbar.y[1]+15);
						boxfill8(buf, sht->bxsize, 7, 8, cursorbar.y[2], 8+7, cursorbar.y[2]+15);
						boxfill8(buf, sht->bxsize, 7, 8, cursorbar.y[3], 8+7, cursorbar.y[3]+15);
						boxfill8(buf, sht->bxsize, 7, 8, cursorbar.y[4], 8+7, cursorbar.y[4]+15);
						boxfill8(buf, sht->bxsize, 7, 8, cursorbar.y[5], 8+7, cursorbar.y[5]+15);
						boxfill8(buf, sht->bxsize, 7, 8, cursorbar.y[6], 8+7, cursorbar.y[6]+15);
						boxfill8(buf, sht->bxsize, 7, 8, cursorbar.y[7], 8+7, cursorbar.y[7]+15);
						boxfill8(buf, sht->bxsize, 7, 8, cursorbar.y[8], 8+7, cursorbar.y[8]+15);
						
						boxfill8(buf, sht->bxsize, 0, 8, cursorbar.y[2], 8+7, cursorbar.y[2]+15);					
						sheet_refresh(sht, 8, cursorbar.y[0], 8+8, cursorbar.y[9]);
					}
					if(f.x > ww && f.x < ww+(8*16) && f.y > cursorbar.y[3] && f.y <cursorbar.y[4] ){
						boxfill8(buf, sht->bxsize, 7, 8, cursorbar.y[0], 8+7, cursorbar.y[0]+15);
						boxfill8(buf, sht->bxsize, 7, 8, cursorbar.y[1], 8+7, cursorbar.y[1]+15);
						boxfill8(buf, sht->bxsize, 7, 8, cursorbar.y[2], 8+7, cursorbar.y[2]+15);
						boxfill8(buf, sht->bxsize, 7, 8, cursorbar.y[3], 8+7, cursorbar.y[3]+15);
						boxfill8(buf, sht->bxsize, 7, 8, cursorbar.y[4], 8+7, cursorbar.y[4]+15);
						boxfill8(buf, sht->bxsize, 7, 8, cursorbar.y[5], 8+7, cursorbar.y[5]+15);
						boxfill8(buf, sht->bxsize, 7, 8, cursorbar.y[6], 8+7, cursorbar.y[6]+15);
						boxfill8(buf, sht->bxsize, 7, 8, cursorbar.y[7], 8+7, cursorbar.y[7]+15);
						boxfill8(buf, sht->bxsize, 7, 8, cursorbar.y[8], 8+7, cursorbar.y[8]+15);
						
						boxfill8(buf, sht->bxsize, 0, 8, cursorbar.y[3], 8+7, cursorbar.y[3]+15);					
						sheet_refresh(sht, 8, cursorbar.y[0], 8+8, cursorbar.y[9]);
					}
					if(f.x > ww && f.x < ww+(8*16) && f.y > cursorbar.y[4] && f.y <cursorbar.y[5] ){
						boxfill8(buf, sht->bxsize, 7, 8, cursorbar.y[0], 8+7, cursorbar.y[0]+15);
						boxfill8(buf, sht->bxsize, 7, 8, cursorbar.y[1], 8+7, cursorbar.y[1]+15);
						boxfill8(buf, sht->bxsize, 7, 8, cursorbar.y[2], 8+7, cursorbar.y[2]+15);
						boxfill8(buf, sht->bxsize, 7, 8, cursorbar.y[3], 8+7, cursorbar.y[3]+15);
						boxfill8(buf, sht->bxsize, 7, 8, cursorbar.y[4], 8+7, cursorbar.y[4]+15);
						boxfill8(buf, sht->bxsize, 7, 8, cursorbar.y[5], 8+7, cursorbar.y[5]+15);
						boxfill8(buf, sht->bxsize, 7, 8, cursorbar.y[6], 8+7, cursorbar.y[6]+15);
						boxfill8(buf, sht->bxsize, 7, 8, cursorbar.y[7], 8+7, cursorbar.y[7]+15);
						boxfill8(buf, sht->bxsize, 7, 8, cursorbar.y[8], 8+7, cursorbar.y[8]+15);
						
						boxfill8(buf, sht->bxsize, 0, 8, cursorbar.y[4], 8+7, cursorbar.y[4]+15);					
						sheet_refresh(sht, 8, cursorbar.y[0], 8+8, cursorbar.y[9]);
					}
					if(f.x > ww && f.x < ww+(8*16) && f.y > cursorbar.y[5] && f.y <cursorbar.y[6] ){
						boxfill8(buf, sht->bxsize, 7, 8, cursorbar.y[0], 8+7, cursorbar.y[0]+15);
						boxfill8(buf, sht->bxsize, 7, 8, cursorbar.y[1], 8+7, cursorbar.y[1]+15);
						boxfill8(buf, sht->bxsize, 7, 8, cursorbar.y[2], 8+7, cursorbar.y[2]+15);
						boxfill8(buf, sht->bxsize, 7, 8, cursorbar.y[3], 8+7, cursorbar.y[3]+15);
						boxfill8(buf, sht->bxsize, 7, 8, cursorbar.y[4], 8+7, cursorbar.y[4]+15);
						boxfill8(buf, sht->bxsize, 7, 8, cursorbar.y[5], 8+7, cursorbar.y[5]+15);
						boxfill8(buf, sht->bxsize, 7, 8, cursorbar.y[6], 8+7, cursorbar.y[6]+15);
						boxfill8(buf, sht->bxsize, 7, 8, cursorbar.y[7], 8+7, cursorbar.y[7]+15);
						boxfill8(buf, sht->bxsize, 7, 8, cursorbar.y[8], 8+7, cursorbar.y[8]+15);
						
						boxfill8(buf, sht->bxsize, 0, 8, cursorbar.y[5], 8+7, cursorbar.y[5]+15);					
						sheet_refresh(sht, 8, cursorbar.y[0], 8+8, cursorbar.y[9]);
					}
					if(f.x > ww && f.x < ww+(8*16) && f.y > cursorbar.y[6] && f.y <cursorbar.y[7] ){
						boxfill8(buf, sht->bxsize, 7, 8, cursorbar.y[0], 8+7, cursorbar.y[0]+15);
						boxfill8(buf, sht->bxsize, 7, 8, cursorbar.y[1], 8+7, cursorbar.y[1]+15);
						boxfill8(buf, sht->bxsize, 7, 8, cursorbar.y[2], 8+7, cursorbar.y[2]+15);
						boxfill8(buf, sht->bxsize, 7, 8, cursorbar.y[3], 8+7, cursorbar.y[3]+15);
						boxfill8(buf, sht->bxsize, 7, 8, cursorbar.y[4], 8+7, cursorbar.y[4]+15);
						boxfill8(buf, sht->bxsize, 7, 8, cursorbar.y[5], 8+7, cursorbar.y[5]+15);
						boxfill8(buf, sht->bxsize, 7, 8, cursorbar.y[6], 8+7, cursorbar.y[6]+15);
						boxfill8(buf, sht->bxsize, 7, 8, cursorbar.y[7], 8+7, cursorbar.y[7]+15);
						boxfill8(buf, sht->bxsize, 7, 8, cursorbar.y[8], 8+7, cursorbar.y[8]+15);
						
						boxfill8(buf, sht->bxsize, 0, 8, cursorbar.y[6], 8+7, cursorbar.y[6]+15);					
						sheet_refresh(sht, 8, cursorbar.y[0], 8+8, cursorbar.y[9]);
					}
					if(f.x > ww && f.x < ww+(8*16) && f.y > cursorbar.y[7] && f.y <cursorbar.y[8] ){
						boxfill8(buf, sht->bxsize, 7, 8, cursorbar.y[0], 8+7, cursorbar.y[0]+15);
						boxfill8(buf, sht->bxsize, 7, 8, cursorbar.y[1], 8+7, cursorbar.y[1]+15);
						boxfill8(buf, sht->bxsize, 7, 8, cursorbar.y[2], 8+7, cursorbar.y[2]+15);
						boxfill8(buf, sht->bxsize, 7, 8, cursorbar.y[3], 8+7, cursorbar.y[3]+15);
						boxfill8(buf, sht->bxsize, 7, 8, cursorbar.y[4], 8+7, cursorbar.y[4]+15);
						boxfill8(buf, sht->bxsize, 7, 8, cursorbar.y[5], 8+7, cursorbar.y[5]+15);
						boxfill8(buf, sht->bxsize, 7, 8, cursorbar.y[6], 8+7, cursorbar.y[6]+15);
						boxfill8(buf, sht->bxsize, 7, 8, cursorbar.y[7], 8+7, cursorbar.y[7]+15);
						boxfill8(buf, sht->bxsize, 7, 8, cursorbar.y[8], 8+7, cursorbar.y[8]+15);
						
						boxfill8(buf, sht->bxsize, 0, 8, cursorbar.y[7], 8+7, cursorbar.y[7]+15);					
						sheet_refresh(sht, 8, cursorbar.y[0], 8+8, cursorbar.y[9]);
					}
					if(f.x > ww && f.x < ww+(8*16) && f.y > cursorbar.y[8] && f.y <cursorbar.y[9] ){
						boxfill8(buf, sht->bxsize, 7, 8, cursorbar.y[0], 8+7, cursorbar.y[0]+15);
						boxfill8(buf, sht->bxsize, 7, 8, cursorbar.y[1], 8+7, cursorbar.y[1]+15);
						boxfill8(buf, sht->bxsize, 7, 8, cursorbar.y[2], 8+7, cursorbar.y[2]+15);
						boxfill8(buf, sht->bxsize, 7, 8, cursorbar.y[3], 8+7, cursorbar.y[3]+15);
						boxfill8(buf, sht->bxsize, 7, 8, cursorbar.y[4], 8+7, cursorbar.y[4]+15);
						boxfill8(buf, sht->bxsize, 7, 8, cursorbar.y[5], 8+7, cursorbar.y[5]+15);
						boxfill8(buf, sht->bxsize, 7, 8, cursorbar.y[6], 8+7, cursorbar.y[6]+15);
						boxfill8(buf, sht->bxsize, 7, 8, cursorbar.y[7], 8+7, cursorbar.y[7]+15);
						boxfill8(buf, sht->bxsize, 7, 8, cursorbar.y[8], 8+7, cursorbar.y[8]+15);
						
						boxfill8(buf, sht->bxsize, 0, 8, cursorbar.y[8], 8+7, cursorbar.y[8]+15);					
						sheet_refresh(sht, 8, cursorbar.y[0], 8+8, cursorbar.y[9]);
					}
					
				}
			sprintf(s, "%d:%d ",f.x,f.y);
			putfouts8_asc_sht(sht, 8  , 300, COL8_000000, COL8_FFFFFF, s, 12);	
			
			sheet_refresh(sht, 8, 30,640, 480);
			
		}
	}
	timer_free(task_timer);
	memman_free_4k(memman, (int) sht->buf, 296 * 290 * 2);
	sheet_free(sht);
	fifo32_put_io(fifo, 2280-1);
	for (;;) {
		task_sleep(task);
	}
}



