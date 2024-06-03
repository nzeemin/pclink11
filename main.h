
#include <stdint.h>
#include <limits.h>

#ifndef PATH_MAX
#define PATH_MAX    _MAX_PATH
#endif

#ifdef _MSC_VER
const char PATH_SEPARATOR_CHAR = '\\';
#else
const char PATH_SEPARATOR_CHAR = '/';
#endif


/////////////////////////////////////////////////////////////////////////////
// Type definitions

#define MAKEDWORD(lo, hi)  ((uint32_t)(((uint16_t)((uint32_t)(lo) & 0xffff)) | ((uint32_t)((uint16_t)((uint32_t)(hi) & 0xffff))) << 16))

#define LOWORD(l)          ((uint16_t)(((uint32_t)(l)) & 0xffff))
#define HIWORD(l)          ((uint16_t)((((uint32_t)(l)) >> 16) & 0xffff))
#define LOBYTE(w)          ((uint8_t)(((uint32_t)(w)) & 0xff))
#define HIBYTE(w)          ((uint8_t)((((uint32_t)(w)) >> 8) & 0xff))


/////////////////////////////////////////////////////////////////////////////

#ifdef _DEBUG
#define APP_VERSION_STRING "DEBUG"
#else
#include "version.h"
#endif

// Macros used to mark and detect unimplemented but needed
#define NOTIMPLEMENTED { printf("*** NOT IMPLEMENTED at %s:%d ***\n", __FILE__, __LINE__); }

void println();  // printf("\n");


/////////////////////////////////////////////////////////////////////////////
// util.cpp

const char* unrad50(uint32_t data);
const char* unrad50(uint16_t loword, uint16_t hiword);
void unrad50(uint16_t word, char *cp);

uint16_t rad50(const char *cp, const char **endp);
uint32_t rad50x2(const char *cp);

[[noreturn]]
void fatal_error(const char* message, ...);

void warning_message(const char* message, ...);


/////////////////////////////////////////////////////////////////////////////

//const uint16_t LINPPG = 60; // NUMBER OF LINES PER PAGE FOR MAP

const int RECSIZ = 128; // MAXIMUM SIZE OF A FORMATTED BINARY RECORD IN BYTES
// DOES NOT COUNT LEADING 1ST WORD OF 1 OR THE CHECKSUM BYTE

// **** OBJECT LIBRARY HEADER DEFINITIONS
enum
{
    F_BRHC = 1,     // FORMATTED BINARY BLOCK FOR LIBRARY
    L_HLEN = 042,   // RECORD LENGTH OF LIBRARY HEADER BLOCK
    L_BR   = 7,     // LIBRARIAN CODE
    L_HVER = 0305,  // LIBRARY VERSION NUMBER (VERSION # = V03.05)
    L_HEPT = 040,   // END OF LBR HEADER (1ST EPT ENTRY SYMBOL NAME)
    L_HX   = 010,   // OFFSET OF /X FLAG IN LIBRARY HEADER
    L_HEAB = 030,   // OFFSET OF EPT SIZE IN LIBRARY HEADER
};

// **** COMMAND STRING SWITCH MASKS
// THE FOLLOWING ARE GLOBAL MASKS FOR THE "SWITCH" WORD IN THE ROOT SEGMENT,
// INDICATING VARIOUS SWITCHES WHICH WERE SPECIFIED IN THE COMMAND STRING.
enum
{
    SW_A =      01, // ALPHABETIZE LOAD MAP ENTRIES
    SW_B =      02, // SPECIFY BOTTOM ADRS (N)
    SW_C =      04, // CONTINUE INPUT LINE
    SW_E =     010, // EXTEND PROGRAM SECTION (NAME & SIZE)
    SW_F =     020, // USE FORTRAN LIBRARY "SY:FORLIB.OBJ"
    SW_I =     040, // INCLUDE SYMBOLS TO BE SEARCHED FROM LIBRARY
    SW_H =    0100, // LINK TO HIGH ADR (N)
    SW_R =    0200, // OUTPUT REL FORMAT & STACK SIZE
    SW_K =    0400, // PUT VALUE (N) IN HEADER BLOCK OF IMAGE FILE
    SW_S =   01000, // MAXIMUM SYMBOL TABLE SIZE
    SW_M =   02000, // USER'S STACK ADR (NUMBER OR NAME)
    SW_T =   04000, // TRANSFER ADR (NUMBER OR NAME)
    SW_U =  010000, // ROUND SECTION (N POWER OF 2)
    SW_X =  020000, // DON'T XMIT BITMAP
    SW_Y =  040000, // START SECTION ON MULTIPLE OF VALUE GIVEN
    SW_L = 0100000, // OUTPUT FILE IN "LDA" FORMAT
};

