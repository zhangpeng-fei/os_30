#include <string.h>		
#include "apilib.h"

struct PADINFO {
	int x;
	int y;
	int c;
};

void cons_putchar(int win ,int chr, char move);
void cons_newline(int win);

struct PADINFO padinfo;
short winbuf[336 * 261];

void HariMain(void)
{
	int win,w_x,w_y;
	int timer,i,j = 0,c=0,n;
	//int yt = 30;
	int key_shift = 0;
	int msgx = 8;
	int cansave = 0;
	char line[40];
	char size[512];
	int x, y;
	char s[40],q[40];
	char *a = " ";
	char *fname = "notepad.txt";
	int sizef = 0;
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
	int fh;
	
	
	
	
	//timer = api_alloctimer();
	//api_inittimer(timer, 128);
	
	w_x = 336;
	w_y = 261;
	x = 76;
	y = 56;
	padinfo.x = 8;
	padinfo.y = 30;
	padinfo.c = 0;
	win = api_openwin(winbuf, w_x, w_y, -1, "notepad");
	api_boxfilwin(win, 6, 26, w_x-7, w_y-7, 15);
	api_boxfilwin(win, 7, 27, w_x-8, w_y-8, 7);
	api_boxfilwin(win, padinfo.x,padinfo.y,padinfo.x+7,padinfo.y+15,padinfo.c);

	fh = api_fopenEx(fname,0x10);
	
	/*for(j = 0;j<15;j++){
		api_boxfilwin(win, 16, yt+j*16,32,yt+16*j+16,j);
	}*/

	for (;;) {
		//åı?çTêß
		//api_boxfilwin(win, padinfo.x,padinfo.y,padinfo.x+7,padinfo.y+15,0);
		/*if(j != 0){
			api_boxfilwin(win, padinfo.x,padinfo.y,padinfo.x+7,padinfo.y+15,7);
			j = 0;
			api_settimer(timer, 50);
		}else{
			api_boxfilwin(win, padinfo.x,padinfo.y,padinfo.x+7,padinfo.y+15,0);
			j = 1;
			api_settimer(timer, 50);
		}
		api_refreshwin(win,  padinfo.x,padinfo.y,padinfo.x+8,padinfo.y+16);*/
		i = api_getkey(1);
		if (i == 0x2a) {
			key_shift |= 1;
			
		}
		if (i < 0x80) { /* •≠©`•≥©`•…§ÚŒƒ◊÷•≥©`•…§Àâ‰ìQ */		
			s[0] = keytable0[i];
			if (key_shift == 0) {
				s[0] = keytable0[i];
			} else {
				s[0] = keytable1[i];
			}
		} else {
			s[0] = 0;
		}
		if ('A' <= s[0] && s[0] <= 'Z') {
			if (key_shift == 0) {
				s[0] += 0x20;
			}
		}		
		if (i == 8) {
			if (padinfo.x > 8) {/*	Delete*/						
				api_boxfilwin(win, padinfo.x,padinfo.y,padinfo.x+7,padinfo.y+15,7);	
				cons_putchar(win, ' ', 0);				
				size[j] = 0;
				i = ' ';
				//size[j++] = 0;
				j--;
				//line[padinfo.x/8 - 1] = ' ';
				padinfo.x -= 8;
				api_boxfilwin(win, padinfo.x,padinfo.y,padinfo.x+7,padinfo.y+15,0);
				if(sizef >0){
					sizef--;
				}							
			}
		} else if (i == 9) {/* Enter */		
			if (padinfo.x < 320+8) {
				api_boxfilwin(win, padinfo.x,padinfo.y,padinfo.x+7,padinfo.y+15,7);				
				cons_putchar(win, ' ', 0);
				//line[padinfo.x/8 - 1] = ' ';
				//line[padinfo.x/8 - 1] = '\n';
				cons_newline(win);				
				api_boxfilwin(win, padinfo.x,padinfo.y,padinfo.x+7,padinfo.y+15,0);			
			}			
		} else {
			if (padinfo.x < 320) {		
				api_boxfilwin(win, padinfo.x,padinfo.y,padinfo.x+7,padinfo.y+15,7);
				line[padinfo.x/8 - 1] = i;
				cons_putchar(win ,i, 1);
				sizef++;
				/*sprintf(s, "%c",i);						
				api_putstrwin(win,padinfo.x, padinfo.y, 0, 1,s);
				
				padinfo.x += 8;	*/
				
				api_boxfilwin(win, padinfo.x,padinfo.y,padinfo.x+7,padinfo.y+15,0);	
			}
		}
		api_refreshwin(win, 8,30,336-7,261-7);	
		
		size[j] = i;
		j++;
		if(j == 1){
			size[j] = 0;
		}
		c++;
		if(j == c){
			size[j+1] = 0;
		}
		if (i == '$') {/* space */	
			sizef--;
			if (fh != 0) {
				api_fwrite((char *)size, sizef, fh);
				api_fcloseEx(fh);
				break;
			}						
		}
		
	}
	api_end();

}
void cons_putchar(int win ,int chr, char move)
{
	char s[2];
	s[0] = chr;
	s[1] = 0;
	if (s[0] == 0x09) {
		for (;;) {		
			//putfouts8_asc_sht(cons->sht, cons->cur_x, cons->cur_y, COL8_FFFFFF, COL8_000000, " ", 1);
			api_putstrwin(win, padinfo.x, padinfo.y, 0, 1," ");
			padinfo.x += 8;
			if (padinfo.x == 8 + 320) {
				cons_newline(win);
			}
			if (((padinfo.x - 8) & 0x1f) == 0) {
				break;	/* 32§«∏Ó§Í«–§Ï§ø§Èbreak */
			}
		}
	} else if (s[0] == 0x0a) {
		cons_newline(win);
	} else if (s[0] == 0x0d) {

	} else {
		if (win != 0) {
			api_putstrwin(win, padinfo.x, padinfo.y, 0, 1,s);
		}
		if (move != 0) {
			padinfo.x += 8;
			if (padinfo.x == 8 + 320) {
				cons_newline(win);
			}
		}
	}
	return;
}

void cons_newline(int win){
{
	int x, y;
	
	if ( padinfo.y < 30 + 16*5) {
		padinfo.y += 16;
	} else {
		for (y = 30; y < 30 + 16*5; y++) {
			for (x = 8; x < 8 + 320; x++) {
				winbuf[x + y * 336] = winbuf[x + (y + 16) * 336];
			}
		}
		/*for (y = 30 + 16*5; y < 30 + 16*6; y++) {
			for (x = 8; x < 8 + 320; x++) {
				winbuf[x + y * 336] = 7;
			}
		}*/
		api_boxfilwin(win, 8,30 + 16*5,8 + 320,30 + 16*6,7);
		api_refreshwin(win, 8, 30, 8 + 320, 30 + 16*7);
		}
	}
	padinfo.x = 8;
	return;
}

