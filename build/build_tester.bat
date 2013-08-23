@echo off
rem

rem call C:\spark\compile-server\scripts\_config.bat

rem set GITBIN=C:\Program Files (x86)\Git\bin
rem set BASHBIN=C:\Program Files (x86)\Git\bin
set MAKEBIN=C:\Program Files (x86)\GnuWin32\bin
set ARMTOOLS=C:\Program Files (x86)\GNU Tools ARM Embedded\4.7 2013q2\bin
rem set PATH=%PATH%;%GITBIN%;%BASHBIN%;%MAKEBIN%;%ARMTOOLS%;

set PATH=%PATH%;%MAKEBIN%;%ARMTOOLS%;

rem make all
del tester.*

set USER_DIR=tester
set CORE_FIRMWARE=wifitester
make clean all

pause