// THE FOLLOWING ARE GLOBAL MASKS FOR THE "SWIT1" WORD IN THE ROOT SEGMENT,
// INDICATING VARIOUS SWITCHES WHICH WERE SPECIFIED IN THE COMMAND STRING.
const uint16_t SW_D = 1; // DUPLICATE SYMBOL
const uint16_t SW_J = 2; // GENERATE SEPARATED I-D SPACE .SAV IMAGE

// **** IDSWIT BIT MASKS
// THE FOLLOWING ARE BIT ASSIGNMENTS FOR VARIOUS FLAGS IN IDSWIT.
// IDSWIT IS ONLY USED IF /J IS USED.
// IDSWIT LOW BYTE IS FOR D-SPACE
enum
{
    D_SWB =  01,    // /B SWITCH FOR D-SPACE
    D_SWE =  02,    // /E SWITCH FOR D-SPACE
    D_SWH =  04,    // /H SWITCH FOR D-SPACE
    D_SWU = 010,    // /U SWITCH FOR D-SPACE
    D_SWY = 020,    // /Y SWITCH FOR D-SPACE
    D_SWZ = 040,    // /Z SWITCH FOR D-SPACE
};
// IDSWIT HI BYTE IS FOR I-SPACE
enum
{
    I_SWB =   0400, // /B SWITCH FOR I-SPACE
    I_SWE =  01000, // /E SWITCH FOR I-SPACE
    I_SWH =  02000, // /H SWITCH FOR I-SPACE
    I_SWU =  04000, // /U SWITCH FOR I-SPACE
    I_SWY = 010000, // /Y SWITCH FOR I-SPACE
    I_SWZ = 020000, // /Z SWITCH FOR I-SPACE
};

// **** ADDITIONAL FLAG WORD BIT MASKS
// THE FOLLOWING ARE BIT ASSIGNMENTS FOR VARIOUS FLAGS IN "FLGWD" WORD.
enum  // THE FOLLOWING ARE BIT ASSIGNMENTS FOR VARIOUS FLAGS IN "FLGWD" WORD.
{
    XM_OVR =      01,   // SET IF /V OVERLAYS ARE USED
    SW_V   =      02,   // SET IF /XM OR /V ON FIRST LINE ONLY
    HD_EPT =      04,   // SET IF EPT HEADER IS NOT CURRENTLY IN CORE
    LB_OBJ =     010,   // SET WHEN PROCESSING A LIBRARY FILE
    AD_LML =     020,   // SET TO ADD TO LML LATER
    FG_LIB =     040,   // SET WHEN FORLIB OR SYSLIB ENTERED IN SAVESTATUS AREA
    NO_SYS =    0100,   // SET WHEN SYSLIB NOT FOUND
    FG_OVR =    0200,   // INDICATE PROGRAM IS OVERLAYED *** MUST BE BIT 7 ***
    FG_NWM =    0400,   // SET SO NO WARNING MSG FOR FILE NOT FOUND ON SYSLIB
    //FG_STB =   01000,   // STB FILE REQUESTED (use Globals.FlagSTB instead)
    FG_XX  =   02000,   // DO NOT DUMP CCB (USED WITH /X SWITCH)
    FG_TT  =   04000,   // MAP OUTPUT IS TO TT:
    FG_IP  =  010000,   // /I LIBRARY PASS INDICATOR
    PA_SS2 = 0100000,   // SET WHEN DOING PASS 2  *** MUST BE BIT 15 ***
};

// OFFSETS INTO JOB'S SYSTEM COMMUNICATION AREA, see LINK1
enum
{
    SysCom_BEGIN  = 040,  // JOB STARTING ADDRESS
    SysCom_STACK  = 042,  // INITIAL VALUE OF STACK POINTER
    SysCom_JSW    = 044,  // JOB STATUS WORD
    SysCom_HIGH   = 050,  // PROGRAM'S HIGHEST AVAILABLE LOCATION(ROOT+/O OVERLAYS)
    SysCom_ERRBYT = 052,  // MONITOR ERROR INDICATOR
    SysCom_USERRB = 053,  // SYSTEM USER ERROR BYTE
    SysCom_RMON   = 054,  // CONTAINS THE START OF THE RESIDENT MONITOR
    SysCom_IDS    = 060,  // RAD50 IDS - I-D SPACE IDENTIFIER IN I-SPACE CCB
    SysCom_STOTAB = 064,  // START OF OVERLAY TABLE ($OTABL)

