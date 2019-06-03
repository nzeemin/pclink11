
#define _CRT_SECURE_NO_WARNINGS

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <stdarg.h>
#include <time.h>
#include <errno.h>

#include "main.h"

#if defined(_DEBUG) && defined(_MSC_VER)
#include <crtdbg.h>
#endif


/////////////////////////////////////////////////////////////////////////////

struct SaveStatusEntry
{
    char     filename[64];
    size_t   filesize;
    bool     islibrary;
    uint8_t* data;
};
const int SaveStatusAreaSize = 31;
SaveStatusEntry SaveStatusArea[SaveStatusAreaSize];
int SaveStatusCount = 0;

char savfilename[64] = { 0 };

// **** GSD ENTRY STRUCTURE
struct GSDentry
{
    uint32_t    symbol;     // SYMBOL CHARS 1-6(RAD50)
    uint8_t     flags;      // FLAGS
    uint8_t     code;       // CODE BYTE
    uint16_t    value;      // SIZE OR OFFSET
};

// **** SYMBOL TABLE STRUCTURE
struct SymbolTableEntry
{
    uint32_t    name;       // 2 WD RAD50 NAME
    uint16_t    flagseg;    // PSECT FLAGS !  SEG #
    uint16_t    value;      // VALUE WORD
    uint16_t    status;     // A!B!C!D!  ENTRY # PTR

    const char* unrad50name() const { return unrad50(name); }
    uint8_t flags() const { return (flagseg >> 8); }
    uint8_t seg() const { return flagseg & 0xff; }
    uint16_t nextindex() const { return status & 07777; }
};

SymbolTableEntry* SymbolTable = nullptr;
SymbolTableEntry* ASECTentry = nullptr;
const int SymbolTableSize = 4095;  // STSIZE
int SymbolTableCount = 0;  // STCNT -- SYMBOL TBL ENTRIES COUNTER

struct LibraryModuleEntry
{
    uint8_t  libfileno;     // LIBRARY FILE # (8 BITS) 1-255
    uint16_t relblockno;    // REL BLK # (15 BITS)
    uint16_t byteoffset;    // BYTE OFFSET (9 BITS)
    uint16_t segmentno;     // SEGMENT NUMBER FOR THIS MODULE

    size_t offset() const { return relblockno * 512 + byteoffset; }
};
const int LibraryModuleListSize = 512; // NUMBER OF LIBRARY MOD LIST ENTRIES (0252 DEFAULT, 0525 FOR RSTS)
LibraryModuleEntry LibraryModuleList[LibraryModuleListSize];
int LibraryModuleCount = 0;  // Count of records in LibraryModuleList, see LMLPTR

struct ModuleSectionEntry
{
    uint16_t stindex;        // SymbolTable index
    uint16_t size;           // Section size
};
const int ModuleSectionTableSize = 256;
ModuleSectionEntry ModuleSectionTable[ModuleSectionTableSize];
int ModuleSectionCount = 0;


const uint32_t RAD50_ABS    = rad50x2(". ABS.");  // ASECT
const uint32_t RAD50_VSEC   = rad50x2(". VIR.");  // VIRTUAL SECTION SYMBOL NAME
const uint32_t RAD50_VIRSZ  = rad50x2("$VIRSZ");  // GLOBAL SYMBOL NAME FOR SIZE OF VIRTUAL SECTION
const uint32_t RAD50_PHNDL  = rad50x2("$OHAND");  // OVERLAY HANDLER PSECT
const uint32_t RAD50_ODATA  = rad50x2("$ODATA");  // OVERLAY DATA TABLE PSECT
const uint32_t RAD50_PTBL   = rad50x2("$OTABL");  // OVERLAY TABLE PSECT
const uint32_t RAD50_GHNDL  = rad50x2("$OVRH");   // /O OVERLAY HANDLER GBL ENTRY
const uint32_t RAD50_GVHNDL = rad50x2("$OVRHV");  // /V OVERLAY HANDLER GBL ENTRY
const uint32_t RAD50_GZHNDL = rad50x2("$OVRHZ");  // I-D SPACE OVERLAY HANDLER GBL ENTRY
const uint32_t RAD50_ZTABL  = rad50x2("$ZTABL");  // I-D SPACE OVERLAY HANDLER PSECT

const char* FORLIB = "FORLIB.OBJ";  // FORTRAN LIBRARY FILENAME
const char* SYSLIB = "SYSLIB.OBJ";  // DEFAULT SYSTEM LIBRARY FILENAME

const char* GSDItemTypeNames[] =
{
    /*0*/ "MODULE NAME",
    /*1*/ "CSECT NAME",
    /*2*/ "ISD ENTRY",
    /*3*/ "TRANSFER ADDR",
    /*4*/ "GLOBAL SYMBOL",
    /*5*/ "PSECT NAME",
    /*6*/ "IDENT DEFINITION",
    /*7*/ "VIRTUAL SECTION"
};

const char* RLDCommandNames[] =
{
    /*000*/ "NOT USED",
    /*001*/ "INTERNAL RELOCATION",
    /*002*/ "GLOBAL",
    /*003*/ "INTERNAL DISPLACED",
    /*004*/ "GLOBAL DISPLACED",
    /*005*/ "GLOBAL ADDITIVE",
    /*006*/ "GLOBAL ADDITIVE DISPLACED",
    /*007*/ "LOCATION COUNTER DEFINITION",
    /*010*/ "LOCATION COUNTER MODIFICATION",
    /*011*/ "SET PROGRAM LIMITS",
    /*012*/ "PSECT",
    /*013*/ "NOT USED",
    /*014*/ "PSECT DISPLACED",
    /*015*/ "PSECT ADDITIVE",
    /*016*/ "PSECT ADDITIVE DISPLACED",
    /*017*/ "COMPLEX",
};

const char* CPXCommandNames[] =
{
    /*000*/ "NOP",
    /*001*/ "ADD",
    /*002*/ "SUB",
    /*003*/ "MUL",
    /*004*/ "DIV",
    /*005*/ "AND",
    /*006*/ "OR",
    /*007*/ "XOR",
    /*010*/ "NEG",
    /*011*/ "COM",
    /*012*/ "STORE",
    /*013*/ "STORED",
    /*014*/ "ILLFMT",
    /*015*/ "ILLFMT",
    /*016*/ "PUSHGLO",
    /*017*/ "PUSHREL",
    /*020*/ "PUSHVAL",
};

