@echo off
for /f %%i in ('git rev-list HEAD --count') do set REVISION=%%i
echo The revision is %REVISION%
echo.> version.h
echo #define APP_REVISION %REVISION%>> version.h
echo.>> version.h
echo #define APP_VERSION_STRING "V0.%REVISION%">> version.h