    SysCom_BITMAP = 0360,  // START OF CORE USE BITMAP OF IMAGE FILE
};

// **** INTERNAL SYMBOL TABLE FLAGS BIT ASSIGNMENT
const uint16_t SY_UDF = 0100000; // SET TO DECLARE SYMBOL IS UNDEFINED (PSECT NEVER UNDEFINED)
const uint16_t SY_DUP = 040000;  // SET TO ALLOW DUPLICATE LIBRARY SYMBOLS
const uint16_t SY_IND = 020000;  // SET TO PUT SYMBOL IN OVERLAY HANDLER TABLE
const uint16_t SY_WK  = 010000;  // SET TO INDICATE SYMBOL IS WEAK
const uint16_t SY_SAV = SY_WK;   // SET TO INDICATE PSECT HAS SAV ATTRIBUTE
const uint16_t SY_ENB = 0170000; // MASK TO ISOLATE SYMBOL ENTRY NUMBER PTR
const uint16_t SY_SEC = 04000;   // CS$NU POSITION IN PSECT FLAGS INDICATING SYMBOL IS A SECTION
const uint16_t SY_SPA = 0100000; // CS$TYP POSITION IN PSECT FLAGS INDICATING SYMBOL IS I OR D
// SPACE - 1 IF D PSECT, 0 IF I PSECT.  SY.SEC MUST ALSO BE
// SET IF THIS BIT IS USED.  NOTE SY.UDF IS NEVER USED FOR
// A PSECT
const uint16_t SY_SWI = 010000; // (CS$ACC)SET IF THIS SYMBOL PUT IN SYMBOL TABLE BY /I UNDEF SYMBOL
const uint16_t SY_SEG = 01777;  // SEGMENT NUMBER BITS IN FLAGS WORD


/////////////////////////////////////////////////////////////////////////////


struct RELEntry
{
    uint16_t addr;
    uint16_t value;
};
// **** GSD ENTRY STRUCTURE
struct GSDentry
{
    uint32_t    symbol;     // SYMBOL CHARS 1-6(RAD50)
    uint8_t     flags;      // FLAGS
    uint8_t     code;       // CODE BYTE
    uint16_t    value;      // SIZE OR OFFSET
};

struct SaveStatusEntry
{
    char     filename[PATH_MAX + 1];
    size_t   filesize;
    bool     islibrary;
    uint8_t* data;
};
const int SaveStatusAreaSize = 31;
extern SaveStatusEntry SaveStatusArea[];
extern int SaveStatusCount;

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
const int SymbolTableSize = 4095;  // STSIZE
extern SymbolTableEntry* SymbolTable;
extern SymbolTableEntry* ASECTentry;
extern int SymbolTableCount;  // STCNT -- SYMBOL TBL ENTRIES COUNTER

const int RelocationTableSize = 4096; // FIXME
extern RELEntry* RelocationTable;
extern int RelocationTableCount;

struct LibraryModuleEntry
{
    uint8_t  libfileno;     // LIBRARY FILE # (8 BITS) 1-255
    uint16_t relblockno;    // REL BLK # (15 BITS)
    uint16_t byteoffset;    // BYTE OFFSET (9 BITS)
    uint8_t  passno;        // Pass 0 - not passed yet
    uint16_t segmentno;     // SEGMENT NUMBER FOR THIS MODULE

    size_t offset() const { return relblockno * 512 + byteoffset; }
};
const int LibraryModuleListSize = 512; // NUMBER OF LIBRARY MOD LIST ENTRIES (0252 DEFAULT, 0525 FOR RSTS)
extern LibraryModuleEntry LibraryModuleList[];
extern int LibraryModuleCount;  // Count of records in LibraryModuleList, see LMLPTR

struct ModuleSectionEntry
{
    uint16_t stindex;        // SymbolTable index
    uint16_t size;           // Section size
};
const int ModuleSectionTableSize = 256;
extern ModuleSectionEntry ModuleSectionTable[];
extern int ModuleSectionCount;


struct tagGlobals
{
    //uint16_t    ODBLK[15]; // BLOCK TO HOLD BINOUT SPEC
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
    //uint16_t    OVCOUN; // NO. OF OVERLAY ENTRY PTS.
    //uint16_t    OVSPTR; // PTR TO OVERLAY SEGMENT BLK
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

