@echo off
if "%CONFIGURATION%"=="" set CONFIGURATION=Debug
CD ..
%CONFIGURATION%\testrunner.exe %1 %2 %3 %4 %5
