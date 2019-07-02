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

prompt {$r}$t$p$g
REM set watcom enviroment variables if not set correctly
call envwatcom.cmd

REM set pmprintf enviroment to enable pmprintf/trace
REM out for release
rem set __DEBUG_PMPRINTF__=1
set __DEBUG_PMPRINTF_LEVEL2__=1

REM set debug enviroment to make a debug build (currently not configured in makefiles)
REM out for release
set __DEBUG__=1

REM touch bldlevel.c to force rebuild for correct build date/time
wtouch bldlevel.c

REM close running instance (sendmsg required)
call sendmsg MyIP.exe WM_QUIT >NUL

REM start wmake, -e for 'do not ask for file deletetion' on build failure
call wmake -e -h

REM @echo Running .....
REM @call MyIp.exe