    //uint16_t    ESWVAL; // /E SWITCH VALUE - I-SPACE
    //uint32_t    ESWNAM; // /E SWITCH NAME - I-SPACE
    //uint16_t    DESWVL; // /E SWITCH VALUE - D-SPACE
    //uint32_t    DESWNM; // /E SWITCH NAME - D-SPACE
    uint16_t    KSWVAL; // /K SWITCH VALUE OR STACK SIZE FOR REL FILE
    //uint16_t    YSWNAM[25]; // /Y SECTION NAME ARRAY(TEMP OVERLAY # IN OV1 & SAV)
    // +2 NEXT ASSIGNABLE OUTPUT BLK(LNKMAP)
    //    RELOCATION INFO BLOCK #(LNKSAV) - I-SPACE
    // YSWVAL==YSWNAM+4
    //uint16_t    DYSWNM[25];
    // DYSWVL==DYSWNM+4
    //uint8_t     YSWT;   // SAY WE ARE USING /Y
    //uint8_t     YCNT;   // NUMBER OF TIMES TO PROMPT FOR /Y (SET IN LINK2)
    uint16_t    DEFALT; // DEFAULT BOUNDARY VALUE FOR /Y (SET IN LINK2)
    //uint16_t    USWVAL; // /U SWITCH VALUE - I-SPACE
    //uint32_t    USWNAM; // /U SWITCH NAME - I-SPACE
    //uint16_t    DUSWVL; // /U SWITCH VALUE - D-SPACE
    //uint32_t    DUSWNM; // /U SWITCH NAME - D-SPACE
    //uint16_t    QSWVAL; // /Q BUFFER POINTER
    //uint16_t    ZSWVAL; // /Z SWITCH VALUE - I-SPACE
    //uint16_t    DZSWVL; // /Z SWITCH VALUE - D-SPACE
    uint16_t    LRUNUM; // USED TO COUNT MAX # OF SECTIONS AND AS
    //  TIME STAMP FOR SAV FILE CACHING
    //uint16_t    BITBAD; // -> START OF BITMAP TABLE (D-SPACE IF /J USED)
    //uint16_t    IBITBD; // -> START OF I-SPACE BITMAP TABLE
    uint8_t     BITMAP[16]; // CONTAINS BITMAP OR IBITBD (IF /J USED)
    //uint16_t    CACHER; // -> CACHE CONTROL BLOCKS
    uint16_t    NUMBUF; // NUMBER OF AVAILABLE CACHING BLOCKS (DEF=3)
    uint16_t    BASE;   // BASE OF CURRENT SECTION
    uint16_t    CKSUM;  // CHECKSUM FOR STB & LDA OUTPUT
    // LNKOV1->TEMP LINK POINTER TO NEW REGION BLK
    // CURRENT REL BLK OVERLAY NUM
    //uint16_t    ENDRT;  // END OF ROOT SYMBOL TBL LIST
    uint16_t    VIRSIZ; // LARGEST REGION IN A PARTITION
    //uint16_t    REGION; // XM REGION NUMBER
    //uint16_t    WDBCNT; // WDB TABLE SIZE ( 14. * NUMBER OF PARTITIONS)
    uint16_t    HIPHYS; // HIGH LIMIT FOR EXTENDED MEMORY (96K MAX)
    uint16_t    SVLML;  // START OF LML LIST FOR WHOLE LIBRARY
    uint16_t    SW_LML; // LML INTO OVERLAY SWITCH, AND PASS INDICATOR
    //uint16_t    LOC0;   // USED FOR CONTENTS OF LOC 0 IN SAV HEADER
    //uint16_t    LOC66;  // # /O SEGMENTS SAVED FOR CONVERSION TO ADDR OF
    //  /V SEGS IN OVERLAY HANLDER TABLE
    //uint16_t    LSTFMT; // CREF LISTING FORMAT (0=80COL, -1=132COL)

    // I-D SPACE VARIABLES

    //uint16_t    IDSWIT; // BITMASK WORD FOR WHICH SWITCHES USE SEPARATED
    // I-D SPACE
    // D-SPACE, LOW BYTE, I-SPACE, HI BYTE
    //uint16_t    ILEN;   // TOTAL LENGTH OF I-SPACE PSECTS IN WORDS
    //uint16_t    DLEN;   // TOTAL LENGTH OF D-SPACE PSECTS IN WORDS
    //uint16_t    IBLK;   // TOTAL LENGTH OF I-SPACE PSECTS IN BLOCKS
    //uint16_t    DBLK;   // TOTAL LENGTH OF D-SPACE PSECTS IN BLOCKS
    //uint16_t    IROOT;  // SIZE OF THE I-SPACE ROOT IN WORDS
    //uint16_t    DROOT;  // SIZE OF THE D-SPACE ROOT IN WORDS
    //uint16_t    IBASE;  // START OF THE I BASE (BLOCKS)
    //uint16_t    DBASE;  // START OF THE D BASE (BLOCKS)
    //uint16_t    ILOC40; // CONTENTS OF LOC 40 FOR I-SPACE CCB
    //uint16_t    IFLG;   // NON-ZERO MEANS WRITING I-SPACE BITMAP
    //uint16_t    IDSTRT; // I-D SPACE ENTRY POINT ($OVRHZ)
    //uint16_t    ZTAB;   // I-D SPACE START ADDRESS OF PSECT $ZTABL
    //uint16_t    OVRCNT; // # OF OVERLAY SEGMENTS, USED FOR COMPUTING $OVDF6
    //uint16_t    DSGBAS; // PASS 2 BASE ADR OF D-SPACE OVERLAY SEGMENT
    //uint16_t    DSGBLK; // PASS 2 BASE BLK OF D-SPACE OVERLAY SEGMENT