static const char* weekday_names[] =
{ "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday" };
static const char month_names[][4] =
{ "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };


uint8_t* OutputBuffer = nullptr;
size_t OutputBufferSize = 0;
int OutputBlockCount = 0;

FILE* outfileobj = nullptr;
FILE* mapfileobj = nullptr;
FILE* stbfileobj = nullptr;


// Macros used to mark and detect unimplemented but needed
#define NOTIMPLEMENTED { printf("*** NOT IMPLEMENTED, line %d\n", __LINE__); }


/////////////////////////////////////////////////////////////////////////////


struct tagGlobals
{
    uint16_t    ODBLK[15]; // BLOCK TO HOLD BINOUT SPEC
    // LNKOV1->STORE TIME TO ROLL OVER DATE
    //uint16_t    TEMP; // TEMPORARY STORAGE
    uint8_t     TXTBLK[RECSIZ];  // SPACE FOR A FORMATTED BINARY RECORD

    int         UNDLST; // START OF UNDEFINED SYMBOL LIST
    uint16_t    SYEN0;  // ADR OF SYMBOL TABLE ENTRY NUMBER 0
    // REL PTR + THIS = ABS ADDR OF SYMBOL NODE
    int         CSECT;  // PTR TO LATEST SECTION (PASS1)

    //uint16_t    PA2LML; // START OF LML BUFR
    // LNKOV1->TEMP. SEGMENT POINTER
    //uint16_t    LMLPTR; // CURRENT PTR TO LIBRARY MOD LIST
    //uint16_t    STLML;  // CURRENT START OF LMLPTR IF MULTI-LIBR FILES
    //uint16_t    ENDLML; // END OF LIB MOD LIST
    //uint16_t    ESZRBA; // SIZE OF CURRENT LIBRARY EPT
    // RELOCATION INFO OUTPUT BUFR ADR
    uint16_t    OVCOUN; // NO. OF OVERLAY ENTRY PTS.
    uint16_t    OVSPTR; // PTR TO OVERLAY SEGMENT BLK
    uint8_t     PAS1_5; // PASS 1.5 SWITCH(0 ON PASS1, 1 IF TO DO LIBRARY,
    // BIT 7 SET IF DOING LIBRARIES
    uint8_t     DUPMOD; // 1 IF LIB MOD IS DUP
    // 0 IF LIB MOD IS NOT DUP
    uint8_t     NUMCOL; // NUMBER OF COLUMNS WIDE FOR MAP
    // IND + OR - RELOCATION DURING PASS 2
    uint8_t     LIBNB;  // LIBRARY FILE NUMBER FOR LML

    uint16_t    SWITCH; // SWITCHES FROM CSI (SEE "LINK1" FOR DETAILS)
    uint16_t    SWIT1;  // Switches
    //uint16_t    FILPT1; // START OF SAVESTATUS AREA -4

    // VARIABLES FOR PROGRAM BEING LINKED
    uint16_t    HGHLIM; // MAX # OF SECTIONS IN ANY MODULE PROCESSED
    // HIGHEST LOCATION USED BY PROGRAM (I-SPACE)
    uint16_t    DHGHLM; // MAX # OF SECTIONS IN ANY MODULE PROCESSED
    // HIGHEST LOCATION USED BY PROGRAM (D-SPACE)

    GSDentry BEGBLK; // TRANSFER ADDRESS BLOCK (4 WD GSD ENTRY)
    //   TRANS ADDR OR REL OFFSET FROM PSECT

    uint16_t    STKBLK[3]; // USER STACK ADDRESS BLOCK(SYMBOL & VALUE)
    // LNKSAV->TEMP 4 WORD STORAGE FOR GSD RECORD

    uint16_t    HSWVAL; // /H SWITCH VALUE - I-SPACE
    uint16_t    DHSWVL; // /H SWITCH VALUE - D-SPACE

    uint16_t    ESWVAL; // /E SWITCH VALUE - I-SPACE
    uint32_t    ESWNAM; // /E SWITCH NAME - I-SPACE
    uint16_t    DESWVL; // /E SWITCH VALUE - D-SPACE
    uint32_t    DESWNM; // /E SWITCH NAME - D-SPACE
    uint16_t    KSWVAL; // /K SWITCH VALUE OR STACK SIZE FOR REL FILE
    uint16_t    YSWNAM[25]; // /Y SECTION NAME ARRAY(TEMP OVERLAY # IN OV1 & SAV)
    // +2 NEXT ASSIGNABLE OUTPUT BLK(LNKMAP)
    //    RELOCATION INFO BLOCK #(LNKSAV) - I-SPACE
    // YSWVAL==YSWNAM+4
    uint16_t    DYSWNM[25];
    // DYSWVL==DYSWNM+4
    uint8_t     YSWT;   // SAY WE ARE USING /Y
    uint8_t     YCNT;   // NUMBER OF TIMES TO PROMPT FOR /Y (SET IN LINK2)
    uint16_t    DEFALT; // DEFAULT BOUNDARY VALUE FOR /Y (SET IN LINK2)
    uint16_t    USWVAL; // /U SWITCH VALUE - I-SPACE
    uint32_t    USWNAM; // /U SWITCH NAME - I-SPACE
    uint16_t    DUSWVL; // /U SWITCH VALUE - D-SPACE
    uint32_t    DUSWNM; // /U SWITCH NAME - D-SPACE
    uint16_t    QSWVAL; // /Q BUFFER POINTER
    uint16_t    ZSWVAL; // /Z SWITCH VALUE - I-SPACE
    uint16_t    DZSWVL; // /Z SWITCH VALUE - D-SPACE
    uint16_t    LRUNUM; // USED TO COUNT MAX # OF SECTIONS AND AS
    //  TIME STAMP FOR SAV FILE CACHING
    uint16_t    BITBAD; // -> START OF BITMAP TABLE (D-SPACE IF /J USED)
    uint16_t    IBITBD; // -> START OF I-SPACE BITMAP TABLE
    uint8_t     BITMAP[16]; // CONTAINS BITMAP OR IBITBD (IF /J USED)
    uint16_t    CACHER; // -> CACHE CONTROL BLOCKS
    uint16_t    NUMBUF; // NUMBER OF AVAILABLE CACHING BLOCKS (DEF=3)
    uint16_t    BASE;   // BASE OF CURRENT SECTION
    uint16_t    CKSUM;  // CHECKSUM FOR STB & LDA OUTPUT
    // LNKOV1->TEMP LINK POINTER TO NEW REGION BLK
    // CURRENT REL BLK OVERLAY NUM
    uint16_t    ENDRT;  // END OF ROOT SYMBOL TBL LIST
    uint16_t    VIRSIZ; // LARGEST REGION IN A PARTITION
    uint16_t    REGION; // XM REGION NUMBER
    uint16_t    WDBCNT; // WDB TABLE SIZE ( 14. * NUMBER OF PARTITIONS)
    uint16_t    HIPHYS; // HIGH LIMIT FOR EXTENDED MEMORY (96K MAX)
    uint16_t    SVLML;  // START OF LML LIST FOR WHOLE LIBRARY
    uint16_t    SW_LML; // LML INTO OVERLAY SWITCH, AND PASS INDICATOR
    uint16_t    LOC0;   // USED FOR CONTENTS OF LOC 0 IN SAV HEADER
    uint16_t    LOC66;  // # /O SEGMENTS SAVED FOR CONVERSION TO ADDR OF
    //  /V SEGS IN OVERLAY HANLDER TABLE
    uint16_t    LSTFMT; // CREF LISTING FORMAT (0=80COL, -1=132COL)

    // I-D SPACE VARIABLES

    uint16_t    IDSWIT; // BITMASK WORD FOR WHICH SWITCHES USE SEPARATED
    // I-D SPACE
    // D-SPACE, LOW BYTE, I-SPACE, HI BYTE
    uint16_t    ILEN;   // TOTAL LENGTH OF I-SPACE PSECTS IN WORDS
    uint16_t    DLEN;   // TOTAL LENGTH OF D-SPACE PSECTS IN WORDS
    uint16_t    IBLK;   // TOTAL LENGTH OF I-SPACE PSECTS IN BLOCKS
    uint16_t    DBLK;   // TOTAL LENGTH OF D-SPACE PSECTS IN BLOCKS
    uint16_t    IROOT;  // SIZE OF THE I-SPACE ROOT IN WORDS
    uint16_t    DROOT;  // SIZE OF THE D-SPACE ROOT IN WORDS
    uint16_t    IBASE;  // START OF THE I BASE (BLOCKS)
    uint16_t    DBASE;  // START OF THE D BASE (BLOCKS)
    uint16_t    ILOC40; // CONTENTS OF LOC 40 FOR I-SPACE CCB
    uint16_t    IFLG;   // NON-ZERO MEANS WRITING I-SPACE BITMAP
    uint16_t    IDSTRT; // I-D SPACE ENTRY POINT ($OVRHZ)
    uint16_t    ZTAB;   // I-D SPACE START ADDRESS OF PSECT $ZTABL
    uint16_t    OVRCNT; // # OF OVERLAY SEGMENTS, USED FOR COMPUTING $OVDF6
    uint16_t    DSGBAS; // PASS 2 BASE ADR OF D-SPACE OVERLAY SEGMENT
    uint16_t    DSGBLK; // PASS 2 BASE BLK OF D-SPACE OVERLAY SEGMENT

    uint32_t    MODNAM; // MODULE NAME, RAD50
    // LDA OUTPUT BUFR PTR OR REL INFO BUFR PTR
    uint32_t    IDENT;  // PROGRAM IDENTIFICATION
    // "RELADR" ADR OF RELOCATION CODE IN TEXT OF REL FILE
    // +2 "RELOVL" NEXT REL BLK OVERLAY #

    //uint16_t    ASECT[8];

    uint16_t    DHLRT;  // D-SPACE HIGH ADDR LIMIT OF REGION (R.GHLD)
    uint16_t    DBOTTM; // ST ADDR OF REGION AREA - D-SPACE (R.GSAD)
    uint16_t    DBOTTM_2; // REGION NUMBER (R.GNB)
    uint16_t    OVRG1;  // -> NEXT ORDB (R.GNXP)
    uint16_t    OVRG1_2; // -> OSDB THIS REGION (R.GSGP)
    uint16_t    HLRT;   // HIGH LIMIT OF AREA (R.GHL)
    uint16_t    BOTTOM; // ST ADDR OF REGION AREA - (I-SPACE IF /J USED)

    //uint16_t    CBUF;   // START OF CREF BUFFER
    //uint16_t    CBEND;  // CBUF + 512. BYTES FOR A 1 BLOCK CREF BUFFER
    uint16_t    QAREA[10]; // EXTRA QUEUE ELEMENT
    uint16_t    PRAREA[5]; // AREA FOR PROGRAMMED REQUESTS

    //uint16_t    EIB512; // IBUF + 512. BYTES FOR A 1 BLOCK MAP BUFR
    uint16_t    SEGBAS; // BASE OF OVERLAY SEGMENT
    uint16_t    SEGBLK; // BASE BLK OF OVERLAY SEGMENT
    uint16_t    TXTLEN; // TEMP FOR /V SWITCH
    uint16_t    LINLFT; // NUMBER OF LINES LEFT ON CURRENT MAP PAGE

    // The following globals are defined inside the code

    uint16_t    FLGWD;  // INTERNAL FLAG WORD
    //uint16_t    ENDOL;  // USE FOR CONTINUE SWITCHES /C OR //
    uint16_t    SEGNUM; // KEEP TRACK OF INPUT SEGMENT #'S

    // INPUT BUFFER INFORMATION

    uint16_t    IRAREA; // CHANNEL NUMBER AND .READ EMT CODE
    uint16_t    CURBLK; // RELATIVE INPUT BLOCK NUMBER
    //uint16_t    IBUF;   // INPUT BUFR ADR(ALSO END OF OUTPUT BUFR (OBUF+512))
    //uint16_t    IBFSIZ; // INPUT BUFR SIZE (MULTIPLE OF 256) WORD COUNT

    //uint16_t    OBLK;   // RELATIVE OUTPUT BLOCK #
    //uint16_t    OBUF;   // OUTPUT BUFR ADR

    //uint16_t    MBLK;   // OUTPUT BLK # (INIT TO -1 FOR BUMP BEFORE WRITE)
    uint16_t    MBPTR;  // OUTPUT BUFR POINTER (0 MEANS NO MAP OUTPUT)

    //uint16_t    CBLK;   // OUTPUT BLK # (INIT TO -1 FOR BUMP BEFORE WRITE)
    //uint16_t    CBPTR;  // DEFAULT IS NO CREF
}
Globals;


/////////////////////////////////////////////////////////////////////////////


void fatal_error(const char* message, ...)
{
    assert(message != nullptr);

    printf("ERROR: ");
    {
        va_list ptr;
        va_start(ptr, message);
        vprintf(message, ptr);
        va_end(ptr);
    }

    exit(EXIT_FAILURE);
}


/////////////////////////////////////////////////////////////////////////////
// Symbol table functions

void symbol_table_enter(int* pindex, uint32_t lkname, uint16_t lkwd)
{
    assert(pindex != nullptr);

    // Find empty entry
    if (SymbolTableCount >= SymbolTableSize)
        fatal_error("ERR1: Symbol table overflow.\n");

    int index = *pindex;
    SymbolTableEntry* entry;
    for (;;)
    {
        entry = SymbolTable + index;
        if (entry->name == 0)
            break;
        index++;
        if (index >= SymbolTableSize)
            index = 0;
    }

    //printf("        Saving entry: index %4d name '%s' segment %d\n", index, unrad50(lkname), Globals.SEGNUM);

    if (lkname == 0)
        lkname = (Globals.SEGNUM + 1) << 2;  // USE SEGMENT # FOR BLANK SECTION NAME

    // Save the entry
    SymbolTableCount++;
    entry->name = lkname;
    entry->flagseg = lkwd;
    *pindex = index;
}

// MAKE A DUPLICATE SYMBOL NON-DUPLICATE, see LINK3\DELETE
void symbol_table_delete(int index)
{
    assert(index > 0);
    assert(index < SymbolTableSize);

    //TODO
    NOTIMPLEMENTED
}

// ADD A REFERENCED SYMBOL TO THE UNDEFINED LIST, see LINK3\ADDUDF
void symbol_table_add_undefined(int index)
{
    assert(index > 0);
    assert(index < SymbolTableSize);

    if (Globals.UNDLST != 0)
    {
        SymbolTableEntry* oldentry = SymbolTable + Globals.UNDLST;
        oldentry->value = (uint16_t)(oldentry->value | index);  // set back reference
    }

    SymbolTableEntry* entry = SymbolTable + index;
    entry->status |= SY_UDF;  // MAKE CUR SYM UNDEFINED
    entry->flagseg = (entry->flagseg & ~SY_SEG) | Globals.SEGNUM;  // SET SEGMENT # WHERE INITIAL REF
    entry->status |= (uint16_t)Globals.UNDLST;

    Globals.UNDLST = index;

    Globals.FLGWD |= AD_LML;  // IND TO ADD TO LML LATER
}

// REMOVE A ENTRY FROM THE UNDEFINED LIST, see LINK3\REMOVE
void symbol_table_remove_undefined(int index)
{
    assert(index > 0);
    assert(index < SymbolTableSize);

    SymbolTableEntry* entry = SymbolTable + index;
    uint16_t previndex = entry->value;
    uint16_t nextindex = entry->nextindex();
    if (previndex == 0)
        Globals.UNDLST = nextindex;
    else
    {
        SymbolTableEntry* preventry = SymbolTable + previndex;
        preventry->status = entry->status;
    }
    if (nextindex != 0)
    {
        SymbolTableEntry* nextentry = SymbolTable + nextindex;
        nextentry->value = previndex;
    }
    entry->value = 0;
    entry->status &= 0170000;
}

// ANYUND
bool is_any_undefined()
{
    return (Globals.UNDLST != 0);
}

// SYMBOL TABLE SEARCH ROUTINE
// In:  lkname = symbol name to lookup
// In:  lnwd   = FLAGS & SEGMENT # MATCH WORD
// In:  lkmsk  = MASK OF BITS DO NOT CARE ABOUT FOR A MATCH
// In:  dupmsk
// Out: return = true if found
// Out: result = index of the found entity, or index of entity to work with
bool symbol_table_search_routine(uint32_t lkname, uint16_t lkwd, uint16_t lkmsk, uint16_t dupmsk, int* result)
{
    assert(result != nullptr);
    assert(SymbolTable != nullptr);

    // Calculate hash
    uint16_t hash = 0;
    if (lkname != 0)
        hash = LOWORD(lkname) + HIWORD(lkname);
    else
    {
        // 0 = BLANK NAME
        hash = (Globals.SEGNUM + 1) << 2;  // USE SEGMENT # FOR BLANK SECTION NAME
        lkname = hash;
    }

    // Normalize hash to table size
    uint16_t stdiv = 0120000;  // NORMALIZED STSIZE USED FOR DIVISION
    while (hash >= SymbolTableSize)
    {
        if (hash > stdiv)
            hash -= stdiv;
        else
            stdiv = stdiv >> 1;
    }

    bool found = false;
    int index = hash;  // Now we have starting index
    int count = SymbolTableCount;
    for (;;)
    {
        SymbolTableEntry* entry = SymbolTable + index;
        if (entry->name == 0)
        {
            *result = index;
            break;  // EMPTY CELL
        }
        if (entry->name == lkname)
        {
            // AT THIS POINT HAVE FOUND A MATCHING SYMBOL NAME, NOW MUST CHECK FOR MATCHING ATTRIBUTES.
            uint16_t flagsmasked = (entry->flagseg & ~lkmsk);
            if (flagsmasked == lkwd)
            {
                if (dupmsk == 0 || (entry->status & SY_DUP) == 0)
                {
                    *result = index;
                    found = true;
                    break;
                }

                //TODO: Process dups
                NOTIMPLEMENTED
            }
        }

        count--;
        if (count == 0)
        {
            *result = index;
            break;  // not found
        }

        index++;
        if (index >= SymbolTableSize)
            index = 0;
    }

    return found;
}

// 'DLOOKE' DOES A LOOKUP & IF NOT FOUND THEN ENTERS THE NEW SYMBOL INTO
//     THE SYMBOL TABLE.  DOES NOT REQUIRE A SEGMENT NUMBER MATCH
//     WHETHER THE SYMBOL IS A DUPLICATE OR NOT.
// Out: return = true if found, false if new entry
bool symbol_table_dlooke(uint32_t lkname, uint16_t lkwd, uint16_t lkmsk, int* pindex)
{
    assert(pindex != nullptr);

    bool found = symbol_table_search_routine(lkname, lkwd, lkmsk, 0, pindex);
    if (found)
        return true;

    symbol_table_enter(pindex, lkname, lkwd);
    return false;
}
// 'LOOKUP' ONLY SEARCHES THE SYMBOL TABLE FOR A SYMBOL MATCH.  IF SYMBOL
//     IS A DUPLICATE, THIS ROUTINE REQUIRES A SEGMENT NUMBER MATCH.
bool symbol_table_lookup(uint32_t lkname, uint16_t lkwd, uint16_t lkmsk, int* pindex)
{
    assert(pindex != nullptr);

    return symbol_table_search_routine(lkname, lkwd, lkmsk, SY_DUP, pindex);
}
// 'LOOKE'  DOES A LOOKUP & IF NOT FOUND THEN ENTERS THE NEW SYMBOL INTO
//     THE SYMBOL TABLE.  IF SYMBOL IS A DUPLICATE, THIS ROUTINE
//     REQUIRES A SEGMENT NUMBER MATCH.
// Out: return = true if found, false if new entry
bool symbol_table_looke(uint32_t lkname, uint16_t lkwd, uint16_t lkmsk, int* pindex)
{
    assert(pindex != nullptr);

    bool found = symbol_table_search_routine(lkname, lkwd, lkmsk, SY_DUP, pindex);
    if (found)
        return true;

    symbol_table_enter(pindex, lkname, lkwd);
    return false;
}
// 'SEARCH' THIS ROUTINE DOES A LOOKUP ONLY AND DOES NOT CARE WHETHER THE
//     SYMBOL IS A DUPLICATE OR NOT.  THIS ROUTINE IS USED REPEATEDLY
//     AFTER A SINGLE CALL TO 'DLOOKE'.
bool symbol_table_search(uint32_t lkname, uint16_t lkwd, uint16_t lkmsk, int* pindex)
{
    assert(pindex != nullptr);

    return symbol_table_search_routine(lkname, lkwd, lkmsk, 0, pindex);
}

// Print Library Module List, for DEBUG only
void print_lml_table()
{
    printf("  LibraryModuleList count = %d.\n", LibraryModuleCount);
    for (int i = 0; i < LibraryModuleCount; i++)
    {
        const LibraryModuleEntry* lmlentry = LibraryModuleList + i;
        printf("    #%04d file %02d block %06ho offset %06ho\n", (uint16_t)i, lmlentry->libfileno, lmlentry->relblockno, lmlentry->byteoffset/*, lmlentry->segmentno*/);
    }
}

// Print the Symbol Table, for DEBUG only
void print_symbol_table()
{
    printf("  SymbolTable count = %d.\n", SymbolTableCount);
    for (int i = 0; i < SymbolTableSize; i++)
    {
        const SymbolTableEntry* entry = SymbolTable + i;
        if (entry->name == 0)
            continue;
        printf("    %06ho '%s' %06ho %06ho %06ho  ", (uint16_t)i, entry->unrad50name(), entry->flagseg, entry->value, entry->status);
        if (entry->flagseg & SY_SEC) printf("SECT ");
        if (entry->status & SY_UDF) printf("UNDEF ");
        if (entry->status & SY_IND) printf("IND ");
        if ((entry->flagseg & SY_SEC) == 0 && (entry->status & SY_WK)) printf("WEAK ");
        if ((entry->flagseg & SY_SEC) && (entry->status & SY_SAV)) printf("SAV ");
        if ((entry->status & 07777) == 0) printf("(eol) ");  // show end-of-list node
        printf("\n");
    }

    printf("  BEGBLK '%s' %06ho\n", unrad50(Globals.BEGBLK.symbol), Globals.BEGBLK.value);

    printf("  ASECT-started list has ");
    // Enumerate entries starting with ASECT, make sure there's no loops or UNDEF entries
    SymbolTableEntry* preventry = ASECTentry;
    int count = 1;
    for (;;)
    {
        int nextindex = preventry->nextindex();
        if (nextindex == 0)  // end of chain
            break;
        SymbolTableEntry* nextentry = SymbolTable + nextindex;
        preventry = nextentry;
        count++;
        if (count > SymbolTableSize)
            break;
    }
    if (count > SymbolTableSize)
        printf("loops.\n");
    else
        printf("%d entries.\n", count);

    printf("  UNDLST = %06ho", (uint16_t)Globals.UNDLST);
    if (Globals.UNDLST == 0)
        printf("\n");
    else  // Enumerate entries starting with UNDLST, make sure there's no loops or non-UNDEF entries
    {
        int index = Globals.UNDLST;
        count = 0;
        while (index != 0)
        {
            SymbolTableEntry* entry = SymbolTable + index;
            index = entry->nextindex();
            count++;
            if (count > SymbolTableSize)
                break;
        }
        if (count > SymbolTableSize)
            printf("; the list has loops.\n");
        else
            printf("; the list has %d entries.\n", count);
    }
}

// Clear Module Section Table (MST)
void mst_table_clear()
{
    memset(ModuleSectionTable, 0, sizeof(ModuleSectionTable));
    ModuleSectionCount = 0;
}

// Print Module Section Table (MST), for DEBUG only
void print_mst_table()
{
    printf("  ModuleSectionTable count = %d.\n", ModuleSectionCount);
    for (int i = 0; i < ModuleSectionCount; i++)
    {
        const ModuleSectionEntry* mstentry = ModuleSectionTable + i;
        const SymbolTableEntry* entry = SymbolTable + mstentry->stindex;
        printf("    #%04d index %06ho '%s' size %06ho\n",
               (uint16_t)i, mstentry->stindex,
               mstentry->stindex == 0 ? "" : entry->unrad50name(),
               mstentry->size);
    }
}


/////////////////////////////////////////////////////////////////////////////


// Initialize arrays and variables
void initialize()
{
    //printf("Initialization\n");

    memset(&Globals, 0, sizeof(Globals));

    memset(SaveStatusArea, 0, sizeof(SaveStatusArea));
    SaveStatusCount = 0;

    SymbolTable = (SymbolTableEntry*) ::calloc(SymbolTableSize, sizeof(SymbolTableEntry));
    SymbolTableCount = 0;

    memset(LibraryModuleList, 0, sizeof(LibraryModuleList));

    // Set globals defaults, see LINK1\START1
    Globals.NUMCOL = 3; // 3-COLUMN MAP IS NORMAL
    Globals.NUMBUF = 3; // NUMBER OF AVAILABLE CACHING BLOCKS (DEF=3)
    Globals.BEGBLK.symbol = RAD50_ABS;
    Globals.BEGBLK.code = 5;
    Globals.BEGBLK.flags = 0100/*CS$GBL*/ | 04/*CS$ALO*/;
    Globals.BEGBLK.value = 000001;  // INITIAL TRANSFER ADR TO 1
    //TODO
    Globals.DBOTTM = 01000;
    Globals.BOTTOM = 01000;
    Globals.KSWVAL = 128/*RELSTK*/;
}

// Free all memory, close all files
void finalize()
{
    //printf("Finalization\n");

    if (SymbolTable != nullptr)
    {
        free(SymbolTable);  SymbolTable = nullptr;
    }

    for (int i = 0; i < SaveStatusCount; i++)
    {
        SaveStatusEntry* sscur = SaveStatusArea + i;
        if (sscur->data != nullptr)
        {
            free(sscur->data);  sscur->data = nullptr;
        }
    }

    if (OutputBuffer != nullptr)
    {
        free(OutputBuffer);  OutputBuffer = nullptr;  OutputBufferSize = 0;
    }

    if (outfileobj != nullptr)
    {
        fclose(outfileobj);  outfileobj = nullptr;
    }
    if (mapfileobj != nullptr)
    {
        fclose(mapfileobj);  mapfileobj = nullptr;
    }
    if (stbfileobj != nullptr)
    {
        fclose(stbfileobj);  stbfileobj = nullptr;
    }
}

// Read all input files into memory
void read_files()
{
    for (int i = 0; i < SaveStatusCount; i++)
    {
        SaveStatusEntry* sscur = SaveStatusArea + i;
        assert(sscur->filename[0] != 0);

        FILE* file = fopen(sscur->filename, "rb");
        if (file == nullptr)
            fatal_error("ERR2: Failed to open input file: %s, errno %d.\n", sscur->filename, errno);

        fseek(file, 0L, SEEK_END);
        size_t filesize = ftell(file);
        if (filesize > 256 * 1024)
            fatal_error("Input file %s too long.\n", sscur->filename);
        sscur->filesize = filesize;

        sscur->data = (uint8_t*)malloc(filesize);
        if (sscur->data == nullptr)
            fatal_error("Failed to allocate memory for input file %s\n", sscur->filename);

        fseek(file, 0L, SEEK_SET);
        size_t bytesread = fread(sscur->data, 1, filesize, file);
        if (bytesread != filesize)
            fatal_error("ERR2: Failed to read input file %s\n", sscur->filename);
        //printf("  File read %s, %d bytes.\n", sscur->filename, bytesread);

        fclose(file);
    }
}

// FORCE0 IS CALLED TO GENERATE THE FOLLOWING WARNING MESSAGE, see LINK3\FORCE0
// ?LINK-W-DUPLICATE SYMBOL "SYMBOL" IS FORCED TO THE ROOT
void pass1_force0(const SymbolTableEntry* entry)
{
    assert(entry != nullptr);

    printf("DUPLICATE SYMBOL \"%s\" IS FORCED TO THE ROOT\n", entry->unrad50name());
}

// LINK3\ORDER, LINK5\ALPHA
void pass1_insert_entry_into_ordered_list(int index, SymbolTableEntry* entry, bool absrel)
{
    assert(index > 0 && index < SymbolTableSize);
    assert(entry != nullptr);

    SymbolTableEntry* sectentry = ASECTentry;
    if (Globals.CSECT > 0)
    {
        sectentry = SymbolTable + Globals.CSECT;
        if (absrel && (sectentry->flags() & 0040) != 0)
            sectentry = ASECTentry;
    }
    assert(sectentry != nullptr);

    SymbolTableEntry* preventry = sectentry;
    for (;;)
    {
        int nextindex = preventry->nextindex();
        if (nextindex == 0)  // end of chain
            break;
        SymbolTableEntry* nextentry = SymbolTable + nextindex;
        if (nextentry->flagseg & SY_SEC)  // next entry is a section
            break;

        if (((Globals.SWITCH & SW_A) == 0 && (nextentry->value > entry->value)) ||
            ((Globals.SWITCH & SW_A) != 0 && (nextentry->name > entry->name)))
            break;

        preventry = nextentry;
    }
    // insert the new entry here
    int fwdindex = preventry->nextindex();
    preventry->status = (uint16_t) ((preventry->status & 0170000) | index);
    entry->status = (uint16_t) ((entry->status & 0170000) | fwdindex);
}

// LINK3\TADDR
void process_pass1_gsd_item_taddr(const uint16_t* itemw, const SaveStatusEntry* sscur)
{
    assert(itemw != nullptr);
    assert(sscur != nullptr);

    if ((Globals.BEGBLK.value & 1) == 0)  // USE ONLY 1ST EVEN ONE ENCOUNTERED
        return; // WE ALREADY HAVE AN EVEN ONE.  RETURN

    uint32_t lkname = MAKEDWORD(itemw[0], itemw[1]);
    uint16_t lkmsk = (uint16_t)~SY_SEC; // CARE ABOUT SECTION FLG
    uint16_t lkwd = (uint16_t)SY_SEC; // SECTION NAME LOOKUP IN THE ROOT
    int index;
    if (!symbol_table_lookup(lkname, lkwd, lkmsk, &index))
        fatal_error("ERR31: Transfer address for '%s' undefined or in overlay.\n", unrad50(lkname));
    SymbolTableEntry* entry = SymbolTable + index;
    printf("        Entry '%s' %06ho %06ho %06ho\n", entry->unrad50name(), entry->flagseg, entry->value, entry->status);
    //TODO

    if (entry->value > 0)  // IF CURRENT SIZE IS 0 THEN OK
    {
        // MUST SCAN CURRENT MODULE TO FIND SIZE CONTRIBUTION TO
        // TRANSFER ADDR SECTION TO CALCULATE PROPER OFFSET
        //const SaveStatusEntry* sscurtmp = sscur;
        size_t offset = 0;
        while (offset < sscur->filesize)
        {
            const uint8_t* data = sscur->data + offset;
            uint16_t blocksize = ((uint16_t*)data)[1];
            uint16_t blocktype = ((uint16_t*)data)[2];
            if (blocktype == 1)
            {
                int itemcounttmp = (blocksize - 6) / 8;
                for (int itmp = 0; itmp < itemcounttmp; itmp++)
                {
                    const uint16_t* itemwtmp = (const uint16_t*)(data + 6 + 8 * itmp);
                    int itemtypetmp = (itemwtmp[2] >> 8) & 0xff;
                    if ((itemtypetmp == 1/*CSECT*/ || itemtypetmp == 5/*PSECT*/) &&
                        itemw[0] == itemwtmp[0] && itemw[1] == itemwtmp[1])  // FOUND THE PROPER SECTION
                    {
                        uint16_t itemvaltmp = itemwtmp[3];
                        uint16_t sectsize = (itemvaltmp + 1) & ~1;  // ROUND SECTION SIZE TO WORD BOUNDARY
                        printf("        Item '%s' type %d - CSECT or PSECT size %06ho\n", unrad50(itemwtmp[0], itemwtmp[1]), itemtypetmp, sectsize);
                        int newvalue = entry->value - sectsize + itemw[3];
                        //TODO: UPDATE OFFSET VALUE
                        //TODO
                        // See LINK3\TADDR\100$
                        Globals.BEGBLK.symbol = entry->name;  // NAME OF THE SECTION
                        Globals.BEGBLK.flags = entry->flags();
                        Globals.BEGBLK.code = entry->seg();
                        Globals.BEGBLK.value = (uint16_t)newvalue /*entry->value*/;  // RELATIVE OFFSET FROM THE SECTION
                        break;
                    }
                }
            }
            data += blocksize; offset += blocksize;
            data += 1; offset += 1;  // Skip checksum
        }
    }
}

// GLOBAL SYMBOL. See LINK3\SYMNAM
void process_pass1_gsd_item_symnam(const uint16_t* itemw)
{
    assert(itemw != nullptr);

    uint32_t lkname = MAKEDWORD(itemw[0], itemw[1]);
    uint16_t lkwd = 0;
    uint16_t lkmsk = (uint16_t)~SY_SEC;
    int index;
    bool found;
    if (itemw[2] & 010/*SY$DEF*/)  // IS SYMBOL DEFINED HERE?
    {
        if (Globals.SW_LML & 0100000)  // LIBRARY PREPROCESS PASS?
            return;  // IGNORE PREPROCESS PASS DEFS

        if ((Globals.PAS1_5 & 0200) != 0 && Globals.SEGNUM == 0) // ROOT DEF?
        {
            found = symbol_table_dlooke(lkname, lkwd, lkmsk, &index);
            SymbolTableEntry* entry = SymbolTable + index;
            if (entry->status & SY_DUP)  // IS IT A DUP SYMBOL?
            {
                symbol_table_delete(index);  // DELETE ALL OTHER COPIES OF SYMBOL
                fatal_error("ERR70: Duplicate symbol '%s' is defined in non-library.\n", unrad50(lkname));
            }
        }
        else  // NOT ROOT, NORMAL LOOKUP
        {
            found = symbol_table_looke(lkname, lkwd, lkmsk, &index);
        }
        // LINK3\SYMNAM\100$
        if (!found)  // LINK3\SYMNAM\140$
        {
            SymbolTableEntry* entry = SymbolTable + index;
            entry->flagseg = (entry->flagseg & ~SY_SEG) | Globals.SEGNUM;
            // LINK3\SYMV
            entry->value = itemw[3] + Globals.BASE;

            pass1_insert_entry_into_ordered_list(index, entry, (itemw[2] & 040) == 0);
        }
        else  // OLD SYMBOL, see LINK3\DEFREF
        {
            SymbolTableEntry* entry = SymbolTable + index;
            if ((entry->status & 0100000/*SY.UDF*/) == 0)  // DEFINED BEFORE?
            {
                // SYMBOL WAS DEFINED BEFORE, ABSOLUTE SYMBOLS WITH SAME VALUE NOT MULTIPLY DEFINED
                if ((itemw[2] & 040/*SY$REL*/) != 0 || entry->value != itemw[3])
                    fatal_error("ERR24: Multiple definition of symbol '%s'.\n", unrad50(lkname));
            }
            else  // DEFINES A REFERENCED SYMBOL, see LINK3\DEFREF\130$
            {
                symbol_table_remove_undefined(index);  // REMOVE ENTRY FROM UNDEFINED LIST
                entry->status &= ~(0117777);  // CLR SY.UDF+SY.WK+^CSY.ENB
                //TODO: IF INPUT SEG # .NE. SYM SEG # THEN SET EXTERNAL REFERENCE BIT
                //TODO: CLEAR SEGMENT # BITS & SET SEGMENT # WHERE DEFINED
                entry->value = itemw[3] + Globals.BASE;

                pass1_insert_entry_into_ordered_list(index, entry, (itemw[2] & 040) == 0);
            }
        }
    }
    else  // Symbol referenced here, not defined here
    {
        if (Globals.SEGNUM == 0) // REFERENCE FROM ROOT?
        {
            found = symbol_table_dlooke(lkname, lkwd, lkmsk, &index);
            SymbolTableEntry* entry = SymbolTable + index;
            if (entry->status & SY_DUP)  // IS IT A DUP SYMBOL?
            {
                if ((entry->status & SY_UDF) == 0)  // IS SYMBOL DEFINED?
                {
                    symbol_table_delete(index);  // DELETE ALL OTHER COPIES OF SYMBOL
                    pass1_force0(entry);
                }
                else
                {
                    entry->status |= SY_IND;  // EXT. REF.
                }
            }
        }
        else // NOT ROOT, NORMAL LOOKUP
        {
            found = symbol_table_looke(lkname, lkwd, lkmsk, &index);
        }
        // LINK3\DOREF - PROCESS SYMBOL REFERENCE
        if (!found)
        {
            symbol_table_add_undefined(index);
        }
        else // Symbol is found
        {
            SymbolTableEntry* entry = SymbolTable + index;
            if ((itemw[2] & 001/*SY$WK*/) != 0)  // IS THIS A STRONG REFERENCE?
                entry->status &= ~SY_WK;  // YES, SO MAKE SURE WEAK BIT IS CLEARED
            //TODO
            if (Globals.SW_LML & 0100000)  // ARE WE ON LIB. PREPROCESS PASS?
                Globals.FLGWD |= 020/*AD.LML*/;  // IND TO ADD TO LML LATER IF LIBRARY CAUSED A NEW UNDEF SYM
        }
    }
    //TODO
}

// LINK3\CSECNM
void process_pass1_gsd_item_csecnm(const uint16_t* itemw, int& itemtype, int& itemflags)
{
    assert(itemw != nullptr);

    uint32_t lkname = MAKEDWORD(itemw[0], itemw[1]);
    uint16_t lkwd = (uint16_t)SY_SEC;
    uint16_t lkmsk = (uint16_t)~SY_SEC;
    if (lkname == 0)  // BLANK .CSECT = .PSECT ,LCL,REL,I,CON,RW
    {
    }
    else if (lkname == RAD50_ABS)  // Case 2: ASECT
    {
        //(SWIT1 & SW_J) ?
        // .ASECT = .PSECT . ABS.,GBL,ABS,I,OVR,RW (NON I-D SPACE)
        // .ASECT = .PSECT . ABS.,GBL,ABS,D,OVR,RW (I-D SPACE)
        itemflags |= 0100 + 4;  // CS$GBL+CS$ALO == GBL,OVR
        if (Globals.SWIT1 & SW_J)
            itemtype = 0200;  // CS$TYP == D
    }
    else  // Case 3: named section
    {
        itemflags |= 0100 + 4 + 040;  // CS$GBL+CS$ALO+CS$REL == GBL,OVR,REL
    }
    //TODO: Set flags according to case and process as PSECT
}

// Process GSD item PROGRAM SECTION NAME, see LINK3\PSECNM
void process_pass1_gsd_item_psecnm(const uint16_t* itemw, int& itemflags)
{
    assert(itemw != nullptr);

    Globals.LRUNUM++; // COUNT SECTIONS IN A MODULE

    uint32_t lkname = MAKEDWORD(itemw[0], itemw[1]);
    uint16_t lkmsk = (uint16_t)~SY_SEC;
    uint16_t lkwd = (uint16_t)SY_SEC;
    if (itemflags & 1/*CS$SAV*/) // DOES PSECT HAVE SAVE ATTRIBUTE?
        itemflags |= 0100/*CS$GBL*/; // FORCE PSECT TO ROOT VIA GBL ATTRIBUTE
    if (itemflags & 0100/*CS$GBL*/) // LOCAL OR GLOBAL SECTION ?
    {
        lkmsk = (uint16_t)~(SY_SEC + SY_SEG); // CARE ABOUT SECTION FLG & SEGMENT #
        lkwd |= Globals.SEGNUM; // LOCAL SECTION QUALIFIED BY SEGMENT #
    }

    int index;
    bool isnewentry = !symbol_table_looke(lkname, lkwd, lkmsk, &index);
    SymbolTableEntry* entry = SymbolTable + index;

    if (itemflags & 1/*CS$SAV*/) // DOES PSECT HAVE SAV ATTRIBUTE?
        entry->status |= 1/*CS$SAV*/; // INDICATE SAV ATTRIBUTE IN PSECT ENTRY
    itemflags &= ~(010/*CS$NU*/ | 2/*CS$LIB*/ | 1/*CS$SAV*/); // ALL UNSUPPORTED FLAG BITS
    Globals.CSECT = index;  // PTR TO SYM TBL ENTRY
    if (isnewentry)
    {
        entry->flagseg |= Globals.SEGNUM; // SET SEGMENT # INDEPEND. OF LCL OR GBL
        //TODO: CREATE FORWARD ENTRY # PTR TO NEW NODE
        entry->flagseg |= (itemflags << 8);
        entry->value = 0;  // LENGTH=0 INITIALLY FOR NEW SECTION
    }
    else // AT THIS POINT SYMBOL WAS ALREADY ENTERED INTO SYMBOL TBL; see LINK3\OLDPCT
    {
        uint16_t R2 = ((entry->flagseg >> 8) & 0377) // GET PSECT FLAG BITS IN R2
                & ~(010/*CS$NU*/ | 2/*CS$LIB*/ | 1/*CS$SAV*/); // GET RID OF UNUSED FLAG BITS
        if (Globals.SWIT1 & SW_J) // ARE WE PROCESSING I-D SPACE?
        {
            //TODO
            NOTIMPLEMENTED
        }
        if (R2 != itemflags) // ARE SECTION ATTRIBUTES THE SAME?
            fatal_error("ERR10: Conflicting section attributes.\n");
    }

    // PLOOP and CHKRT
    if (lkname == RAD50_ABS)
        ASECTentry = entry;
    else if (isnewentry)
    {
        SymbolTableEntry* pSect = ASECTentry;
        while (pSect != nullptr)  // find section chain end
        {
            int nextindex = (pSect->status & 07777);
            if (nextindex == 0)  // end of chain
                break;
            pSect = SymbolTable + nextindex;
        }
        if (pSect != nullptr)
        {
            //int sectindex = pSect - SymbolTable;
            pSect->status |= index;  // set link to the new segment
        }
    }

    //TODO: primitive version, depends on section flags
    uint16_t sectsize = itemw[3];
    //if ((itemflags & 0040/*CS$REL*/) == 0 || (Globals.SWITCH & SW_X) != 0)
    //    sectsize = 0;
    if ((itemflags & 0200/*CS$TYP*/) == 0 || (itemflags & 0004/*CS$ALO*/) != 0)  // INSTRUCTION SECTION? CON SECTION ?
        sectsize = (sectsize + 1) & ~1;  // ROUND SECTION SIZE TO WORD BOUNDARY
    if (itemflags & 4/*CS$ALO*/)  // OVERLAYED SECTION?
    {
        Globals.BASE = 0;  // OVR PSECT, GBL SYM OFFSET IS FROM START OF SECTION
        //if (itemw[3] > entry->value)
    }
    if ((itemflags & 0040/*CS$REL*/) == 0)  // ASECT
    {
        if (sectsize > entry->value)  // Size is maximum from all ASECT sections
            entry->value = sectsize;
        Globals.BASE = 0;
    }
    else
    {
        Globals.BASE = entry->value;
        entry->value += sectsize;
    }
}

// PROCESS GSD TYPES, see LINK3\GSD
void process_pass1_gsd_item(const uint16_t* itemw, const SaveStatusEntry* sscur)
{
    assert(itemw != nullptr);
    assert(sscur != nullptr);

    uint32_t itemnamerad50 = MAKEDWORD(itemw[0], itemw[1]);
    int itemtype = (itemw[2] >> 8) & 0xff;
    int itemflags = (itemw[2] & 0377);

    printf("      Item '%s' type %d - %s", unrad50(itemnamerad50), itemtype, (itemtype > 7) ? "UNKNOWN" : GSDItemTypeNames[itemtype]);
    switch (itemtype)
    {
    case 0: // 0 - MODULE NAME FROM .TITLE, see LINK3\MODNME
        printf("\n");
        if (Globals.MODNAM == 0)
            Globals.MODNAM = itemnamerad50;
        break;
    case 2: // 2 - ISD ENTRY (IGNORED), see LINK3\ISDNAM
        printf(", ignored\n");
        break;
    case 3: // 3 - TRANSFER ADDRESS; see LINK3\TADDR
        printf(" %06ho\n", itemw[3]);
        process_pass1_gsd_item_taddr(itemw, sscur);
        break;
    case 4: // 4 - GLOBAL SYMBOL, see LINK3\SYMNAM
        printf(" flags %03o addr %06ho\n", itemflags, itemw[3]);
        process_pass1_gsd_item_symnam(itemw);
        break;
    case 1: // 1 - CSECT NAME, see LINK3\CSECNM
        printf(" %06ho\n", itemw[3]);
        process_pass1_gsd_item_csecnm(itemw, itemtype, itemflags);
        process_pass1_gsd_item_psecnm(itemw, itemflags);
    case 5: // 5 - PSECT NAME; see LINK3\PSECNM
        printf(" flags %03o maxlen %06ho\n", itemflags, itemw[3]);
        process_pass1_gsd_item_psecnm(itemw, itemflags);
        break;
    case 6: // 6 - IDENT DEFINITION; see LINK3\PGMIDN
        printf("\n");
        if (Globals.IDENT == 0)
            Globals.IDENT = itemnamerad50;
        break;
    case 7: // 7 - VIRTUAL SECTION; see LINK3\VSECNM
        printf("\n");
        //TODO
        NOTIMPLEMENTED
        break;
    default:
        printf("\n");
        fatal_error("ERR21: Bad GSD type %d found in %s.\n", itemtype, sscur->filename);
    }
}

void process_pass1_gsd_block(const SaveStatusEntry* sscur, const uint8_t* data)
{
    assert(sscur != nullptr);
    assert(data != nullptr);

    uint16_t blocksize = ((uint16_t*)data)[1];
    int itemcount = (blocksize - 6) / 8;
    //printf("    Processing GSD block, %d items\n", itemcount);

    for (int i = 0; i < itemcount; i++)
    {
        const uint16_t* itemw = (const uint16_t*)(data + 6 + 8 * i);
        memcpy(Globals.TXTBLK, itemw, 8);

        process_pass1_gsd_item(itemw, sscur);
    }
}

// PASS1: GSD PROCESSING, see LINK3\PASS1
void process_pass1()
{
    printf("PASS 1\n");
    Globals.PAS1_5 = 0;  // PASS 1 PHASE INDICATOR
    // PROCESS FORMATTED BINARY RECORDS, see LINK3\PA1
    for (int i = 0; i < SaveStatusCount; i++)
    {
        SaveStatusEntry* sscur = SaveStatusArea + i;
        assert(sscur->data != nullptr);

        printf("  Processing %s\n", sscur->filename);
        size_t offset = 0;
        while (offset < sscur->filesize)
        {
            uint8_t* data = sscur->data + offset;
            uint16_t* dataw = (uint16_t*)(data);
            if (*dataw != 1)
            {
                if (*dataw == 0)  // Possibly that is filler at the end of the block
                {
                    size_t offsetnext = (offset + 511) & ~511;
                    while (*data == 0 && offset < sscur->filesize && offset < offsetnext)
                    {
                        data++; offset++;
                    }
                    if (offset == sscur->filesize)
                        break;  // End of file
                }
                dataw = (uint16_t*)(data);
                if (*dataw != 1)
                    fatal_error("Unexpected word %06ho at %06ho in %s\n", *dataw, offset, sscur->filename);
            }

            uint16_t blocksize = ((uint16_t*)data)[1];
            uint16_t blocktype = ((uint16_t*)data)[2];

            if (blocktype == 0 || blocktype > 8)
                fatal_error("Illegal record type at %06ho in %s\n", offset, sscur->filename);
            else if (blocktype == 1)  // 1 - START GSD RECORD, see LINK3\GSD
            {
                printf("    Block type 1 - GSD at %06ho size %06ho\n", (uint16_t)offset, blocksize);
                process_pass1_gsd_block(sscur, data);
            }
            else if (blocktype == 6)  // 6 - MODULE END, see LINK3\MODND
            {
                //printf("    Block type 6 - ENDMOD at %06ho size %06ho\n", (uint16_t)offset, blocksize);
                if (Globals.HGHLIM < Globals.LRUNUM)
                    Globals.HGHLIM = Globals.LRUNUM;
                Globals.LRUNUM = 0;
                //TODO
            }
            else if (blocktype == 7)  // See LINK3\LIBRA
            {
                uint16_t libver = ((uint16_t*)data)[3];
                if (libver < L_HVER)
                    fatal_error("ERR23: Old library format (%03ho) in %s\n", libver, sscur->filename);
                sscur->islibrary = true;
                Globals.PAS1_5 |= 1;  // ALSO SAY LIBRARY FILE PROCESSING REQUIRED
                break;  // Skipping library files on Pass 1
            }

            data += blocksize; offset += blocksize;
            data += 1; offset += 1;  // Skip checksum
        }
    }
}

// SEARCH THE ENTRY POINT TABLE FOR A MATCH OF THE INDICATED SYMBOL. See LINK3\EPTSER
uint16_t* process_pass15_eptsearch(uint8_t* data, uint16_t eptsize, uint32_t symbol)
{
    for (uint16_t eptno = 0; eptno < eptsize; eptno++)
    {
        uint16_t* itemw = (uint16_t*)(data + 8 * eptno);
        if (itemw[0] == 0)
            break;  // IF 0 AT END OF TBL
        if (itemw[2] == 0)
            continue;  // SKIP MODULE NAMES IN EPT, THEY ARE NOT VALID ENTRY POINTS
        uint32_t itemnamerad50 = MAKEDWORD(itemw[0], itemw[1]);
        if (itemnamerad50 == symbol)  // MATCH
            return itemw;
    }
    return nullptr;
}

// PLACE THE ADDR OF A LIBR SYMBOL INTO THE LML. See LINK3\LMLBLD
void process_pass15_lmlbld(uint16_t blockno, uint16_t offset)
{
    if (LibraryModuleCount >= LibraryModuleListSize)
        fatal_error("ERR22: Library list overflow.\n");

    // SEARCH ALL PREVIOUS LML'S FOR A DUPLICATE
    for (int i = 0; i < LibraryModuleCount; i++)
    {
        const LibraryModuleEntry* lmlentry0 = LibraryModuleList + i;
        if (lmlentry0->libfileno == Globals.LIBNB &&
            lmlentry0->relblockno == (blockno & 077777) && lmlentry0->byteoffset == offset)
            return;  // MATCH, DON'T ADD LML TO LIST
    }

    LibraryModuleEntry* lmlentry = LibraryModuleList + LibraryModuleCount;
    lmlentry->libfileno = Globals.LIBNB;
    lmlentry->relblockno = blockno & 077777;
    lmlentry->byteoffset = offset;
    lmlentry->segmentno = 0;  // SET SEGMNT NUMBER TO ZERO=ROOT
    //TODO

    LibraryModuleCount++;
}

// ORDER LIBRARY MODULE LIST. See LINK3\ORLIB
void process_pass15_lmlorder()
{
    if (LibraryModuleCount < 2)
        return;  // NOTHING TO ORDER.  RETURN

    for (int k = 1; k < LibraryModuleCount; k++)
    {
        LibraryModuleEntry* ek = LibraryModuleList + k;
        for (int j = 0; j < k; j++)
        {
            LibraryModuleEntry* ej = LibraryModuleList + j;
            if (ek->libfileno < ej->libfileno ||
                ek->libfileno == ej->libfileno && (ek->offset() < ej->offset()))
            {
                LibraryModuleEntry etemp = *ek;
                memmove(ej + 1, ej, sizeof(LibraryModuleEntry) * (k - j));
                *ej = etemp;
            }
        }
    }
}

// READ MODULES FROM THE LIBRARY. See LINK3\LIBPRO
void process_pass15_libpro(const SaveStatusEntry* sscur)
{
    assert(sscur != nullptr);
    assert(sscur->data != nullptr);

    Globals.DUPMOD = 0;  // ASSUME THIS IS NOT A DUP MOD

    //printf("      process_pass15_libpro() for library #%d %s\n", (int)Globals.LIBNB, sscur->filename);
    size_t offset = 0;
    for (int i = 0; i < LibraryModuleCount; i++)
    {
        const LibraryModuleEntry* lmlentry = LibraryModuleList + i;
        if (lmlentry->libfileno != Globals.LIBNB)
            continue;  // not this library file
        if (lmlentry->offset() == offset)
            continue;  // same offset
        offset = lmlentry->offset();
        printf("      Module #%d offset %06o\n", i, offset);
        while (offset < sscur->filesize)
        {
            uint8_t* data = sscur->data + offset;
            uint16_t* dataw = (uint16_t*)(data);
            uint16_t blocksize = ((uint16_t*)data)[1];
            uint16_t blocktype = ((uint16_t*)data)[2];

            if (blocktype == 0 || blocktype > 8)
                fatal_error("Illegal record type at %06ho in %s\n", offset, sscur->filename);
            else if (blocktype == 1)  // 1 - START GSD RECORD, see LINK3\GSD
            {
                printf("    Block type 1 - GSD at %06ho size %06ho\n", (uint16_t)offset, blocksize);
                process_pass1_gsd_block(sscur, data);
            }
            else if (blocktype == 6)  // 6 - MODULE END, see LINK3\MODND
            {
                //printf("    Block type 6 - ENDMOD at %06ho size %06ho\n", (uint16_t)offset, blocksize);
                break;
            }

            data += blocksize; offset += blocksize;
            data += 1; offset += 1;  // Skip checksum
        }
    }
}

void process_pass15_library(const SaveStatusEntry* sscur)
{
    size_t offset = 0;
    while (offset < sscur->filesize)
    {
        uint8_t* data = sscur->data + offset;
        uint16_t* dataw = (uint16_t*)(data);
        if (*dataw != 1)
        {
            if (*dataw == 0)  // Possibly that is filler at the end of the block
            {
                size_t offsetnext = (offset + 511) & ~511;
                while (*data == 0 && offset < sscur->filesize && offset < offsetnext)
                {
                    data++; offset++;
                }
                if (offset == sscur->filesize)
                    break;  // End of file
            }
            dataw = (uint16_t*)(data);
            if (*dataw != 1)
                fatal_error("Unexpected word %06ho at %06ho in %s\n", *dataw, offset, sscur->filename);
        }

        uint16_t blocksize = ((uint16_t*)data)[1];
        uint16_t blocktype = ((uint16_t*)data)[2];

        if (blocktype < 0 || blocktype > 8)
            fatal_error("Illegal record type %03ho at %06ho in %s\n", blocktype, offset, sscur->filename);
        if (blocktype == 7)  // See LINK3\LIBRA, WE ARE ON PASS 1.5 , SO PROCESS LIBRARIES
        {
            printf("    Block type 7 - TITLIB at %06ho size %06ho\n", (uint16_t)offset, blocksize);
            uint16_t eptsize = *(uint16_t*)(data + L_HEAB);
            printf("      EPT size %06ho bytes, %d. records\n", eptsize, (int)(eptsize / 8));

            //Globals.SVLML = Globals.STLML; // SAVE START OF ALL LML'S FOR THIS LIB
            Globals.FLGWD |= LB_OBJ; // IND LIB FILE TO LINKER
            //TODO: R4 -> 1ST WD OF BUFR & C=0
            if (Globals.SWITCH & SW_I) // ANY /I MODULES?
                Globals.FLGWD |= FG_IP; // SET FLAG INDICATING /I PASS FIRST
            if (Globals.SW_LML) // IS SW.LML SET?
                Globals.SW_LML |= 0100000; // MAKE SURE BIT IS SET, /I TURNS IT OFF
            //Globals.ESZRBA = *(uint16_t*)(data + L_HEAB); // SIZE OF EPT IN BYTES
            Globals.SEGBAS = 0; // SEGBAS->TEMP FOR /X LIB FLAG
            //Globals.ESZRBA = Globals.ESZRBA >> 1; // NOW WORDS
            if (*(uint16_t*)(data + L_HX)) // IS /X SWITCH SET IN LIB. HEADER?
            {
                Globals.SW_LML &= ~0100000; // NO PREPROCESSING ON /X LIBRARIES
                Globals.SEGBAS++; // /X LIBRARY ->SEGBAS=1
                //TODO: WILL  /X EPT FIT IN BUFFER?
            }
            //data += L_HEPT; offset += L_HEPT;  // Move to 1ST EPT ENTRY

            // Resolve undefined symbols using EPT
            if (is_any_undefined())  // IF NO UNDEFS, THEN END LIBRARY
            {
                int index = Globals.UNDLST;  // GET A WEAK SYMBOL FROM THE UNDEFINED SYMBOL TABLE LIST
                while (index > 0)
                {
                    SymbolTableEntry* entry = SymbolTable + index;
                    if ((entry->status & SY_WK) == 0)  // IS THIS A WEAK SYMBOL? SKIP WEAK SYMBOL
                    {
                        //TODO: IS SYMBOL A DUP SYMBOL?
                        uint16_t* itemw = process_pass15_eptsearch(data + L_HEPT, eptsize, entry->name);
                        if (itemw == nullptr)  // CONTINUE THRU UNDEF LIST
                        {
                            index = entry->nextindex();
                            continue;
                        }
                        // THE UNDEFINED SYMBOL HAS BEEN FOUND IN THE CURRENT ENTRY POINT TABLE.
                        uint16_t eptblock = itemw[2];
                        uint16_t eptoffset = itemw[3] & 0777;
                        printf("        Found EPT for '%s' block %06ho offset %06ho\n", entry->unrad50name(), eptblock, eptoffset);
                        // CALL LMLBLD - PLACE MOD ADR IN LML
                        process_pass15_lmlbld(eptblock, eptoffset);
                        //TODO: IS THIS A /I SYMBOL
                        //TODO: Find module name
                    }
                    index = entry->nextindex();
                }
            }

            data += eptsize; offset += eptsize;
        }
        else if (blocktype == 8)  // LINK3\ENDLIB
        {
            printf("    Block type 10 - ENDLIB at %06ho size %06ho\n", (uint16_t)offset, blocksize);

            process_pass15_lmlorder();  // ORDER THIS LIBRARY LML

            Globals.SW_LML &= ~0100000;  // RESET FOR FINAL OBJ. PROCESSING
            Globals.FLGWD &= ~AD_LML;  // CLEAR NEW UNDF FLAG

            // READ MODULES FROM THE LIBRARY
            process_pass15_libpro(sscur);
        }

        data += blocksize; offset += blocksize;
        data += 1; offset += 1;  // Skip checksum
    }
}

// PASS 1.5 SCANS ONLY LIBRARIES
void process_pass15()
{
    printf("PASS 1.5\n");
    Globals.PAS1_5 = 0200;
    Globals.LIBNB = 0;
    for (int i = 0; i < SaveStatusCount; i++)
    {
        SaveStatusEntry* sscur = SaveStatusArea + i;
        assert(sscur->data != nullptr);

        // Skipping non-library files on Pass 1.5
        if (!sscur->islibrary)  // SKIP ALL NON-LIBRARY FILES ON LIBRARY PASS
            continue;

        Globals.LIBNB++;  // BUMP LIBRARY FILE # FOR LML

        for (int j = 1; ; j++)
        {
            Globals.FLGWD &= ~AD_LML;  // CLEAR NEW UNDF FLAG

            printf("  Processing %s (%d)\n", sscur->filename, j);
            process_pass15_library(sscur);

            //if ((Globals.FLGWD & AD_LML) == 0)  // NEW UNDEF'S ADDED WHILE PROCESSING LIBR ?
            break;
        }
    }
}

// END PASS ONE PROCESSING; see LINK4\ENDP1
void process_pass1_endp1()
{
    //TODO

    if (ASECTentry == nullptr)  //HACK: add default ABS section if we still don't have one
    {
        // Enter ABS section into the symbol table
        uint16_t lkwd = (uint16_t)SY_SEC;
        uint16_t lkmsk = (uint16_t)~SY_SEC;
        int index;
        symbol_table_dlooke(RAD50_ABS, lkwd, lkmsk, &index);
        ASECTentry = SymbolTable + index;
        ASECTentry->flagseg |= (0100/*CS$GBL*/ | 04/*CS$ALO*/) << 8;
    }
}

// STB subroutine, see LINK5\FBNEW
void process_pass_map_fbnew(uint16_t blocktype)
{
    if (stbfileobj == nullptr)  // IS THERE AN STB FILE?
        return;

    uint16_t* txtblkwp = (uint16_t*)(Globals.TXTBLK);

    if (Globals.TXTLEN > 0)  // Save the previous block
    {
        txtblkwp[1] = Globals.TXTLEN;

        uint8_t chksum = 0;
        for (int i = 0; i < Globals.TXTLEN; i++)  // Calculate checksum
            chksum += Globals.TXTBLK[i];
        Globals.TXTBLK[Globals.TXTLEN] = (256 - chksum) & 0xff;

        fwrite(Globals.TXTBLK, Globals.TXTLEN + 1, 1, stbfileobj);
    }

    memset(Globals.TXTBLK, 0, sizeof(Globals.TXTBLK));
    txtblkwp[0] = 1;
    txtblkwp[1] = 0;
    txtblkwp[2] = blocktype;
    Globals.TXTLEN = 6;
}

// Map processing: see LINK4/MAP
void process_pass_map_init()
{
    // START PROCESSING FOR MAP, AND Q,U,V,Y SWITCHES

    Globals.SEGBAS = 0; // N VALUE (V:N:M) FOR /V REGION FLAG
    Globals.VIRSIZ = 0; // SIZE OF THE LARGEST PARTITION IN /V REGION
    Globals.HIPHYS = 0; // PARTITION EXTENDED ADDR HIGH LIMIT
    Globals.SEGBLK = 0; // BASE OF PREVIOUS XM PARTITION

    // Prepare SAV file name
    if (*savfilename == 0)
    {
        memcpy(savfilename, SaveStatusArea[0].filename, 64);
        char* pext = strrchr(savfilename, '.');
        pext++; *pext++ = 'S'; *pext++ = 'A'; *pext++ = 'V';
    }

    if (Globals.FLGWD & FG_STB) // IS THERE AN STB FILE?
    {
        // Prepare STB file name
        char stbfilename[64];
        memcpy(stbfilename, savfilename, 64);
        char* pext = strrchr(stbfilename, '.');
        pext++; *pext++ = 'S'; *pext++ = 'T'; *pext++ = 'B';

        // Open STB file
        assert(stbfileobj == nullptr);
        stbfileobj = fopen(stbfilename, "wb");
        if (stbfileobj == nullptr)
            fatal_error("ERR5: Failed to open %s file, errno %d.\n", stbfilename, errno);
        // Prepare STB buffer
        memset(Globals.TXTBLK, 0, sizeof(Globals.TXTBLK));
        Globals.TXTLEN = 0;
        process_pass_map_fbnew(1/*GSD*/);
        // First GSD entry is the module name
        uint16_t* txtblkwp = (uint16_t*)(Globals.TXTBLK + Globals.TXTLEN);
        txtblkwp[0] = LOWORD(Globals.MODNAM);
        txtblkwp[1] = HIWORD(Globals.MODNAM);
        txtblkwp[2] = 0; // MODULE NAME
        txtblkwp[3] = 0;
        Globals.TXTLEN += 8;
        if (Globals.IDENT != 0)
        {
            txtblkwp = (uint16_t*)(Globals.TXTBLK + Globals.TXTLEN);
            txtblkwp[0] = LOWORD(Globals.IDENT);
            txtblkwp[1] = HIWORD(Globals.IDENT);
            txtblkwp[2] = 6 << 8; // IDENT
            txtblkwp[3] = 0;
            Globals.TXTLEN += 8;
        }
    }

    uint16_t& bottom = (Globals.SWIT1 & SW_J) ? Globals.DBOTTM : Globals.BOTTOM;
    if (ASECTentry->value > bottom)  // IS "BOTTOM" .GE. SIZE OF ASECT ?
    {
        //TODO: IF /R  GIVE ERROR
        bottom = ASECTentry->value;  // USE SIZE OF ASECT AS BOTTOM ADDRESS
    }
    //TODO: IS PROGRAM OVERLAID?

    //TODO: PROCESS /H SWITCH

    //TODO: PROCESS /U & /Q SWITCHES

    //TODO: /V AND /R WITH NO /Y HIGH LIMIT ADJUSTMENT

    //TODO: /Q SWITCH PROCESSING

    //TODO: ROUND ALL XM SEGMENTS TO 32. WORDS
}

void process_pass_map_done()
{
    if (mapfileobj != nullptr)
    {
        fclose(mapfileobj);  mapfileobj = nullptr;
    }
    if (stbfileobj != nullptr)
    {
        fclose(stbfileobj);  stbfileobj = nullptr;
    }
}

// STB subroutine, see LINK5\FBGEG
void process_pass_map_fbseg(const SymbolTableEntry * entry)
{
    assert(entry != nullptr);

    if (stbfileobj == nullptr)  // IS THERE AN STB FILE?
        return;

    if (Globals.TXTLEN > 40)
        process_pass_map_fbnew(1/*GSD*/);

    uint16_t* txtblkwp = (uint16_t*)(Globals.TXTBLK + Globals.TXTLEN);
    txtblkwp[0] = LOWORD(entry->name);  // PUT NAME INTO GSD BLOCK
    txtblkwp[1] = HIWORD(entry->name);
    txtblkwp[2] = 0400; // .ASECT (CONTROL SECTION NAME)
    txtblkwp[3] = entry->value;
    Globals.TXTLEN += 8;
}

// STB subroutine, see LINK5\FBGSD
void process_pass_map_fbgsd(const SymbolTableEntry * entry)
{
    assert(entry != nullptr);

    if (stbfileobj == nullptr)  // IS THERE AN STB FILE?
        return;

    if (Globals.TXTLEN > 40)
        process_pass_map_fbnew(1/*GSD*/);

    uint16_t* txtblkwp = (uint16_t*)(Globals.TXTBLK + Globals.TXTLEN);
    txtblkwp[0] = LOWORD(entry->name);  // PUT NAME INTO GSD BLOCK
    txtblkwp[1] = HIWORD(entry->name);
    txtblkwp[2] = 4 * 0400 + 0110; // GSD DEFINITION OF ABSOLUTE SYMBOL
    txtblkwp[3] = entry->value;
    Globals.TXTLEN += 8;
}

// STB subroutine, see LINK5\ENDSTB
void process_pass_map_endstb()
{
    process_pass_map_fbnew(2/*ENDGSD*/);  // CLOSE FB BLOCK AND START A NEW ONE WHICH IS AN ENDGSD BLOCK

    process_pass_map_fbnew(6/*ENDMOD*/);  // CLOSE THAT, START ANOTHER WHICH IS AN END MODULE BLOCK

    process_pass_map_fbnew(0);  // CLOSE THAT

    if (stbfileobj == nullptr)  // IS THERE AN STB FILE?
        return;

    // Write zeroes till the end of 512-byte file block
    long stblen = ftell(stbfileobj);
    long stbalign = 512 - (stblen % 512);
    if (stbalign > 0 && stbalign != 512)
    {
        memset(Globals.TXTBLK, 0, sizeof(Globals.TXTBLK));
        while (stbalign >= sizeof(Globals.TXTBLK))
        {
            fwrite(Globals.TXTBLK, sizeof(Globals.TXTBLK), 1, stbfileobj);
            stbalign -= sizeof(Globals.TXTBLK);
        }
        if (stbalign > 0)
            fwrite(Globals.TXTBLK, stbalign, 1, stbfileobj);
    }

    // Close STB file
    fclose(stbfileobj);  stbfileobj = nullptr;
}

// PRINT UNDEFINED GLOBALS IF ANY, see LINK5\DOUDFS
void print_undefined_globals()
{
    process_pass_map_endstb();  // CLOSE OUT THE STB FILE

    int index = Globals.UNDLST;
    if (index == 0)
        return;
    printf("  Undefined globals:\n  ");
    fprintf(mapfileobj, "\nUndefined globals:\n");
    int count = 0;
    while (index != 0)
    {
        if (count > 0 && count % 8 == 0) printf("\n  ");
        SymbolTableEntry* entry = SymbolTable + index;
        printf(" '%s'", entry->unrad50name());
        fprintf(mapfileobj, "%s\n", entry->unrad50name());
        index = entry->nextindex();
        count++;
    }
    printf("\n");
}

void process_pass_map_output_sectionline(const SymbolTableEntry* entry, uint16_t baseaddr, uint16_t sectsize)
{
    assert(entry != nullptr);

    // OUTPUT SECTION NAME, BASE ADR, SIZE & ATTRIBUTES
    uint8_t entryflags = entry->flags();
    char bufsize[20];
    sprintf(bufsize, "%06ho = %d.", sectsize, (sectsize + 1) / 2);
    const char* sectaccess = (entryflags & 0020) ? "RO" : "RW";
    const char* secttypedi = (entryflags & 0200) ? "D" : "I";
    const char* sectscope = (entryflags & 0100) ? "GBL" : "LCL";
    const char* sectsav = ((entryflags & 0100) && (entryflags & 010000)) ? ",SAV" : "";
    const char* sectreloc = (entryflags & 0040) ? "REL" : "ABS";
    const char* sectalloc = (entryflags & 0004) ? "OVR" : "CON";
    fprintf(mapfileobj, " %s\t %06ho\t%-16s words  (%s,%s,%s%s,%s,%s)\n",
            (entry->name >= 03100) ? entry->unrad50name() : "",
            baseaddr, bufsize, sectaccess, secttypedi, sectscope, sectsav, sectreloc, sectalloc);
    printf("  '%s' %06ho %-16s words  (%s,%s,%s%s,%s,%s)\n",
           (entry->name >= 03100) ? entry->unrad50name() : "      ",
           baseaddr, bufsize, sectaccess, secttypedi, sectscope, sectsav, sectreloc, sectalloc);
}

// Map output: see LINK5\MAPHDR
void process_pass_map_output()
{
    printf("PASS MAP\n");

    // Prepare MAP file name
    char mapfilename[64];
    memcpy(mapfilename, savfilename, 64);
    char* pext = strrchr(mapfilename, '.');
    pext++; *pext++ = 'M'; *pext++ = 'A'; *pext++ = 'P';

    // Open MAP file
    assert(mapfileobj == nullptr);
    mapfileobj = fopen(mapfilename, "wt");
    if (mapfileobj == nullptr)
        fatal_error("ERR5: Failed to open file %s, errno %d.\n", mapfilename, errno);

    Globals.LINLFT = LINPPG;

    // OUTPUT THE HEADERS

    fprintf(mapfileobj, "PCLINK11 %-8s", APP_VERSION_STRING);
    fprintf(mapfileobj, "\tLoad Map \t");

    time_t curtime;  time(&curtime); // DETERMINE DATE & TIME; see LINK5\MAPHDR
    struct tm * timeptr = localtime(&curtime);
    fprintf(mapfileobj, "%s %.2d-%s-%d %.2d:%.2d\n",
            weekday_names[timeptr->tm_wday],
            timeptr->tm_mday, month_names[timeptr->tm_mon], 1900 + timeptr->tm_year,
            timeptr->tm_hour, timeptr->tm_min);

    char savname[64];
    strcpy(savname, savfilename);
    char* pdot = strrchr(savname, '.');
    if (pdot != nullptr) *pdot = 0;
    fprintf(mapfileobj, "%-6s", savname);
    if (pdot != nullptr)
    {
        *pdot = '.';
        fprintf(mapfileobj, "%-4s   ", pdot);
    }

    fprintf(mapfileobj, "\tTitle:\t");
    if (Globals.MODNAM != 0)
    {
        fprintf(mapfileobj, "%s", unrad50(Globals.MODNAM));
    }
    fprintf(mapfileobj, "\tIdent:\t");
    fprintf(mapfileobj, "%s", unrad50(Globals.IDENT));
    fprintf(mapfileobj, "\t\n\n");
    fprintf(mapfileobj, "Section  Addr\tSize");
    //printf("  Section  Addr   Size ");
    for (uint8_t i = 0; i < Globals.NUMCOL; i++)
    {
        fprintf(mapfileobj, "\tGlobal\tValue");
        printf("   Global  Value");
    }
    fprintf(mapfileobj, "\n\n");
    printf("\n");

    // RESOLV  SECTION STARTS & GLOBAL SYMBOL VALUES; see LINK5\RESOLV
    uint16_t baseaddr = 0; // ASECT BASE ADDRESS IS 0
    SymbolTableEntry* entry = ASECTentry;
    uint16_t sectsize = (entry != nullptr && entry->value > 0) ?
            ((entry->value > Globals.DBOTTM) ? entry->value : Globals.DBOTTM) : Globals.DBOTTM;
    int tabcount = 0;
    while (entry != nullptr)
    {
        if (entry->flagseg & SY_SEC)  // SECTION
        {
            if (tabcount > 0)
            {
                fprintf(mapfileobj, "\n");
                printf("\n");
                tabcount = 0;
            }

            entry->value = baseaddr;

            // IS THIS BLANK SECTION 0-LENGTH?
            bool skipsect = ((entry->name >= 03100) == 0 && sectsize == 0);
            if (!skipsect)
            {
                process_pass_map_output_sectionline(entry, baseaddr, sectsize);

                if ((entry->flags() & 0040) == 0)  // For ABS section only
                    process_pass_map_fbseg(entry);  // PUT NEW SEGMENT INTO STB FILE
            }
        }
        else  // OUTPUT SYMBOL NAME & VALUE, see LINK5\OUTSYM
        {
            //if ((entry->flags() & 0040/*CS$REL*/) != 0)  // IF ABS SYMBOL THEN DON'T ADD BASE
            //TODO
            entry->value += baseaddr;

            if (tabcount == 0)
            {
                fprintf(mapfileobj, "\t\t\t");
                printf("                        ");
            }
            fprintf(mapfileobj, "%s\t%06ho\t", entry->unrad50name(), entry->value);
            printf("  %s  %06ho", entry->unrad50name(), entry->value);
            tabcount++;
            if (tabcount >= Globals.NUMCOL)
            {
                fprintf(mapfileobj, "\n");
                printf("\n");
                tabcount = 0;
            }

            process_pass_map_fbgsd(entry);  // GENERATE AN STB ENTRY
        }
        // Next entry index should be in status field
        if (entry->nextindex() == 0)
        {
            entry = nullptr;
            if (tabcount > 0)
            {
                fprintf(mapfileobj, "\n");
                printf("\n");
            }
            break;
        }
        entry = SymbolTable + entry->nextindex();  // next entry

        if (entry->flagseg & SY_SEC)
        {
            //TODO: Check NEW SECTION?
            //TODO: Calculate new baseaddr and sectsize, see LINK5\RES1
            baseaddr += (sectsize + 1) & ~1;
            sectsize = entry->value;
        }
    }
    //TODO: see LINK5\POSTN\10$
    uint16_t totalsize = baseaddr + sectsize;
    uint16_t blockcount = (totalsize + 511) / 512;
    printf("  Total size %06ho = %u. bytes, %u. blocks\n", totalsize, totalsize, blockcount);
    OutputBlockCount = blockcount;  //HACK for now

    print_undefined_globals();

    // OUTPUT TRANSFER ADR & CHECK ITS VALIDITY, see LINK5\DOTADR
    uint16_t lkmsk = (uint16_t) ~(SY_SEC + SY_SEG);  // LOOK AT SECTION & SEGMENT # BITS
    uint16_t segnum = 0;  // MUST BE IN ROOT SEGMENT
    uint16_t lkwd = SY_SEC;  // ASSUME SECTION NAME LOOKUP
    if (Globals.BEGBLK.code == 4)  // IS SYMBOL A GLOBAL?
        lkwd = 0;  // GLOBAL SYM IN SEGMENT 0
    int index;
    bool found = symbol_table_lookup(Globals.BEGBLK.symbol, lkwd, lkmsk, &index);
    if (!found)
        fatal_error("ERR31: Transfer address undefined or in overlay.\n");
    SymbolTableEntry* entrybeg = SymbolTable + index;
    //TODO: Calculate transfer address
    Globals.BEGBLK.value += entrybeg->value;
    uint16_t taddr = Globals.BEGBLK.value;
    //TODO: Calculate high limit
    uint16_t highlim = totalsize - 2;
    Globals.HGHLIM = totalsize;

    char bufhlim[20];
    sprintf(bufhlim, "%06ho = %d.", highlim, highlim / 2);
    fprintf(mapfileobj, "\nTransfer address = %06ho, High limit = %-16s words\n", taddr, bufhlim);
    printf("  Transfer address = %06ho, High limit = %-16s words\n", taddr, bufhlim);
}

SymbolTableEntry* process_pass2_rld_lookup(const uint8_t* data, bool global)
{
    assert(data != nullptr);

    uint32_t lkname = *((uint32_t*)data);
    uint16_t lkwd = global ? 0 : (SY_SEC | Globals.SEGNUM);
    uint16_t lkmsk = global ? ~SY_SEC : ~(SY_SEC | SY_SEG);

    int index;
    bool found = symbol_table_lookup(lkname, lkwd, lkmsk, &index);
    if (!found && !global)
    {
        lkwd = SY_SEC; // LOOKUP SECTION NAME IN ROOT
        found = symbol_table_lookup(lkname, lkwd, lkmsk, &index);
    }
    if (!found) // MUST FIND IT THIS TIME
        fatal_error("ERR46: Invalid RLD symbol '%s'.\n", unrad50(lkname));

    SymbolTableEntry* entry = SymbolTable + index;
    //uint16_t R3 = entry->value; // GET SYMBOL'S VALUE
    //TODO
    return entry;
}

// COMPLEX RELOCATION STRING PROCESSING (GLOBAL ARITHMETIC), see LINK7\RLDCPX
uint16_t process_pass2_rld_complex(const SaveStatusEntry* sscur, const uint8_t* &data, uint16_t &offset, uint16_t blocksize)
{
    assert(sscur != nullptr);
    assert(data != nullptr);

    bool cpxbreak = false;
    uint16_t cpxresult = 0;
    const int cpxstacksize = 16;
    uint16_t cpxstack[cpxstacksize];  memset(cpxstack, 0, sizeof(cpxstack));
    uint16_t cpxstacktop = 0;
    while (!cpxbreak && offset < blocksize)
    {
        uint8_t cpxcmd = *data;  data += 1;  offset += 1;
        SymbolTableEntry* cpxentry = nullptr;
        ModuleSectionEntry* mstentry = nullptr;
        uint8_t cpxsect;
        uint16_t cpxval;
        uint32_t cpxname;
        printf("        Complex cmd %03ho %s", (uint16_t)cpxcmd, (cpxcmd > 020) ? "UNKNOWN" : CPXCommandNames[cpxcmd]);
        switch (cpxcmd)
        {
        case 000:  // NOP
            printf("\n");
            break;
        case 001:  // ADD -- ADD TOP 2 ITEMS
            printf("\n");
            if (cpxstacktop > 0)
            {
                cpxstacktop--;
                cpxstack[cpxstacktop] = cpxstack[cpxstacktop] + cpxstack[cpxstacktop + 1];
            }
            break;
        case 002:  // SUBTRACT -- NEGATE TOP ITEM ON STACK
            printf("\n");
            if (cpxstacktop > 0)
            {
                cpxstacktop--;
                cpxstack[cpxstacktop] = cpxstack[cpxstacktop] - cpxstack[cpxstacktop + 1];
            }
            break;
        case 003:  // MULTIPLY
            printf("\n");
            if (cpxstacktop > 0)
            {
                cpxstacktop--;
                cpxstack[cpxstacktop] = cpxstack[cpxstacktop] * cpxstack[cpxstacktop + 1];
            }
            break;
        case 004:  // DIVIDE, see LINK7\CPXDIV
            printf("\n");
            if (cpxstacktop > 0)
            {
                if (cpxstack[cpxstacktop] == 0)
                    fatal_error("ERR33: Complex relocation divide by zero in %s\n", sscur->filename);
                cpxstacktop--;
                cpxstack[cpxstacktop] = cpxstack[cpxstacktop] / cpxstack[cpxstacktop + 1];
            }
            break;
        case 005:  // AND
            printf("\n");
            if (cpxstacktop > 0)
            {
                cpxstacktop--;
                cpxstack[cpxstacktop] = cpxstack[cpxstacktop] & cpxstack[cpxstacktop + 1];
            }
            break;
        case 006:  // OR
            printf("\n");
            if (cpxstacktop > 0)
            {
                cpxstacktop--;
                cpxstack[cpxstacktop] = cpxstack[cpxstacktop] | cpxstack[cpxstacktop + 1];
            }
            break;
        case 007:  // XOR
            printf("\n");
            if (cpxstacktop > 0)
            {
                cpxstacktop--;
                cpxstack[cpxstacktop] = cpxstack[cpxstacktop] ^ cpxstack[cpxstacktop + 1];
            }
            break;
        case 010:  // NEGATE TOP ITEM
            printf("\n");
            cpxstack[cpxstacktop] = 0 - cpxstack[cpxstacktop];
            break;
        case 011:  // COMPLEMENT -- COMPLEMENT TOP ITEM
            printf("\n");
            cpxstack[cpxstacktop] = ~cpxstack[cpxstacktop];
            break;
        case 012:  // STORE NOT DISPLACED
            cpxresult = cpxstack[cpxstacktop];
            printf(" %06ho\n", cpxresult);
            cpxbreak = true;
            break;
        case 013:  // STORE DISPLACED
            cpxresult = cpxstack[cpxstacktop];  //TODO
            printf(" %06ho\n", cpxresult);
            cpxbreak = true;
            break;
        case 014:  // ILLEGAL FORMAT
        case 015:  // ILLEGAL FORMAT
            printf("\n");
            fatal_error("ERR35: Invalid complex relocation in %s\n", sscur->filename);
            break;
        case 016:  // PUSH GLOBAL SYMBOL VALUE
            cpxname = MAKEDWORD(((uint16_t*)data)[0], ((uint16_t*)data)[1]);
            printf(" '%s'\n", unrad50(cpxname));
            cpxentry = process_pass2_rld_lookup(data, true);
            data += 4;  offset += 4;
            cpxstacktop++;
            if (cpxstacktop >= cpxstacksize)
                fatal_error("ERR35: Complex relocation stack overflow in %s\n", sscur->filename);
            cpxstack[cpxstacktop] = cpxentry->value;
            break;
        case 017:  // PUSH RELOCATABLE VALUE, see LINK7\CPXPRL
            // GET SECTION NUMBER
            cpxsect = *data;  data += 1;  offset += 1;
            cpxval = *((uint16_t*)data);  data += 2;  offset += 2;  // GET OFFSET WITHIN SECTION
            printf(" %03ho %06ho\n", (uint16_t)cpxsect, cpxval);
            // SEE IF SECTION NO. OK
            if (cpxsect >= ModuleSectionCount)
                fatal_error("ERR35: Invalid complex relocation section number in %s\n", sscur->filename);
            mstentry = ModuleSectionTable + (size_t)cpxsect;
            cpxentry = SymbolTable + mstentry->stindex;
            //print_mst_table();//DEBUG
            cpxstacktop++;
            if (cpxstacktop >= cpxstacksize)
                fatal_error("ERR35: Complex relocation stack overflow in %s\n", sscur->filename);
            cpxstack[cpxstacktop] = cpxval + cpxentry->value;
            break;
        case 020:  // PUSH CONSTANT
            cpxval = *((uint16_t*)data);  data += 2;  offset += 2;
            printf(" %06ho\n", cpxval);
            cpxstacktop++;
            if (cpxstacktop >= cpxstacksize)
                fatal_error("ERR35: Complex relocation stack overflow in %s\n", sscur->filename);
            cpxstack[cpxstacktop] = cpxval;
            break;
        default:
            printf("\n");
            fatal_error("ERR36: Unknown complex relocation command %03ho in %s\n", (uint16_t)cpxcmd, sscur->filename);
        }
    }
    return cpxresult;
}

void process_pass2_rld(const SaveStatusEntry* sscur, const uint8_t* data)
{
    assert(sscur != nullptr);
    assert(data != nullptr);

    uint16_t blocksize = ((uint16_t*)data)[1];
    uint16_t offset = 6;  data += 6;
    uint16_t baseaddr = *((uint16_t*)Globals.TXTBLK);
    while (offset < blocksize)
    {
        uint8_t command = *data;  data++;  offset++;  // CMD BYTE OF RLD
        uint8_t disbyte = *data;  data++;  offset++;  // DISPLACEMENT BYTE
        uint8_t* dest = Globals.TXTBLK + (disbyte - 2);
        uint16_t addr = baseaddr + (disbyte - 2) - 2;

        printf("      %06ho RLD type %03ho %s",
               addr, (uint16_t)(command & 0177), ((command & 0177) > 017) ? "UNKNOWN" : RLDCommandNames[command & 0177]);
        uint16_t constdata;
        switch (command & 0177)
        {
        case 001:  // INTERNAL REL, see LINK7\RLDIR
            // DIRECT POINTER TO AN ADDRESS WITHIN A MODULE. THE CURRENT SECTION BASE ADDRESS IS ADDED
            // TO A SPECIFIED CONSTANT ANT THE RESULT STORED IN THE IMAGE FILE AT THE CALCULATED ADDRESS
            // (I.E., DISPLACEMENT BYTE ADDED TO VALUE CALCULATED FROM THE LOAD ADDRESS OF THE PREVIOUS TEXT BLOCK).
            constdata = *((uint16_t*)data);
            printf(" %06ho\n", constdata);
            *((uint16_t*)dest) = constdata + Globals.BASE;
            data += 2;  offset += 2;
            break;
        case 002:  // GLOBAL
        case 012:  // PSECT
            // RELOCATES A DIRECT POINTER TO A GLOBAL SYMBOL. THE VALUE OF THE GLOBAL SYMBOL IS OBTAINED & STORED.
            printf(" '%s'\n", unrad50(*((uint32_t*)data)));
            {
                SymbolTableEntry* entry = process_pass2_rld_lookup(data, (command & 010) == 0);
                //printf("        Entry '%s' value = %06ho %04X dest = %06ho\n", entry->unrad50name(), entry->value, entry->value, *((uint16_t*)dest));
                *((uint16_t*)dest) = entry->value;
            }
            data += 4;  offset += 4;
            break;
        case 003:  // INTERNAL DISPLACED REL, see LINK7\RLDIDR
            // RELATIVE REFERENCE TO AN ABSOLUTE ADDRESS FROM WITHIN A RELOCATABLE SECTION.
            // THE ADDRESS + 2 THAT THE RELOCATED VALUE IS TO BE WRITTEN INTO IS SUBTRACTED FROM THE SPECIFIED CONSTANT & RESULTS STORED.
            constdata = *((uint16_t*)data);
            printf(" %06ho\n", constdata);
            *((uint16_t*)dest) = constdata - (addr + 2);
            data += 2;  offset += 2;
            break;
        case 004:  // GLOBAL DISPLACED, see LINK7\RLDGDR
        case 014:  // PSECT DISPLACED
            printf(" '%s'\n", unrad50(*((uint32_t*)data)));
            {
                SymbolTableEntry* entry = process_pass2_rld_lookup(data, (command & 010) == 0);
                //printf("        Entry '%s' value = %06ho %04X dest = %06ho\n", entry->unrad50name(), entry->value, entry->value, *((uint16_t*)dest));
                *((uint16_t*)dest) = entry->value - addr - 2;
            }
            data += 4;  offset += 4;
            break;
        case 005:  // GLOBAL ADDITIVE REL
        case 015:  // PSECT ADDITIVE
            // RELOCATED A DIRECT POINTER TO A GLOBAL SYMBOL WITH AN ADDITIVE CONSTANT
            // THE SYMBOL VALUE IS ADDED TO THE SPECIFIED CONSTANT & STORED.
            constdata = ((uint16_t*)data)[2];
            printf(" '%s' %06ho\n", unrad50(*((uint32_t*)data)), constdata);
            {
                SymbolTableEntry* entry = process_pass2_rld_lookup(data, (command & 010) == 0);
                *((uint16_t*)dest) = entry->value + constdata;
            }
            data += 6;  offset += 6;
            break;
        case 006:  // GLOBAL ADDITIVE DISPLACED
        case 016:  // PSECT ADDITIVE DISPLACED
            // RELATIVE REFERENCE TO A GLOBAL SYMBOL WITH AN ADDITIVE CONSTANT.
            // THE GLOBAL VALUE AND THE CONSTANT ARE ADDED. THE ADDRESS + 2 THAT THE RELOCATED VALUE IS
            // TO BE WRITTEN INTO IS SUBTRACTED FROM THE RESULTANT ADDITIVE VALUE & STORED.
            constdata = ((uint16_t*)data)[2];
            printf(" '%s' %06ho\n", unrad50(*((uint32_t*)data)), constdata);
            {
                SymbolTableEntry* entry = process_pass2_rld_lookup(data, (command & 010) == 0);
                *((uint16_t*)dest) = entry->value + constdata - (addr + 2);
                //TODO: IS SYMBOL IN OVERLAY BY ISOLATING THE SEGMENT #
            }
            data += 6;  offset += 6;
            break;
        case 007:  // LOCATION COUNTER DEFINITION, see LINK7\RLDLCD
            // DECLARES A CURRENT SECTION & LOCATION COUNTER VALUE
            constdata = ((uint16_t*)data)[2];
            printf(" '%s' %06ho\n", unrad50(*((uint32_t*)data)), constdata);
            {
                Globals.MBPTR = 0;  // 0 SAYS TO STORE TXT INFO
                SymbolTableEntry* entry = process_pass2_rld_lookup(data, false);
                if (entry->flags() & 0040/*SY$REL*/)  // IS SYMBOL ABSOLUTE ?
                {
                    if (entry->name != RAD50_ABS)  // ARE WE LOOKING AT THE ASECT?
                        Globals.MBPTR++;  // SAY NOT TO STORE TXT FOR ABS SECTION
                }
                Globals.BASE = entry->value;  // SET UP NEW SECTION BASE
                //TODO: ARE WE DOING I-D SPACE?
                *((uint16_t*)dest) = constdata + Globals.BASE;  // BASE OF SECTION + OFFSET = CURRENT LOCATION COUNTER
            }
            data += 6;  offset += 6;
            break;
        case 010:  // LOCATION COUNTER MODIFICATION, see LINK7\RLDLCM
            // THE CURRENT SECTION BASE IS ADDED TO THE SPECIFIED CONSTANT & RESULT IS STORED AS THE CURRENT LOCATION CTR.
            constdata = ((uint16_t*)data)[0];
            printf(" %06ho\n", constdata);
            *((uint16_t*)dest) = constdata + Globals.BASE;  // BASE OF SECTION + OFFSET = CURRENT LOCATION COUNTER
            data += 2;  offset += 2;
            break;
        case 011:  // SET PROGRAM LIMITS, see LINK7\RLDSPL
            printf("\n");
            *((uint16_t*)dest) = Globals.BOTTOM;
            *((uint16_t*)dest + 1) = Globals.HGHLIM;
            break;
        case 017:  // COMPLEX RELOCATION STRING PROCESSING (GLOBAL ARITHMETIC)
            printf("\n");
            *((uint16_t*)dest) = process_pass2_rld_complex(sscur, data, offset, blocksize);
            break;
        default:
            fatal_error("ERR36: Unknown RLD command: %d in %s\n", (int)command, sscur->filename);
        }
    }
}

// Prapare SYSCOM area, pass 2 initialization; see LINK6
void process_pass2_init()
{
    //printf("PASS 2 init\n");

    mst_table_clear();

    // Allocate space for .SAV file image
    OutputBufferSize = 65536;
    OutputBuffer = (uint8_t*) calloc(OutputBufferSize, 1);
    if (OutputBuffer == nullptr)
        fatal_error("ERR11: Failed to allocate memory for output buffer.\n");

    // See LINK6\DOCASH
    Globals.LRUNUM = 0; // INIT LEAST RECENTLY USED TIME STAMP
    Globals.BASE = 0;
    //Globals.BITBAD = Globals.LMLPTR + 2 + Globals.STLML * 4;  // START OF SAV FILE MASTER BITMAP IN BLK 0
    //TODO

    // CLEAR & SETUP SYSCOM AREA OF IMAGE FILE

    memset(Globals.BITMAP, 0, sizeof(Globals.BITMAP));
    //Globals.BITMAP = Globals.BITBAD;

    *((uint16_t*)(OutputBuffer + SysCom_BEGIN)) = Globals.BEGBLK.value; // PROG START ADDR
    //TODO: ARE WE DOING I-D SPACE?

    if (Globals.STKBLK[0] != 0)  // GET USER SUPLIED STK SYM ADR, SYMBOL SUPPLIED ?
    {
        uint32_t lkname = MAKEDWORD(Globals.STKBLK[0], Globals.STKBLK[1]);
        uint16_t lkwd = 0; // MUST BE A GLOBAL SYMBOL
        uint16_t lkmsk = 0; //TODO
        int index;
        if (!symbol_table_lookup(lkname, lkwd, lkmsk, &index))
        {
            fatal_error("ERR32: Stack address undefined or in overlay.\n");  // ERROR IF UNDEF STACK ADR
            //TODO: USE DEFALULT ADR
        }
        NOTIMPLEMENTED
    }
    else if (Globals.STKBLK[2] != 0)  // GET USER'S SUPPLIED STK ADR
    {
        *((uint16_t*)(OutputBuffer + SysCom_STACK)) = Globals.STKBLK[2];
    }
    else
    {
        uint16_t stack = (Globals.SWIT1 & SW_J) ? Globals.DBOTTM : Globals.BOTTOM;
        *((uint16_t*)(OutputBuffer + SysCom_STACK)) = stack;
    }

    uint16_t highlim = (Globals.SWIT1 & SW_J) ? Globals.DHGHLM : Globals.HGHLIM;
    *((uint16_t*)(OutputBuffer + SysCom_HIGH)) = highlim - 2;  // HIGH LIMIT
    if (Globals.SWITCH & SW_K)  // For /K switch STORE IT AT LOC. 56 IN SYSCOM AREA
        *((uint16_t*)(OutputBuffer + SysCom_HIGH + 6)) = Globals.KSWVAL;

    //TODO: SYSCOM AREA FOR REL FILE

    //TODO: BINOUT REQUESTED?

    // Open SAV file for writing
    assert(outfileobj == nullptr);
    outfileobj = fopen(savfilename, "wb");
    if (outfileobj == nullptr)
        fatal_error("ERR6: Failed to open %s file, errno %d.\n", savfilename, errno);

    // See LINK6\INITP2
    //TODO: Some code for REL
    Globals.VIRSIZ = 0;
    //TODO: ARE WE DOING I-D SPACE?
    //TODO: /V OVERLAY, OR /XM?
    Globals.SEGBLK = 0;
    //TODO: INIT FOR LIBRARY PROCESSING
    mst_table_clear();  // RESET NUMBER OF ENTRIES IN MODULE SECTION TBL
    Globals.LIBNB = 1;  // INITIAL LIBRARY FILE # FOR LML

    //TODO: FORCE BASE OF ZERO FOR VSECT IF ANY
}

void mark_bitmap_bits(uint16_t addr, uint16_t length)
{
    int block = addr / 512;
    int endblock = (addr + length - 1) / 512;
    //printf("mark_bitmap_bits for blocks %d..%d\n", block, endblock);
    for (; block <= endblock; block++)
    {
        Globals.BITMAP[block / 8] |= (uint8_t)(1 << (7 - block % 8));
    }
}

void process_pass2_dump_txtblk()  // DUMP TEXT SUBROUTINE, see LINK7\TDMP0, LINK7\DMP0
{
    if (Globals.TXTLEN == 0)
        return;

    uint16_t addr = *((uint16_t*)Globals.TXTBLK);  //NOTE: Should be absoulte address
    uint8_t* dest = OutputBuffer + addr;
    uint8_t* src = Globals.TXTBLK + 2;
    memcpy(dest, src, Globals.TXTLEN);

    mark_bitmap_bits(addr, Globals.TXTLEN);
    //TODO: if ((Globals.SWITCH & SW_X) != 0)

    if (addr < 0400 && addr + Globals.TXTLEN > 0360 && (Globals.SWITCH & SW_X) != 0)
        Globals.FLGWD |= 02000/*FG.XX*/;  // YES->SET FLAG NOT TO OUTPUT BITMAP

    Globals.TXTLEN = 0;  // MARK TXT BLOCK EMPTY, see LINK7\CLRTXL
}

void proccess_pass2_libpa2(const SaveStatusEntry* sscur)
{
    assert(sscur != nullptr);
    assert(sscur->data != nullptr);

    size_t offset = 0;
    for (int i = 0; i < LibraryModuleCount; i++)
    {
        const LibraryModuleEntry* lmlentry = LibraryModuleList + i;
        if (lmlentry->libfileno != Globals.LIBNB)
            continue;  // not this library file
        if (lmlentry->offset() == offset)
            continue;  // same offset
        offset = lmlentry->offset();
        //printf("      process_pass15_libpro() for offset %06o\n", offset);
        while (offset < sscur->filesize)
        {
            uint8_t* data = sscur->data + offset;
            uint16_t* dataw = (uint16_t*)(data);
            uint16_t blocksize = ((uint16_t*)data)[1];
            uint16_t blocktype = ((uint16_t*)data)[2];

            if (blocktype == 0 || blocktype > 8)
                fatal_error("Illegal record type at %06ho in %s\n", offset, sscur->filename);
            else if (blocktype == 3)  // See LINK7\DMPTXT
            {
                process_pass2_dump_txtblk();

                uint16_t addr = ((uint16_t*)data)[3];
                uint16_t datasize = blocksize - 8;
                printf("    Block type 3 - TXT at %06ho size %06ho addr %06ho %06ho len %06ho\n",
                       (uint16_t)offset, blocksize, addr, addr + Globals.BASE, datasize);
                Globals.TXTLEN = datasize;
                assert(datasize <= sizeof(Globals.TXTBLK));
                memcpy(Globals.TXTBLK, data + 6, blocksize - 6);

                *((uint16_t*)Globals.TXTBLK) = addr + Globals.BASE;  // ADD BASE TO GIVE ABS LOAD ADDR
            }
            else if (blocktype == 4)  // See LINK7\RLD
            {
                uint16_t base = *((uint16_t*)Globals.TXTBLK);
                printf("    Block type 4 - RLD at %06ho size %06ho base %06ho\n", (uint16_t)offset, blocksize, base);
                process_pass2_rld(sscur, data);
            }
            else if (blocktype == 6)  // MODULE END RECORD, See LINK7\MODND
            {
                //printf("    Block type 6 - ENDMOD at %06ho size %06ho\n", (uint16_t)offset, blocksize);
                process_pass2_dump_txtblk();
                break;
            }

            data += blocksize; offset += blocksize;
            data += 1; offset += 1;  // Skip checksum
        }
    }
}

// PROCESS GSD RECORD DURING PASS 2. See LINK7\GSD
void process_pass2_gsd_block(const SaveStatusEntry* sscur, const uint8_t* data)
{
    assert(sscur != nullptr);
    assert(data != nullptr);

    uint16_t blocksize = ((uint16_t*)data)[1];
    int itemcount = (blocksize - 6) / 8;
    //printf("    Processing GSD block, %d items\n", itemcount);

    for (int i = 0; i < itemcount; i++)
    {
        const uint16_t* itemw = (const uint16_t*)(data + 6 + 8 * i);

        uint32_t itemnamerad50 = MAKEDWORD(itemw[0], itemw[1]);
        int itemtype = (itemw[2] >> 8) & 0xff;
        int itemflags = (itemw[2] & 0377);

        printf("      Item '%s' type %d - %s\n", unrad50(itemnamerad50), itemtype, (itemtype > 7) ? "UNKNOWN" : GSDItemTypeNames[itemtype]);
        if (itemtype == 5)
        {
            uint32_t lkname = itemnamerad50;
            uint16_t lkmsk = (uint16_t)~SY_SEC;
            uint16_t lkwd = (uint16_t)SY_SEC;
            int index;
            bool isfound = symbol_table_lookup(lkname, lkwd, lkmsk, &index);
            if (!isfound)
                fatal_error("ERR21: Invalid GSD in %s\n", sscur->filename);

            ModuleSectionEntry* msentry = ModuleSectionTable + ModuleSectionCount;
            msentry->stindex = (uint16_t)index;
            msentry->size = 0;  // ASSUME CONTRIBUTION OF 0 FOR THIS SECTION
            ModuleSectionCount++;
            SymbolTableEntry* entry = SymbolTable + index;
            //TODO: DON'T UPDATE SECTION BASE IF OVR SECT
            uint16_t sectsize = itemw[3];
            //TODO: ROUND ALL SECTIONS EXCEPT "CON" DATA SECTIONS TO WORD BOUNDARIES
            if ((entry->flags() & 4/*CS$ALO*/) == 0 && (entry->flags() & 0200/*CS$TYP*/) == 0)
                sectsize = (sectsize + 1) & ~1;
            msentry->size = sectsize;
        }
    }
}

// PRODUCE SAVE IMAGE FILE, see LINK7\PASS2
void process_pass2()
{
    printf("PASS 2\n");
    Globals.LIBNB = 0;
    for (int i = 0; i < SaveStatusCount; i++)
    {
        SaveStatusEntry* sscur = SaveStatusArea + i;
        assert(sscur->data != nullptr);

        printf("  Processing %s\n", sscur->filename);
        if (sscur->islibrary)
            Globals.LIBNB++;

        size_t offset = 0;
        while (offset < sscur->filesize)
        {
            uint8_t* data = sscur->data + offset;
            uint16_t* dataw = (uint16_t*)(data);
            if (*dataw != 1)
            {
                if (*dataw == 0)  // Possibly that is filler at the end of the block
                {
                    size_t offsetnext = (offset + 511) & ~511;
                    while (*data == 0 && offset < sscur->filesize && offset < offsetnext)
                    {
                        data++; offset++;
                    }
                    if (offset == sscur->filesize)
                        break;  // End of file
                }
                dataw = (uint16_t*)(data);
                if (*dataw != 1)
                    fatal_error("Unexpected word %06ho at %06ho in %s\n", *dataw, offset, sscur->filename);
            }

            uint16_t blocksize = ((uint16_t*)data)[1];
            uint16_t blocktype = ((uint16_t*)data)[2];

            if (blocktype == 0 || blocktype > 8)
                fatal_error("ERR4: Illegal record type at %06ho in %s\n", offset, sscur->filename);
            else if (blocktype == 1)  // START GSD RECORD, see LINK7\GSD
            {
                printf("    Block type 1 - GSD at %06ho size %06ho\n", (uint16_t)offset, blocksize);
                process_pass2_gsd_block(sscur, data);
            }
            else if (blocktype == 3)  // See LINK7\DMPTXT
            {
                process_pass2_dump_txtblk();

                uint16_t addr = ((uint16_t*)data)[3];
                uint16_t datasize = blocksize - 8;
                printf("    Block type 3 - TXT at %06ho size %06ho addr %06ho %06ho len %06ho\n",
                       (uint16_t)offset, blocksize, addr, addr + Globals.BASE, datasize);
                Globals.TXTLEN = datasize;
                assert(datasize <= sizeof(Globals.TXTBLK));
                memcpy(Globals.TXTBLK, data + 6, blocksize - 6);

                *((uint16_t*)Globals.TXTBLK) = addr + Globals.BASE;  // ADD BASE TO GIVE ABS LOAD ADDR
            }
            else if (blocktype == 4)  // See LINK7\RLD
            {
                uint16_t base = *((uint16_t*)Globals.TXTBLK);
                printf("    Block type 4 - RLD at %06ho size %06ho base %06ho\n", (uint16_t)offset, blocksize, base);
                process_pass2_rld(sscur, data);
            }
            else if (blocktype == 6)  // MODULE END RECORD, See LINK7\MODND
            {
                printf("    Block type 6 - ENDMOD at %06ho size %06ho\n", (uint16_t)offset, blocksize);

                process_pass2_dump_txtblk();

                // AT THE END OF EACH MODULE THE BASE ADR OF EACH SECTION IS UPDATED AS DETERMINED BY THE MST.
                for (int i = 0; i < ModuleSectionCount; i++)
                {
                    ModuleSectionEntry* mstentry = ModuleSectionTable + i;
                    SymbolTableEntry* entry = SymbolTable + mstentry->stindex;
                    if (entry->name != RAD50_ABS)
                        entry->value += mstentry->size;
                }
                mst_table_clear();
            }
            else if (blocktype == 7)  // See LINK7\LIBPA2
            {
                printf("    Block type 7 - TITLIB at %06ho size %06ho\n", (uint16_t)offset, blocksize);
                //uint16_t eptsize = *(uint16_t*)(data + 24/*L_HEAB*/);  // EPT SIZE IN LIBRARY HEADER
                //TODO
                proccess_pass2_libpa2(sscur);
                break;
                //data += eptsize; offset += eptsize;
            }

            data += blocksize; offset += blocksize;
            data += 1; offset += 1;  // Skip checksum
        }
    }
}

void process_pass2_done()
{
    uint16_t highlim = (Globals.SWIT1 & SW_J) ? Globals.DHGHLM : Globals.HGHLIM;

    // Copy the bitmap to block 0
    if ((Globals.FLGWD & 02000) == 0)
    {
        int bitstowrite = ((int)highlim + 511) / 512;
        int bytestowrite = (bitstowrite + 7) / 8;
        memcpy(OutputBuffer + SysCom_BITMAP, Globals.BITMAP, bytestowrite);
    }

    // Write the SAV file
    size_t bytestowrite = OutputBlockCount == 0 ? 65536 : OutputBlockCount * 512;
    size_t byteswrit = fwrite(OutputBuffer, 1, bytestowrite, outfileobj);
    if (byteswrit != bytestowrite)
        fatal_error("ERR6: Failed to write output file.\n");
}

// PROCESS COMMAND STRING SWITCHES, see LINK1\SWLOOP
void parse_commandline(int argc, char **argv)
{
    assert(argv != nullptr);

    uint16_t TMPIDD = 0;  // D BIT TO TEST FOR /J PROCESSING
    uint16_t TMPIDI = 0;  // I BIT TO TEST FOR /J PROCESSING

    for (int arg = 1; arg < argc; arg++)
    {
        const char* argvcur = argv[arg];

        if (*argvcur == '/' || *argvcur == '-')  // Parse global arguments
        {
            //TODO: Parse arguments like Command String Interpreter
            const char* cur = argvcur + 1;
            uint16_t param1, param2;
            if (*cur != 0)
            {
                // EXECUTE:filespec - Specifies the name of the memory image file
                if (strncmp(cur, "EXECUTE:", 8) == 0)
                {
                    strcpy_s(savfilename, cur + 8);
                    continue;
                }

                int result;
                param1 = param2 = 0;  result = 0;
                int option = toupper(*cur++);
                switch (option)
                {
                case 'T': // /T:addr - SPECIFY TRANSFER ADR
                    result = sscanf(cur, ":%ho", &param1);
                    if (result < 1)
                        fatal_error("Invalid /T option, use /T:addr\n");
                    Globals.SWITCH |= SW_T;
                    Globals.BEGBLK.value = param1;
                    break;

                case 'M':  // /M - MODIFY INITIAL STACK
                    result = sscanf(cur, ":%ho", &param1);
                    if (result < 1)
                        fatal_error("Invalid /M option, use /M:addr\n");
                    if (param1 & 1)
                        fatal_error("Invalid /M option value, use even address\n");
                    Globals.SWITCH |= SW_M;
                    Globals.STKBLK[2] = param1;
                    break;

                case 'B':  // /B:addr - SPECIFY BOTTOM ADR FOR LINK
                    result = sscanf(cur, ":%ho", &param1);
                    if (result < 1)
                        fatal_error("Invalid /B option, use /B:addr\n");
                    Globals.SWITCH |= SW_B;
                    TMPIDD = 0;
                    TMPIDI = 0;
                    //TODO
                    break;

                    //case 'U':  // /U - ROUND SECTION
                    //    result = sscanf(cur, ":%ho:%ho", &param1, &param2);
                    //    Globals.SWITCH |= SW_U;
                    //    //TMPIDD = D.SWU;
                    //    //TMPIDI = I.SWU;
                    //    //TODO
                    //    break;

                    //case 'E':  // /E - EXTEND SECTION
                    //    result = sscanf(cur, ":%ho:%ho", &param1, &param2);
                    //    Globals.SWITCH |= SW_E;
                    //    //TMPIDD = D.SWE;
                    //    //TMPIDI = I.SWE;
                    //    //TODO
                    //    break;

                    //case 'Y':  // /Y - START SECTION ON MULTIPLE OF VALUE
                    //    result = sscanf(cur, ":%ho:%ho", &param1, &param2);
                    //    Globals.SWITCH |= SW_Y;
                    //    //TMPIDD = D.SWY;
                    //    //TMPIDI = I.SWY;
                    //    //TODO
                    //    break;

                    //case 'H':  // /H - SPECIFY TOP ADR FOR LINK
                    //    result = sscanf(cur, ":%ho:%ho", &param1, &param2);
                    //    Globals.SWITCH |= SW_H;
                    //    //TMPIDD = D.SWH;
                    //    //TMPIDI = I.SWH;
                    //    //TODO
                    //    break;

                    //case 'K':  // /K - SPECIFY MINIMUM SIZE
                    //    result = sscanf(cur, ":%ho", &param1);
                    //    if (result < 1)
                    //        fatal_error("Invalid /K option, use /K:value\n");
                    //    if (param1 < 2 || param1 > 28)
                    //        fatal_error("Invalid /K option value, should be: 2 < value < 28\n");
                    //    Globals.SWITCH |= SW_K;
                    //    //TODO: if (Globals.SWITCH & SW_R)
                    //    Globals.KSWVAL = param1;
                    //    break;

                    //case 'P':  // /P:N  SIZE OF LML TABLE
                    //    result = sscanf(cur, ":%ho", &param1);
                    //    //TODO
                    //    break;

                    //case 'Z':  // /Z - ZERO UNFILLED LOCATIONS
                    //    result = sscanf(cur, ":%ho:%ho", &param1, &param2);
                    //    //TMPIDD = D.SWZ;
                    //    //TMPIDI = I.SWZ;
                    //    //TODO
                    //    break;

                    //case 'R':  // /R[:stacksize] - INDICATE FOREGROUND LINK
                    //    result = sscanf(cur, ":%ho", &param1);
                    //    Globals.SWITCH |= SW_R;
                    //    //TODO
                    //    break;

                    //case 'V':  // /XM, OR /V ON 1ST LINE
                    //    result = sscanf(cur, ":%ho", &param1);
                    //    //TODO
                    //    break;

                    //case 'L':  // /L - INDICATE LDA OUTPUT
                    //    //TODO
                    //    break;

                case 'W':  // /W - SPECIFY WIDE MAP LISTING
                    Globals.NUMCOL = 6; // 6 COLUMNS
                    Globals.LSTFMT--; // WIDE CREF
                    break;

                case 'X':  // /X - DO NOT EMIT BIT MAP
                    Globals.SWITCH |= SW_X;
                    break;

                    //case 'I':  // /I - INCLUDE MODULES FROM LIBRARY
                    //    Globals.SWITCH |= SW_I;
                    //    break;

                    //case 'F':  // /F - INCLUDE FORLIB.OBJ IN LINK
                    //    Globals.SWITCH |= SW_F;
                    //    break;

                case 'A':  // /A - ALPHABETIZE MAP
                    Globals.SWITCH |= SW_A;
                    break;

                    //case 'S':  // /S - SYMBOL TABLE AS LARGE AS POSSIBLE
                    //    //TODO
                    //    break;

                    //case 'D':  // /D - ALLOW DUPLICATE SYMBOLS
                    //    Globals.SWIT1 |= SW_D;
                    //    break;

                    //case 'N':  // /N - GENERATE CROSS REFERENCE
                    //    result = sscanf(cur, ":%ho", &param1);
                    //    //TODO
                    //    break;

                    //case 'G':  // /G - CALC. EPT SIZE ON RT-11
                    //    result = sscanf(cur, ":%ho", &param1);
                    //    //TODO
                    //    break;

                    //case 'Q':  // /Q:addr - SET PSECTS TO ABSOLUTE ADDRESSES
                    //    result = sscanf(cur, ":%ho", &param1);
                    //    //TODO
                    //    break;

                case 'J':  // /J - USE SEPARATED I-D SPACE
                    if (Globals.SWITCH & SW_R)
                        fatal_error("Invalid option: /R illegal with /J\n"); //TODO: Should be warning only
                    else
                        Globals.SWIT1 |= SW_J;
                    break;

                default:
                    fatal_error("Unknown command line option '%c'\n", option);
                }
            }
        }
        else  // Parse filename and arguments
        {
            if (SaveStatusCount == SaveStatusAreaSize)
                fatal_error("Too many files specified.\n");

            SaveStatusEntry* sscur = SaveStatusArea + SaveStatusCount;

            // Parse filename
            char* filenamecur = sscur->filename;
            const char* cur = argvcur;
            int filenamelen = 0;
            while (*cur != 0 && (isalnum(*cur) || *cur == '.' || *cur == '_' || *cur == '-'))
            {
                *filenamecur = *cur;
                filenamecur++;  cur++;
                filenamelen++;
                if (filenamelen >= sizeof(sscur->filename))
                    fatal_error("Too long filename: %s\n", argvcur);
            }
            SaveStatusCount++;

            //TODO: Parse options associated with the file
            //while (*cur == '/')
            //{
            //  cur++;
            //  switch (*cur)
            //  {
            //  default:
            //      fatal_error("Bad switch: %s\n", argvcur);
            //  }
            //}
        }
    }

    if (SaveStatusCount == 0)
        fatal_error("Input file not specified.\n");
}

void print_help()
{
    printf("\n");
    printf(
        "Usage: link11 <input files> <options>\n"
        "Options:\n"
        "  /T:addr  Specifies the starting address of the linked program\n"
        "  /M:addr  Specifies the stack address for the linked program\n"
        "  /B:addr  Specifies the lowest address to be used by the linked program\n"
        "  /W       Produces a load map that is 132 columns wide\n"
        "  /X       Do not emit bit map\n"
        "  /A       Lists global symbols on the link map in alphabetical order\n"
        "\n");
    //TODO
}

int main(int argc, char *argv[])
{

#if defined(_DEBUG) && defined(_MSC_VER)
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF);
    int n = 0;
    _CrtSetBreakAlloc(n);
    _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDOUT);
    _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDOUT);
#endif

    printf("PCLINK11  %s  %s\n", APP_VERSION_STRING, __DATE__);

    assert(sizeof(uint8_t) == 1);
    assert(sizeof(uint16_t) == 2);
    assert(sizeof(uint32_t) == 4);

    if (argc <= 1)
    {
        print_help();
        exit(EXIT_FAILURE);
    }

    atexit(finalize);

    initialize();

    parse_commandline(argc, argv);
    Globals.FLGWD |= FG_STB; //DEBUG: Always generate STB

    read_files();

    process_pass1();
    if (Globals.PAS1_5 & 1)  // Check if we need Pass 1.5 (we have library files)
    {
        process_pass15();  // SCANS ONLY LIBRARIES
    }
    process_pass1_endp1();

    process_pass_map_init();
    process_pass_map_output();
    process_pass_map_done();

    process_pass2_init();
    process_pass2();
    process_pass2_done();
    //TODO: Pass 2.5

    printf("SUCCESS\n");
    finalize();

#if defined(_DEBUG) && defined(_MSC_VER)
    _CrtDumpMemoryLeaks();
#endif

    return EXIT_SUCCESS;
}


/////////////////////////////////////////////////////////////////////////////
