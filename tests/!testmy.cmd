@echo off
SET PCLINK11=..\..\Debug\pclink11.exe
DEL /S /Q *-my.log *-my.SAV *-my.MAP *-my.STB > NUL
rem
echo ASHC4
CD ASHC4
%PCLINK11% ASHC4.OBJ > ASHC4-my.log
RENAME ASHC4.SAV ASHC4-my.SAV
RENAME ASHC4.MAP ASHC4-my.MAP
RENAME ASHC4.STB ASHC4-my.STB
rem
echo DISSAV-DBIT
CD ..\DISSAV-DBIT
%PCLINK11% DISSAV.OBJ > DISSAV-my.log
RENAME DISSAV.SAV DISSAV-my.SAV
RENAME DISSAV.MAP DISSAV-my.MAP
RENAME DISSAV.STB DISSAV-my.STB
rem
echo EXPRES-24
CD ..\EXPRES-24
%PCLINK11% EXPRES.OBJ > EXPRES-my.log
RENAME EXPRES.SAV EXPRES-my.SAV
RENAME EXPRES.MAP EXPRES-my.MAP
RENAME EXPRES.STB EXPRES-my.STB
rem
echo EXPRES-BK-12
CD ..\EXPRES-BK-12
%PCLINK11% EXPRES.OBJ > EXPRES-my.log
RENAME EXPRES.SAV EXPRES-my.SAV
RENAME EXPRES.MAP EXPRES-my.MAP
RENAME EXPRES.STB EXPRES-my.STB
rem
echo FCU
CD ..\FCU
%PCLINK11% FCU.OBJ FCUE.OBJ FCUMNG.OBJ FCUD.OBJ SELF.OBJ > FCU-my.log
RENAME FCU.SAV FCU-my.SAV
RENAME FCU.MAP FCU-my.MAP
RENAME FCU.STB FCU-my.STB
rem
echo FIG-FORTH-1-3
CD ..\FIG-FORTH-1-3
%PCLINK11% FORTH.OBJ > FORTH-my.log
RENAME FORTH.SAV FORTH-my.SAV
RENAME FORTH.MAP FORTH-my.MAP
RENAME FORTH.STB FORTH-my.STB
rem
echo GAD2-HOBOT
CD ..\GAD2-HOBOT
%PCLINK11% GAD2.OBJ > GAD2-my.log
RENAME GAD2.SAV GAD2-my.SAV
RENAME GAD2.MAP GAD2-my.MAP
RENAME GAD2.STB GAD2-my.STB
rem
echo HAND-11
CD ..\HAND-11
%PCLINK11% HAND.OBJ > HAND-my.log
RENAME HAND.SAV HAND-my.SAV
RENAME HAND.MAP HAND-my.MAP
RENAME HAND.STB HAND-my.STB
rem
echo HANDLE-503
CD ..\HANDLE-503
%PCLINK11% HANDLE.OBJ > HANDLE-my.log
RENAME HANDLE.SAV HANDLE-my.SAV
RENAME HANDLE.MAP HANDLE-my.MAP
RENAME HANDLE.STB HANDLE-my.STB
rem
echo HELLO
CD ..\HELLO
%PCLINK11% HELLO.OBJ > HELLO-my.log
RENAME HELLO.SAV HELLO-my.SAV
RENAME HELLO.MAP HELLO-my.MAP
RENAME HELLO.STB HELLO-my.STB
rem
echo HWYENC-62
CD ..\HWYENC-62
%PCLINK11% HWYENC.OBJ > HWYENC-my.log
RENAME HWYENC.SAV HWYENC-my.SAV
RENAME HWYENC.MAP HWYENC-my.MAP
RENAME HWYENC.STB HWYENC-my.STB
rem
echo LZSAV
CD ..\LZSAV
%PCLINK11% LZSAV.OBJ > LZSAV-my.log
RENAME LZSAV.SAV LZSAV-my.SAV
RENAME LZSAV.MAP LZSAV-my.MAP
RENAME LZSAV.STB LZSAV-my.STB
rem
echo MAIN-50
CD ..\MAIN-50
%PCLINK11% MAIN.OBJ LEVELS.OBJ > MAIN-my.log
RENAME MAIN.SAV MAIN-my.SAV
RENAME MAIN.MAP MAIN-my.MAP
RENAME MAIN.STB MAIN-my.STB
rem
echo MUL
CD ..\MUL
%PCLINK11% MUL.OBJ > MUL-my.log
RENAME MUL.SAV MUL-my.SAV
RENAME MUL.MAP MUL-my.MAP
RENAME MUL.STB MUL-my.STB
rem
echo PDPCLK
CD ..\PDPCLK
%PCLINK11% PDPCLK.OBJ > PDPCLK-my.log
RENAME PDPCLK.SAV PDPCLK-my.SAV
RENAME PDPCLK.MAP PDPCLK-my.MAP
RENAME PDPCLK.STB PDPCLK-my.STB
rem
echo RTLEM
CD ..\RTLEM
%PCLINK11% RTLEM.OBJ > RTLEM-my.log
RENAME RTLEM.SAV RTLEM-my.SAV
RENAME RTLEM.MAP RTLEM-my.MAP
RENAME RTLEM.STB RTLEM-my.STB
rem
echo SNDOFF-ANDREY14
CD ..\SNDOFF-ANDREY14
%PCLINK11% SNDOFF.OBJ > SNDOFF-my.log
RENAME SNDOFF.SAV SNDOFF-my.SAV
RENAME SNDOFF.MAP SNDOFF-my.MAP
RENAME SNDOFF.STB SNDOFF-my.STB
rem
echo SOKOBA
CD ..\SOKOBA
%PCLINK11% SOKOBA.OBJ SOKDAT.OBJ ALP.OBJ RALP.OBJ SOKMAZ.OBJ > SOKOBA-my.log
RENAME SOKOBA.SAV SOKOBA-my.SAV
RENAME SOKOBA.MAP SOKOBA-my.MAP
RENAME SOKOBA.STB SOKOBA-my.STB
rem
echo SOKOED
CD ..\SOKOED
%PCLINK11% SOKOED.OBJ SOKED.OBJ SOKDAT.OBJ > SOKOED-my.log
RENAME SOKOED.SAV SOKOED-my.SAV
RENAME SOKOED.MAP SOKOED-my.MAP
RENAME SOKOED.STB SOKOED-my.STB
rem
echo SPCINV
CD ..\SPCINV
%PCLINK11% SPCINV.OBJ > SPCINV-my.log
RENAME SPCINV.SAV SPCINV-my.SAV
RENAME SPCINV.MAP SPCINV-my.MAP
RENAME SPCINV.STB SPCINV-my.STB
rem
echo TSTPAL-01B
CD ..\TSPAL-01B
%PCLINK11% TSPAL.OBJ > TSPAL-my.log
RENAME TSPAL.SAV TSPAL-my.SAV
RENAME TSPAL.MAP TSPAL-my.MAP
RENAME TSPAL.STB TSPAL-my.STB
rem
echo TSTVM1-01A
CD ..\TSTVM1-01A
%PCLINK11% TSTVM1.OBJ > TSTVM1-my.log
RENAME TSTVM1.SAV TSTVM1-my.SAV
RENAME TSTVM1.MAP TSTVM1-my.MAP
RENAME TSTVM1.STB TSTVM1-my.STB
rem
echo TSTVM2-03A
CD ..\TSTVM2-03A
%PCLINK11% TSTVM2.OBJ > TSTVM2-my.log
RENAME TSTVM2.SAV TSTVM2-my.SAV
RENAME TSTVM2.MAP TSTVM2-my.MAP
RENAME TSTVM2.STB TSTVM2-my.STB
CD ..
