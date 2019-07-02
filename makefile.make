# MyIP master makefile

.EXTENSIONS:.o

# Tools setup

#cc = wcc386
cc = gcc
as = wasm
#link = wlink   --- use wlink hll instead from \usr\bin
link = wl

# Build flags

# 20090430 AB added -ef, used for 'jump to error' in VisualSlickEdit (-ef doesn't work with OW1.7!)
#cflags = -zq -wx -we -ef -of -I..\h -I$(%WATCOM)\h\os2
cflags = -O2 -Wall -g -c
aflags = -zq
lflags =

ifdef __DEBUG__
#cflags += -d2
#cflags += -d__DEBUG__
cflags += -D__DEBUG__=1
aflags += -d2
#lflags += debug hll all op hLLpack
#lflags += debug Hll 
lflags += debug all
else
cflags += -oaxt
endif

# 20100111 AB added pmprintf / trace via enviroment setting
# set pmprintf in your enviroment to enable output to pmprintf (f.i. set pmprintf=1)
ifdef __DEBUG_PMPRINTF__
#cflags += -d__DEBUG_PMPRINTF__
cflags += -D__DEBUG_PMPRINTF__=1
lflags += lib pmprintf.lib
endif

ifdef __DEBUG_PMPRINTF_LEVEL2__
#cflags += -d__DEBUG_PMPRINTF_LEVEL2__
cflags += -D__DEBUG_PMPRINTF_LEVEL2__=1
lflags += lib pmprintf.lib
endif

# File locations

.c: c
.h: h
.asm: a
.o: o

# Implicit build rules

#.c.obj: .autodepend
#   $(cc) $(cflags) $@

%.o : %.c
	$(cc) $(cflags) $< 
	
#%.obj : %.o	
#	rename $[@.o $@.obj 

%.o : %.asm
	$(as) $(aflags) $@

# Targets and explicit rules

all : MyIP.exe

#MyIPobjs = myip.obj bldlevel.obj getopt.obj
MyIPobjs = myip.o bldlevel.o getopt.o

MyIP.exe : $(MyIPobjs) MyIP.wlk bldlevel.txt
	$(link) $(lflags) file { $(MyIPobjs) }  
	mapxqs Wat_MyIP.map -o MyIP.xqs -l                
	wat2map.cmd Wat_MyIP.map MyIP.map

bldlevel.txt : $(MyIPobjs) MyIP.wlk
	@make_buildlevel.cmd

# Clean target

clean : .symbolic
	@if exist *.o 	del *.o                           
	@if exist *.obj del *.obj                         
	@if exist *.exe del *.exe                         
	@if exist *.dll del *.dll                         
	@if exist *.res del *.res                         
	@if exist *.msg del *.msg                         
	@if exist *.inf del *.inf                         
	@if exist *.map del *.map
