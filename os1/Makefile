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

kernel.img : kernel/ipl16.bin kernel/kernel.sys Makefile 
	$(EDIMG)   imgin:../z_tools/fdimg0at.tek \
		wbinimg src:kernel/ipl16.bin len:512 from:0 to:0 \
		copy from:kernel/kernel.sys to:@: \
		copy from:winhelo/winhelo.hrb to:@: \
		copy from:cpuid/cpuid.hrb to:@: \
		copy from:hello4/hello4.hrb to:@: \
		copy from:app/readfile.hrb to:@: \
		copy from:app/type.hrb to:@: \
		copy from:app/notepad.hrb to:@: \
		copy from:app/tview.hrb to:@: \
		copy from:calc/calc.hrb  to:@: \
		copy from:src/pic/rapuwp.jpg to:@: \
		copy from:src/pic/night.bmp to:@: \
		copy from:src/txt/notepad.txt to:@: \
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
	$(MAKE) -C winhelo
	$(MAKE) -C cpuid
	$(MAKE) -C hello4
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
	$(MAKE) -C lib		clean
	$(MAKE) -C winhelo		clean
	$(MAKE) -C cpuid		clean
	$(MAKE) -C hello4		clean

src_only_full :
	$(MAKE) -C kernel		src_only
	$(MAKE) -C lib		src_only
	$(MAKE) -C winhelo		src_only
	$(MAKE) -C cpuid		src_only
	$(MAKE) -C hello4		src_only
	-$(DEL) kernel.img

refresh :
	$(MAKE) full
	$(MAKE) clean_full
	-$(DEL) kernel.img
