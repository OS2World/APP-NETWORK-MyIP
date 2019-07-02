@echo off
REM sample script to check periodically your IP address
REM -r5     check every 5 seconds for IP address
REM -c 180  log only 180 times 5 seconds (= 15 minutes) when no change in IP address is detected
REM

MyIP.exe -f MyIP.cfg -lMyIP.log -r2 -c 20 -t2 -s" Own IP is " %1 %2 %3 %4
