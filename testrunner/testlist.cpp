// testdest.cpp : Test list.
//

#include "testrunner.h"

const TestDescriptor g_Tests[] =
{
    /* directory           name        commandline */
    { "ADD32-PATRON",     "ADD32",    "/SYMBOLTABLE /MAP ADD32.OBJ" },
    { "ASHC4",            "ASHC4",    "/SYMBOLTABLE /MAP ASHC4.OBJ" },
    { "BATCH-V14",        "BATCH",    "/SYMBOLTABLE /MAP BATCH.OBJ" },
    { "BIRITM-FORTRANIV", "BIRITM",   "/SYMBOLTABLE /MAP BIRITM.OBJ FORLIB.OBJ SYSLIB.OBJ" },
    { "CPS-PATRON",       "CPS",      "/SYMBOLTABLE /MAP CPS.OBJ" },
    { "CLINE-UKNC",       "CLINE",    "/SYMBOLTABLE /MAP CLINE.OBJ CENV.OBJ CWENV.OBJ CSPR.OBJ CKING.OBJ" },
    { "CLINE-UKNC-2",     "CLINE",    "/SYMBOLTABLE /MAP CLINE.OBJ CWENV.OBJ CENV.OBJ CSPR.OBJ CKING.OBJ" },
    { "DATIME",           "DATIME",   "/SYMBOLTABLE /MAP DATIME.OBJ" },
    { "DEMO-FORTRANIV",   "DEMO",     "/SYMBOLTABLE /MAP DEMO.OBJ FORLIB.OBJ" },
    { "DEMO2-FORTRANIV",  "DEMO2",    "/SYMBOLTABLE /MAP DEMO2.OBJ FORLIB.OBJ" },
    { "DISSAV-DBIT",      "DISSAV",   "/SYMBOLTABLE /MAP DISSAV.OBJ" },
    { "DW-ANDREY",        "DW",       "/SYMBOLTABLE /MAP /EXECUTE:DW.SYS DW.OBJ /X" },
    { "DW-FD6W",          "DW",       "/SYMBOLTABLE /MAP /EXECUTE:DW.SYS DW.OBJ /X" },
    { "DWOPT",            "DWOPT",    "/SYMBOLTABLE /MAP DWOPT.OBJ" },
    { "DX-0517",          "DX",       "/SYMBOLTABLE /MAP /EXECUTE:DX.SYS DX.OBJ /X" },
    { "EDINST-V2-17-NYS", "EDINST",   "/SYMBOLTABLE /MAP EDINST.OBJ VER.OBJ" },
    { "ELCOPY-FD6W",      "ELCOPY",   "/SYMBOLTABLE /MAP ELCOPY.OBJ ELTASK.OBJ" },
    { "ELINIT-FD6W",      "ELINIT",   "/SYMBOLTABLE /MAP ELINIT.OBJ" },
    { "EMPIRE-WBRIGHT",   "EMPIRE",   "/SYMBOLTABLE /MAP /EXECUTE:EMPIRE.SAV VAR.OBJ EMPIRE.OBJ MOVE2.OBJ SUB1.OBJ TTY.OBJ HMOVE.OBJ HMOVE2.OBJ PATH.OBJ CITY.OBJ SUB2.OBJ MAPS.OBJ CMOVE.OBJ CMOVE2.OBJ ARMYMV.OBJ FIGHMV.OBJ SHIPMV.OBJ SECTOR.OBJ MOVE.OBJ IOMOD.OBJ INIT.OBJ" },
    { "EXPRES-24",        "EXPRES",   "/SYMBOLTABLE /MAP EXPRES.OBJ" },
    { "EXPRES-BK-12",     "EXPRES",   "/SYMBOLTABLE /MAP EXPRES.OBJ" },
    { "EMUL-DISK25",      "EMUL",     "/SYMBOLTABLE /MAP EMUL.OBJ" },
    { "FCU",              "FCU",      "/SYMBOLTABLE /MAP FCU.OBJ FCUE.OBJ FCUMNG.OBJ FCUD.OBJ SELF.OBJ" },
    { "FDTVER-FORTRANIV", "FDTVER",   "/SYMBOLTABLE /MAP FDTVER.OBJ FORLIB.OBJ" },
    { "FIG-FORTH-1-3",    "FORTH",    "/SYMBOLTABLE /MAP FORTH.OBJ" },
    { "GAD2-HOBOT",       "GAD2",     "/SYMBOLTABLE /MAP GAD2.OBJ" },
    { "GRAFOR-DEMO-FORTRANIV", "DEMO", "/SYMBOLTABLE /MAP DEMO.OBJ ENDPG.OBJ PAGE.OBJ GRAFOR.OBJ FORLIB.OBJ" },
    { "GRAFOR-MOVE-FORTRANIV", "MOVE", "/SYMBOLTABLE /MAP MOVE.OBJ GRAFOR.OBJ FORLIB.OBJ" },
    { "HAND-11",          "HAND",     "/SYMBOLTABLE /MAP HAND.OBJ" },
    { "HANDLE-503",       "HANDLE",   "/SYMBOLTABLE /MAP HANDLE.OBJ" },
    { "HD-DVKEmulator",   "HD",       "/SYMBOLTABLE /MAP /EXECUTE:HD.SYS HD.OBJ /X" },
    { "HELLO",            "HELLO",    "/SYMBOLTABLE /MAP HELLO.OBJ" },
    { "HWYENC-62",        "HWYENC",   "/SYMBOLTABLE /MAP HWYENC.OBJ" },
    { "INTERFACE-DISK02", "IF",       "/SYMBOLTABLE /MAP IF.OBJ IFMAIN.OBJ IFCOPY.OBJ IFTRP.OBJ IFSPEC.OBJ IFPP.OBJ" },
    { "LD-FD6W",          "LD",       "/SYMBOLTABLE /MAP /EXECUTE:LD.SYS LD.OBJ SYSLIB.OBJ /X" },
    { "LZSAV",            "LZSAV",    "/SYMBOLTABLE /MAP LZSAV.OBJ" },
    { "LP-DISK09",        "LP",       "/SYMBOLTABLE /MAP /EXECUTE:LP.SYS LP.OBJ /X" },
    { "MAIN-50",          "MAIN",     "/SYMBOLTABLE /MAP MAIN.OBJ LEVELS.OBJ" },
    { "MUL",              "MUL",      "/SYMBOLTABLE /MAP MUL.OBJ" },
    { "MYTEST1",          "MYTST1",   "/SYMBOLTABLE /MAP MYTST1.OBJ OTHER.OBJ" },
    { "MYTEST2",          "MYTST2",   "/SYMBOLTABLE /MAP MYTST2.OBJ OTHER.OBJ" },
    { "MYTEST3",          "MYTST3",   "/SYMBOLTABLE /MAP MYTST3.OBJ OTHER.OBJ" },
    { "MYTEST3-ALPHAMAP", "MYTST3",   "/SYMBOLTABLE /MAP /ALPHABETIZE MYTST3.OBJ OTHER.OBJ" },
    { "MYTEST3-NOMAP",    "MYTST3",   "/SYMBOLTABLE MYTST3.OBJ OTHER.OBJ" },
    { "MYTEST3-NOSTB",    "MYTST3",   "/MAP MYTST3.OBJ OTHER.OBJ" },
    { "MYTEST3-WIDEMAP",  "MYTST3",   "/SYMBOLTABLE /MAP /WIDE MYTST3.OBJ OTHER.OBJ" },
    { "NET-DISK09",       "NET",      "/SYMBOLTABLE /MAP NET.OBJ" },
    { "NETTST-DISK09",    "NETTST",   "/SYMBOLTABLE /MAP NETTST.OBJ" },
    { "NL-FD6W",          "NL",       "/SYMBOLTABLE /MAP /EXECUTE:NL.SYS NL.OBJ /X" },
    { "PAKDMP-PHOOKY",    "PAKDMP",   "/SYMBOLTABLE /MAP PAKDMP.OBJ" },
    { "PAKWRT-PHOOKY",    "PAKWRT",   "/SYMBOLTABLE /MAP PAKWRT.OBJ" },
    { "PCBUG-1801BM1",    "PCBUG",    "/SYMBOLTABLE /MAP PCBUG.OBJ" },
    { "PCM-DISK09",       "PCM",      "/SYMBOLTABLE /MAP PCM.OBJ" },
    { "PDPCLK",           "PDPCLK",   "/SYMBOLTABLE /MAP PDPCLK.OBJ" },
    { "PEEKPOKE-FORTRANIV", "PEKPOK", "/SYMBOLTABLE /MAP PEKPOK.OBJ FORLIB.OBJ SYSLIB.OBJ" },
    { "PITEST-1801BM1",   "PITEST",   "/SYMBOLTABLE /MAP PITEST.OBJ" },
    { "PLOT-FORTRANIV",   "PLOT",     "/SYMBOLTABLE /MAP PLOT.OBJ PIX.OBJ FORLIB.OBJ" },
    { "PRIM-FORTRANIV",   "PRIM",     "/SYMBOLTABLE /MAP PRIM.OBJ FORLIB.OBJ" },
    { "RDMTUN-DSKMXC",    "RDMTUN",   "/SYMBOLTABLE /MAP RDMTUN.OBJ SYSLIB.OBJ" },
    { "REGCOP-ODE11",     "REGCOP",   "/SYMBOLTABLE /MAP REGCOP.OBJ" },
    { "REGPER-ODE11",     "REGPER",   "/SYMBOLTABLE /MAP REGPER.OBJ" },
    { "RTS",              "RTS",      "/SYMBOLTABLE /MAP RTS.OBJ" },
    { "RTLEM",            "RTLEM",    "/SYMBOLTABLE /MAP RTLEM.OBJ" },
    { "SATMON-HADI3DSK",  "SATMON",   "/SYMBOLTABLE /MAP SATMON.OBJ" },
    { "SNDOFF-ANDREY14",  "SNDOFF",   "/SYMBOLTABLE /MAP SNDOFF.OBJ" },
    { "SOKOBA",           "SOKOBA",   "/SYMBOLTABLE /MAP SOKOBA.OBJ SOKDAT.OBJ ALP.OBJ RALP.OBJ SOKMAZ.OBJ" },
    { "SOKOED",           "SOKOED",   "/SYMBOLTABLE /MAP SOKOED.OBJ SOKED.OBJ SOKDAT.OBJ" },
    { "SPCINV",           "SPCINV",   "/SYMBOLTABLE /MAP SPCINV.OBJ" },
    { "SPLIT-DSKMXC",     "SPLIT",    "/SYMBOLTABLE /MAP SPLIT.OBJ" },
    { "T401-1801BM1",     "T401",     "/SYMBOLTABLE /MAP T401.OBJ" },
    { "T402-1801BM1",     "T402",     "/SYMBOLTABLE /MAP T402.OBJ" },
    { "T404-1801BM1",     "T404",     "/SYMBOLTABLE /MAP T404.OBJ" },
    { "TEST-PASCAL",      "TEST",     "/SYMBOLTABLE /MAP TEST.OBJ PASCAL.OBJ" },
    { "TPF-FORTRANIV",    "TPF",      "/SYMBOLTABLE /MAP TPF.OBJ GRAFOR.OBJ FORLIB.OBJ" },
    { "TPK-FORTRANIV",    "TPK",      "/SYMBOLTABLE /MAP TPK.OBJ FORLIB.OBJ" },
    { "TSPAL-01B",        "TSPAL",    "/SYMBOLTABLE /MAP TSPAL.OBJ" },
    { "TSTVM1-01A",       "TSTVM1",   "/SYMBOLTABLE /MAP TSTVM1.OBJ" },
    { "TSTVM2-03A",       "TSTVM2",   "/SYMBOLTABLE /MAP TSTVM2.OBJ" },
    { "TT-FD6W",          "TT",       "/SYMBOLTABLE /MAP /EXECUTE:TT.SYS TT.OBJ /X" },
    { "TVE1-1801BM1",     "TVE1",     "/SYMBOLTABLE /MAP TVE1.OBJ" },
    { "UCL-HOBOT",        "UCL",      "/SYMBOLTABLE /MAP UCL.OBJ" },
    { "UKFONT",           "UKFONT",   "/SYMBOLTABLE /MAP UKFONT.OBJ" },
    { "VM-FD6W",          "VM",       "/SYMBOLTABLE /MAP /EXECUTE:VM.SYS VM.OBJ /X" },
};
const int g_TestNumber = sizeof(g_Tests) / sizeof(g_Tests[0]);
