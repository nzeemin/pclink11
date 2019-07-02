// testdest.cpp : Test list.
//

#include "testrunner.h"

const TestDescriptor g_Tests[] =
{
    /* directory           name        commandline */
    { "ADD32-PATRON",     "ADD32",    "/SYMBOLTABLE ADD32.OBJ" },
    { "ASHC4",            "ASHC4",    "/SYMBOLTABLE ASHC4.OBJ" },
    { "BATCH-V14",        "BATCH",    "/SYMBOLTABLE BATCH.OBJ" },
    { "CPS-PATRON",       "CPS",      "/SYMBOLTABLE CPS.OBJ" },
    { "CLINE-UKNC",       "CLINE",    "/SYMBOLTABLE CLINE.OBJ CENV.OBJ CWENV.OBJ CSPR.OBJ CKING.OBJ" },
    { "CLINE-UKNC-2",     "CLINE",    "/SYMBOLTABLE CLINE.OBJ CWENV.OBJ CENV.OBJ CSPR.OBJ CKING.OBJ" },
    { "DATIME",           "DATIME",   "/SYMBOLTABLE DATIME.OBJ" },
    { "DEMO-FORTRANIV",   "DEMO",     "/SYMBOLTABLE DEMO.OBJ FORLIB.OBJ" },
    { "DEMO2-FORTRANIV",  "DEMO2",    "/SYMBOLTABLE DEMO2.OBJ FORLIB.OBJ" },
    { "DISSAV-DBIT",      "DISSAV",   "/SYMBOLTABLE DISSAV.OBJ" },
    { "DW-ANDREY",        "DW",       "/SYMBOLTABLE /EXECUTE:DW.SYS DW.OBJ /X" },
    { "DW-FD6W",          "DW",       "/SYMBOLTABLE /EXECUTE:DW.SYS DW.OBJ /X" },
    { "DWOPT",            "DWOPT",    "/SYMBOLTABLE DWOPT.OBJ" },
    { "DX-0517",          "DX",       "/SYMBOLTABLE /EXECUTE:DX.SYS DX.OBJ /X" },
    { "EDINST-V2-17-NYS", "EDINST",   "/SYMBOLTABLE EDINST.OBJ VER.OBJ" },
    { "ELCOPY-FD6W",      "ELCOPY",   "/SYMBOLTABLE ELCOPY.OBJ ELTASK.OBJ" },
    { "ELINIT-FD6W",      "ELINIT",   "/SYMBOLTABLE ELINIT.OBJ" },
    { "EMPIRE-WBRIGHT",   "EMPIRE",   "/SYMBOLTABLE /EXECUTE:EMPIRE.SAV VAR.OBJ EMPIRE.OBJ MOVE2.OBJ SUB1.OBJ TTY.OBJ HMOVE.OBJ HMOVE2.OBJ PATH.OBJ CITY.OBJ SUB2.OBJ MAPS.OBJ CMOVE.OBJ CMOVE2.OBJ ARMYMV.OBJ FIGHMV.OBJ SHIPMV.OBJ SECTOR.OBJ MOVE.OBJ IOMOD.OBJ INIT.OBJ" },
    { "EXPRES-24",        "EXPRES",   "/SYMBOLTABLE EXPRES.OBJ" },
    { "EXPRES-BK-12",     "EXPRES",   "/SYMBOLTABLE EXPRES.OBJ" },
    { "EMUL-DISK25",      "EMUL",     "/SYMBOLTABLE EMUL.OBJ" },
    { "FCU",              "FCU",      "/SYMBOLTABLE FCU.OBJ FCUE.OBJ FCUMNG.OBJ FCUD.OBJ SELF.OBJ" },
    { "FIG-FORTH-1-3",    "FORTH",    "/SYMBOLTABLE FORTH.OBJ" },
    { "GAD2-HOBOT",       "GAD2",     "/SYMBOLTABLE GAD2.OBJ" },
    { "HAND-11",          "HAND",     "/SYMBOLTABLE HAND.OBJ" },
    { "HANDLE-503",       "HANDLE",   "/SYMBOLTABLE HANDLE.OBJ" },
    { "HD-DVKEmulator",   "HD",       "/SYMBOLTABLE /EXECUTE:HD.SYS HD.OBJ /X" },
    { "HELLO",            "HELLO",    "/SYMBOLTABLE HELLO.OBJ" },
    { "HWYENC-62",        "HWYENC",   "/SYMBOLTABLE HWYENC.OBJ" },
    { "INTERFACE-DISK02", "IF",       "/SYMBOLTABLE IF.OBJ IFMAIN.OBJ IFCOPY.OBJ IFTRP.OBJ IFSPEC.OBJ IFPP.OBJ" },
    { "LD-FD6W",          "LD",       "/SYMBOLTABLE /EXECUTE:LD.SYS LD.OBJ SYSLIB.OBJ /X" },
    { "LZSAV",            "LZSAV",    "/SYMBOLTABLE LZSAV.OBJ" },
    { "LP-DISK09",        "LP",       "/SYMBOLTABLE /EXECUTE:LP.SYS LP.OBJ /X" },
    { "MAIN-50",          "MAIN",     "/SYMBOLTABLE MAIN.OBJ LEVELS.OBJ" },
    { "MUL",              "MUL",      "/SYMBOLTABLE MUL.OBJ" },
    { "MYTEST1",          "MYTST1",   "/SYMBOLTABLE MYTST1.OBJ OTHER.OBJ" },
    { "MYTEST2",          "MYTST2",   "/SYMBOLTABLE MYTST2.OBJ OTHER.OBJ" },
    { "MYTEST3",          "MYTST3",   "/SYMBOLTABLE MYTST3.OBJ OTHER.OBJ" },
    { "MYTEST3-ALPHAMAP", "MYTST3",   "/SYMBOLTABLE /ALPHABETIZE MYTST3.OBJ OTHER.OBJ" },
    { "MYTEST3-NOSTB",    "MYTST3",   "MYTST3.OBJ OTHER.OBJ" },
    { "MYTEST3-WIDEMAP",  "MYTST3",   "/SYMBOLTABLE /WIDE MYTST3.OBJ OTHER.OBJ" },
    { "NET-DISK09",       "NET",      "/SYMBOLTABLE NET.OBJ" },
    { "NETTST-DISK09",    "NETTST",   "/SYMBOLTABLE NETTST.OBJ" },
    { "NL-FD6W",          "NL",       "/SYMBOLTABLE /EXECUTE:NL.SYS NL.OBJ /X" },
    { "PAKDMP-PHOOKY",    "PAKDMP",   "/SYMBOLTABLE PAKDMP.OBJ" },
    { "PAKWRT-PHOOKY",    "PAKWRT",   "/SYMBOLTABLE PAKWRT.OBJ" },
    { "PCBUG-1801BM1",    "PCBUG",    "/SYMBOLTABLE PCBUG.OBJ" },
    { "PCM-DISK09",       "PCM",      "/SYMBOLTABLE PCM.OBJ" },
    { "PDPCLK",           "PDPCLK",   "/SYMBOLTABLE PDPCLK.OBJ" },
    { "PITEST-1801BM1",   "PITEST",   "/SYMBOLTABLE PITEST.OBJ" },
    { "PRIM-FORTRANIV",   "PRIM",     "/SYMBOLTABLE PRIM.OBJ FORLIB.OBJ" },
    { "RDMTUN-DSKMXC",    "RDMTUN",   "/SYMBOLTABLE RDMTUN.OBJ SYSLIB.OBJ" },
    { "REGCOP-ODE11",     "REGCOP",   "/SYMBOLTABLE REGCOP.OBJ" },
    { "REGPER-ODE11",     "REGPER",   "/SYMBOLTABLE REGPER.OBJ" },
    { "RTS",              "RTS",      "/SYMBOLTABLE RTS.OBJ" },
    { "RTLEM",            "RTLEM",    "/SYMBOLTABLE RTLEM.OBJ" },
    { "SATMON-HADI3DSK",  "SATMON",   "/SYMBOLTABLE SATMON.OBJ" },
    { "SNDOFF-ANDREY14",  "SNDOFF",   "/SYMBOLTABLE SNDOFF.OBJ" },
    { "SOKOBA",           "SOKOBA",   "/SYMBOLTABLE SOKOBA.OBJ SOKDAT.OBJ ALP.OBJ RALP.OBJ SOKMAZ.OBJ" },
    { "SOKOED",           "SOKOED",   "/SYMBOLTABLE SOKOED.OBJ SOKED.OBJ SOKDAT.OBJ" },
    { "SPCINV",           "SPCINV",   "/SYMBOLTABLE SPCINV.OBJ" },
    { "SPLIT-DSKMXC",     "SPLIT",    "/SYMBOLTABLE SPLIT.OBJ" },
    { "T401-1801BM1",     "T401",     "/SYMBOLTABLE T401.OBJ" },
    { "T402-1801BM1",     "T402",     "/SYMBOLTABLE T402.OBJ" },
    { "T404-1801BM1",     "T404",     "/SYMBOLTABLE T404.OBJ" },
    { "TEST-PASCAL",      "TEST",     "/SYMBOLTABLE TEST.OBJ PASCAL.OBJ" },
    { "TPF-FORTRANIV",    "TPF",      "/SYMBOLTABLE TPF.OBJ GRAFOR.OBJ FORLIB.OBJ" },
    { "TSPAL-01B",        "TSPAL",    "/SYMBOLTABLE TSPAL.OBJ" },
    { "TSTVM1-01A",       "TSTVM1",   "/SYMBOLTABLE TSTVM1.OBJ" },
    { "TSTVM2-03A",       "TSTVM2",   "/SYMBOLTABLE TSTVM2.OBJ" },
    { "TT-FD6W",          "TT",       "/SYMBOLTABLE /EXECUTE:TT.SYS TT.OBJ /X" },
    { "TVE1-1801BM1",     "TVE1",     "/SYMBOLTABLE TVE1.OBJ" },
    { "UCL-HOBOT",        "UCL",      "/SYMBOLTABLE UCL.OBJ" },
    { "UKFONT",           "UKFONT",   "/SYMBOLTABLE UKFONT.OBJ" },
    { "VM-FD6W",          "VM",       "/SYMBOLTABLE /EXECUTE:VM.SYS VM.OBJ /X" },
};
const int g_TestNumber = sizeof(g_Tests) / sizeof(g_Tests[0]);
