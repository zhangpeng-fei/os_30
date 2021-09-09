/* マルチタスク関係 */

#include <stdio.h>
#include <string.h>
#include "bootpack.h"

#define DSP_LINE	12
#define min(a, b)	(((a) < (b)) ? (a) : (b))

void task_display(struct SHEET *sht, int offset, int cpu, int time, unsigned int memtotal);

unsigned int prev_count;
struct TASKCTL *taskctl;
struct TIMER *task_timer;
extern struct TASK *taskmgr;

struct TASK *task_now(void)
{
	struct TASKLEVEL *tl = &taskctl->level[taskctl->now_lv];
	return tl->tasks[tl->now];
}

void task_add(struct TASK *task)
{
	struct TASKLEVEL *tl = &taskctl->level[task->level];
	tl->tasks[tl->running] = task;
	tl->running++;
	task->flags = 2; /* 動作中 */
	return;
}

void task_remove(struct TASK *task)
{
	int i;
	struct TASKLEVEL *tl = &taskctl->level[task->level];

	/* taskがどこにいるかを探す */
	for (i = 0; i < tl->running; i++) {
		if (tl->tasks[i] == task) {
			/* ここにいた */
			break;
		}
	}

	tl->running--;
	if (i < tl->now) {
		tl->now--; /* ずれるので、これもあわせておく */
	}
	if (tl->now >= tl->running) {
		/* nowがおかしな値になっていたら、修正する */
		tl->now = 0;
	}
	task->flags = 1; /* スリープ中 */

	/* ずらし */
	for (; i < tl->running; i++) {
		tl->tasks[i] = tl->tasks[i + 1];
	}

	return;
}

void task_switchsub(void)
{
	int i;
	/* 一番上のレベルを探す */
	for (i = 0; i < MAX_TASKLEVELS; i++) {
		if (taskctl->level[i].running > 0) {
			break; /* 見つかった */
		}
	}
	taskctl->now_lv = i;
	taskctl->lv_change = 0;
	return;
}

void task_idle(void)
{
	for (;;) {
		io_hlt();
	}
}

struct TASK *task_init(struct MEMMAN *memman)
{
	int i;
	struct TASK *task, *idle;
	struct SEGMENT_DESCRIPTOR *gdt = (struct SEGMENT_DESCRIPTOR *) ADR_GDT;

	taskctl = (struct TASKCTL *) memman_alloc_4k(memman, sizeof (struct TASKCTL));
	for (i = 0; i < MAX_TASKS; i++) {
		taskctl->tasks0[i].flags = 0;
		taskctl->tasks0[i].sel = (TASK_GDT0 + i) * 8;
		taskctl->tasks0[i].tss.ldtr = (TASK_GDT0 + MAX_TASKS + i) * 8;
		set_segmdesc(gdt + TASK_GDT0 + i, 103, (int) &taskctl->tasks0[i].tss, AR_TSS32);
		set_segmdesc(gdt + TASK_GDT0 + MAX_TASKS + i, 15, (int) taskctl->tasks0[i].ldt, AR_LDT);
	}
	for (i = 0; i < MAX_TASKLEVELS; i++) {
		taskctl->level[i].running = 0;
		taskctl->level[i].now = 0;
	}
	taskctl->alloc = 0;
	taskctl->alive = 0;

	task = task_alloc();
	task->flags = 2;	/* 動作中マーク */
	task->priority = 2; /* 0.02秒 */
	task->level = 0;	/* 最高レベル */
	task->time = 0;
	strcpy(task->name, "system");
	task_add(task);
	task_switchsub();	/* レベル設定 */
	load_tr(task->sel);
	task_timer = timer_alloc();
	timer_settime(task_timer, task->priority);

	idle = task_alloc();
	idle->tss.esp = memman_alloc_4k(memman, 64 * 1024) + 64 * 1024;
	idle->tss.eip = (int) &task_idle;
	idle->tss.es = 1 * 8;
	idle->tss.cs = 2 * 8;
	idle->tss.ss = 1 * 8;
	idle->tss.ds = 1 * 8;
	idle->tss.fs = 1 * 8;
	idle->tss.gs = 1 * 8;
	idle->time = 0;
	strcpy(idle->name, "idle");
	task_run(idle, MAX_TASKLEVELS - 1, 1);

	taskctl->task_fpu = 0;

