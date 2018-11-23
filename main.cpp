
#define _CRT_SECURE_NO_WARNINGS

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <stdarg.h>
#include <time.h>

#include "main.h"


/////////////////////////////////////////////////////////////////////////////

struct SaveStatusEntry
{
    char	filename[64];
    FILE*	fileobj;
    WORD	filesize;
    bool	islibrary;
    void*	data;
};
const int SaveStatusAreaSize = 8;
SaveStatusEntry SaveStatusArea[SaveStatusAreaSize];
int SaveStatusCount = 0;

struct LibraryModuleEntry
{
    //TODO
    WORD stub[3];
};
const int LibraryModuleListSize = LMLSIZ;
LibraryModuleEntry LibraryModuleList[LibraryModuleListSize];

// ****	GSD ENTRY STRUCTURE
struct GSDentry
{
    DWORD	symbol;		// SYMBOL CHARS 1-6(RAD50)
    BYTE	flags;		// FLAGS
    BYTE	code;		// CODE BYTE
    WORD	value;		// SIZE OR OFFSET
};

// ****	SYMBOL TABLE STRUCTURE
struct SymbolTableEntry
{
    DWORD	name;		// 2 WD RAD50 NAME
    WORD	flagseg;    // PSECT FLAGS !  SEG #
    WORD	value;      // VALUE WORD
    WORD	status;     // A!B!C!D!  ENTRY # PTR
};

SymbolTableEntry* SymbolTable = NULL;
SymbolTableEntry* ASECTentry = NULL;
const int SymbolTableSize = 4095;  // STSIZE
const int SymbolTableLength = SymbolTableSize * sizeof(SymbolTableEntry);
int SymbolTableCount = 0;  // STCNT -- SYMBOL TBL ENTRIES COUNTER

// ****	INTERNAL SYMBOL TABLE FLAGS BIT ASSIGNMENT
const WORD SY_UDF = 0100000;	// SET TO DECLARE SYMBOL IS UNDEFINED (PSECT NEVER UNDEFINED)
const WORD SY_DUP =  040000;	// SET TO ALLOW DUPLICATE LIBRARY SYMBOLS
const WORD SY_IND =  020000;	// SET TO PUT SYMBOL IN OVERLAY HANDLER TABLE
const WORD SY_WK  =  010000;	// SET TO INDICATE SYMBOL IS WEAK
const WORD SY_SAV = SY_WK;		// SET TO INDICATE PSECT HAS SAV ATTRIBUTE
const WORD SY_ENB = 0170000;	// MASK TO ISOLATE SYMBOL ENTRY NUMBER PTR
const WORD SY_SEC =   04000;	// CS$NU POSITION IN PSECT FLAGS INDICATING SYMBOL IS A SECTION
const WORD SY_SPA = 0100000;	// CS$TYP POSITION IN PSECT FLAGS INDICATING SYMBOL IS I OR D
// SPACE - 1 IF D PSECT, 0 IF I PSECT.  SY.SEC MUST ALSO BE
// SET IF THIS BIT IS USED.  NOTE SY.UDF IS NEVER USED FOR
// A PSECT
const WORD SY_SWI =  010000;	// (CS$ACC)SET IF THIS SYMBOL PUT IN SYMBOL TABLE BY /I UNDEF SYMBOL
const WORD SY_SEG =   01777;	// SEGMENT NUMBER BITS IN FLAGS WORD


const DWORD RAD50_ABS    = rad50x2(". ABS.");  // ASECT
const DWORD RAD50_VSEC   = rad50x2(". VIR.");  // VIRTUAL SECTION SYMBOL NAME
const DWORD RAD50_VIRSZ  = rad50x2("$VIRSZ");  // GLOBAL SYMBOL NAME FOR SIZE OF VIRTUAL SECTION
const DWORD RAD50_PHNDL  = rad50x2("$OHAND");  // OVERLAY HANDLER PSECT
const DWORD RAD50_ODATA  = rad50x2("$ODATA");  // OVERLAY DATA TABLE PSECT
const DWORD RAD50_PTBL   = rad50x2("$OTABL");  // OVERLAY TABLE PSECT
const DWORD RAD50_GHNDL  = rad50x2("$OVRH");   // /O OVERLAY HANDLER GBL ENTRY
const DWORD RAD50_GVHNDL = rad50x2("$OVRHV");  // /V OVERLAY HANDLER GBL ENTRY
const DWORD RAD50_GZHNDL = rad50x2("$OVRHZ");  // I-D SPACE OVERLAY HANDLER GBL ENTRY
const DWORD RAD50_ZTABL  = rad50x2("$ZTABL");  // I-D SPACE OVERLAY HANDLER PSECT

const char* FORLIB = "FORLIB.OBJ";  // FORTRAN LIBRARY FILENAME
const char* SYSLIB = "SYSLIB.OBJ";  // DEFAULT SYSTEM LIBRARY FILENAME

BYTE* OutputBuffer = NULL;
int OutputBufferSize = 0;

FILE* outfileobj = NULL;
FILE* mapfileobj = NULL;
FILE* stbfileobj = NULL;


/////////////////////////////////////////////////////////////////////////////


struct tagGlobals
{
    WORD	ODBLK[15]; // BLOCK TO HOLD BINOUT SPEC
    // LNKOV1->STORE TIME TO ROLL OVER DATE
    WORD	TEMP;	// TEMPORARY STORAGE
    BYTE	TXTBLK[RECSIZ];  // SPACE FOR A FORMATTED BINARY RECORD

    int 	UNDLST; // START OF UNDEFINED SYMBOL LIST
    WORD	SYEN0;	// ADR OF SYMBOL TABLE ENTRY NUMBER 0
    // REL PTR + THIS = ABS ADDR OF SYMBOL NODE
    WORD	CSECT;	// PTR TO LATEST SECTION (PASS1)

    WORD	PA2LML;	// START OF LML BUFR
    // LNKOV1->TEMP. SEGMENT POINTER
    WORD	LMLPTR;	// CURRENT PTR TO LIBRARY MOD LIST
    WORD	STLML;	// CURRENT START OF LMLPTR IF MULTI-LIBR FILES
    WORD	ENDLML;	// END OF LIB MOD LIST
    WORD	ESZRBA;	// SIZE OF CURRENT LIBRARY EPT
    // RELOCATION INFO OUTPUT BUFR ADR
    WORD	OVCOUN;	// NO. OF OVERLAY ENTRY PTS.
    WORD	OVSPTR;	// PTR TO OVERLAY SEGMENT BLK
    BYTE	PAS1_5; // PASS 1.5 SWITCH(0 ON PASS1, 1 IF TO DO LIBRARY,
    // BIT 7 SET IF DOING LIBRARIES
    BYTE	DUPMOD;	// 1 IF LIB MOD IS DUP
    // 0 IF LIB MOD IS NOT DUP
    BYTE	NUMCOL;	// NUMBER OF COLUMNS WIDE FOR MAP
    // IND + OR - RELOCATION DURING PASS 2
    BYTE	LIBNB;  // LIBRARY FILE NUMBER FOR LML

    WORD	SWITCH;	// SWITCHES FROM CSI (SEE "LINK1" FOR DETAILS)
    WORD	SWIT1;	// Switches
    WORD	FILPT1; // START OF SAVESTATUS AREA -4

    // VARIABLES FOR PROGRAM BEING LINKED
    WORD	HGHLIM;	// MAX # OF SECTIONS IN ANY MODULE PROCESSED
    // HIGHEST LOCATION USED BY PROGRAM (I-SPACE)
    WORD	DHGHLM;	// MAX # OF SECTIONS IN ANY MODULE PROCESSED
    // HIGHEST LOCATION USED BY PROGRAM (D-SPACE)

    GSDentry BEGBLK; // TRANSFER ADDRESS BLOCK (4 WD GSD ENTRY)
    //   TRANS ADDR OR REL OFFSET FROM PSECT

    WORD	STKBLK[3]; // USER STACK ADDRESS BLOCK(SYMBOL & VALUE)
    // LNKSAV->TEMP 4 WORD STORAGE FOR GSD RECORD

    WORD	HSWVAL;	// /H SWITCH VALUE - I-SPACE
    WORD	DHSWVL;	// /H SWITCH VALUE - D-SPACE

