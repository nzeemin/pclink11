@echo off
for /f %%i in ('git rev-list HEAD --count') do set REVISION=%%i
echo The revision is %REVISION%
echo. > Version.h
echo #define APP_REVISION %REVISION% >> Version.h
echo. >> Version.h
echo #define APP_VERSION_STRING "V0.%REVISION%" >> Version.h
