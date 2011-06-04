#!/bin/make


LIBS=-lGL -lopenal -lglfw

DEFINES=
#-D__IGNORE_TYPES
OPTIMISATIONS=-O3 -g
CFLAGS=$(DEFINES) $(OPTIMISATIONS) -Wall -Wextra -Werror -Wuninitialized -Winit-self -Wstrict-aliasing -Wfloat-equal -Wshadow -pedantic-errors -ansi 
#-Wconversion -ansi -pedantic-errors 

cheapDeps=*.h gui/*.h

megadrive=out/cpu68000.o out/cpu68000_ops.o out/cpu68000_dis.o out/cpuZ80.o out/cpuZ80_ops.o out/cpuZ80_dis.o out/sn76489.o out/ym2612.o out/memory.o out/debugger.o out/os.o

mega:	$(megadrive)
	gcc $(CFLAGS) $(megadrive) $(LIBS) -o mega

out/cpu68000.o:	cpu.c $(cheapDeps)
	gcc -c $(CFLAGS) $< -o $@

out/cpu68000_ops.o:	cpu_ops.c $(cheapDeps)
	gcc -c $(CFLAGS) $< -o $@

out/cpu68000_dis.o:	cpu_dis.c $(cheapDeps)
	gcc -c $(CFLAGS) $< -o $@

out/cpuZ80.o:	z80.c $(cheapDeps)
	gcc -c $(CFLAGS) $< -o $@

out/cpuZ80_ops.o:	z80_ops.c $(cheapDeps)
	gcc -c $(CFLAGS) $< -o $@

out/cpuZ80_dis.o:	z80_dis.c $(cheapDeps)
	gcc -c $(CFLAGS) $< -o $@

out/sn76489.o:	psg.c $(cheapDeps)
	gcc -c $(CFLAGS) $< -o $@

out/ym2612.o:	lj_ym2612.c $(cheapDeps)
	gcc -c $(CFLAGS) $< -o $@

out/memory.o:	memory.c $(cheapDeps)
	gcc -c $(CFLAGS) $< -o $@

out/debugger.o:	gui/debugger.c $(cheapDeps)
	gcc -c $(CFLAGS) $< -o $@

out/os.o:	mgmain.c
	gcc -c $(CFLAGS) $< -o $@ 


clean:
	rm out/*.o
	rm mega