	return task;
}

struct TASK *task_alloc(void)
{
	int i;
	struct TASK *task;
	for (i = 0; i < MAX_TASKS; i++) {
		if (taskctl->tasks0[i].flags == 0) {
			task = &taskctl->tasks0[i];
			task->flags = 1; /* 使用中マーク */
			task->tss.eflags = 0x00000202; /* IF = 1; */
			task->tss.eax = 0; /* とりあえず0にしておくことにする */
			task->tss.ecx = 0;
			task->tss.edx = 0;
			task->tss.ebx = 0;
			task->tss.ebp = 0;
			task->tss.esi = 0;
			task->tss.edi = 0;
			task->tss.es = 0;
			task->tss.ds = 0;
			task->tss.fs = 0;
			task->tss.gs = 0;
			task->tss.iomap = 0x40000000;
			task->tss.ss0 = 0;
			task->fpu[0] = 0x037f; /* CW(control word) */
			task->fpu[1] = 0x0000; /* SW(status word)  */
			task->fpu[2] = 0xffff; /* TW(tag word)     */
			for (i = 3; i < 108 / 4; i++) {
				task->fpu[i] = 0;
			}
			if (i >= taskctl->alloc) {
				taskctl->alloc = i + 1;
			}
			taskctl->alive++;
			return task;
		}
	}
	return 0; /* もう全部使用中 */
}

void task_run(struct TASK *task, int level, int priority)
{
	if (level < 0) {
		level = task->level; /* レベルを変更しない */
	}
	if (priority > 0) {
		task->priority = priority;
	}

	if (task->flags == 2 && task->level != level) { /* 動作中のレベルの変更 */
		task_remove(task); /* これを実行するとflagsは1になるので下のifも実行される */
	}
	if (task->flags != 2) {
		/* スリープから起こされる場合 */
		task->level = level;
		task_add(task);
	}

	taskctl->lv_change = 1; /* 次回タスクスイッチのときにレベルを見直す */
	return;
}

void task_sleep(struct TASK *task)
{
	struct TASK *now_task;
	if (task->flags == 2) {
		/* 動作中だったら */
		now_task = task_now();
		now_task->time += (timerctl.count - prev_count);
		prev_count = timerctl.count;
		task_remove(task); /* これを実行するとflagsは1になる */
		if (task == now_task) {
			/* 自分自身のスリープだったので、タスクスイッチが必要 */
			task_switchsub();
			now_task = task_now(); /* 設定後での、「現在のタスク」を教えてもらう */
			farjmp(0, now_task->sel);
		}
	}
	return;
}

void task_switch(void)
{
	struct TASKLEVEL *tl = &taskctl->level[taskctl->now_lv];
	struct TASK *new_task, *now_task = tl->tasks[tl->now];
	now_task->time += (timerctl.count - prev_count);
	prev_count = timerctl.count;
	tl->now++;
	if (tl->now == tl->running) {
		tl->now = 0;
	}
	if (taskctl->lv_change != 0) {
		task_switchsub();
		tl = &taskctl->level[taskctl->now_lv];
	}
	new_task = tl->tasks[tl->now];
	timer_settime(task_timer, new_task->priority);
	if (new_task != now_task) {
		farjmp(0, new_task->sel);
	}
	return;
}

void task_free(struct TASK *task)
{
	int i, j;
	io_cli();
	task->flags = 0;
	if (taskctl->task_fpu == task) {
		taskctl->task_fpu = 0;
	}
	io_sti();
	i = (int) (task - taskctl->tasks0);
	if (i + 1 >= taskctl->alloc) {
		for (j = i - 1; j >= 0; j--) {
			if (taskctl->tasks0[j].flags != 0) {
				break;
			}
		}
		taskctl->alloc = j + 1;
	}
	taskctl->alive--;
	return;
}

