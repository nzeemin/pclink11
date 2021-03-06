@echo off
SET RT11EXE=C:\bin\rt11\rt11.exe
DEL /S /Q *-11.log *-11.SAV *-11.LDA *-11.MAP *-11.STB > NUL
rem
echo ADD32-PATRON
CD ADD32-PATRON
%RT11EXE% LINK ADD32/SYMBOLTABLE/MAP > ADD32-11.log
RENAME ADD32.SAV ADD32-11.SAV
RENAME ADD32.MAP ADD32-11.MAP
RENAME ADD32.STB ADD32-11.STB
rem
echo ASHC4
CD ..\ASHC4
%RT11EXE% LINK ASHC4/SYMBOLTABLE/MAP > ASHC4-11.log
RENAME ASHC4.SAV ASHC4-11.SAV
RENAME ASHC4.MAP ASHC4-11.MAP
RENAME ASHC4.STB ASHC4-11.STB
rem
echo BATCH-V14
CD ..\BATCH-V14
%RT11EXE% LINK BATCH/SYMBOLTABLE/MAP > BATCH-11.log
RENAME BATCH.SAV BATCH-11.SAV
RENAME BATCH.MAP BATCH-11.MAP
RENAME BATCH.STB BATCH-11.STB
rem
echo BIRITM-FORTRANIV
CD ..\BIRITM-FORTRANIV
%RT11EXE% LINK BIRITM/SYMBOLTABLE/MAP,FORLIB > BIRITM-11.log
RENAME BIRITM.SAV BIRITM-11.SAV
RENAME BIRITM.MAP BIRITM-11.MAP
RENAME BIRITM.STB BIRITM-11.STB
rem
echo CPS-PATRON
CD ..\CPS-PATRON
%RT11EXE% LINK CPS/SYMBOLTABLE/MAP > CPS-11.log
RENAME CPS.SAV CPS-11.SAV
RENAME CPS.MAP CPS-11.MAP
RENAME CPS.STB CPS-11.STB
rem
echo CPS2-PATRON
CD ..\CPS2-PATRON
%RT11EXE% LINK CPS2/SYMBOLTABLE/MAP > CPS2-11.log
RENAME CPS2.SAV CPS2-11.SAV
RENAME CPS2.MAP CPS2-11.MAP
RENAME CPS2.STB CPS2-11.STB
rem
echo CLINE-UKNC
CD ..\CLINE-UKNC
%RT11EXE% LINK CLINE/SYMBOLTABLE/MAP,CENV,CWENV,CSPR,CKING > CLINE-11.log
RENAME CLINE.SAV CLINE-11.SAV
RENAME CLINE.MAP CLINE-11.MAP
RENAME CLINE.STB CLINE-11.STB
rem
echo CLINE-UKNC-2
CD ..\CLINE-UKNC-2
%RT11EXE% LINK CLINE/SYMBOLTABLE/MAP,CWENV,CENV,CSPR,CKING > CLINE-11.log
RENAME CLINE.SAV CLINE-11.SAV
RENAME CLINE.MAP CLINE-11.MAP
RENAME CLINE.STB CLINE-11.STB
rem
echo DATIME
CD ..\DATIME
%RT11EXE% LINK DATIME/SYMBOLTABLE/MAP > DATIME-11.log
RENAME DATIME.SAV DATIME-11.SAV
RENAME DATIME.MAP DATIME-11.MAP
RENAME DATIME.STB DATIME-11.STB
rem
echo DEMO-FORTRANIV
CD ..\DEMO-FORTRANIV
%RT11EXE% LINK DEMO/SYMBOLTABLE/MAP,FORLIB > DEMO-11.log
RENAME DEMO.SAV DEMO-11.SAV
RENAME DEMO.MAP DEMO-11.MAP
RENAME DEMO.STB DEMO-11.STB
rem
echo DEMO2-FORTRANIV
CD ..\DEMO2-FORTRANIV
%RT11EXE% LINK DEMO2/SYMBOLTABLE/MAP,FORLIB > DEMO2-11.log
RENAME DEMO2.SAV DEMO2-11.SAV
RENAME DEMO2.MAP DEMO2-11.MAP
RENAME DEMO2.STB DEMO2-11.STB
rem
echo DISSAV-DBIT
CD ..\DISSAV-DBIT
%RT11EXE% LINK DISSAV/SYMBOLTABLE/MAP > DISSAV-11.log
RENAME DISSAV.SAV DISSAV-11.SAV
RENAME DISSAV.MAP DISSAV-11.MAP
RENAME DISSAV.STB DISSAV-11.STB
rem
echo DW-ANDREY
CD ..\DW-ANDREY
%RT11EXE% LINK /NOBIT/EXECUTE:DW.SYS DW/SYMBOLTABLE/MAP > DW-11.log
RENAME DW.SYS DW-11.SAV
RENAME DW.MAP DW-11.MAP
RENAME DW.STB DW-11.STB
rem
echo DW-FD6W
CD ..\DW-FD6W
%RT11EXE% LINK /NOBIT/EXECUTE:DW.SYS DW/SYMBOLTABLE/MAP > DW-11.log
RENAME DW.SYS DW-11.SAV
RENAME DW.MAP DW-11.MAP
RENAME DW.STB DW-11.STB
rem
CD ..\DWKQOBJ-HOBOT
%RT11EXE% LINK DWKQCO/SYMBOLTABLE/MAP,PASDWK > DWKQCO-11.log
RENAME DWKQCO.SAV DWKQCO-11.SAV
RENAME DWKQCO.MAP DWKQCO-11.MAP
RENAME DWKQCO.STB DWKQCO-11.STB
rem
echo DWOPT
CD ..\DWOPT
%RT11EXE% LINK DWOPT/SYMBOLTABLE/MAP > DWOPT-11.log
RENAME DWOPT.SAV DWOPT-11.SAV
RENAME DWOPT.MAP DWOPT-11.MAP
RENAME DWOPT.STB DWOPT-11.STB
rem
echo DX-0517
CD ..\DX-0517
%RT11EXE% LINK /NOBIT/EXECUTE:DX.SYS DX/SYMBOLTABLE/MAP > DX-11.log
RENAME DX.SYS DX-11.SAV
RENAME DX.MAP DX-11.MAP
RENAME DX.STB DX-11.STB
rem
echo DYABIN-novay_kopiya_161
CD ..\DYABIN-novay_kopiya_161
%RT11EXE% @L > F-11.log
RENAME F.SAV F-11.SAV
RENAME F.MAP F-11.MAP
RENAME F.STB F-11.STB
rem
echo EDINST-V2-17-NYS
CD ..\EDINST-V2-17-NYS
%RT11EXE% LINK EDINST/SYMBOLTABLE/MAP,VER > EDINST-11.log
RENAME EDINST.SAV EDINST-11.SAV
RENAME EDINST.MAP EDINST-11.MAP
RENAME EDINST.STB EDINST-11.STB
rem
echo ELCOPY-FD6W
CD ..\ELCOPY-FD6W
%RT11EXE% LINK ELCOPY/SYMBOLTABLE/MAP,ELTASK > ELCOPY-11.log
RENAME ELCOPY.SAV ELCOPY-11.SAV
RENAME ELCOPY.MAP ELCOPY-11.MAP
RENAME ELCOPY.STB ELCOPY-11.STB
rem
echo ELINIT-FD6W
CD ..\ELINIT-FD6W
%RT11EXE% LINK ELINIT/SYMBOLTABLE/MAP > ELINIT-11.log
RENAME ELINIT.SAV ELINIT-11.SAV
RENAME ELINIT.MAP ELINIT-11.MAP
RENAME ELINIT.STB ELINIT-11.STB
rem
echo EMPIRE-WBRIGHT
CD ..\EMPIRE-WBRIGHT
%RT11EXE% @EMPIRE.COM > EMPIRE-11.log
RENAME EMPIRE.SAV EMPIRE-11.SAV
RENAME EMPIRE.MAP EMPIRE-11.MAP
RENAME EMPIRE.STB EMPIRE-11.STB
rem
echo EXPRES-24
CD ..\EXPRES-24
%RT11EXE% LINK EXPRES/SYMBOLTABLE/MAP > EXPRES-11.log
RENAME EXPRES.SAV EXPRES-11.SAV
RENAME EXPRES.MAP EXPRES-11.MAP
RENAME EXPRES.STB EXPRES-11.STB
%RT11EXE% LINK EXPRES/LDA >> EXPRES-11.log
RENAME EXPRES.LDA EXPRES-11.LDA
rem
echo EXPRES-BK-12
CD ..\EXPRES-BK-12
%RT11EXE% LINK EXPRES/SYMBOLTABLE/MAP > EXPRES-11.log
RENAME EXPRES.SAV EXPRES-11.SAV
RENAME EXPRES.MAP EXPRES-11.MAP
RENAME EXPRES.STB EXPRES-11.STB
rem
echo EMUL-DISK25
CD ..\EMUL-DISK25
%RT11EXE% LINK EMUL/SYMBOLTABLE/MAP > EMUL-11.log
RENAME EMUL.SAV EMUL-11.SAV
RENAME EMUL.MAP EMUL-11.MAP
RENAME EMUL.STB EMUL-11.STB
rem
echo F4V28-LTHR
CD ..\F4V28-LTHR
%RT11EXE% @F4LTHR.COM > FORTRA-11.log
RENAME FORTRA.SAV FORTRA-11.SAV
RENAME FORTRA.MAP FORTRA-11.MAP
RENAME FORTRA.STB FORTRA-11.STB
rem
echo FCU
CD ..\FCU
%RT11EXE% LINK FCU/SYMBOLTABLE/MAP,FCUE,FCUMNG,FCUD,SELF > FCU-11.log
RENAME FCU.SAV FCU-11.SAV
RENAME FCU.MAP FCU-11.MAP
RENAME FCU.STB FCU-11.STB
rem
echo FDTVER-FORTRANIV
CD ..\FDTVER-FORTRANIV
%RT11EXE% LINK FDTVER/SYMBOLTABLE/MAP,FORLIB > FDTVER-11.log
RENAME FDTVER.SAV FDTVER-11.SAV
RENAME FDTVER.MAP FDTVER-11.MAP
RENAME FDTVER.STB FDTVER-11.STB
rem
echo FIG-FORTH-1-3
CD ..\FIG-FORTH-1-3
%RT11EXE% LINK FORTH/SYMBOLTABLE/MAP > FORTH-11.log
RENAME FORTH.SAV FORTH-11.SAV
RENAME FORTH.MAP FORTH-11.MAP
RENAME FORTH.STB FORTH-11.STB
rem
echo GAD2-HOBOT
CD ..\GAD2-HOBOT
%RT11EXE% LINK GAD2/SYMBOLTABLE/MAP > GAD2-11.log
RENAME GAD2.SAV GAD2-11.SAV
RENAME GAD2.MAP GAD2-11.MAP
RENAME GAD2.STB GAD2-11.STB
rem
echo GRAFOR-DEMO-FORTRANIV
CD ..\GRAFOR-DEMO-FORTRANIV
%RT11EXE% LINK DEMO/SYMBOLTABLE/MAP,ENDPG,PAGE,GRAFOR,FORLIB > DEMO-11.log
RENAME DEMO.SAV DEMO-11.SAV
RENAME DEMO.MAP DEMO-11.MAP
RENAME DEMO.STB DEMO-11.STB
rem
echo GRAFOR-MOVE-FORTRANIV
CD ..\GRAFOR-MOVE-FORTRANIV
%RT11EXE% LINK MOVE/SYMBOLTABLE/MAP,GRAFOR,FORLIB > MOVE-11.log
RENAME MOVE.SAV MOVE-11.SAV
RENAME MOVE.MAP MOVE-11.MAP
RENAME MOVE.STB MOVE-11.STB
rem
echo HAND-11
CD ..\HAND-11
%RT11EXE% LINK HAND/SYMBOLTABLE/MAP > HAND-11.log
RENAME HAND.SAV HAND-11.SAV
RENAME HAND.MAP HAND-11.MAP
RENAME HAND.STB HAND-11.STB
rem
echo HANDLE-503
CD ..\HANDLE-503
%RT11EXE% LINK HANDLE/SYMBOLTABLE/MAP > HANDLE-11.log
RENAME HANDLE.SAV HANDLE-11.SAV
RENAME HANDLE.MAP HANDLE-11.MAP
RENAME HANDLE.STB HANDLE-11.STB
rem
echo HD-DVKEmulator
CD ..\HD-DVKEmulator
%RT11EXE% LINK /NOBIT/EXECUTE:HD.SYS HD/SYMBOLTABLE/MAP > HD-11.log
RENAME HD.SYS HD-11.SAV
RENAME HD.MAP HD-11.MAP
RENAME HD.STB HD-11.STB
rem
echo HELLO
CD ..\HELLO
%RT11EXE% LINK HELLO/SYMBOLTABLE/MAP > HELLO-11.log
RENAME HELLO.SAV HELLO-11.SAV
RENAME HELLO.MAP HELLO-11.MAP
RENAME HELLO.STB HELLO-11.STB
rem
echo HWYENC-62
CD ..\HWYENC-62
%RT11EXE% LINK HWYENC/SYMBOLTABLE/MAP > HWYENC-11.log
RENAME HWYENC.SAV HWYENC-11.SAV
RENAME HWYENC.MAP HWYENC-11.MAP
RENAME HWYENC.STB HWYENC-11.STB
rem
echo INTERFACE-DISK02
CD ..\INTERFACE-DISK02
%RT11EXE% LINK IF/SYMBOLTABLE/MAP,IFMAIN,IFCOPY,IFTRP,IFSPEC,IFPP > IF-11.log
RENAME IF.SAV IF-11.SAV
RENAME IF.MAP IF-11.MAP
RENAME IF.STB IF-11.STB
rem
echo LD-FD6W
CD ..\LD-FD6W
%RT11EXE% LINK /NOBIT/EXECUTE:LD.SYS LD/SYMBOLTABLE/MAP > LD-11.log
RENAME LD.SYS LD-11.SAV
RENAME LD.MAP LD-11.MAP
RENAME LD.STB LD-11.STB
rem
echo LP-DISK09
CD ..\LP-DISK09
%RT11EXE% LINK /NOBIT/EXECUTE:LP.SYS LP/SYMBOLTABLE/MAP > LP-11.log
RENAME LP.SYS LP-11.SAV
RENAME LP.MAP LP-11.MAP
RENAME LP.STB LP-11.STB
rem
echo LZSAV
CD ..\LZSAV
%RT11EXE% LINK LZSAV/SYMBOLTABLE/MAP > LZSAV-11.log
RENAME LZSAV.SAV LZSAV-11.SAV
RENAME LZSAV.MAP LZSAV-11.MAP
RENAME LZSAV.STB LZSAV-11.STB
rem
echo MAIN-50
CD ..\MAIN-50
%RT11EXE% LINK MAIN/SYMBOLTABLE/MAP,LEVELS > MAIN-11.log
RENAME MAIN.SAV MAIN-11.SAV
RENAME MAIN.MAP MAIN-11.MAP
RENAME MAIN.STB MAIN-11.STB
rem
echo MUL
CD ..\MUL
%RT11EXE% LINK MUL/SYMBOLTABLE/MAP > MUL-11.log
RENAME MUL.SAV MUL-11.SAV
RENAME MUL.MAP MUL-11.MAP
RENAME MUL.STB MUL-11.STB
rem
echo MY304
CD ..\MY304
%RT11EXE% LINK /NOBIT/EXECUTE:MY.SYS MY/SYMBOLTABLE/MAP > MY-11.log
RENAME MY.SAV MY-11.SAV
RENAME MY.MAP MY-11.MAP
RENAME MY.STB MY-11.STB
rem
echo MYTEST1
CD ..\MYTEST1
%RT11EXE% LINK MYTST1/SYMBOLTABLE/MAP,OTHER > MYTST1-11.log
RENAME MYTST1.SAV MYTST1-11.SAV
RENAME MYTST1.MAP MYTST1-11.MAP
RENAME MYTST1.STB MYTST1-11.STB
rem
echo MYTEST2
CD ..\MYTEST2
%RT11EXE% LINK MYTST2/SYMBOLTABLE/MAP,OTHER > MYTST2-11.log
RENAME MYTST2.SAV MYTST2-11.SAV
RENAME MYTST2.MAP MYTST2-11.MAP
RENAME MYTST2.STB MYTST2-11.STB
rem
echo MYTEST3
CD ..\MYTEST3
%RT11EXE% LINK MYTST3/SYMBOLTABLE/MAP,OTHER > MYTST3-11.log
RENAME MYTST3.SAV MYTST3-11.SAV
RENAME MYTST3.MAP MYTST3-11.MAP
RENAME MYTST3.STB MYTST3-11.STB
rem
echo MYTEST3-ALPHAMAP
CD ..\MYTEST3-ALPHAMAP
%RT11EXE% LINK MYTST3/SYMBOLTABLE/MAP/ALPHABETIZE,OTHER > MYTST3-11.log
RENAME MYTST3.SAV MYTST3-11.SAV
RENAME MYTST3.MAP MYTST3-11.MAP
RENAME MYTST3.STB MYTST3-11.STB
rem
echo MYTEST3-NOMAP
CD ..\MYTEST3-NOMAP
%RT11EXE% LINK MYTST3/SYMBOLTABLE,OTHER > MYTST3-11.log
RENAME MYTST3.SAV MYTST3-11.SAV
RENAME MYTST3.STB MYTST3-11.STB
rem
echo MYTEST3-NOSTB
CD ..\MYTEST3-NOSTB
%RT11EXE% LINK MYTST3/MAP,OTHER > MYTST3-11.log
RENAME MYTST3.SAV MYTST3-11.SAV
RENAME MYTST3.MAP MYTST3-11.MAP
rem
echo MYTEST3-WIDEMAP
CD ..\MYTEST3-WIDEMAP
%RT11EXE% LINK MYTST3/SYMBOLTABLE/MAP/WIDE,OTHER > MYTST3-11.log
RENAME MYTST3.SAV MYTST3-11.SAV
RENAME MYTST3.MAP MYTST3-11.MAP
RENAME MYTST3.STB MYTST3-11.STB
rem
echo NET-DISK09
CD ..\NET-DISK09
%RT11EXE% LINK NET/SYMBOLTABLE/MAP > NET-11.log
RENAME NET.SAV NET-11.SAV
RENAME NET.MAP NET-11.MAP
RENAME NET.STB NET-11.STB
rem
echo NETTST-DISK09
CD ..\NETTST-DISK09
%RT11EXE% LINK NETTST/SYMBOLTABLE/MAP > NETTST-11.log
RENAME NETTST.SAV NETTST-11.SAV
RENAME NETTST.MAP NETTST-11.MAP
RENAME NETTST.STB NETTST-11.STB
rem
echo NL-FD6W
CD ..\NL-FD6W
%RT11EXE% LINK /NOBIT/EXECUTE:NL.SYS NL/SYMBOLTABLE/MAP > NL-11.log
RENAME NL.SYS NL-11.SAV
RENAME NL.MAP NL-11.MAP
RENAME NL.STB NL-11.STB
rem
echo PAKDMP-PHOOKY
CD ..\PAKDMP-PHOOKY
%RT11EXE% LINK PAKDMP/SYMBOLTABLE/MAP > PAKDMP-11.log
RENAME PAKDMP.SAV PAKDMP-11.SAV
RENAME PAKDMP.MAP PAKDMP-11.MAP
RENAME PAKDMP.STB PAKDMP-11.STB
rem
echo PAKWRT-PHOOKY
CD ..\PAKWRT-PHOOKY
%RT11EXE% LINK PAKWRT/SYMBOLTABLE/MAP > PAKWRT-11.log
RENAME PAKWRT.SAV PAKWRT-11.SAV
RENAME PAKWRT.MAP PAKWRT-11.MAP
RENAME PAKWRT.STB PAKWRT-11.STB
rem
echo PC20-PAF40
CD ..\PC20-PAF40
%RT11EXE% LINK DIR40,PAF40/EXE:PAF40/MAP/SYMBOLTABLE > PAF40-11.log
RENAME PAF40.SAV PAF40-11.SAV
RENAME PAF40.MAP PAF40-11.MAP
RENAME PAF40.STB PAF40-11.STB
rem
echo PC20-PAF80
CD ..\PC20-PAF80
%RT11EXE% LINK DIR80,PAF80/EXE:PAF80/MAP/SYMBOLTABLE > PAF80-11.log
RENAME PAF80.SAV PAF80-11.SAV
RENAME PAF80.MAP PAF80-11.MAP
RENAME PAF80.STB PAF80-11.STB
rem
echo PCBUG-1801BM1
CD ..\PCBUG-1801BM1
%RT11EXE% LINK PCBUG/SYMBOLTABLE/MAP > PCBUG-11.log
RENAME PCBUG.SAV PCBUG-11.SAV
RENAME PCBUG.MAP PCBUG-11.MAP
RENAME PCBUG.STB PCBUG-11.STB
rem
echo PCM-DISK09
CD ..\PCM-DISK09
%RT11EXE% LINK PCM/SYMBOLTABLE/MAP > PCM-11.log
RENAME PCM.SAV PCM-11.SAV
RENAME PCM.MAP PCM-11.MAP
RENAME PCM.STB PCM-11.STB
rem
echo PDPCLK
CD ..\PDPCLK
%RT11EXE% LINK PDPCLK/SYMBOLTABLE/MAP > PDPCLK-11.log
RENAME PDPCLK.SAV PDPCLK-11.SAV
RENAME PDPCLK.MAP PDPCLK-11.MAP
RENAME PDPCLK.STB PDPCLK-11.STB
rem
echo PEEKPOKE-FORTRANIV
CD ..\PEEKPOKE-FORTRANIV
%RT11EXE% LINK PEKPOK/SYMBOLTABLE/MAP,FORLIB > PEKPOK-11.log
RENAME PEKPOK.SAV PEKPOK-11.SAV
RENAME PEKPOK.MAP PEKPOK-11.MAP
RENAME PEKPOK.STB PEKPOK-11.STB
rem
echo PIFPAF
CD ..\PIFPAF
C:\bin\rt11\rt11.exe LINK PIFPAF/SYMBOLTABLE/MAP > PIFPAF-11.log
RENAME PIFPAF.SAV PIFPAF-11.SAV
RENAME PIFPAF.MAP PIFPAF-11.MAP
RENAME PIFPAF.STB PIFPAF-11.STB
rem
echo PITEST-1801BM1
CD ..\PITEST-1801BM1
%RT11EXE% LINK PITEST/SYMBOLTABLE/MAP > PITEST-11.log
RENAME PITEST.SAV PITEST-11.SAV
RENAME PITEST.MAP PITEST-11.MAP
RENAME PITEST.STB PITEST-11.STB
rem
echo PLOT-FORTRANIV
CD ..\PLOT-FORTRANIV
%RT11EXE% LINK PLOT/SYMBOLTABLE/MAP,PIX,FORLIB > PLOT-11.log
RENAME PLOT.SAV PLOT-11.SAV
RENAME PLOT.MAP PLOT-11.MAP
RENAME PLOT.STB PLOT-11.STB
rem
echo PPTEST-REL
CD ..\PPTEST-REL
%RT11EXE% LINK/FO PPTEST/SYMBOLTABLE/MAP > PPTEST-11.log
RENAME PPTEST.REL PPTEST-11.SAV
RENAME PPTEST.MAP PPTEST-11.MAP
RENAME PPTEST.STB PPTEST-11.STB
rem
echo PRIM-FORTRANIV
CD ..\PRIM-FORTRANIV
%RT11EXE% LINK PRIM/SYMBOLTABLE/MAP,FORLIB > PRIM-11.log
RENAME PRIM.SAV PRIM-11.SAV
RENAME PRIM.MAP PRIM-11.MAP
RENAME PRIM.STB PRIM-11.STB
rem
echo RDMTUN-DSKMXC
CD ..\RDMTUN-DSKMXC
%RT11EXE% LINK RDMTUN/SYMBOLTABLE/MAP > RDMTUN-11.log
RENAME RDMTUN.SAV RDMTUN-11.SAV
RENAME RDMTUN.MAP RDMTUN-11.MAP
RENAME RDMTUN.STB RDMTUN-11.STB
rem
echo REGCOP-ODE11
CD ..\REGCOP-ODE11
%RT11EXE% LINK REGCOP/SYMBOLTABLE/MAP > REGCOP-11.log
RENAME REGCOP.SAV REGCOP-11.SAV
RENAME REGCOP.MAP REGCOP-11.MAP
RENAME REGCOP.STB REGCOP-11.STB
rem
echo REGPER-ODE11
CD ..\REGPER-ODE11
%RT11EXE% LINK REGPER/SYMBOLTABLE/MAP > REGPER-11.log
RENAME REGPER.SAV REGPER-11.SAV
RENAME REGPER.MAP REGPER-11.MAP
RENAME REGPER.STB REGPER-11.STB
rem
echo RTS
CD ..\RTS
%RT11EXE% LINK RTS/SYMBOLTABLE/MAP > RTS-11.log
RENAME RTS.SAV RTS-11.SAV
RENAME RTS.MAP RTS-11.MAP
RENAME RTS.STB RTS-11.STB
rem
echo RTLEM
CD ..\RTLEM
%RT11EXE% LINK RTLEM/SYMBOLTABLE/MAP > RTLEM-11.log
RENAME RTLEM.SAV RTLEM-11.SAV
RENAME RTLEM.MAP RTLEM-11.MAP
RENAME RTLEM.STB RTLEM-11.STB
rem
echo SATMON-HADI3DSK
CD ..\SATMON-HADI3DSK
%RT11EXE% LINK SATMON/SYMBOLTABLE/MAP > SATMON-11.log
RENAME SATMON.SAV SATMON-11.SAV
RENAME SATMON.MAP SATMON-11.MAP
RENAME SATMON.STB SATMON-11.STB
rem
echo SFUN-SCHORS
CD ..\SFUN-SCHORS
%RT11EXE% @SFUN.COM > SFUN-11.log
RENAME SFUN.SAV SFUN-11.SAV
RENAME SFUN.MAP SFUN-11.MAP
RENAME SFUN.STB SFUN-11.STB
rem
echo SNDOFF-ANDREY14
CD ..\SNDOFF-ANDREY14
%RT11EXE% LINK SNDOFF/SYMBOLTABLE/MAP > SNDOFF-11.log
RENAME SNDOFF.SAV SNDOFF-11.SAV
RENAME SNDOFF.MAP SNDOFF-11.MAP
RENAME SNDOFF.STB SNDOFF-11.STB
rem
echo SOKOBA
CD ..\SOKOBA
%RT11EXE% LINK SOKOBA/SYMBOLTABLE/MAP,SOKDAT,ALP,RALP,SOKMAZ > SOKOBA-11.log
RENAME SOKOBA.SAV SOKOBA-11.SAV
RENAME SOKOBA.MAP SOKOBA-11.MAP
RENAME SOKOBA.STB SOKOBA-11.STB
rem
echo SOKOED
CD ..\SOKOED
%RT11EXE% LINK SOKOED/SYMBOLTABLE/MAP,SOKED,SOKDAT > SOKOED-11.log
RENAME SOKOED.SAV SOKOED-11.SAV
RENAME SOKOED.MAP SOKOED-11.MAP
RENAME SOKOED.STB SOKOED-11.STB
rem
echo SPCINV
CD ..\SPCINV
%RT11EXE% LINK SPCINV/SYMBOLTABLE/MAP > SPCINV-11.log
RENAME SPCINV.SAV SPCINV-11.SAV
RENAME SPCINV.MAP SPCINV-11.MAP
RENAME SPCINV.STB SPCINV-11.STB
rem
echo SPLIT-DSKMXC
CD ..\SPLIT-DSKMXC
%RT11EXE% LINK SPLIT/SYMBOLTABLE/MAP > SPLIT-11.log
RENAME SPLIT.SAV SPLIT-11.SAV
RENAME SPLIT.MAP SPLIT-11.MAP
RENAME SPLIT.STB SPLIT-11.STB
rem
echo T401-1801BM1
CD ..\T401-1801BM1
%RT11EXE% LINK T401/SYMBOLTABLE/MAP > T401-11.log
RENAME T401.SAV T401-11.SAV
RENAME T401.MAP T401-11.MAP
RENAME T401.STB T401-11.STB
rem
echo T402-1801BM1
CD ..\T402-1801BM1
%RT11EXE% LINK T402/SYMBOLTABLE/MAP > T402-11.log
RENAME T402.SAV T402-11.SAV
RENAME T402.MAP T402-11.MAP
RENAME T402.STB T402-11.STB
rem
echo T404-1801BM1
CD ..\T404-1801BM1
%RT11EXE% LINK T404/SYMBOLTABLE/MAP > T404-11.log
RENAME T404.SAV T404-11.SAV
RENAME T404.MAP T404-11.MAP
RENAME T404.STB T404-11.STB
rem
echo TEST-PASCAL
CD ..\TEST-PASCAL
%RT11EXE% LINK TEST/SYMBOLTABLE/MAP,PASCAL > TEST-11.log
RENAME TEST.SAV TEST-11.SAV
RENAME TEST.MAP TEST-11.MAP
RENAME TEST.STB TEST-11.STB
rem
echo TPF-FORTRANIV
CD ..\TPF-FORTRANIV
%RT11EXE% LINK TPF/SYMBOLTABLE/MAP,GRAFOR,FORLIB > TPF-11.log
RENAME TPF.SAV TPF-11.SAV
RENAME TPF.MAP TPF-11.MAP
RENAME TPF.STB TPF-11.STB
rem
echo TPK-FORTRANIV
CD ..\TPK-FORTRANIV
%RT11EXE% LINK TPK/SYMBOLTABLE/MAP,FORLIB > TPK-11.log
RENAME TPK.SAV TPK-11.SAV
RENAME TPK.MAP TPK-11.MAP
RENAME TPK.STB TPK-11.STB
rem
echo TSPAL-01B
CD ..\TSPAL-01B
%RT11EXE% LINK TSPAL/SYMBOLTABLE/MAP > TSPAL-11.log
RENAME TSPAL.SAV TSPAL-11.SAV
RENAME TSPAL.MAP TSPAL-11.MAP
RENAME TSPAL.STB TSPAL-11.STB
rem
echo TSTVM1-01A
CD ..\TSTVM1-01A
%RT11EXE% LINK TSTVM1/SYMBOLTABLE/MAP > TSTVM1-11.log
RENAME TSTVM1.SAV TSTVM1-11.SAV
RENAME TSTVM1.MAP TSTVM1-11.MAP
RENAME TSTVM1.STB TSTVM1-11.STB
rem
echo TSTVM2-03A
CD ..\TSTVM2-03A
%RT11EXE% LINK TSTVM2/SYMBOLTABLE/MAP > TSTVM2-11.log
RENAME TSTVM2.SAV TSTVM2-11.SAV
RENAME TSTVM2.MAP TSTVM2-11.MAP
RENAME TSTVM2.STB TSTVM2-11.STB
rem
echo TT-FD6W
CD ..\TT-FD6W
%RT11EXE% LINK /NOBIT/EXECUTE:TT.SYS TT/SYMBOLTABLE/MAP > TT-11.log
RENAME TT.SYS TT-11.SAV
RENAME TT.MAP TT-11.MAP
RENAME TT.STB TT-11.STB
rem
echo TVE1-1801BM1
CD ..\TVE1-1801BM1
%RT11EXE% LINK TVE1/SYMBOLTABLE/MAP > TVE1-11.log
RENAME TVE1.SAV TVE1-11.SAV
RENAME TVE1.MAP TVE1-11.MAP
RENAME TVE1.STB TVE1-11.STB
rem
echo UCL-HOBOT
CD ..\UCL-HOBOT
%RT11EXE% LINK UCL/SYMBOLTABLE/MAP > UCL-11.log
RENAME UCL.SAV UCL-11.SAV
RENAME UCL.MAP UCL-11.MAP
RENAME UCL.STB UCL-11.STB
rem
echo UKFONT
CD ..\UKFONT
%RT11EXE% LINK UKFONT/SYMBOLTABLE/MAP > UKFONT-11.log
RENAME UKFONT.SAV UKFONT-11.SAV
RENAME UKFONT.MAP UKFONT-11.MAP
RENAME UKFONT.STB UKFONT-11.STB
rem
%RT11EXE% LINK UKROM/SYMBOLTABLE/MAP > UKROM-11.log
RENAME UKROM.SAV UKROM-11.SAV
RENAME UKROM.MAP UKROM-11.MAP
RENAME UKROM.STB UKROM-11.STB
rem
echo VM-FD6W
CD ..\VM-FD6W
%RT11EXE% LINK /NOBIT/EXECUTE:VM.SYS VM/SYMBOLTABLE/MAP > VM-11.log
RENAME VM.SYS VM-11.SAV
RENAME VM.MAP VM-11.MAP
RENAME VM.STB VM-11.STB
CD ..
