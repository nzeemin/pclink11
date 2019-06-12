cd tests
call !runtestmy.cmd
cd ..
if "%CONFIGURATION%"=="" set CONFIGURATION=Debug
%CONFIGURATION%\testanalyzer.exe
