#include "apilib.h"

#include <string.h>	/* strlen */
 
int strtol(char *s1, char **endp, int base);	/* 標準関数（stdlib.h） */

void end(char *s1);
void putstr(int win, char *winbuf, int x, int y, int col, unsigned char *s);
void wait(int i, int timer, char *keyflag);
void setdec8(char *s, int i);

static unsigned char charset[16 * 8] = {

	/* invader(0) */
	0x00, 0x00, 0x00, 0x43, 0x5f, 0x5f, 0x5f, 0x7f,
	0x1f, 0x1f, 0x1f, 0x1f, 0x00, 0x20, 0x3f, 0x00,

	/* invader(1) */
	0x00, 0x0f, 0x7f, 0xff, 0xcf, 0xcf, 0xcf, 0xff,
	0xff, 0xe0, 0xff, 0xff, 0xc0, 0xc0, 0xc0, 0x00,

	/* invader(2) */
	0x00, 0xf0, 0xfe, 0xff, 0xf3, 0xf3, 0xf3, 0xff,
	0xff, 0x07, 0xff, 0xff, 0x03, 0x03, 0x03, 0x00,

	/* invader(3) */
	0x00, 0x00, 0x00, 0xc2, 0xfa, 0xfa, 0xfa, 0xfe,
	0xf8, 0xf8, 0xf8, 0xf8, 0x00, 0x04, 0xfc, 0x00,

	/* fighter(0) */
	0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
	0x01, 0x43, 0x47, 0x4f, 0x5f, 0x7f, 0x7f, 0x00,

	/* fighter(1) */
	0x18, 0x7e, 0xff, 0xc3, 0xc3, 0xc3, 0xc3, 0xff,
	0xff, 0xff, 0xe7, 0xe7, 0xe7, 0xe7, 0xff, 0x00,

	/* fighter(2) */
	0x00, 0x00, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
	0x80, 0xc2, 0xe2, 0xf2, 0xfa, 0xfe, 0xfe, 0x00,

	/* laser */
	0x00, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18,
	0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x00
};
/* invader:"abcd", fighter:"efg", laser:"h" */