    uint32_t    MODNAM; // MODULE NAME, RAD50
    // LDA OUTPUT BUFR PTR OR REL INFO BUFR PTR
    uint32_t    IDENT;  // PROGRAM IDENTIFICATION
    // "RELADR" ADR OF RELOCATION CODE IN TEXT OF REL FILE
    // +2 "RELOVL" NEXT REL BLK OVERLAY #

    //uint16_t    ASECT[8];

    //uint16_t    DHLRT;  // D-SPACE HIGH ADDR LIMIT OF REGION (R.GHLD)
    uint16_t    DBOTTM; // ST ADDR OF REGION AREA - D-SPACE (R.GSAD)
    //uint16_t    DBOTTM_2; // REGION NUMBER (R.GNB)
    //uint16_t    OVRG1;  // -> NEXT ORDB (R.GNXP)
    //uint16_t    OVRG1_2; // -> OSDB THIS REGION (R.GSGP)
    //uint16_t    HLRT;   // HIGH LIMIT OF AREA (R.GHL)
    uint16_t    BOTTOM; // ST ADDR OF REGION AREA - (I-SPACE IF /J USED)

    //uint16_t    CBUF;   // START OF CREF BUFFER
    //uint16_t    CBEND;  // CBUF + 512. BYTES FOR A 1 BLOCK CREF BUFFER
    //uint16_t    QAREA[10]; // EXTRA QUEUE ELEMENT
    //uint16_t    PRAREA[5]; // AREA FOR PROGRAMMED REQUESTS

    //uint16_t    EIB512; // IBUF + 512. BYTES FOR A 1 BLOCK MAP BUFR
    uint16_t    SEGBAS; // BASE OF OVERLAY SEGMENT
    uint16_t    SEGBLK; // BASE BLK OF OVERLAY SEGMENT
    uint16_t    TXTLEN; // TEMP FOR /V SWITCH
    uint16_t    LINLFT; // NUMBER OF LINES LEFT ON CURRENT MAP PAGE

    // The following globals are defined inside the code

    uint16_t    FLGWD;  // INTERNAL FLAG WORD
    bool        FlagSTB;  // STB FILE REQUESTED
    bool        FlagMAP;  // MAP FILE REQUESTED
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
};
extern struct tagGlobals Globals;

extern uint8_t* OutputBuffer;
extern size_t OutputBufferSize;
extern int OutputBlockCount;

extern FILE* outfileobj;
extern FILE* mapfileobj;
extern FILE* stbfileobj;

extern char outfilename[];  // could be .SAV / .REL / .LDA or user-defined


/////////////////////////////////////////////////////////////////////////////


void symbol_table_enter(int* pindex, uint32_t lkname, uint16_t lkwd);

void symbol_table_delete(int index);

void symbol_table_add_undefined(int index);

void symbol_table_remove_undefined(int index);

bool is_any_undefined();

bool symbol_table_dlooke(uint32_t lkname, uint16_t lkwd, uint16_t lkmsk, int* pindex);

bool symbol_table_lookup(uint32_t lkname, uint16_t lkwd, uint16_t lkmsk, int* pindex);

bool symbol_table_looke(uint32_t lkname, uint16_t lkwd, uint16_t lkmsk, int* pindex);

bool symbol_table_search(uint32_t lkname, uint16_t lkwd, uint16_t lkmsk, int* pindex);

void print_lml_table();

void print_symbol_table();

void mst_table_clear();

void print_mst_table();


void process_pass1();

void process_pass15();

void process_pass1_endp1();

void process_pass_map_init();

void process_pass_map_done();

void process_pass_map_output();

void process_pass2_init();

void process_pass2();

void process_pass2_done();


/////////////////////////////////////////////////////////////////////////////
