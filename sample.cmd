@echo off
REM sample script to check periodically your IP address
REM -f      use MyIP.cfg from current directory instead from \mptn\etc
REM -l      log output to file too
REM -r300   check every 300 seconds for your IP address
REM -c 180  when no change in IP address is detected then log only 180 times r seconds (180 x 5 = 15 minutes)
REM -t 2    use time format 2
REM -s      precede output with string " Own IP is "
REM -p2     ping every 2 seconds the ping server to check for connection drops

MyIP.exe -f MyIP.cfg -lMyIP.log -r300 -c 180 -t2 -s" Own IP is " -p 2 %1 %2 %3 %4