void HariMain(void)
{
	char  txtbuf[100 * 1024];
	char s1[32], *p1, *r;
	int   t = 120, l = 192 / 4, o = 4, q = 7, note_old = 0;
	
	int win, timer, i, j, fx, laserwait, lx = 0, ly;
	int ix, iy, movewait0, movewait, idir;
	int invline, score, high, point;
	char winbuf[336 * 261], invstr[32 * 6], s[12], keyflag[4], *p;
	static char invstr0[32] = " abcd abcd abcd abcd abcd ";

	win = api_openwin(winbuf, 336, 261, -1, "invader");//鈞岥
	api_boxfilwin(win, 6, 27, 329, 254, 0);//麷怓攚宨
	timer = api_alloctimer();
	api_inittimer(timer, 128);

	high = 0;
	putstr(win, winbuf, 22, 0, 7, "HIGH:00000000");//嵟崅暘

restart:
	score = 0;//暘悢
	point = 1;//?樍
	putstr(win, winbuf,  4, 0, 7, "SCORE:00000000");
	movewait0 = 5;//弶巒?
	fx = 18;//帺婘揑x嵖?
	putstr(win, winbuf, fx, 13, 6, "efg");//帺婘
	wait(100, timer, keyflag);//帺婘憖嶌

next_group:
	wait(100, timer, keyflag);
	ix = 7;
	iy = 1;//奜惎恖孮嵖?
	invline = 6;//奜惎恖峴悢
	for (i = 0; i < 6; i++) {
		for (j = 0; j < 27; j++) {
			invstr[i * 32 + j] = invstr0[j];
		}
		putstr(win, winbuf, ix, iy + i, 2, invstr + i * 32);
	}
	keyflag[0] = 0;
	keyflag[1] = 0;
	keyflag[2] = 0;

	ly = 0; /* 晄?帵 */
	laserwait = 0;//鄘?檾梋廩擻??
	movewait = movewait0;//?0?奜惎恖堏?
	idir = +1;//奜惎恖堏?曽岦
	wait(100, timer, keyflag);

	/* 音番号と周波数(mHz)の対応表 */
	/* たとえば、O4Aは440Hzなので、440000 */
	/* オクターブ16のAは1802240Hzなので、1802240000 */
	/* 以下はオクターブ16のリスト(C～B) */
	static int tonetable[12] = {
		1071618315, 1135340056, 1202850889, 1274376125, 1350154473, 1430438836,
		1515497155, 1605613306, 1701088041, 1802240000, 1909406767, 2022946002
	};
	static int notetable[7] = { +9, +11, +0 /* C */, +2, +4, +5, +7 };

	/* コマンドライン解析 */
	api_cmdline(s1, 30);
	for (p1 = s1; *p1 > ' '; p1++) { }	/* スペースが来るまで読み飛ばす */
	for (; *p1 == ' '; p1++) { }	/* スペースを読み飛ばす */
	i = strlen(p1);
	if (i > 12) {
file_error:
		end("file open error.\n");
	}
	if (i == 0) {
		end(0);
	}


	/* ファイル読み込み */
	i = api_fopen(p1);
	if (i == 0) {
		goto file_error;
	}
	j = api_fsize(i, 0);
	if (j >= 100 * 1024) {
		j = 100 * 1024 - 1;
	}
	api_fread(txtbuf, j, i);
	api_fclose(i);
	txtbuf[j] = 0;
	r = txtbuf;
	i = 0; /* 通常モード */
	for (p1 = txtbuf; *p1 != 0; p1++) {	/* 処理を簡単にするためにコメントや空白を消す */
		if (i == 0 && *p1 > ' ') {	/* スペースや改行コードなどではない */
			if (*p1 == '/') {
				if (p1[1] == '*') {
					i = 1;
				} else if (p1[1] == '/') {
					i = 2;
				} else {
					*r = *p1;
					if ('a' <= *p1 && *p1 <= 'z') {
						*r += 'A' - 'a';	/* 小文字は大文字に変換 */
					}
					r++;
				}
			} else if (*p1 == 0x22) {
				*r = *p1;
				r++;
				i = 3;
			} else {
				*r = *p1;
				r++;
			}
		} else if (i == 1 && *p1 == '*' && p1[1] == '/') {	/* ブロックコメント */
			p1++;
			i = 0;
		} else if (i == 2 && *p1 == 0x0a) {	/* 行コメント */
			i = 0;
		} else if (i == 3) {	/* 文字列 */
			*r = *p1;
			r++;
			if (*p1 == 0x22) {
				i = 0;
			} else if (*p1 == '%') {
				p1++;
				*r = *p1;
				r++;
			}
		}
	}
	*r = 0;

	/* タイマの準備 */
	timer = api_alloctimer();
	api_inittimer(timer, 128);

	/* メイン */
	p1 = txtbuf;
	for (;;) {
		if (('A' <= *p1 && *p1 <= 'G') || *p1 == 'R') {	/* 音符・休符 */
			/* 周波数計算 */
			if (*p1 == 'R') {
				i = 0;
				s1[0] = 0;
			} else {
				i = o * 12 + notetable[*p1 - 'A'] + 12;
				s1[0] = 'O';
				s1[1] = '0' + o;
				s1[2] = *p1;
				s1[3] = ' ';
				s1[4] = 0;
			}
			p1++;
			if (*p1 == '+' || *p1 == '-' || *p1 == '#') {
				s1[3] = *p1;
				if (*p1 == '-') {
					i--;
				} else {
					i++;
				}
				p1++;
			}
			if (i != note_old) {
				
				if (s1[0] != 0) {
					api_beep(tonetable[i % 12] >> (17 - i / 12));
				} else {
					api_beep(0);
				}
				note_old = i;
			}
			/* 音長計算 */
			if ('0' <= *p1 && *p1 <= '9') {
				i = 192 / strtol(p1, &p1, 10);
			} else {
				i = l;
			}
			for (; *p1 == '.'; ) {
				p1++;
				i += i / 2;
			}
			i *= (60 * 100 / 48);
			i /= t;
			if (s1[0] != 0 && q < 8 && *p1 != '&') {
				j = i * q / 8;
				wait(1,timer, keyflag);
				
				note_old = 0;
				api_beep(0);
			} else {
				j = 0;
				if (*p1 == '&') {
					p1++;
				}
			}
			wait(1,timer, keyflag);
		} else if (*p1 == '<') {	/* オクターブ-- */
			p1++;
			o--;
		} else if (*p1 == '>') {	/* オクターブ++ */
			p1++;
			o++;
		} else if (*p1 == 'O') {	/* オクターブ指定 */
			o = strtol(p1 + 1, &p1, 10);
		} else if (*p1 == 'Q') {	/* Qパラメータ指定 */
			q = strtol(p1 + 1, &p1, 10);
		} else if (*p1 == 'L') { /* デフォルト音長指定 */ 
			l = strtol(p1 + 1, &p1, 10);
			if (l == 0) {
				goto syntax_error;
			}
			l = 192 / l;
			for (; *p1 == '.'; ) {
				p1++;
				l += l / 2;
			}
		} else if (*p1 == 'T') {	/* テンポ指定 */
			t = strtol(p1 + 1, &p1, 10);
		} else if (*p1 == '$') {	/* 拡張コマンド */
			if (p1[1] == 'K') {	/* カラオケコマンド */
				p1 += 2;
				for (; *p1 != 0x22; p1++) {
					if (*p1 == 0) {
						goto syntax_error;
					}
				}
				p1++;
				for (i = 0; i < 32; i++) {
					if (*p1 == 0) {
						goto syntax_error;
					}
					if (*p1 == 0x22) {
						break;
					}
					if (*p1 == '%') {
						s1[i] = p1[1];
						p1 += 2;
					} else {
						s1[i] = *p1;
						p1++;
					}
				}
				if (i > 30) {
					end("karaoke too long.\n");
				}
				
				s1[i] = 0;

			}
			for (; *p1 != ';'; p1++) {
				if (*p1 == 0) {
					goto syntax_error;
				}
			}
			p1++;
		} else if (*p1 == 0) {
			p1 = txtbuf;
		} else {
syntax_error:
			end("mml syntax error.\n");
		}
		
		
		if (laserwait != 0) {
			laserwait--;
			keyflag[2 /* space */] = 0;
		}

		wait(1, timer, keyflag);

		/* 帺婘揑?棟 */
		if (keyflag[0 /* left */]  != 0 && fx > 0) {
			fx--;
			putstr(win, winbuf, fx, 13, 6, "efg ");
			keyflag[0 /* left */]  = 0;
		}
		if (keyflag[1 /* right */] != 0 && fx < 37) {
			putstr(win, winbuf, fx, 13, 6, " efg");
			fx++;
			keyflag[1 /* right */] = 0;
		}
		if (keyflag[2 /* space */] != 0 && laserwait == 0) {
			laserwait = 15;
			lx = fx + 1;
			ly = 13;
		}

		/* 奜惎恖堏? */
		if (movewait != 0) {
			movewait--;
		} else {
			movewait = movewait0;
			if (ix + idir > 14 || ix + idir < 0) {
				if (iy + invline == 13) {
					break; /* GAME OVER */
				}
				idir = - idir;
				putstr(win, winbuf, ix + 1, iy, 0, "                         ");//嶞怴忋峴巆棷嵀鐟
				iy++;
			} else {
				ix += idir;
			}
			for (i = 0; i < invline; i++) {
				putstr(win, winbuf, ix, iy + i, 2, invstr + i * 32);//堏?
			}
		}

		/* 鄘??棟 */
		if (ly > 0) {
			if (ly < 13) {
				if (ix < lx && lx < ix + 25 && iy <= ly && ly < iy + invline) {
					putstr(win, winbuf, ix, ly, 2, invstr + (ly - iy) * 32);
				} else {
					putstr(win, winbuf, lx, ly, 0, " ");
				}
			}
			ly--;
			if (ly > 0) {
				putstr(win, winbuf, lx, ly, 3, "h");
			} else {
				point -= 10;
				if (point <= 0) {
					point = 1;
				}
			}
			if (ix < lx && lx < ix + 25 && iy <= ly && ly < iy + invline) {
				p = invstr + (ly - iy) * 32 + (lx - ix);
				if (*p != ' ') {
					/* hit ! */
					score += point;
					point++;
					setdec8(s, score);
					putstr(win, winbuf, 10, 0, 7, s);
					if (high < score) {
						high = score;
						putstr(win, winbuf, 27, 0, 7, s);
					}
					for (p--; *p != ' '; p--) { }
					for (i = 1; i < 5; i++) {
						p[i] = ' ';
					}
					putstr(win, winbuf, ix, ly, 2, invstr + (ly - iy) * 32);
					for (; invline > 0; invline--) {
						for (p = invstr + (invline - 1) * 32; *p != 0; p++) {
							if (*p != ' ') {
								goto alive;
							}
						}
					}
					/* 慡晹徚? */
					movewait0 -= movewait0 / 3;
					goto next_group;
	alive:
					ly = 0;
				}
			}
		}

	
	
	
	

	
		
	}
	/* GAME OVER */
	putstr(win, winbuf, 15, 6, 1, "GAME OVER");
	wait(0, timer, keyflag);
	for (i = 1; i < 14; i++) {
		putstr(win, winbuf, 0, i, 0, "                                        ");
	}
	goto restart;
	
	
}



