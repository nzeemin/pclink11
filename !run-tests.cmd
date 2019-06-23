@if "%CONFIGURATION%"=="" set CONFIGURATION=Debug
%CONFIGURATION%\testrunner.exe
%CONFIGURATION%\testanalyzer.exe
