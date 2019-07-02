@echo off
REM ***************************************************************************
REM 20100818 Andreas Buchinger
REM
REM Script to build MyIP especially within VisualSlickEdit
REM
REM Closes a running MyIP instances before building (sendmsg required)
REM
REM Set the different DEBUG enviroment variables to tune debug mesage output
REM pmprintf to enable TRACE via pmprintf library (pmprintf.dll required)
REM DEBUG generall debug messages
REM DEBUG_TERM is for bloatcom
REM ***************************************************************************

REM prompt {$r}$t$p$g
REM set watcom enviroment variables if not set correctly
call envwatcom.cmd

REM set pmprintf enviroment to enable pmprintf/trace
set __DEBUG_PMPRINTF__=

REM set debug enviroment to make a debug build (currently not configured in makefiles)
set __DEBUG__=

REM touch bldlevel.c to force rebuild for correct build date/time
REM wtouch bldlevel.c

wmake clean

REM close running instance (sendmsg required)
call sendmsg MyIP.exe WM_QUIT >NUL

REM start wmake, -e for 'do not ask for file deletetion' on build failure
call wmake -h -e

REM @echo Running .....
REM @call MyIp.exe