void taskmgr_task(unsigned int memtotal)
{
	int i;
	int x = 12, y = 28, offset = 0;
	int prev_time, curr_time;
	char msg[40];
	struct TASK *task = task_now();
	struct TIMER *task_timer = timer_alloc();
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	struct FIFO32 *fifo = (struct FIFO32 *) *((int *) 0x0fec);
	struct SHTCTL *ctl = (struct SHTCTL *) *((int *) 0x0fe4);
	struct SHEET *sht = sheet_alloc(ctl);
	unsigned short *buf = (unsigned short *) memman_alloc_4k(memman, 296 * 290 * 2);

	task->langmode = 1;
	sheet_setbuf(sht, buf, 296, 290, -1); /* 透明色なし */
	make_window8(buf, 296, 290, "taskmgr", 0);
	make_textbox8(sht, 5, 24, 291, 284, COL8_000000);
	//make_textbox8(sht, 5, 250, 291, 284, COL8_FFFFFF);
	make_header8(sht,   5, 24,  291, 284, COL8_C6C6C6);
	
	memset(msg, 0, sizeof(msg));
	strcpy(msg, "ID  NAME            LV  TIME");
	putfonts8_asc(sht->buf, sht->bxsize, x + 1, y + 1, COL8_FFFFFF, 1, msg);
	putfonts8_asc(sht->buf, sht->bxsize, x + 0, y + 0, COL8_000000, 1, msg);

	sht->task = taskmgr;
	sheet_slide(sht, 336, 8);
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
			if (i == 1) {
				timer_settime(task_timer, 100);
				curr_time = taskctl->tasks0[1].time;
				task_display(sht, offset, 1, curr_time - prev_time, memtotal);
				prev_time = curr_time;
			} else if (i == 4) {	/* 「×」ボタンクリック */
				timer_cancel(task_timer);
				break;
			} else if (256 <= i && i <= 511) { /* キーボードデータ（タスクA経由） */
				if (i == '2' + 256) {
					if (offset < taskctl->alive - DSP_LINE) {
						offset++;
					}
				} else if (i == '8' + 256) {
					if (offset > 0) {
						offset--;
					}
				}
				task_display(sht, offset, 0, 0, 0);
			}
		}
	}

	timer_free(task_timer);
	memman_free_4k(memman, (int) sht->buf, 296 * 290 * 2);
	sheet_free(sht);
	fifo32_put_io(fifo, 2280);
	for (;;) {
		task_sleep(task);
	}
}

void task_display(struct SHEET *sht, int offset, int cpu, int time, unsigned int memtotal)
{
	int i, j = 0;
	int x = 12, y = 48;
	char msg[40];
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;

	boxfill8(sht->buf, sht->bxsize, COL8_000000, 8, 48, 8 + 280 - 1, 48 + 192 - 1);
	for (i = offset; i < taskctl->alloc; i++) {
		if (taskctl->tasks0[i].flags == 0) {
			continue;
		}
		memset(msg, 0, sizeof(msg));
		sprintf(msg, "%3d %-15s   %1d %7d.%02d", i,
			taskctl->tasks0[i].name, taskctl->tasks0[i].level,
			taskctl->tasks0[i].time / 100, taskctl->tasks0[i].time % 100);
		putfonts8_asc(sht->buf, sht->bxsize, x, y + 16 * j++, COL8_FFFFFF, 1, msg);
		if (j >= DSP_LINE) {
			break;
		}
	}
	if (cpu == 0) {
		sheet_refresh(sht, 8, 48, 8 + 280, 48 + 192);
		return;
	}

	boxfill8(sht->buf, sht->bxsize, COL8_FFFFFF, 8, 250, 8 + 280 - 1, 250 + 32 - 1);
	memset(msg, 0, sizeof(msg));
	sprintf(msg, "CPU    :        %3d ％     %4d TS",
		100 - min(time, 100), taskctl->alive);
	putfonts8_asc(sht->buf, sht->bxsize, x, y + 202, COL8_0000FF, 1, msg);
	memset(msg, 0, sizeof(msg));
	sprintf(msg, "Memory :    %7d ／  %7d KB",
		(memtotal - memman_total(memman)) / 1024, memtotal / 1024);
	putfonts8_asc(sht->buf, sht->bxsize, x, y + 218, COL8_0000FF, 1, msg);
	sheet_refresh(sht, 8, 48, 8 + 280, 48 + 234);

	return;
}

int *inthandler07(int *esp)
{
	struct TASK *now = task_now();
	io_cli();
	clts();
	if (taskctl->task_fpu != now) {
		if (taskctl->task_fpu != 0) {
			fnsave(taskctl->task_fpu->fpu);
		}
		frstor(now->fpu);
		taskctl->task_fpu = now;
	}
	io_sti();
	return 0;
}