    WORD	ESWVAL;	// /E SWITCH VALUE - I-SPACE
    DWORD	ESWNAM; // /E SWITCH NAME - I-SPACE
    WORD	DESWVL; // /E SWITCH VALUE - D-SPACE
    DWORD	DESWNM;	// /E SWITCH NAME - D-SPACE
    WORD	KSWVAL;	// /K SWITCH VALUE OR STACK SIZE FOR REL FILE
    WORD	YSWNAM[25]; // /Y SECTION NAME ARRAY(TEMP OVERLAY # IN OV1 & SAV)
    // +2 NEXT ASSIGNABLE OUTPUT BLK(LNKMAP)
    //    RELOCATION INFO BLOCK #(LNKSAV) - I-SPACE
    // YSWVAL==YSWNAM+4
    WORD	DYSWNM[25];
    // DYSWVL==DYSWNM+4
    BYTE	YSWT;	// SAY WE ARE USING /Y
    BYTE	YCNT;	// NUMBER OF TIMES TO PROMPT FOR /Y (SET IN LINK2)
    WORD	DEFALT;	// DEFAULT BOUNDARY VALUE FOR /Y (SET IN LINK2)
    WORD	USWVAL;	// /U SWITCH VALUE - I-SPACE
    DWORD	USWNAM;	// /U SWITCH NAME - I-SPACE
    WORD	DUSWVL;	// /U SWITCH VALUE - D-SPACE
    DWORD	DUSWNM;	// /U SWITCH NAME - D-SPACE
    WORD	QSWVAL;	// /Q BUFFER POINTER
    WORD	ZSWVAL;	// /Z SWITCH VALUE - I-SPACE
    WORD	DZSWVL;	// /Z SWITCH VALUE - D-SPACE
    WORD	LRUNUM;	// USED TO COUNT MAX # OF SECTIONS AND AS
    //  TIME STAMP FOR SAV FILE CACHING
    WORD	BITBAD;	// -> START OF BITMAP TABLE (D-SPACE IF /J USED)
    WORD	IBITBD;	// -> START OF I-SPACE BITMAP TABLE
    WORD	BITMAP;	// CONTAINS BITMAP OR IBITBD (IF /J USED)
    WORD	CACHER;	// -> CACHE CONTROL BLOCKS
    WORD	NUMBUF;	// NUMBER OF AVAILABLE CACHING BLOCKS (DEF=3)
    WORD	BASE;	// BASE OF CURRENT SECTION
    WORD	CKSUM;	// CHECKSUM FOR STB & LDA OUTPUT
    // LNKOV1->TEMP LINK POINTER TO NEW REGION BLK
    // CURRENT REL BLK OVERLAY NUM
    WORD	ENDRT;	// END OF ROOT SYMBOL TBL LIST
    WORD	VIRSIZ;	// LARGEST REGION IN A PARTITION
    WORD	REGION;	// XM REGION NUMBER
    WORD	WDBCNT;	// WDB TABLE SIZE ( 14. * NUMBER OF PARTITIONS)
    WORD	HIPHYS;	// HIGH LIMIT FOR EXTENDED MEMORY (96K MAX)
    WORD	SVLML;	// START OF LML LIST FOR WHOLE LIBRARY
    WORD	SW_LML;	// LML INTO OVERLAY SWITCH, AND PASS INDICATOR
    WORD	LOC0;	// USED FOR CONTENTS OF LOC 0 IN SAV HEADER
    WORD	LOC66;	// # /O SEGMENTS SAVED FOR CONVERSION TO ADDR OF
    //  /V SEGS IN OVERLAY HANLDER TABLE
    WORD	LSTFMT;	// CREF LISTING FORMAT (0=80COL, -1=132COL)

    // I-D SPACE VARIABLES

    WORD	IDSWIT;	// BITMASK WORD FOR WHICH SWITCHES USE SEPARATED
    // I-D SPACE
    // D-SPACE, LOW BYTE, I-SPACE, HI BYTE
    WORD	ILEN;	// TOTAL LENGTH OF I-SPACE PSECTS IN WORDS
    WORD	DLEN;	// TOTAL LENGTH OF D-SPACE PSECTS IN WORDS
    WORD	IBLK;	// TOTAL LENGTH OF I-SPACE PSECTS IN BLOCKS
    WORD	DBLK;	// TOTAL LENGTH OF D-SPACE PSECTS IN BLOCKS
    WORD	IROOT;	// SIZE OF THE I-SPACE ROOT IN WORDS
    WORD	DROOT;	// SIZE OF THE D-SPACE ROOT IN WORDS
    WORD	IBASE;	// START OF THE I BASE (BLOCKS)
    WORD	DBASE;	// START OF THE D BASE (BLOCKS)
    WORD	ILOC40;	// CONTENTS OF LOC 40 FOR I-SPACE CCB
    WORD	IFLG;	// NON-ZERO MEANS WRITING I-SPACE BITMAP
    WORD	IDSTRT;	// I-D SPACE ENTRY POINT ($OVRHZ)
    WORD	ZTAB;	// I-D SPACE START ADDRESS OF PSECT $ZTABL
    WORD	OVRCNT;	// # OF OVERLAY SEGMENTS, USED FOR COMPUTING $OVDF6
    WORD	DSGBAS;	// PASS 2 BASE ADR OF D-SPACE OVERLAY SEGMENT
    WORD	DSGBLK;	// PASS 2 BASE BLK OF D-SPACE OVERLAY SEGMENT

    DWORD	MODNAM;	// MODULE NAME, RAD50
    // LDA OUTPUT BUFR PTR OR REL INFO BUFR PTR
    DWORD	IDENT;	// PROGRAM IDENTIFICATION
    // "RELADR" ADR OF RELOCATION CODE IN TEXT OF REL FILE
    // +2 "RELOVL" NEXT REL BLK OVERLAY #

    WORD	ASECT[8];

    WORD	DHLRT;	// D-SPACE HIGH ADDR LIMIT OF REGION (R.GHLD)
    WORD	DBOTTM;	// ST ADDR OF REGION AREA - D-SPACE (R.GSAD)
    WORD	DBOTTM_2; // REGION NUMBER (R.GNB)
    WORD	OVRG1;	// -> NEXT ORDB (R.GNXP)
    WORD	OVRG1_2; // -> OSDB THIS REGION (R.GSGP)
    WORD	HLRT;	// HIGH LIMIT OF AREA (R.GHL)
    WORD	BOTTOM;	// ST ADDR OF REGION AREA - (I-SPACE IF /J USED)

    WORD	CBUF;	// START OF CREF BUFFER
    WORD	CBEND;	// CBUF + 512. BYTES FOR A 1 BLOCK CREF BUFFER
    WORD	QAREA[10]; // EXTRA QUEUE ELEMENT
    WORD	PRAREA[5]; // AREA FOR PROGRAMMED REQUESTS

    WORD	EIB512;	// IBUF + 512. BYTES FOR A 1 BLOCK MAP BUFR
    WORD	SEGBAS;	// BASE OF OVERLAY SEGMENT
    WORD	SEGBLK;	// BASE BLK OF OVERLAY SEGMENT
    WORD	TXTLEN;	// TEMP FOR /V SWITCH
    WORD	LINLFT; // NUMBER OF LINES LEFT ON CURRENT MAP PAGE

    // The following globals are defined inside the code

    WORD	FLGWD;	// INTERNAL FLAG WORD
    WORD	ENDOL;	// USE FOR CONTINUE SWITCHES /C OR //
    WORD	SEGNUM;	// KEEP TRACK OF INPUT SEGMENT #'S

    // INPUT BUFFER INFORMATION

    WORD	IRAREA;	// CHANNEL NUMBER AND .READ EMT CODE
    WORD	CURBLK;	// RELATIVE INPUT BLOCK NUMBER
    WORD	IBUF;	// INPUT BUFR ADR(ALSO END OF OUTPUT BUFR (OBUF+512))
    WORD	IBFSIZ;	// INPUT BUFR SIZE (MULTIPLE OF 256) WORD COUNT

    WORD	OBLK;	// RELATIVE OUTPUT BLOCK #
    WORD	OBUF;	// OUTPUT BUFR ADR

    WORD	MBLK;	// OUTPUT BLK # (INIT TO -1 FOR BUMP BEFORE WRITE)
    WORD	MBPTR;	// OUTPUT BUFR POINTER (0 MEANS NO MAP OUTPUT)

    WORD	CBLK;	// OUTPUT BLK # (INIT TO -1 FOR BUMP BEFORE WRITE)
    WORD	CBPTR;	// DEFAULT IS NO CREF
}
Globals;


/////////////////////////////////////////////////////////////////////////////


void print_help()
{
    printf("\n");
    //
    printf("Usage: link11 <input files> <options>\n");
    //TODO
}

void fatal_error(const char* message, ...)
{
    printf("ERROR: ");
    {
        va_list ptr;
        va_start(ptr, message);
        vprintf(message, ptr);
        va_end(ptr);
    }

    exit(EXIT_FAILURE);
}

void initialize()
{
    //printf("Initialization\n");

    memset(&Globals, 0, sizeof(Globals));

    memset(SaveStatusArea, 0, sizeof(SaveStatusArea));
    SaveStatusCount = 0;

    SymbolTable = (SymbolTableEntry*) ::malloc(SymbolTableLength);
    memset(SymbolTable, 0, SymbolTableLength);
    SymbolTableCount = 0;

    // Set globals defaults, see LINK1\START1
    Globals.NUMCOL = 3; // 3-COLUMN MAP IS NORMAL
    Globals.NUMBUF = 3; // NUMBER OF AVAILABLE CACHING BLOCKS (DEF=3)
    Globals.BEGBLK.symbol = RAD50_ABS;
    //Globals.BEGBLK.code = ???;
    //Globals.BEGBLK.flags = ???;
    Globals.BEGBLK.value = 000001;
    Globals.DBOTTM = 01000;
    Globals.BOTTOM = 01000;
}

