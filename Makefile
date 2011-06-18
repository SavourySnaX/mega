#!/bin/make


LIBS=-lGL -lopenal -lglfw

DEFINES=-Isrc/ -Isrc/audio -Isrc/gui/ -Isrc/cpu/m68k -Isrc/cpu/z80 -Isrc/cpu/sh2
OPTIMISATIONS=-O3 -g
CFLAGS=$(DEFINES) $(OPTIMISATIONS) -Wall -Wextra -Werror -Wuninitialized -Winit-self -Wstrict-aliasing -Wfloat-equal -Wshadow -pedantic-errors -ansi 
#-Wconversion

cheapDeps=src/*.h src/cpu/m68k/*.h src/gui/*.h src/audio/*.h src/cpu/z80/*.h src/cpu/sh2/*.h

megadrive=out/cpu68000.o out/cpu68000_ops.o out/cpu68000_dis.o out/cpuZ80.o out/cpuZ80_ops.o out/cpuZ80_dis.o out/sn76489.o out/ym2612.o out/memory.o out/debugger.o out/os.o out/cpuSH2.o out/cpuSH2_Memory.o out/cpuSH2_ops.o out/cpuSH2_dis.o out/cpuSH2_IO.o

mega:	$(megadrive)
	gcc $(CFLAGS) $(megadrive) $(LIBS) -o mega

out/cpu68000.o:	src/cpu/m68k/cpu.c $(cheapDeps)
	gcc -c $(CFLAGS) $< -o $@

out/cpu68000_ops.o:	src/cpu/m68k/cpu_ops.c $(cheapDeps)
	gcc -c $(CFLAGS) $< -o $@

out/cpu68000_dis.o:	src/cpu/m68k/cpu_dis.c $(cheapDeps)
	gcc -c $(CFLAGS) $< -o $@

out/cpuZ80.o:	src/cpu/z80/z80.c $(cheapDeps)
	gcc -c $(CFLAGS) $< -o $@

out/cpuZ80_ops.o:	src/cpu/z80/z80_ops.c $(cheapDeps)
	gcc -c $(CFLAGS) $< -o $@

out/cpuZ80_dis.o:	src/cpu/z80/z80_dis.c $(cheapDeps)
	gcc -c $(CFLAGS) $< -o $@

out/cpuSH2.o:	src/cpu/sh2/sh2.c $(cheapDeps)
	gcc -c $(CFLAGS) $< -o $@

out/cpuSH2_ops.o:	src/cpu/sh2/sh2_ops.c $(cheapDeps)
	gcc -c $(CFLAGS) $< -o $@

out/cpuSH2_dis.o:	src/cpu/sh2/sh2_dis.c $(cheapDeps)
	gcc -c $(CFLAGS) $< -o $@

out/cpuSH2_Memory.o:	src/cpu/sh2/sh2_memory.c $(cheapDeps)
	gcc -c $(CFLAGS) $< -o $@

out/cpuSH2_IO.o:	src/cpu/sh2/sh2_ioregisters.c $(cheapDeps)
	gcc -c $(CFLAGS) $< -o $@

out/sn76489.o:	src/audio/psg.c $(cheapDeps)
	gcc -c $(CFLAGS) $< -o $@

out/ym2612.o:	src/audio/lj_ym2612.c $(cheapDeps)
	gcc -c $(CFLAGS) $< -o $@

out/memory.o:	src/memory.c $(cheapDeps)
	gcc -c $(CFLAGS) $< -o $@

out/debugger.o:	src/gui/debugger.c $(cheapDeps)
	gcc -c $(CFLAGS) $< -o $@

out/os.o:	src/mgmain.c $(cheapDeps)
	gcc -c $(CFLAGS) $< -o $@ 


clean:
	rm out/*.o
	rm mega
