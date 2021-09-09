TOOLPATH = ../z_tools/
INCPATH  = ../z_tools/haribote/

MAKE     = $(TOOLPATH)make.exe -r
EDIMG    = $(TOOLPATH)edimg.exe
IMGTOL   = $(TOOLPATH)imgtol.com
COPY     = copy
DEL      = del

# デフォルト動作

default :
	$(MAKE) kernel.img

# ファイル生成規則

kernel.img : kernel/ipl16.bin kernel/kernel.sys Makefile \
		app/cpuid.hrb app/readfile.hrb app/notepad.hrb app/tview.hrb calc/calc.hrb 
	$(EDIMG)   imgin:../z_tools/fdimg0at.tek \
		wbinimg src:kernel/ipl16.bin len:512 from:0 to:0 \
		copy from:kernel/kernel.sys to:@: \
		copy from:app/cpuid.hrb to:@: \
		copy from:app/readfile.hrb to:@: \
		copy from:app/type.hrb to:@: \
		copy from:app/notepad.hrb to:@: \
		copy from:app/tview.hrb to:@: \
		copy from:calc/calc.hrb  to:@: \
		copy from:src/pic/rapuwp.jpg to:@: \
		copy from:src/pic/night.bmp to:@: \
		copy from:src/txt/notepad.txt to:@: \
		copy from:src/fnt/nihongo.fnt to:@: \
		imgout:kernel.img

# コマンド

run :
	$(MAKE) kernel.img
	$(COPY) kernel.img ..\z_tools\qemu\fdimage0.bin
	$(MAKE) -C ../z_tools/qemu

run_only :
	$(MAKE) -C ../z_tools/qemu

install :
	$(MAKE) kernel.img
	$(IMGTOL) w a: kernel.img

full :
	$(MAKE) -C kernel
	$(MAKE) -C lib
	$(MAKE) -C dev/winhelo
	$(MAKE) -C dev/winhelo2
	$(MAKE) -C dev/winhelo3
	$(MAKE) -C dev/star1
	$(MAKE) -C dev/stars
	$(MAKE) -C dev/stars2
	$(MAKE) -C dev/lines
	$(MAKE) -C dev/walk
	$(MAKE) -C dev/noodle
	$(MAKE) -C dev/color
	$(MAKE) -C dev/color2
	$(MAKE) -C dev/sosu3
	$(MAKE) -C dev/notrec
	$(MAKE) -C dev/bball
	$(MAKE) -C dev/invader
	$(MAKE) -C dev/tview
	$(MAKE) -C dev/mmlplay
	$(MAKE) -C dev/gview
	$(MAKE) -C dev/vclock
	$(MAKE) -C dev/cpuid
	$(MAKE) -C dev/puzzle
	$(MAKE) -C dev/sincurve
	$(MAKE) -C dev/wframe
	$(MAKE) kernel.img

run_full :
	$(MAKE) full
	$(COPY) kernel.img ..\z_tools\qemu\fdimage0.bin
	$(MAKE) -C ../z_tools/qemu

install_full :
	$(MAKE) full
	$(IMGTOL) w a: kernel.img

run_os :
	$(MAKE) -C kernel
	$(MAKE) run

clean :
# 何もしない

src_only :
	$(MAKE) clean
	-$(DEL) kernel.img

clean_full :
	$(MAKE) -C kernel		clean
	$(MAKE) -C lib			clean
	$(MAKE) -C dev/winhelo		clean
	$(MAKE) -C dev/winhelo2		clean
	$(MAKE) -C dev/winhelo3		clean
	$(MAKE) -C dev/star1		clean
	$(MAKE) -C dev/stars		clean
	$(MAKE) -C dev/stars2		clean
	$(MAKE) -C dev/lines		clean
	$(MAKE) -C dev/walk			clean
	$(MAKE) -C dev/noodle		clean
	$(MAKE) -C dev/color		clean
	$(MAKE) -C dev/color2		clean
	$(MAKE) -C dev/sosu3		clean
	$(MAKE) -C dev/notrec		clean
	$(MAKE) -C dev/bball		clean
	$(MAKE) -C dev/invader		clean
	$(MAKE) -C dev/tview		clean
	$(MAKE) -C dev/mmlplay		clean
	$(MAKE) -C dev/gview		clean
	$(MAKE) -C dev/vclock		clean
	$(MAKE) -C dev/cpuid		clean
	$(MAKE) -C dev/puzzle		clean
	$(MAKE) -C dev/sincurve		clean
	$(MAKE) -C dev/wframe		clean

src_only_full :
	$(MAKE) -C kernel		src_only
	$(MAKE) -C lib			src_only
	$(MAKE) -C dev/winhelo		src_only
	$(MAKE) -C dev/winhelo2		src_only
	$(MAKE) -C dev/winhelo3		src_only
	$(MAKE) -C dev/star1		src_only
	$(MAKE) -C dev/stars		src_only
	$(MAKE) -C dev/stars2		src_only
	$(MAKE) -C dev/lines		src_only
	$(MAKE) -C dev/walk			src_only
	$(MAKE) -C dev/noodle		src_only
	$(MAKE) -C dev/color		src_only
	$(MAKE) -C dev/color2		src_only
	$(MAKE) -C dev/sosu3		src_only
	$(MAKE) -C dev/notrec		src_only
	$(MAKE) -C dev/bball		src_only
	$(MAKE) -C dev/invader		src_only
	$(MAKE) -C dev/tview		src_only
	$(MAKE) -C dev/mmlplay		src_only
	$(MAKE) -C dev/gview		src_only
	$(MAKE) -C dev/vclock		src_only
	$(MAKE) -C dev/cpuid		src_only
	$(MAKE) -C dev/puzzle		src_only
	$(MAKE) -C dev/sincurve		src_only
	$(MAKE) -C dev/wframe		src_only
	-$(DEL) kernel.img

refresh :
	$(MAKE) full
	$(MAKE) clean_full
	-$(DEL) kernel.img