void finalize()
{
    //printf("Finalization\n");

    if (SymbolTable != NULL)
    {
        free(SymbolTable);  SymbolTable = NULL;
    }

    for (int i = 0; i < SaveStatusCount; i++)
    {
        SaveStatusEntry* sscur = SaveStatusArea + i;
        if (sscur->fileobj == NULL)
            continue;

        fclose(sscur->fileobj);
        sscur->fileobj = NULL;
        //printf("  File closed: %s\n", sscur->filename);
    }

    if (OutputBuffer != NULL)
    {
        free(OutputBuffer);  OutputBuffer = NULL;  OutputBufferSize = 0;
    }

    if (outfileobj != NULL)
    {
        fclose(outfileobj);  outfileobj = NULL;
    }
    if (mapfileobj != NULL)
    {
        fclose(mapfileobj);  mapfileobj = NULL;
    }
    if (stbfileobj != NULL)
    {
        fclose(stbfileobj);  stbfileobj = NULL;
    }
}

// PROCESS COMMAND STRING SWITCHES, see LINK1\SWLOOP in source
void parse_commandline(int argc, char **argv)
{
    for (int arg = 1; arg < argc; arg++)
    {
        const char* argvcur = argv[arg];

        if (*argvcur == '/' || *argvcur == '-')  // Parse global arguments
        {
            //TODO: Parse arguments like Command String Interpreter
            const char* cur = argvcur + 1;
            int result;
            WORD param1, param2;
            if (*cur != 0)
            {
                param1 = param2 = result = 0;
                char option = toupper(*cur++);
                switch (option)
                {
                    // /T - SPECIFY TRANSFER ADR
                case 'T': // /T:address
                    result = sscanf(cur, ":%ho", &param1);
                    if (result < 1)
                        fatal_error("Invalid /T option, use /T:addr\n");
                    Globals.SWITCH |= SW_T;
                    Globals.BEGBLK.value = param1;
                    break;
                    // /M - MODIFY INITIAL STACK
                case 'M':
                    result = sscanf(cur, ":%ho", &param1);
                    if (result < 1)
                        fatal_error("Invalid /M option, use /M:addr\n");
                    if (param1 & 1)
                        fatal_error("Invalid /M option value, use even address\n");
                    Globals.SWITCH |= SW_M;
                    Globals.STKBLK[2] = param1;
                    break;
                    // /B - SPECIFY BOTTOM ADR FOR LINK
                case 'B':
                    result = sscanf(cur, ":%ho", &param1);
                    if (result < 1)
                        fatal_error("Invalid /B option, use /B:addr\n");
                    Globals.SWITCH |= SW_B;
                    //Globals.TMPIDD = 0;
                    //Globals.TMPIDI = 0;
                    //TODO
                    break;
                    // /U - ROUND SECTION
                case 'U':
                    result = sscanf(cur, ":%ho:%ho", &param1, &param2);
                    Globals.SWITCH |= SW_U;
                    //Globals.TMPIDD = D.SWU;
                    //Globals.TMPIDI = I.SWU;
                    //TODO
                    break;
                    // /E - EXTEND SECTION
                case 'E':
                    result = sscanf(cur, ":%ho:%ho", &param1, &param2);
                    Globals.SWITCH |= SW_E;
                    //Globals.TMPIDD = D.SWE;
                    //Globals.TMPIDI = I.SWE;
                    //TODO
                    break;
                    // /Y - START SECTION ON MULTIPLE OF VALUE
                case 'Y':
                    result = sscanf(cur, ":%ho:%ho", &param1, &param2);
                    Globals.SWITCH |= SW_Y;
                    //Globals.TMPIDD = D.SWY;
                    //Globals.TMPIDI = I.SWY;
                    //TODO
                    break;
                    // /H - SPECIFY TOP ADR FOR LINK
                case 'H':
                    result = sscanf(cur, ":%ho:%ho", &param1, &param2);
                    Globals.SWITCH |= SW_H;
                    //Globals.TMPIDD = D.SWH;
                    //Globals.TMPIDI = I.SWH;
                    //TODO
                    break;
                    // /K - SPECIFY MINIMUM SIZE
                case 'K':
                    result = sscanf(cur, ":%ho", &param1);
                    Globals.SWITCH |= SW_K;
                    //TODO
                    break;
                    // /P:N  SIZE OF LML TABLE
                case 'P':
                    result = sscanf(cur, ":%ho", &param1);
                    //TODO
                    break;
                    // /Z - ZERO UNFILLED LOCATIONS
                case 'Z':
                    result = sscanf(cur, ":%ho:%ho", &param1, &param2);
                    //Globals.TMPIDD = D.SWZ;
                    //Globals.TMPIDI = I.SWZ;
                    //TODO
                    break;
                    // /R - INDICATE FOREGROUND LINK
                case 'R':
                    result = sscanf(cur, ":%ho", &param1);
                    Globals.SWITCH |= SW_R;
                    //TODO
                    break;
                    // /XM, OR /V ON 1ST LINE
                case 'V':
                    result = sscanf(cur, ":%ho", &param1);
                    //TODO
                    break;
                    // /L - INDICATE LDA OUTPUT
                case 'L':
                    //TODO
                    break;
                    // /W - SPECIFY WIDE MAP LISTING
                case 'W':
                    Globals.NUMCOL = 6; // 6 COLUMNS
                    Globals.LSTFMT--; // WIDE CREF
                    break;
                    // /C - CONTINUE ON ANOTHER LINE
                    // /
                    // /X - DO NOT EMIT BIT MAP
                case 'X':
                    Globals.SWITCH |= SW_X;
                    break;
                    // /I - INCLUDE MODULES FROM LIBRARY
                case 'I':
                    Globals.SWITCH |= SW_I;
                    break;
                    // /F - INCLUDE FORLIB.OBJ IN LINK
                case 'F':
                    Globals.SWITCH |= SW_F;
                    break;
                    // /A - ALPHABETIZE MAP
                case 'A':
                    Globals.SWITCH |= SW_A;
                    break;
                    // /S - SYMBOL TABLE AS LARGE AS POSSIBLE
                case 'S':
                    //TODO
                    break;
                    // /D - ALLOW DUPLICATE SYMBOLS
                case 'D':
                    Globals.SWIT1 |= SW_D;
                    break;
                    // /N - GENERATE CROSS REFERENCE
                case 'N':
                    result = sscanf(cur, ":%ho", &param1);
                    //TODO
                    break;
                    // /G - CALC. EPT SIZE ON RT-11
                case 'G':
                    result = sscanf(cur, ":%ho", &param1);
                    //TODO
                    break;
                    // /Q - SET PSECTS TO ABSOLUTE ADDRESSES
                case 'Q':
                    result = sscanf(cur, ":%ho", &param1);
                    //TODO
                    break;
                    // /J - USE SEPARATED I-D SPACE
                case 'J':
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
            //	cur++;
            //	switch (*cur)
            //	{
            //	default:
            //		fatal_error("Bad switch: %s\n", argvcur);
            //	}
            //}
        }
    }

    if (SaveStatusCount == 0)
        fatal_error("Input file not specified\n");
}


/////////////////////////////////////////////////////////////////////////////
// Symbol table functions

void symbol_table_enter(int* pindex, DWORD lkname, WORD lkwd)
{
    // Find empty entry
    if (SymbolTableCount >= SymbolTableSize)
        fatal_error("ERR1: Symbol table overflow\n");

    int index = *pindex;
    SymbolTableEntry* entry;
    while (true)
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
    //TODO
}

// ADD A REFERENCED SYMBOL TO THE UNDEFINED LIST, see LINK3\ADDUDF
void symbol_table_add_undefined(int index)
{
    assert(index > 0);
    assert(index < SymbolTableSize);

    if (Globals.UNDLST != 0)
    {
        SymbolTableEntry* oldentry = SymbolTable + Globals.UNDLST;
        oldentry->status = (oldentry->status & 0170000) | index;  // set back reference
    }

    SymbolTableEntry* entry = SymbolTable + index;
    entry->status |= SY_UDF;  // MAKE CUR SYM UNDEFINED
    entry->flagseg = (entry->flagseg & ~SY_SEG) | Globals.SEGNUM;  // SET SEGMENT # WHERE INITIAL REF
    entry->value = Globals.UNDLST;

    Globals.UNDLST = index;
}

// REMOVE A ENTRY FROM THE UNDEFINED LIST, see LINK3\REMOVE
void symbol_table_remove_undefined(int index)
{
    assert(index > 0);
    assert(index < SymbolTableSize);

    SymbolTableEntry* entry = SymbolTable + index;
    int previndex = entry->status & 07777;
    if (previndex == 0)
        Globals.UNDLST = entry->value;  // exclude entry from the list
    else
    {
        SymbolTableEntry* preventry = SymbolTable + previndex;
        preventry->value = entry->value;  // exclude entry from the list
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
bool symbol_table_search_routine(DWORD lkname, WORD lkwd, WORD lkmsk, WORD dupmsk, int* result)
{
    assert(SymbolTable != NULL);

    // Calculate hash
    WORD hash = 0;
    if (lkname != 0)
        hash = LOWORD(lkname) + HIWORD(lkname);
    else
    {
        // 0 = BLANK NAME
        hash = (Globals.SEGNUM + 1) << 2;  // USE SEGMENT # FOR BLANK SECTION NAME
        lkname = hash;
    }

    // Normalize hash to table size
    WORD stdiv = 0120000;  // NORMALIZED STSIZE USED FOR DIVISION
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
    while (true)
    {
        SymbolTableEntry* entry = SymbolTable + index;
        if (entry->name == 0)
        {
            *result = index;
            break;  // EMPTY CELL
        }
        if (entry->name = lkname)
        {
            // AT THIS POINT HAVE FOUND A MATCHING SYMBOL NAME, NOW MUST CHECK FOR MATCHING ATTRIBUTES.
            WORD flagsmasked = (entry->flagseg & ~lkmsk);
            if (flagsmasked == lkwd)
            {
                if (dupmsk == 0 || (entry->status & SY_DUP) == 0)
                {
                    *result = index;
                    found = true;
                    break;
                }

                //TODO: Process dups
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
//	   THE SYMBOL TABLE.  DOES NOT REQUIRE A SEGMENT NUMBER MATCH
//	   WHETHER THE SYMBOL IS A DUPLICATE OR NOT.
// Out: return = true if found, false if new entry
bool symbol_table_dlooke(DWORD lkname, WORD lkwd, WORD lkmsk, int* pindex)
{
    bool found = symbol_table_search_routine(lkname, lkwd, lkmsk, 0, pindex);
    if (found)
        return true;

    symbol_table_enter(pindex, lkname, lkwd);
    return false;
}
// 'LOOKUP' ONLY SEARCHES THE SYMBOL TABLE FOR A SYMBOL MATCH.  IF SYMBOL
//	   IS A DUPLICATE, THIS ROUTINE REQUIRES A SEGMENT NUMBER MATCH.
bool symbol_table_lookup(DWORD lkname, WORD lkwd, WORD lkmsk, int* pindex)
{
    return symbol_table_search_routine(lkname, lkwd, lkmsk, SY_DUP, pindex);
}
// 'LOOKE'  DOES A LOOKUP & IF NOT FOUND THEN ENTERS THE NEW SYMBOL INTO
//	   THE SYMBOL TABLE.  IF SYMBOL IS A DUPLICATE, THIS ROUTINE
//	   REQUIRES A SEGMENT NUMBER MATCH.
// Out: return = true if found, false if new entry
bool symbol_table_looke(DWORD lkname, WORD lkwd, WORD lkmsk, int* pindex)
{
    bool found = symbol_table_search_routine(lkname, lkwd, lkmsk, SY_DUP, pindex);
    if (found)
        return true;

    symbol_table_enter(pindex, lkname, lkwd);
    return false;
}
// 'SEARCH' THIS ROUTINE DOES A LOOKUP ONLY AND DOES NOT CARE WHETHER THE
//	   SYMBOL IS A DUPLICATE OR NOT.  THIS ROUTINE IS USED REPEATEDLY
//	   AFTER A SINGLE CALL TO 'DLOOKE'.
bool symbol_table_search(DWORD lkname, WORD lkwd, WORD lkmsk, int* pindex)
{
    return symbol_table_search_routine(lkname, lkwd, lkmsk, 0, pindex);
}


/////////////////////////////////////////////////////////////////////////////


void read_files()
{
    for (int i = 0; i < SaveStatusCount; i++)
    {
        SaveStatusEntry* sscur = SaveStatusArea + i;
        assert(sscur->fileobj == NULL);

        FILE* file = fopen(sscur->filename, "rb");
        if (file == NULL)
            fatal_error("ERR2: Failed to open input file: %s, errno %d.\n", sscur->filename, errno);
        sscur->fileobj = file;
        //printf("  File opened: %s\n", sscur->filename);

        fseek(file, 0L, SEEK_END);
        long filesize = ftell(file);
        if (filesize > 65535)
            fatal_error("Input file %s too long.\n", sscur->filename);
        sscur->filesize = (WORD)filesize;

        sscur->data = malloc(filesize);
        if (sscur->data == NULL)
            if (filesize > 65535)
                fatal_error("Failed to allocate memory for input file %s.\n", sscur->filename);

        fseek(file, 0L, SEEK_SET);
        size_t bytesread = fread(sscur->data, 1, filesize, file);
        if (bytesread != filesize)
            fatal_error("ERR2: Failed to read input file %s.\n", sscur->filename);
        //printf("  File read %s, %d bytes.\n", sscur->filename, bytesread);

        fclose(file);
        //printf("  File closed: %s\n", sscur->filename);
        sscur->fileobj = NULL;
    }
}

// FORCE0 IS CALLED TO GENERATE THE FOLLOWING WARNING MESSAGE, see LINK3\FORCE0
// ?LINK-W-DUPLICATE SYMBOL "SYMBOL" IS FORCED TO THE ROOT
void pass1_force0(const SymbolTableEntry* entry)
{
    printf("DUPLICATE SYMBOL \"%s\" IS FORCED TO THE ROOT\n", entry->name);
}

void pass1_insert_entry_into_ordered_list(int index, SymbolTableEntry* entry, bool absrel)
{
    SymbolTableEntry* sectentry = ASECTentry;
    if (Globals.CSECT > 0)
    {
        sectentry = SymbolTable + Globals.CSECT;
        if (absrel && (sectentry->flagseg & (040 << 8)) != 0)
            sectentry = ASECTentry;
    }
    assert(sectentry != NULL);

    SymbolTableEntry* preventry = sectentry;
    while (true)
    {
        int nextindex = preventry->status & 07777;
        if (nextindex == 0)  // end of chain
            break;
        SymbolTableEntry* nextentry = SymbolTable + nextindex;
        if (nextentry->flagseg & SY_SEC)  // next entry is a section
            break;

        //TODO: implement alpha insertion
        if (nextentry->value > entry->value)
            break;

        preventry = nextentry;
    }
    int fwdindex = preventry->status & 07777;
    preventry->status = (preventry->status & 0170000) | index;
    entry->status = (entry->status & 0170000) | fwdindex;
}

void process_pass1_gsd_block(const SaveStatusEntry* sscur, const BYTE* data)
{
    assert(data != NULL);

    WORD blocksize = ((WORD*)data)[1];
    int itemcount = (blocksize - 6) / 8;
    //printf("    Processing GSD block, %d items\n", itemcount);

    for (int i = 0; i < itemcount; i++)
    {
        const WORD* itemw = (const WORD*)(data + 6 + 8 * i);
        memcpy(Globals.TXTBLK, itemw, 8);
        WORD itemw0 = itemw[0];
        WORD itemw1 = itemw[1];
        WORD itemw2 = itemw[2];
        WORD itemw3 = itemw[3];
        char buffer[7];  memset(buffer, 0, sizeof(buffer));
        unrad50(itemw0, buffer);
        unrad50(itemw1, buffer + 3);
        int itemtype = (itemw2 >> 8) & 0xff;
        int itemflags = (itemw2 & 0377);

        switch (itemtype)
        {
        case 0: // 0 - MODULE NAME FROM .TITLE, see LINK3\MODNME
            printf("      Item '%s' type 0 - MODULE NAME\n", buffer);
            if (Globals.MODNAM == 0)
                Globals.MODNAM = MAKEDWORD(itemw0, itemw1);
            break;
        case 2: // 2 - ISD ENTRY (IGNORED), see LINK3\ISDNAM
            printf("      Item '%s' type 2 - ISD ENTRY, ignored\n", buffer);
            break;
        case 3: // 3 - TRANSFER ADDRESS; see LINK3\TADDR
            printf("      Item '%s' type 3 - TRANSFER ADDR %06o\n", buffer, itemw3);
            if ((Globals.BEGBLK.value & 1) == 0)  // USE ONLY 1ST EVEN ONE ENCOUNTERED
                break; // WE ALREADY HAVE AN EVEN ONE.  RETURN
            {
                DWORD lkname = MAKEDWORD(itemw0, itemw1);
                WORD lkmsk = ~SY_SEC; // CARE ABOUT SECTION FLG
                WORD lkwd = SY_SEC; // SECTION NAME LOOKUP IN THE ROOT
                int index;
                if (!symbol_table_lookup(lkname, lkwd, lkmsk, &index))
                    fatal_error("ERR31: Transfer address for '%s' undefined or in overlay.\n", buffer);
                SymbolTableEntry* entry = SymbolTable + index;
                printf("        Entry '%s' %06o %06o %06o\n", unrad50(entry->name), entry->flagseg, entry->value, entry->status);
                //TODO

                if (entry->value > 0)  // IF CURRENT SIZE IS 0 THEN OK
                {
                    // MUST SCAN CURRENT MODULE TO FIND SIZE CONTRIBUTION TO
                    // TRANSFER ADDR SECTION TO CALCULATE PROPER OFFSET
                    const SaveStatusEntry* sscurtmp = sscur;
                    int offset = 0;
                    while (offset < sscur->filesize)
                    {
                        WORD blocksize = ((WORD*)data)[1];
                        WORD blocktype = ((WORD*)data)[2];
                        if (blocktype == 1)
                        {
                            int itemcounttmp = (blocksize - 6) / 8;
                            for (int itmp = 0; itmp < itemcounttmp; itmp++)
                            {
                                const WORD* itemwtmp = (const WORD*)(data + 6 + 8 * itmp);
                                int itemtypetmp = (itemwtmp[2] >> 8) & 0xff;
                                if ((itemtypetmp == 1/*CSECT*/ || itemtypetmp == 5/*PSECT*/) &&
                                    itemw[0] == itemwtmp[0] && itemw[1] == itemwtmp[1])  // FOUND THE PROPER SECTION
                                {
                                    WORD itemvaltmp = itemwtmp[3];
                                    WORD sectsize = (itemvaltmp + 1) & ~1;  // ROUND SECTION SIZE TO WORD BOUNDARY
                                    printf("        Item '%s' type %d - CSECT or PSECT size %06o\n", unrad50(itemwtmp[0], itemwtmp[1]), itemtypetmp, sectsize);
                                    //TODO: UPDATE OFFSET VALUE
                                    //TODO
                                }
                            }
                        }
                        data += blocksize; offset += blocksize;
                        data += 1; offset += 1;  // Skip checksum
                    }
                }

                // See LINK3\TADDR\100$ in source
                Globals.BEGBLK.symbol = entry->name;
                Globals.BEGBLK.flags = entry->flagseg & 0xff;
                Globals.BEGBLK.code = (entry->flagseg << 8) & 0xff;
                Globals.BEGBLK.value = entry->value;
            }
            break;
        case 4: // 4 - GLOBAL SYMBOL, see LINK3\SYMNAM
            printf("      Item '%s' type 4 - GLOBAL SYMBOL flags %03o addr %06o\n", buffer, itemflags, itemw3);
            {
                DWORD lkname = MAKEDWORD(itemw0, itemw1);
                WORD lkwd = 0;
                WORD lkmsk = ~SY_SEC;
                int index;
                bool found;
                if (itemw2 & 010/*SY$DEF*/)  // IS SYMBOL DEFINED HERE?
                {
                    if (Globals.SEGNUM == 0) // ROOT DEF?
                    {
                        found = symbol_table_dlooke(lkname, lkwd, lkmsk, &index);
                        SymbolTableEntry* entry = SymbolTable + index;
                        if (entry->status & SY_DUP)  // IS IT A DUP SYMBOL?
                        {
                            symbol_table_delete(index);  // DELETE ALL OTHER COPIES OF SYMBOL
                            fatal_error("ERR70: Duplicate symbol '%s' is defined in non-library", buffer);
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
                        entry->value = itemw3 + Globals.BASE;
                        pass1_insert_entry_into_ordered_list(index, entry, (itemw2 & 040) == 0);
                        //TODO: ORDER
                        //TODO: ALPHA
                    }
                    else  // LINK3\DEFREF
                    {
                        //TODO
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
                    // LINK3\DOREF
                    if (!found)
                    {
                        symbol_table_add_undefined(index);
                    }
                    else
                    {
                        //TODO
                    }
                }
                //TODO
            }
            break;
        case 1: // 1 - CSECT NAME, see LINK3\CSECNM
            printf("      Item '%s' type 1 - CSECT NAME  %06o\n", buffer, itemw3);
            {
                DWORD lkname = MAKEDWORD(itemw0, itemw1);
                WORD lkwd = SY_SEC;
                WORD lkmsk = ~SY_SEC;
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
            goto PSECT;
        case 5: // 5 - PSECT NAME; see LINK3\PSECNM
            printf("      Item '%s' type 5 - PSECT NAME flags %03o maxlen %06o\n", buffer, itemflags, itemw3);
PSECT:
            {
                Globals.LRUNUM++; // COUNT SECTIONS IN A MODULE

                DWORD lkname = MAKEDWORD(itemw0, itemw1);
                WORD lkmsk = ~SY_SEC;
                WORD lkwd = SY_SEC;
                if (itemflags & 1/*CS$SAV*/) // DOES PSECT HAVE SAVE ATTRIBUTE?
                    itemflags |= 0100/*CS$GBL*/; // FORCE PSECT TO ROOT VIA GBL ATTRIBUTE
                if (itemflags & 0100/*CS$GBL*/) // LOCAL OR GLOBAL SECTION ?
                {
                    lkmsk = ~(SY_SEC + SY_SEG); // CARE ABOUT SECTION FLG & SEGMENT #
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
                    //TODO: SET SEGMENT # INDEPEND. OF LCL OR GBL
                    //TODO: CALL GEN0  ;CREATE FORWARD ENTRY # PTR TO NEW NODE
                    entry->flagseg |= (itemflags << 8);
                    entry->value = 0;  // LENGTH=0 INITIALLY FOR NEW SECTION
                }
                else // AT THIS POINT SYMBOL WAS ALREADY ENTERED INTO SYMBOL TBL; see LINK3\OLDPCT
                {
                    WORD R2 = (entry->flagseg & 0377) // GET PSECT FLAG BITS IN R2
                            & ~(010/*CS$NU*/ | 2/*CS$LIB*/ | 1/*CS$SAV*/); // GET RID OF UNUSED FLAG BITS
                    if (Globals.SWIT1 & SW_J) // ARE WE PROCESSING I-D SPACE?
                    {
                        //TODO
                    }
                    if (R2 != itemflags) // ARE SECTION ATTRIBUTES THE SAME?
                        fatal_error("ERR10: Conflicting section attributes");
                }

                // PLOOP
                //TODO: CALL CHKRT
                if (lkname == RAD50_ABS)
                    ASECTentry = entry;
                else
                {
                    SymbolTableEntry* pSect = ASECTentry;
                    while (true)  // find section chain end
                    {
                        if (pSect == NULL) break;
                        if ((pSect->status & 07777) == 0)
                            break;
                        pSect = SymbolTable + (pSect->status & ~0170000);
                    }
                    if (pSect != NULL)
                        pSect->status |= index;  // set link to the new segment
                }

                entry->value += itemw3; //TODO: very primitive version, depends on flags

                Globals.BASE = 0; //TODO
            }
            break;
        case 6: // 6 - IDENT DEFINITION; see LINK3\PGMIDN in source
            printf("      Item '%s' type 6 - IDENT DEFINITION\n", buffer);
            if (Globals.IDENT == 0)
                Globals.IDENT = MAKEDWORD(itemw0, itemw1);
            break;
        case 7: // 7 - VIRTUAL SECTION; see LINK3\VSECNM in source
            printf("      Item '%s' type 7 - VIRTUAL SECTION\n", buffer);
            //TODO
            break;
        default:
            fatal_error("ERR21: Bad GSD type %d found in %s.\n", itemtype, sscur->filename);
        }
    }
}

// PASS1: GSD PROCESSING, see LINK3\PASS1
void process_pass1()
{
    printf("Pass 1 started\n");
    // PROCESS FORMATTED BINARY RECORDS, see LINK3\PA1 in source
    for (int i = 0; i < SaveStatusCount; i++)
    {
        SaveStatusEntry* sscur = SaveStatusArea + i;
        assert(sscur->fileobj == NULL);
        assert(sscur->data != NULL);

        printf("  Processing %s\n", sscur->filename);
        int offset = 0;
        while (offset < sscur->filesize)
        {
            BYTE* data = (BYTE*)(sscur->data) + offset;
            WORD* dataw = (WORD*)(data);
            if (*dataw != 1)
            {
                if (*dataw == 0)  // Possibly that is filler at the end of the file
                {
                    while (*data == 0 && offset < sscur->filesize)
                    {
                        data++; offset++;
                    }
                    if (offset == sscur->filesize)
                        break;  // End of file
                }
                fatal_error("Unexpected word %06o at %06o in %s\n", *dataw, offset, sscur->filename);
            }

            WORD blocksize = ((WORD*)data)[1];
            WORD blocktype = ((WORD*)data)[2];

            if (blocktype == 0 || blocktype > 8)
                fatal_error("Illegal record type at %06o in %s\n", offset, sscur->filename);
            else if (blocktype == 1)  // 1 - START GSD RECORD, see LINK3\GSD
            {
                printf("    Block type 1 - GSD at %06o size %06o\n", offset, blocksize);
                process_pass1_gsd_block(sscur, data);
            }
            else if (blocktype == 6)  // 6 - MODULE END, see LINK3\MODND
            {
                printf("    Block type 6 - ENDMOD at %06o size %06o\n", offset, blocksize);
                if (Globals.HGHLIM < Globals.LRUNUM)
                    Globals.HGHLIM = Globals.LRUNUM;
                Globals.LRUNUM = 0;
                //TODO
            }
            else if (blocktype == 7)  // See LINK3\LIBRA
            {
                WORD libver = ((WORD*)data)[3];
                if (libver < L_HVER)
                    fatal_error("ERR23 Old library format (%03o).\n", (int)libver);
                sscur->islibrary = true;
                break;  // Skipping library files on Pass 1
            }

            data += blocksize; offset += blocksize;
            data += 1; offset += 1;  // Skip checksum
        }
    }
}

// PASS 1.5 SCANS ONLY LIBRARIES
void process_pass15()
{
    printf("Pass 1.5 started\n");

    Globals.LIBNB = 0;
    for (int i = 0; i < SaveStatusCount; i++)
    {
        SaveStatusEntry* sscur = SaveStatusArea + i;
        assert(sscur->fileobj == NULL);
        assert(sscur->data != NULL);

        // Skipping non-library files on Pass 1.5
        if (!sscur->islibrary)
            continue;

        printf("  Processing %s\n", sscur->filename);
        Globals.LIBNB++;  // BUMP LIBRARY FILE # FOR LML
        int offset = 0;
        while (offset < sscur->filesize)
        {
            BYTE* data = (BYTE*)(sscur->data) + offset;
            WORD* dataw = (WORD*)(data);
            if (*dataw != 1)
            {
                if (*dataw == 0)  // Possibly that is filler at the end of the file
                {
                    while (*data == 0 && offset < sscur->filesize)
                    {
                        data++; offset++;
                    }
                    if (offset == sscur->filesize)
                        break;  // End of file
                }
                fatal_error("Unexpected word %06o at %06o in %s\n", *dataw, offset, sscur->filename);
            }

            WORD blocksize = ((WORD*)data)[1];
            WORD blocktype = ((WORD*)data)[2];

            if (blocktype != 7 && blocktype != 8)
                fatal_error("Illegal record type at %06o in %s\n", offset, sscur->filename);
            if (blocktype == 7)  // See LINK3\LIBRA, WE ARE ON PASS 1.5 , SO PROCESS LIBRARIES
            {
                printf("    Block type 7 - TITLIB at %06o size %06o\n", offset, blocksize);
                WORD eptsize = *(WORD*)(data + L_HEAB);
                printf("      EPT size %06o bytes, %d. records\n", eptsize, (int)(eptsize / 8));

                Globals.SVLML = Globals.STLML; // SAVE START OF ALL LML'S FOR THIS LIB
                Globals.FLGWD |= LB_OBJ; // IND LIB FILE TO LINKER
                //TODO: R4 -> 1ST WD OF BUFR & C=0
                if (Globals.SWITCH & SW_I) // ANY /I MODULES?
                    Globals.FLGWD |= FG_IP; // SET FLAG INDICATING /I PASS FIRST
                if (Globals.SW_LML) // IS SW.LML SET?
                    Globals.SW_LML |= 0100000; // MAKE SURE BIT IS SET, /I TURNS IT OFF
                Globals.ESZRBA = *(WORD*)(data + L_HEAB); // SIZE OF EPT IN BYTES
                Globals.SEGBAS = 0; // SEGBAS->TEMP FOR /X LIB FLAG
                Globals.ESZRBA = Globals.ESZRBA >> 1; // NOW WORDS
                if (*(WORD*)(data + L_HX)) // IS /X SWITCH SET IN LIB. HEADER?
                {
                    Globals.SW_LML &= ~0100000; // NO PREPROCESSING ON /X LIBRARIES
                    Globals.SEGBAS++; // /X LIBRARY ->SEGBAS=1
                    //TODO: WILL  /X EPT FIT IN BUFFER?
                }
                data += L_HEPT; offset += L_HEPT;  // Move to 1ST EPT ENTRY

                // Resolve undefined symbols using EPT
                if (is_any_undefined())
                {
                    int index = Globals.UNDLST;  // GET A WEAK SYMBOL FROM THE UNDEFINED SYMBOL TABLE LIST
                    while (index > 0)
                    {
                        SymbolTableEntry* entry = SymbolTable + index;
                        if (entry->status & SY_WK)  // IS THIS A WEAK SYMBOL?
                        {
                            //TODO: WE HAVE A WEAK SYMBOL;  NOW DEFINE IT WITH ABS VALUE OF 0
                            //TODO: CSECT = ASECT;  // ALL WEAK SYMBOLS GO IN . ABS. PSECT
                            Globals.BASE = 0;
                            //TODO: CALL DEFREF
                        }
                        index = entry->value & 07777;
                    }
                }

                break;
            }
            else if (blocktype == 8)
            {
                printf("    Block type 10 - ENDLIB at %06o size %06o\n", offset, blocksize);
                //TODO
            }

            data += blocksize; offset += blocksize;
            data += 1; offset += 1;  // Skip checksum
        }
    }
}

// Map processing: see LINK4/MAP in source
void process_pass_map_init()
{
    // START PROCESSING FOR MAP, AND Q,U,V,Y SWITCHES

    Globals.SEGBAS = 0; // N VALUE (V:N:M) FOR /V REGION FLAG
    Globals.VIRSIZ = 0; // SIZE OF THE LARGEST PARTITION IN /V REGION
    Globals.HIPHYS = 0; // PARTITION EXTENDED ADDR HIGH LIMIT
    Globals.SEGBLK = 0; // BASE OF PREVIOUS XM PARTITION

    if (Globals.FLGWD & FG_STB) // IS THERE AN STB FILE?
    {
        //TODO: Start .STB file preparation
    }

    WORD R4 = (Globals.SWIT1 & SW_J) ? Globals.DBOTTM : Globals.BOTTOM;
    //TODO: IS "BOTTOM" .GE. SIZE OF ASECT ?
    //TODO

    //TODO: PROCESS /H SWITCH

    //TODO: PROCESS /U & /Q SWITCHES

    //TODO: /V AND /R WITH NO /Y HIGH LIMIT ADJUSTMENT

    //TODO: /Q SWITCH PROCESSING

    //TODO: ROUND ALL XM SEGMENTS TO 32. WORDS
}

static const char LINE4[] = "Section  Addr\tSize";
static const char MTITL4[] = "\tGlobal\tValue";
static const char* wday_name[] =
{ "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday" };
static const char mon_name[][4] =
{ "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

// PRINT UNDEFINED GLOBALS IF ANY, see LINK5\DOUDFS
void print_undefined_globals()
{
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
        printf(" '%s'", unrad50(entry->name));
        fprintf(mapfileobj, "%s\n", unrad50(entry->name));
        index = entry->value;
        count++;
    }
    printf("\n");
}

// Map output: see LINK5\MAPHDR in source
void process_pass_map_output()
{
    printf("Pass map started\n");

    // Prepare MAP file name
    char mapfilename[64];
    memcpy(mapfilename, SaveStatusArea[0].filename, 64);
    char* pext = strrchr(mapfilename, '.');
    pext++; *pext++ = 'M'; *pext++ = 'A'; *pext++ = 'P';

    // Open MAP file
    assert(mapfileobj == NULL);
    mapfileobj = fopen(mapfilename, "wt");
    if (mapfileobj == NULL)
        fatal_error("ERR5: Failed to open %s file, errno %d.\n", mapfilename, errno);

    // Prepare STB file name
    char stbfilename[64];
    memcpy(stbfilename, SaveStatusArea[0].filename, 64);
    pext = strrchr(stbfilename, '.');
    pext++; *pext++ = 'S'; *pext++ = 'T'; *pext++ = 'B';

    // Open STB file
    assert(stbfileobj == NULL);
    stbfileobj = fopen(stbfilename, "wb");
    if (stbfileobj == NULL)
        fatal_error("ERR5: Failed to open %s file, errno %d.\n", stbfilename, errno);

    Globals.LINLFT = LINPPG;

    // OUTPUT THE HEADERS

    fprintf(mapfileobj, "PCLINK11 %-8s", APP_VERSION_STRING);
    fprintf(mapfileobj, "\tLoad Map \t");

    time_t curtime;  time(&curtime); // DETERMINE DATE & TIME
    struct tm * timeptr = localtime(&curtime);
    fprintf(mapfileobj, "%s %.2d-%s-%d %.2d:%.2d\n",
            wday_name[timeptr->tm_wday],
            timeptr->tm_mday, mon_name[timeptr->tm_mon], 1900 + timeptr->tm_year,
            timeptr->tm_hour, timeptr->tm_min);

    char savname[64];
    strcpy_s(savname, SaveStatusArea[0].filename);
    char* pdot = strrchr(savname, '.');
    if (pdot != NULL) *pdot = 0;
    fprintf(mapfileobj, "%-8s .SAV", savname);

    fprintf(mapfileobj, "\tTitle:\t");
    if (Globals.MODNAM != 0)
    {
        fprintf(mapfileobj, unrad50(Globals.MODNAM));
    }
    fprintf(mapfileobj, "\tIdent:\t");
    fprintf(mapfileobj, unrad50(Globals.IDENT));
    fprintf(mapfileobj, "\t\n\n");
    fprintf(mapfileobj, LINE4);
    printf("  Section  Addr   Size    Global  Value   Global  Value   Global  Value\n");
    for (BYTE i = 0; i < Globals.NUMCOL; i++)
        fprintf(mapfileobj, MTITL4);
    fprintf(mapfileobj, "\n\n");

    // LINK5\RESOLV
    WORD sectsize = Globals.DBOTTM;
    WORD baseaddr = 0; // ASECT BASE ADDRESS IS 0
    SymbolTableEntry* entry = ASECTentry;
    int tabcount = 0;
    while (entry != NULL)
    {
        if (entry->flagseg & SY_SEC)
        {
            if (tabcount > 0)
            {
                fprintf(mapfileobj, "\n");
                printf("\n");
                tabcount = 0;
            }

            entry->value = baseaddr;

            // IS THIS BLANK SECTION 0-LENGTH?
            bool skipsect = ((entry->name & 03100) == 0 && sectsize == 0);
            if (!skipsect)
            {
                // OUTPUT SECTION NAME, BASE ADR, SIZE & ATTRIBUTES
                BYTE entryflags = (entry->flagseg) >> 8;
                char bufsize[20];
                sprintf_s(bufsize, "%06o = %u.", sectsize, sectsize / 2);
                const char* sectaccess = (entryflags & 0020) ? "RO" : "RW";
                const char* secttypedi = (entryflags & 0200) ? "D" : "I";
                const char* sectscope = (entryflags & 0100) ? "GBL" : "LCL";
                const char* sectsav = ((entryflags & 0100) && (entryflags & 010000)) ? ",SAV" : "";
                const char* sectreloc = (entryflags & 0040) ? "REL" : "ABS";
                const char* sectalloc = (entryflags & 0004) ? "OVR" : "CON";
                const char* sectname = (entry->name & 03100) ? unrad50(entry->name) : "      ";
                fprintf(mapfileobj, " %s\t %06o\t%-16s words  (%s,%s,%s%s,%s,%s)\n",
                        sectname, baseaddr, bufsize,
                        sectaccess, secttypedi, sectscope, sectsav, sectreloc, sectalloc);
                printf("  '%s' %06o %-16s words  (%s,%s,%s%s,%s,%s)\n",
                       sectname, baseaddr, bufsize,
                       sectaccess, secttypedi, sectscope, sectsav, sectreloc, sectalloc);
            }
        }
        else  // OUTPUT SYMBOL NAME & VALUE, see LINK5\OUTSYM
        {
            entry->value += baseaddr; //TODO

            if (tabcount == 0)
            {
                fprintf(mapfileobj, "\t\t");
                printf("                        ");
            }
            fprintf(mapfileobj, "\t%s\t%06o", unrad50(entry->name), entry->value);
            printf("  %s  %06o", unrad50(entry->name), entry->value);
            tabcount++;
            if (tabcount >= 3)
            {
                fprintf(mapfileobj, "\n");
                printf("\n");
                tabcount = 0;
            }
        }
        // Next section entry index should be in status field
        if ((entry->status & 07777) == 0)
        {
            entry = NULL;
            if (tabcount > 0)
            {
                fprintf(mapfileobj, "\n");
                printf("\n");
            }
            break;
        }
        entry = SymbolTable + (entry->status & 07777);

        if (entry->flagseg & SY_SEC)
        {
            //TODO: Check NEW SECTION?
            //TODO: Calculate new baseaddr and sectsize, see LINK5\RES1
            baseaddr += sectsize;
            sectsize = (entry->value + 1) & ~1;
        }
    }
    //TODO: see LINK5\POSTN\10$ in source
    WORD totalsize = baseaddr + sectsize;
    WORD blockcount = (totalsize + 511) / 512;
    printf("  Total size %06o = %u. bytes, %u. blocks\n", totalsize, totalsize, blockcount);
    //OutputBufferSize = (blockcount == 0) ? 65536 : blockcount * 512;

    //TODO: OUTPUT SYMBOL NAME & VALUE

    print_undefined_globals();

    // OUTPUT TRANSFER ADR & CHECK ITS VALIDITY, see LINK5\DOTADR
    WORD lkmsk = ~(SY_SEC + SY_SEG);  // LOOK AT SECTION & SEGMENT # BITS
    WORD segnum = 0;  // MUST BE IN ROOT SEGMENT
    WORD lkwd = SY_SEC;  // ASSUME SECTION NAME LOOKUP
    if (Globals.BEGBLK.code == 4)  // IS SYMBOL A GLOBAL?
        lkwd = 0;  // GLOBAL SYM IN SEGMENT 0
    int index;
    bool found = symbol_table_lookup(Globals.BEGBLK.symbol, lkwd, lkmsk, &index);
    if (!found)
        fatal_error("ERR31: Transfer address undefined or in overlay\n");
    SymbolTableEntry* entrybeg = SymbolTable + index;

    //TODO: Calculate transfer address
    WORD taddr = entrybeg->value;
    //TODO: Calculate high limit
    WORD highlim = totalsize - 2; //Globals.HGHLIM;

    char bufhlim[20];
    sprintf_s(bufhlim, "%06o = %u.", highlim, highlim / 2);
    fprintf(mapfileobj, "\nTransfer address = %06o, High limit = %-16s words\n", taddr, bufhlim);
    printf("  Transfer address = %06o, High limit = %-16s words\n", taddr, bufhlim);

    fclose(mapfileobj);  mapfileobj = NULL;

    fclose(stbfileobj);  stbfileobj = NULL;
}

void process_pass2_rld_lookup(const BYTE* data, bool global)
{
    DWORD lkname = *((DWORD*)data);
    WORD lkwd = global ? (SY_SEC | Globals.SEGNUM) : 0;
    WORD lkmsk = global ? ~(SY_SEC | SY_SEG) : ~SY_SEC;

    int index;
    bool found = symbol_table_lookup(lkname, lkwd, lkmsk, &index);
    if (!found && !global)
    {
        lkwd = SY_SEC; // LOOKUP SECTION NAME IN ROOT
        found = symbol_table_lookup(lkname, lkwd, lkmsk, &index);
    }
    if (!found) // MUST FIND IT THIS TIME
        fatal_error("ERR46: Invalid RLD symbol '%s'", unrad50(lkname));

    SymbolTableEntry* entry = SymbolTable + index;
    WORD R3 = entry->value; // GET SYMBOL'S VALUE
    //TODO
}

void process_pass2_rld(const SaveStatusEntry* sscur, const BYTE* data)
{
    assert(data != NULL);

    WORD blocksize = ((WORD*)data)[1];
    WORD offset = 6;  data += 6;
    while (offset < blocksize)
    {
        BYTE command = *data;  data++;  offset++;  // CMD BYTE OF RLD
        BYTE disbyte = *data;  data++;  offset++;  // DISPLACEMENT BYTE

        WORD constdata;
        switch (command & 0177)
        {
        case 1:  // See LINK7\RLDIR
            printf("      Item type 001 INTERNAL RELOCATION  %03o %06o\n", (int)disbyte, *((WORD*)data));
            data += 2;  offset += 2;
            break;
        case 2:
            printf("      Item type 002 GLOBAL\n");
            data += 4;  offset += 4;
            break;
        case 3:
            printf("      Item type 003 INTERNAL DISPLACED\n");
            data += 2;  offset += 2;
            break;
        case 4:  // See LINK7\RLDGDR
            printf("      Item type 004 GLOBAL DISPLACED  %03o '%s'\n", (int)disbyte, unrad50(*((DWORD*)data)));
            process_pass2_rld_lookup(data, false);
            data += 4;  offset += 4;
            break;
        case 5:
            printf("      Item type 005 GLOBAL ADDITIVE\n");
            data += 6;  offset += 6;
            break;
        case 6:
            constdata = ((WORD*)data)[2];
            printf("      Item type 006 GLOBAL ADDITIVE DISPLACED  %03o '%s' %06o\n", (int)disbyte, unrad50(*((DWORD*)data)), constdata);
            data += 6;  offset += 6;
            break;
        case 7:
            constdata = ((WORD*)data)[2];
            printf("      Item type 007 LOCATION COUNTER DEFINITION  %03o '%s' %06o\n", (int)disbyte, unrad50(*((DWORD*)data)), constdata);
            data += 6;  offset += 6;
            break;
        case 010:
            constdata = ((WORD*)data)[0];
            printf("      Item type 010 LOCATION COUNTER MODIFICATION\n  %06o", constdata);
            data += 2;  offset += 2;
            break;
        case 011:
            printf("      Item type 011 SET PROGRAM LIMITS\n");
            break;
        case 012:
            printf("      Item type 012 PSECT  '%s'\n", unrad50(*((DWORD*)data)));
            data += 4;  offset += 4;
            break;
        case 014:
            printf("      Item type 014 PSECT DISPLACED  '%s'\n", unrad50(*((DWORD*)data)));
            data += 4;  offset += 4;
            break;
        case 015:
            printf("      Item type 015 PSECT ADDITIVE\n");
            data += 6;  offset += 6;
            break;
        case 016:
            printf("      Item type 016 PSECT ADDITIVE DISPLACED\n");
            data += 6;  offset += 6;
            break;
        case 017:
            printf("      Item type 017 COMPLEX\n");
            data += 4;  offset += 4;  //TODO: length is variable
            break;
        default:
            fatal_error("ERR36: Unknown RLD command: %d\n", (int)command);
        }
    }
}

void print_symbol_table()
{
    printf("  SymbolTable count = %d.\n", SymbolTableCount);
    for (int i = 0; i < SymbolTableSize; i++)
    {
        const SymbolTableEntry* entry = SymbolTable + i;
        if (entry->name == 0)
            continue;
        printf("    %06o '%s' %06o %06o %06o  ", i, unrad50(entry->name), entry->flagseg, entry->value, entry->status);
        if (entry->flagseg & SY_SEC) printf("SECT ");
        if (entry->status & SY_UDF) printf("UNDEF ");
        if (entry->status & SY_IND) printf("IND ");
        if ((entry->flagseg & SY_SEC) == 0 && (entry->status & SY_WK)) printf("WEAK ");
        if ((entry->flagseg & SY_SEC) && (entry->status & SY_SAV)) printf("SAV ");
        printf("\n");
    }
    printf("  UNDLST = %06o\n", Globals.UNDLST);
}

// Prapare SYSCOM area, pass 2 initialization; see LINK6
void process_pass2_init()
{
    printf("Pass 2 initialization\n");

    print_symbol_table();

    // Allocate space for .SAV file image
    OutputBufferSize = 65536;
    OutputBuffer = (BYTE*) malloc(OutputBufferSize);
    if (OutputBuffer == NULL)
        fatal_error("ERR11: Failed to allocate memory for output buffer.\n");
    memset(OutputBuffer, 0, OutputBufferSize);

    // See LINK6\DOCASH
    Globals.LRUNUM = 0; // INIT LEAST RECENTLY USED TIME STAMP
    //TODO

    *((WORD*)(OutputBuffer + SysCom_BEGIN)) = Globals.BEGBLK.value; // PROG START ADDR

    if (Globals.STKBLK[0] != 0)
    {
        WORD lkwd = 0; // MUST BE A GLOBAL SYMBOL
        //TODO: Lookup for the symbol address
    }
    else
    {
        *((WORD*)(OutputBuffer + SysCom_STACK)) = Globals.STKBLK[2];
    }
    //TODO: *((WORD*)(OutputBuffer + SysCom_HIGH)) = ???

    //TODO: For /K switch STORE IT AT LOC. 56 IN SYSCOM AREA

    //TODO: SYSCOM AREA FOR REL FILE

    //TODO: BINOUT REQUESTED?
    // Prepare SAV file name
    char savfilename[64];
    memcpy(savfilename, SaveStatusArea[0].filename, 64);
    char* pext = strrchr(savfilename, '.');
    pext++; *pext++ = 'S'; *pext++ = 'A'; *pext++ = 'V';

    // Open SAV file for writing
    assert(outfileobj == NULL);
    outfileobj = fopen(savfilename, "wb");
    if (outfileobj == NULL)
        fatal_error("ERR6: Failed to open %s file, errno %d.\n", savfilename, errno);

    // See LINK6\INITP2
    //TODO: Some code for REL
    Globals.VIRSIZ = 0;
    //TODO: ARE WE DOING I-D SPACE?
    //TODO: /V OVERLAY, OR /XM?
    Globals.SEGBLK = 0;
    //TODO: INIT FOR LIBRARY PROCESSING
    //TODO: RESET NUMBER OF ENTRIES IN MODULE SECTION TBL
    Globals.LIBNB = 1;

    //TODO: FORCE BASE OF ZERO FOR VSECT IF ANY
}

void process_pass2_dump_txtblk()  // See TDMP0
{
    if (Globals.TXTLEN == 0)
        return;

    WORD addr = *((WORD*)Globals.TXTBLK);
    WORD baseaddr = 512; //Globals.BASE;
    BYTE* dest = OutputBuffer + (baseaddr + addr);
    BYTE* src = Globals.TXTBLK + 2;
    memcpy(dest, src, Globals.TXTLEN);

    Globals.TXTLEN = 0;
}

// See LINK7\PASS2 -- PRODUCE SAVE IMAGE FILE
void process_pass2()
{
    printf("Pass 2 started\n");
    for (int i = 0; i < SaveStatusCount; i++)
    {
        SaveStatusEntry* sscur = SaveStatusArea + i;
        assert(sscur->fileobj == NULL);
        assert(sscur->data != NULL);

        printf("  Processing %s\n", sscur->filename);
        int offset = 0;
        while (offset < sscur->filesize)
        {
            BYTE* data = (BYTE*)(sscur->data) + offset;
            WORD* dataw = (WORD*)(data);
            if (*dataw != 1)
            {
                if (*dataw == 0)  // Possibly that is filler at the end of the file
                {
                    while (*data == 0 && offset < sscur->filesize)
                    {
                        data++; offset++;
                    }
                    if (offset == sscur->filesize)
                        break;  // End of file
                }
                fatal_error("Unexpected word %06o at %06o in %s\n", *dataw, offset, sscur->filename);
            }

            WORD blocksize = ((WORD*)data)[1];
            WORD blocktype = ((WORD*)data)[2];

            if (blocktype == 0 || blocktype > 8)
                fatal_error("ERR4: Illegal record type at %06o in %s\n", offset, sscur->filename);
            else if (blocktype == 1)
            {
                printf("    Block type 1 - GSD at %06o size %06o\n", offset, blocksize);
                //TODO
            }
            else if (blocktype == 3)  // See LINK7\DMPTXT
            {
                process_pass2_dump_txtblk();

                WORD addr = ((WORD*)data)[3];
                WORD datasize = blocksize - 8;
                printf("    Block type 3 - TXT at %06o size %06o addr %06o len %06o\n", offset, blocksize, addr, datasize);
                Globals.TXTLEN = datasize;
                assert(datasize <= sizeof(Globals.TXTBLK));
                memcpy(Globals.TXTBLK, data + 6, blocksize - 6);
            }
            else if (blocktype == 4)  // See LINK\RLD
            {
                printf("    Block type 4 - RLD at %06o size %06o\n", offset, blocksize);
                process_pass2_rld(sscur, data);
            }
            else if (blocktype == 6)  // See LINK7\MODND
            {
                printf("    Block type 6 - ENDMOD at %06o size %06o\n", offset, blocksize);

                process_pass2_dump_txtblk();
                //TODO
            }
            else if (blocktype == 7)  // See LINK7\LIBPA2
            {
                printf("    Block type 7 - TITLIB at %06o size %06o\n", offset, blocksize);
                //TODO
                break;  // Skip for now
            }

            data += blocksize; offset += blocksize;
            data += 1; offset += 1;  // Skip checksum
        }
    }
}

int main(int argc, char *argv[])
{
    printf("PCLINK11  %s  %s\n", APP_VERSION_STRING, __DATE__);

    assert(sizeof(BYTE) == 1);
    assert(sizeof(WORD) == 2);
    assert(sizeof(DWORD) == 4);

    //assert(sizeof(SymbolTableEntry) == 10);

    if (argc <= 1)
    {
        print_help();
        exit(EXIT_FAILURE);
    }

    atexit(finalize);

    initialize();

    parse_commandline(argc, argv);

    read_files();

    Globals.PAS1_5 = 0;  // PASS 1 PHASE INDICATOR
    process_pass1();

    //TODO: Check if we need Pass 1.5
    Globals.PAS1_5 = 0200;
    process_pass15();

    process_pass_map_init();
    process_pass_map_output();

    process_pass2_init();
    process_pass2();
    //TODO: Pass 2.5

    size_t byteswrit = fwrite(OutputBuffer, 1, OutputBufferSize, outfileobj);
    if (byteswrit != OutputBufferSize)
        fatal_error("ERR6: Failed to write output file.\n");

    printf("SUCCESS\n");
    exit(EXIT_SUCCESS);
}


/////////////////////////////////////////////////////////////////////////////
