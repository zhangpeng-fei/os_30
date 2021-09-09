/* �}���`�^�X�N�֌W */

#include "bootpack.h"

#define min(a, b)	(((a) < (b)) ? (a) : (b))

struct TASKCTL *taskctl;
struct TIMER *task_timer;
int prev_count = 0;

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
	task->flags = 2; /* ���쒆 */

	return;
}

void task_remove(struct TASK *task)
{
	int i;
	struct TASKLEVEL *tl = &taskctl->level[task->level];

	/* task���ǂ��ɂ��邩��T�� */
	for (i = 0; i < tl->running; i++) {
		if (tl->tasks[i] == task) {
			/* �����ɂ��� */
			break;
		}
	}

	tl->running--;
	if (i < tl->now) {
		tl->now--; /* �����̂ŁA��������킹�Ă��� */
	}
	if (tl->now >= tl->running) {
		/* now���������Ȓl�ɂȂ��Ă�����A�C������ */
		tl->now = 0;
	}
	task->flags = 1; /* �X���[�v�� */

	/* ���炵 */
	for (; i < tl->running; i++) {
		tl->tasks[i] = tl->tasks[i + 1];
	}

	return;
}

void task_switchsub(void)
{
	int i;
	/* ��ԏ�̃��x����T�� */
	for (i = 0; i < MAX_TASKLEVELS; i++) {
		if (taskctl->level[i].running > 0) {
			break; /* �������� */
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

void sysclock_task(void)
{
	unsigned char t[7];
	int i, j;
	char err, cnt;
	unsigned char s[6];
	static unsigned char adr[7] = { 0x00, 0x02, 0x04, 0x07, 0x08, 0x09, 0x32 };
	static unsigned char max[7] = { 0x60, 0x59, 0x23, 0x31, 0x12, 0x99, 0x99 };
	struct TASK *task = task_now();
	struct TIMER *clock_timer = timer_alloc();
	timer_init(clock_timer, &task->fifo, 1);
	timer_settime(clock_timer, 100);

	for (;;) {
		io_cli();
		if (fifo32_status(&task->fifo) == 0) {
			task_sleep(task);
			io_sti();
		} else {
			i = fifo32_get(&task->fifo);
			io_sti();
			if (i == 1) {
				for (cnt = 0; cnt < 3; cnt++) {
					err = 0;
					for (j = 0; j < 7; j++) {
						io_out8(0x70, adr[j]);
						t[j] = io_in8(0x71);
					}
					for (j = 0; j < 7; j++) {
						io_out8(0x70, adr[j]);
						if (t[j] != io_in8(0x71) || (t[j] & 0x0f) > 9 || t[j] > max[j]) {
							err = 1;
						}
					}
					if (err == 0) {
						break;
					}
				}
				struct SHTCTL *ctl = (struct SHTCTL *) *((int *) 0x0fe4);
				struct SHEET *sht = ctl->sheets[0];
				sprintf(s, "%02X:%02X:%02x\0", t[2], t[1], t[0]);
				putfonts8_asc_sht(sht, sht->bxsize - 45 - 24, sht->bysize - 21,
								  COL8_000000, COL8_C6C6C6, s, 9);

				sprintf(s, "%02X%02X/%02X/%02X\0", t[6], t[5], t[4], t[3]);
				putfonts8_asc_sht(sht, sht->bxsize - 135 -24, sht->bysize - 21,COL8_000000, COL8_C6C6C6, s, 10);

				timer_settime(clock_timer, 100);
			}
		}
	}
}


//struct TASK *open_taskmgrtask(struct SHTCTL *shtctl,unsigned int memtotal) {
//	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
//	struct TASK *task = task_alloc("taskmgr", 10);
//	int *taskmgr_fifo = (int *) memman_alloc_4k(memman, 128 * 4);
//	fifo32_init(&task->fifo, 128, taskmgr_fifo, task);
//	task->cons_stack = memman_alloc_4k(memman, 64 * 1024);
//	task->tss.esp = task->cons_stack + 64 * 1024 - 12;
//	task->tss.eip = (int) &taskmgr_task;
//	task->tss.es = 1 * 8;
//	task->tss.cs = 2 * 8;
//	task->tss.ss = 1 * 8;
//	task->tss.ds = 1 * 8;
//	task->tss.fs = 1 * 8;
//	task->tss.gs = 1 * 8;
//	*((int *) (task->tss.esp + 4)) = (int) shtctl;
//	*((int *) (task->tss.esp + 8)) = memtotal;
//	task_run(task, 2, 2); /* level=2, priority=2 */
//	return task;
//}
//
//void taskmgr_task(struct SHTCTL *shtctl, unsigned int memtotal){
//	int i;
//	int x = 12, y = 28, offset = 0;
//	int prev_time, curr_time;
//	char msg[40];
//	struct TASK *task = task_now();
//	struct TIMER *task_timer = timer_alloc();
//	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
//	struct FIFO32 *fifo = (struct FIFO32 *) *((int *) 0x0fec);
//	struct SHEET *sht = sheet_alloc(shtctl);
//	unsigned char *buf = (unsigned char *) memman_alloc_4k(memman, 400 * 300);
//
//	task->langmode = 3;
//
//	sheet_setbuf(sht, buf, 400, 300, -1); /* �����F�Ȃ� */
//	make_window8(buf, 400, 300, "�������", 0);
//	make_textbox8(sht, 5, 24, sht->bxsize - 5, sht->bysize - 5, COL8_FFFFFF);
//	//sht->flags |= 0x10;
//	memset(msg, 0, sizeof(msg));
//	strcpy(msg, "������   ������          �ȼ�    ����ʱ��");
//	putfonts8_asc(sht->buf, sht->bxsize, x + 1, y + 1, COL8_FFFFFF, msg);
//	putfonts8_asc(sht->buf, sht->bxsize, x + 0, y + 0, COL8_000000, msg);
//
//	sht->task = task;
//	sheet_slide(sht, 336, 8);
//	sheet_updown(sht, shtctl->top);
//	keywin_on(sht);
//	timer_init(task_timer, &task->fifo, 1);
//	prev_time = taskctl->tasks0[1].times - 99;
//	timer_settime(task_timer, 1);
//
//	for (;;) {
//		io_cli();
//		if (fifo32_status(&task->fifo) == 0) {
//			task_sleep(task);
//			io_sti();
//		} else {
//			i = fifo32_get(&task->fifo);
//			io_sti();
//			if (i == 1) {
//				timer_settime(task_timer, 100);
//				curr_time = taskctl->tasks0[1].times;
//				task_display(sht, offset, 1, curr_time - prev_time, memtotal);
//				prev_time = curr_time;
//			} else if (i == 4) {	/* �u�~�v�{�^���N���b�N */
//				timer_cancel(task_timer);
//				break;
//			} else if (256 <= i && i <= 511) { /* �L�[�{�[�h�f�[�^�i�^�X�NA�o�R�j */
////				if (i == '2' + 256) {
////					if (offset < taskctl->alive - DSP_LINE) {
////						offset++;
////					}
////				} else if (i == '8' + 256) {
////					if (offset > 0) {
////						offset--;
////					}
////				}
//				task_display(sht, offset, 0, 0, 0);
//			}
//		}
//	}
//
//	timer_free(task_timer);
//	memman_free_4k(memman, (int) sht->buf, 296 * 290 * 2);
//	sheet_free(sht);
////	fifo32_put_io(fifo, 2280);
//	for (;;) {
//		task_sleep(task);
//	}
//}
//
//void task_display(struct SHEET *sht, int offset, int cpu, int time, unsigned int memtotal)
//{
//	int i, j = 0;
//	int x = 12, y = 48;
//	char msg[40];
//	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
//
//	boxfill8(sht->buf, sht->bxsize, COL8_FFFFFF, 5, 24, sht->bxsize -5, sht->bysize -5);
//	for (i = offset; i < taskctl->alloc; i++) {
//		if (taskctl->tasks0[i].flags == 0) {
//			continue;
//		}
//		memset(msg, 0, sizeof(msg));
//		sprintf(msg, "%3d %-15s   %1d %7d.%02d", taskctl->tasks0[i].id,
//			taskctl->tasks0[i].name, taskctl->tasks0[i].level,
//			taskctl->tasks0[i].times / 100, taskctl->tasks0[i].times % 100);
//		putfonts8_asc(sht->buf, sht->bxsize, x, y + 16 * j++, COL8_000000, msg);
////		if (j >= DSP_LINE) {
////			break;
////		}
//	}
//	if (cpu == 0) {
//		sheet_refresh(sht, 8, 48, 8 + 280, 48 + 192);
//		return;
//	}
//
//	//boxfill8(sht->buf, sht->bxsize, COL8_FFFFFF, 8, 250, 8 + 280 - 1, 250 + 32 - 1);
//	memset(msg, 0, sizeof(msg));
//	sprintf(msg, "CPU    :        %3d%     %4d TS",
//		100 - min(time, 100), taskctl->alive);
//	putfonts8_asc(sht->buf, sht->bxsize, x, y + 202, COL8_0000FF, msg);
//	memset(msg, 0, sizeof(msg));
//	sprintf(msg, "Memory :    %7d //  %7d KB",
//		(memtotal - memman_total(memman)) / 1024, memtotal / 1024);
//	putfonts8_asc(sht->buf, sht->bxsize, x, y + 218, COL8_0000FF, msg);
//	sheet_refresh(sht, 8, 48, 8 + 280, 48 + 234);
//
//	return;
//}


void taskmgr_task(struct SHEET *sht, unsigned int memtotal){
	putfonts8_asc(sht->buf, sht->bxsize, 10, 30, COL8_000000, "ȥ����Ĳ���ϵͳ!");
}
//{
//	int i;
//	int x = 12, y = 28, offset = 0;
//	int prev_time, curr_time;
//	char msg[40];
//	struct TASK *task = task_now();
//	struct TIMER *task_timer = timer_alloc();
//	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
//	struct FIFO32 *fifo = (struct FIFO32 *) *((int *) 0x0fec);
//	struct SHTCTL *ctl = (struct SHTCTL *) *((int *) 0x0fe4);
//	struct SHEET *sht = sheet_alloc(ctl);
//	unsigned short *buf = (unsigned short *) memman_alloc_4k(memman, 296 * 290 * 2);
//
//	task->langmode = 1;
//	sheet_setbuf(sht, buf, 296, 290, -1); /* �����F�Ȃ� */
//	make_window8(buf, 296, 290, "taskmgr", 0);
//	make_textbox8(sht, 5, 24, 291, 284, COL8_000000);
//	//make_textbox8(sht, 5, 250, 291, 284, COL8_FFFFFF);
//	make_header8(sht,   5, 24,  291, 284, COL8_C6C6C6);
//
//	memset(msg, 0, sizeof(msg));
//	strcpy(msg, "ID  NAME            LV  TIME");
//	putfonts8_asc(sht->buf, sht->bxsize, x + 1, y + 1, COL8_FFFFFF, 1, msg);
//	putfonts8_asc(sht->buf, sht->bxsize, x + 0, y + 0, COL8_000000, 1, msg);
//
//	sht->task = taskmgr;
//	sheet_slide(sht, 336, 8);
//	sheet_updown(sht, ctl->top);
//	keywin_on(sht);
//	timer_init(task_timer, &task->fifo, 1);
//	prev_time = taskctl->tasks0[1].time - 99;
//	timer_settime(task_timer, 1);
//
//	for (;;) {
//		io_cli();
//		if (fifo32_status(&task->fifo) == 0) {
//			task_sleep(task);
//			io_sti();
//		} else {
//			i = fifo32_get(&task->fifo);
//			io_sti();
//			if (i == 1) {
//				timer_settime(task_timer, 100);
//				curr_time = taskctl->tasks0[1].time;
//				task_display(sht, offset, 1, curr_time - prev_time, memtotal);
//				prev_time = curr_time;
//			} else if (i == 4) {	/* �u�~�v�{�^���N���b�N */
//				timer_cancel(task_timer);
//				break;
//			} else if (256 <= i && i <= 511) { /* �L�[�{�[�h�f�[�^�i�^�X�NA�o�R�j */
//				if (i == '2' + 256) {
//					if (offset < taskctl->alive - DSP_LINE) {
//						offset++;
//					}
//				} else if (i == '8' + 256) {
//					if (offset > 0) {
//						offset--;
//					}
//				}
//				task_display(sht, offset, 0, 0, 0);
//			}
//		}
//	}
//
//	timer_free(task_timer);
//	memman_free_4k(memman, (int) sht->buf, 296 * 290 * 2);
//	sheet_free(sht);
//	fifo32_put_io(fifo, 2280);
//	for (;;) {
//		task_sleep(task);
//	}
//}
//
//void task_display(struct SHEET *sht, int offset, int cpu, int time, unsigned int memtotal)
//{
//	int i, j = 0;
//	int x = 12, y = 48;
//	char msg[40];
//	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
//
//	boxfill8(sht->buf, sht->bxsize, COL8_000000, 8, 48, 8 + 280 - 1, 48 + 192 - 1);
//	for (i = offset; i < taskctl->alloc; i++) {
//		if (taskctl->tasks0[i].flags == 0) {
//			continue;
//		}
//		memset(msg, 0, sizeof(msg));
//		sprintf(msg, "%3d %-15s   %1d %7d.%02d", i,
//			taskctl->tasks0[i].name, taskctl->tasks0[i].level,
//			taskctl->tasks0[i].time / 100, taskctl->tasks0[i].time % 100);
//		putfonts8_asc(sht->buf, sht->bxsize, x, y + 16 * j++, COL8_FFFFFF, 1, msg);
//		if (j >= DSP_LINE) {
//			break;
//		}
//	}
//	if (cpu == 0) {
//		sheet_refresh(sht, 8, 48, 8 + 280, 48 + 192);
//		return;
//	}
//
//	boxfill8(sht->buf, sht->bxsize, COL8_FFFFFF, 8, 250, 8 + 280 - 1, 250 + 32 - 1);
//	memset(msg, 0, sizeof(msg));
//	sprintf(msg, "CPU    :        %3d ��     %4d TS",
//		100 - min(time, 100), taskctl->alive);
//	putfonts8_asc(sht->buf, sht->bxsize, x, y + 202, COL8_0000FF, 1, msg);
//	memset(msg, 0, sizeof(msg));
//	sprintf(msg, "Memory :    %7d �^  %7d KB",
//		(memtotal - memman_total(memman)) / 1024, memtotal / 1024);
//	putfonts8_asc(sht->buf, sht->bxsize, x, y + 218, COL8_0000FF, 1, msg);
//	sheet_refresh(sht, 8, 48, 8 + 280, 48 + 234);
//
//	return;
//}

struct TASK *task_init(struct MEMMAN *memman)
{
	int i;
	struct TASK *task, *idle;
	struct SEGMENT_DESCRIPTOR *gdt = (struct SEGMENT_DESCRIPTOR *) ADR_GDT;

	taskctl = (struct TASKCTL *) memman_alloc_4k(memman, sizeof (struct TASKCTL));

	taskctl->ids = 0;
	taskctl->alive = 0;
	taskctl->alloc = 0;

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
	prev_count = timerctl.count;

	task = task_alloc("system", 6);
	task->flags = 2;	/* ���쒆�}�[�N */
	task->priority = 2; /* 0.02�b */
	task->level = 0;	/* �ō����x�� */
	task_add(task);
	task_switchsub();	/* ���x���ݒ� */
	load_tr(task->sel);
	task_timer = timer_alloc();
	timer_settime(task_timer, task->priority);

	idle = task_alloc("idle", 4);
	idle->tss.esp = memman_alloc_4k(memman, 64 * 1024) + 64 * 1024;
	idle->tss.eip = (int) &task_idle;
	idle->tss.es = 1 * 8;
	idle->tss.cs = 2 * 8;
	idle->tss.ss = 1 * 8;
	idle->tss.ds = 1 * 8;
	idle->tss.fs = 1 * 8;
	idle->tss.gs = 1 * 8;
	task_run(idle, MAX_TASKLEVELS - 1, 1);

	return task;
}

struct TASK *task_alloc(char *name, int len)
{
	int i;
	struct TASK *task;
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	if(len > MAX_TASKNAME - 1){
		return 0;
	}
	for (i = 0; i < MAX_TASKS; i++) {
		if (taskctl->tasks0[i].flags == 0) {
			task = &(taskctl->tasks0[i]);
			strncpy(task->name, name, len);
			task->name[len] = 0;
			task->id = taskctl->ids++;
			task->times = 0;

			task->flags = 1;  /* �g�p���}�[�N */
			task->tss.eflags = 0x00000202; /* IF = 1; */
			task->tss.eax = 0; /* �Ƃ肠����0�ɂ��Ă������Ƃɂ��� */
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

			if (i >= taskctl->alloc) {
				taskctl->alloc = i + 1;
			}
			taskctl->alive++;
			return task;
		}
	}
	return 0; /* �����S���g�p�� */
}

void task_run(struct TASK *task, int level, int priority)
{
	if (level < 0) {
		level = task->level; /* ���x����ύX���Ȃ� */
	}
	if (priority > 0) {
		task->priority = priority;
	}

	if (task->flags == 2 && task->level != level) { /* ���쒆�̃��x���̕ύX */
		task_remove(task); /* ��������s�����flags��1�ɂȂ�̂ŉ���if�����s����� */
	}
	if (task->flags != 2) {
		/* �X���[�v����N�������ꍇ */
		task->level = level;
		task_add(task);
	}

	taskctl->lv_change = 1; /* ����^�X�N�X�C�b�`�̂Ƃ��Ƀ��x���������� */
	return;
}

void task_sleep(struct TASK *task)
{
	struct TASK *now_task;
	if (task->flags == 2) {
		/* ���쒆�������� */
		now_task = task_now();
		task_remove(task); /* ��������s�����flags��1�ɂȂ� */
		if (task == now_task) {
			/* �������g�̃X���[�v�������̂ŁA�^�X�N�X�C�b�`���K�v */
//			now_task->times += (timerctl.count - prev_count);
//			prev_count = timerctl.count;
			task_switchsub();
			now_task = task_now(); /* �ݒ��ł́A�u���݂̃^�X�N�v�������Ă��炤 */
			farjmp(0, now_task->sel);
		}
	}
	return;
}

void task_switch(void)
{

	struct TASKLEVEL *tl = &taskctl->level[taskctl->now_lv];
	struct TASK *new_task, *now_task = tl->tasks[tl->now];
	tl->now++;
//	now_task->times += (timerctl.count) - prev_count;
//	prev_count = timerctl.count;
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