void end(char *s1)
{
	if (s1 != 0) {
		api_putstr0(s1);
	}
	api_beep(0);
	api_end();
}

void putstr(int win, char *winbuf, int x, int y, int col, unsigned char *s)
{
	int c, x0, i;
	char *p, *q, t[2];
	x = x * 8 + 8;
	y = y * 16 + 29;
	x0 = x;
	i = strlen(s);	/* ?嶼s揑帤晞悢 */
	api_boxfilwin(win + 1, x, y, x + i * 8 - 1, y + 15, 0);
	q = winbuf + y * 336;
	t[1] = 0;
	for (;;) {
		c = *s;
		if (c == 0) {
			break;
		}
		if (c != ' ') {
			if ('a' <= c && c <= 'h') {
				p = charset + 16 * (c - 'a');
				q += x;
				for (i = 0; i < 16; i++) {
					if ((p[i] & 0x80) != 0) { q[0] = col; }
					if ((p[i] & 0x40) != 0) { q[1] = col; }
					if ((p[i] & 0x20) != 0) { q[2] = col; }
					if ((p[i] & 0x10) != 0) { q[3] = col; }
					if ((p[i] & 0x08) != 0) { q[4] = col; }
					if ((p[i] & 0x04) != 0) { q[5] = col; }
					if ((p[i] & 0x02) != 0) { q[6] = col; }
					if ((p[i] & 0x01) != 0) { q[7] = col; }
					q += 336;
				}
				q -= 336 * 16 + x;
			} else {
				t[0] = *s;
				api_putstrwin(win + 1, x, y, col, 1, t);
			}
		}
		s++;
		x += 8;
	}
	api_refreshwin(win, x0, y, x, y + 16);
	return;
}

void wait(int i, int timer, char *keyflag)
{
	int j;
	if (i > 0) {
		/* 摍懸堦抜?? */
		api_settimer(timer, i);
		i = 128;
	} else {
		i = 0x0a; /* Enter */
	}
	for (;;) {
		j = api_getkey(1);
		if (i == j) {
			break;
		}
		if (j == '4') {
			keyflag[0 /* left */]  = 1;
		}
		if (j == '6') {
			keyflag[1 /* right */] = 1;
		}
		if (j == ' ') {
			keyflag[2 /* space */] = 1;
		}
		if (j == 'Q' || j == 'q') {
			api_beep(0);
			api_end();
		}
	}
	return;
}

void setdec8(char *s, int i)
/* i傪10恑悢昞婰偱s偵奿擺 */
{
	int j;
	for (j = 7; j >= 0; j--) {
		s[j] = '0' + i % 10;
		i /= 10;
	}
	s[8] = 0;
	return;
}

