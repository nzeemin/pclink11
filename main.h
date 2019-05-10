
#include <stdint.h>


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


/////////////////////////////////////////////////////////////////////////////

const int LMLSIZ = 0525; // DEFAULT NUMBER OF LIBRARY MOD LIST ENTRIES
// EACH ENTRY IS 6 BYTES LONG

const uint16_t LINPPG = 60;	// NUMBER OF LINES PER PAGE FOR MAP

const int RECSIZ = 128;	// MAXIMUM SIZE OF A FORMATTED BINARY RECORD IN BYTES
// DOES NOT COUNT LEADING 1ST WORD OF 1 OR THE CHECKSUM BYTE

// ****	OBJECT LIBRARY HEADER DEFINITIONS
enum
{
    F_BRHC = 1,		// FORMATTED BINARY BLOCK FOR LIBRARY
    L_HLEN = 042,	// RECORD LENGTH OF LIBRARY HEADER BLOCK
    L_BR   = 7,		// LIBRARIAN CODE
    L_HVER = 0305,	// LIBRARY VERSION NUMBER (VERSION # = V03.05)
    L_HEPT = 040,	// END OF LBR HEADER (1ST EPT ENTRY SYMBOL NAME)
    L_HX   = 010,	// OFFSET OF /X FLAG IN LIBRARY HEADER
    L_HEAB = 030,	// OFFSET OF EPT SIZE IN LIBRARY HEADER
};

// ****	COMMAND STRING SWITCH MASKS
// THE FOLLOWING ARE GLOBAL MASKS FOR THE "SWITCH" WORD IN THE ROOT SEGMENT,
// INDICATING VARIOUS SWITCHES WHICH WERE SPECIFIED IN THE COMMAND STRING.
enum
{
    SW_A =      01,	// ALPHABETIZE LOAD MAP ENTRIES
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

// ****	IDSWIT BIT MASKS
// THE FOLLOWING ARE BIT ASSIGNMENTS FOR VARIOUS FLAGS IN IDSWIT.
// IDSWIT IS ONLY USED IF /J IS USED.
// IDSWIT LOW BYTE IS FOR D-SPACE
enum
{
    D_SWB =  01,	// /B SWITCH FOR D-SPACE
    D_SWE =	 02,	// /E SWITCH FOR D-SPACE
    D_SWH =	 04,	// /H SWITCH FOR D-SPACE
    D_SWU =	010,	// /U SWITCH FOR D-SPACE
    D_SWY =	020,	// /Y SWITCH FOR D-SPACE
    D_SWZ =	040,	// /Z SWITCH FOR D-SPACE
};
// IDSWIT HI BYTE IS FOR I-SPACE
enum
{
    I_SWB =   0400,	// /B SWITCH FOR I-SPACE
    I_SWE =  01000,	// /E SWITCH FOR I-SPACE
    I_SWH =  02000,	// /H SWITCH FOR I-SPACE
    I_SWU =  04000,	// /U SWITCH FOR I-SPACE
    I_SWY = 010000,	// /Y SWITCH FOR I-SPACE
    I_SWZ = 020000,	// /Z SWITCH FOR I-SPACE
};

// ****	ADDITIONAL FLAG WORD BIT MASKS
// THE FOLLOWING ARE BIT ASSIGNMENTS FOR VARIOUS FLAGS IN "FLGWD" WORD.
enum  // THE FOLLOWING ARE BIT ASSIGNMENTS FOR VARIOUS FLAGS IN "FLGWD" WORD.
{
    XM_OVR =      01,	// SET IF /V OVERLAYS ARE USED
    SW_V   =      02,	// SET IF /XM OR /V ON FIRST LINE ONLY
    HD_EPT =      04,	// SET IF EPT HEADER IS NOT CURRENTLY IN CORE
    LB_OBJ =     010,	// SET WHEN PROCESSING A LIBRARY FILE
    AD_LML =     020,	// SET TO ADD TO LML LATER
    FG_LIB =     040,	// SET WHEN FORLIB OR SYSLIB ENTERED IN SAVESTATUS AREA
    NO_SYS =    0100,	// SET WHEN SYSLIB NOT FOUND
    FG_OVR =    0200,	// INDICATE PROGRAM IS OVERLAYED	*** MUST BE BIT 7 ***
    FG_NWM =    0400,	// SET SO NO WARNING MSG FOR FILE NOT FOUND ON SYSLIB
    FG_STB =   01000,	// STB FILE REQUESTED
    FG_XX  =   02000,	// DO NOT DUMP CCB (USED WITH /X SWITCH)
    FG_TT  =   04000,	// MAP OUTPUT IS TO TT:
    FG_IP  =  010000,	// /I LIBRARY PASS INDICATOR
    PA_SS2 = 0100000,	// SET WHEN DOING PASS 2		*** MUST BE BIT 15 ***
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


/////////////////////////////////////////////////////////////////////////////


const char* unrad50(uint32_t data);
const char* unrad50(uint16_t loword, uint16_t hiword);
void unrad50(uint16_t word, char *cp);

uint16_t rad50(const char *cp, const char **endp);
uint32_t rad50x2(const char *cp);

void print_symbol_table();


/////////////////////////////////////////////////////////////////////////////
