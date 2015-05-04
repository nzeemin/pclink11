
#define _CRT_SECURE_NO_WARNINGS

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <stdarg.h>

#include "main.h"


/////////////////////////////////////////////////////////////////////////////

const int LMLSIZ = 0525; // DEFAULT NUMBER OF LIBRARY MOD LIST ENTRIES
						 // EACH ENTRY IS 6 BYTES LONG

const WORD LINPPG = 60;	// NUMBER OF LINES PER PAGE FOR MAP

const int RECSIZ = 128;	// MAXIMUM SIZE OF A FORMATTED BINARY RECORD IN BYTES
						// DOES NOT COUNT LEADING 1ST WORD OF 1 OR THE CHECKSUM BYTE

// .SBTTL	****	OBJECT LIBRARY HEADER DEFINITIONS
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

//.SBTTL	****	COMMAND STRING SWITCH MASKS
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
const WORD SW_D = 1; // DUPLICATE SYMBOL
const WORD SW_J = 2; // GENERATE SEPARATED I-D SPACE .SAV IMAGE

//.SBTTL	****	IDSWIT BIT MASKS
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

//.SBTTL	****	ADDITIONAL FLAG WORD BIT MASKS
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

//.SBTTL	****	GSD ENTRY STRUCTURE
struct GSDentry
{
	DWORD	symbol;		// SYMBOL CHARS 1-6(RAD50)
	BYTE	flags;		// FLAGS
	BYTE	code;		// CODE BYTE
	WORD	value;		// SIZE OR OFFSET
};

//.SBTTL	****	SYMBOL TABLE STRUCTURE
struct SymbolTableEntry
{
	DWORD	name;		// 2 WD RAD50 NAME
	WORD	flags;
	WORD	value;
	WORD	status;
};

SymbolTableEntry* SymbolTable = NULL;
const int SymbolTableSize = 4095;  // STSIZE
const int SymbolTableLength = SymbolTableSize * sizeof(SymbolTableEntry);
int SymbolTableCount = 0;  // STCNT -- SYMBOL TBL ENTRIES COUNTER

//.SBTTL	****	INTERNAL SYMBOL TABLE FLAGS BIT ASSIGNMENT
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

BYTE* OutputBuffer = NULL;
int OutputBufferSize = 0;
FILE* outfileobj = NULL;


/////////////////////////////////////////////////////////////////////////////


struct tagGlobals
{
	BYTE	TXTBLK[RECSIZ];  // SPACE FOR A FORMATTED BINARY RECORD

	WORD	FLGWD;	// INTERNAL FLAG WORD
	WORD	SEGBLK;	// BASE BLK OF OVERLAY SEGMENT
	WORD	SEGBAS;	// BASE OF OVERLAY SEGMENT
	WORD	ENDOL;	// USE FOR CONTINUE SWITCHES /C OR //
	WORD	SEGNUM;	// KEEP TRACK OF INPUT SEGMENT #'S
	WORD	ENDRT;	// END OF ROOT SYMBOL TBL LIST
	WORD	TXTLEN;	// TEMP FOR /V SWITCH
	WORD	CBPTR;	// DEFAULT IS NO CREF

	BYTE	LIBNB;  // LIBRARY FILE NUMBER FOR LML

	WORD	SWIT1;	// Switches

	WORD	PAS1_5; // PASS 1.5 SWITCH(0 ON PASS1, 1 IF TO DO LIBRARY,
					// BIT 7 SET IF DOING LIBRARIES

	WORD	LRUNUM;	// USED TO COUNT MAX # OF SECTIONS AND AS
					//  TIME STAMP FOR SAV FILE CACHING

	WORD	BASE;	// BASE OF CURRENT SECTION

	WORD	VIRSIZ;	// LARGEST REGION IN A PARTITION

	DWORD	MODNAM;	// MODULE NAME, RAD50
	DWORD	IDENT;	// PROGRAM IDENTIFICATION

	GSDentry BEGBLK; // TRANSFER ADDRESS BLOCK (4 WD GSD ENTRY)
					//   TRANS ADDR OR REL OFFSET FROM PSECT
}
Globals;


/////////////////////////////////////////////////////////////////////////////


void print_help()
{
    printf("\n");
	//
    printf("Usage: link11 <input files>\n");
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
	printf("Initialization\n");

	memset(&Globals, 0, sizeof(Globals));

	memset(SaveStatusArea, 0, sizeof(SaveStatusArea));
	SaveStatusCount = 0;

	SymbolTable = (SymbolTableEntry*) ::malloc(SymbolTableLength);
	memset(SymbolTable, 0, SymbolTableLength);
	SymbolTableCount = 0;
}

void finalize()
{
	printf("Finalization\n");
	
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
		printf("  File closed: %s\n", sscur->filename);
	}

	if (OutputBuffer != NULL)
	{
		free(OutputBuffer);  OutputBuffer = NULL;  OutputBufferSize = 0;
	}
	if (outfileobj != NULL)
	{
		fclose(outfileobj);  outfileobj = NULL;
	}
}

void parse_commandline(int argc, char **argv)
{
	for (int arg = 1; arg < argc; arg++)
	{
		const char* argvcur = argv[arg];

		if (*argvcur == '/')  // Parse global arguments
		{
		    //TODO: Parse arguments like Command String Interpreter
			const char* cur = argvcur + 1;
			while (*cur != 0)
			{
				char option = toupper(*cur);
				switch (option)
				{
				//case 'T':
				//	break;
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
				if (filenamelen > sizeof(sscur->filename))
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

// /T - SPECIFY TRANSFER ADR
// /M - MODIFY INITIAL STACK
// /B - SPECIFY BOTTOM ADR FOR LINK
// /U - ROUND SECTION
// /E - EXTEND SECTION
// /Y - START SECTION ON MULTIPLE OF VALUE
// /H - SPECIFY TOP ADR FOR LINK
// /K - SPECIFY MINIMUM SIZE
// /P:N  SIZE OF LML TABLE
// /Z - ZERO UNFILLED LOCATIONS
// /R - INDICATE FOREGROUND LINK
// /XM, OR /V ON 1ST LINE
// /L - INDICATE LDA OUTPUT
// /W - SPECIFY WIDE MAP LISTING
// /C - CONTINUE ON ANOTHER LINE
// /
// /X - DO NOT EMIT BIT MAP
// /I - INCLUDE MODULES FROM LIBRARY
// /F - INCLUDE FORLIB.OBJ IN LINK
// /A - ALPHABETIZE MAP
// /S - SYMBOL TABLE AS LARGE AS POSSIBLE
// /D - ALLOW DUPLICATE SYMBOLS
// /N - GENERATE CROSS REFERENCE
// /G - CALC. EPT SIZE ON RT-11
// /Q - SET PSECTS TO ABSOLUTE ADDRESSES
// /J -USE SEPARATED I-D SPACE
}

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

	//printf("        Saving entry: index %4d name '%s'\n", index, unrad50(lkname));

	// Save the entry
	SymbolTableCount++;
	entry->name = lkname;
	entry->flags = lkwd;
	*pindex = index;
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
	{  // 0 = BLANK NAME
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
			WORD flagsmasked = (entry->flags & ~lkmsk);
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
		printf("  File read %s, %d bytes.\n", sscur->filename, bytesread);

		fclose(file);
		//printf("  File closed: %s\n", sscur->filename);
		sscur->fileobj = NULL;
	}
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
		case 1: // 1 - CSECT NAME, see LINK3\CSECNM
			printf("      Item '%s' type 1 - CSECT NAME  %06o\n", buffer, itemw3);
			{
				DWORD lkname = MAKEDWORD(itemw0, itemw1);
				WORD lkwd = SY_SEC;
				WORD lkmsk = ~SY_SEC;
				if (lkname == 0)  // BLANK .CSECT = .PSECT ,LCL,REL,I,CON,RW
					{}
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

				Globals.LRUNUM++; // COUNT SECTIONS IN A MODULE

				// SECTION NAME LOOKUP
				int index;
				symbol_table_looke(lkname, lkwd, lkmsk, &index);
			}
			break;
		case 2: // 2 - ISD ENTRY (IGNORED), see LINK3\ISDNAM
			printf("      Item '%s' type 2 - ISD ENTRY, ignored\n", buffer);
			break;
		case 3: // 3 - TRANSFER ADDRESS; see LINK3\TADDR
			printf("      Item '%s' type 3 - TRANSFER ADDR %06o\n", buffer, itemw3);
			//WORD taddr = itemw3;
			if ((itemw3 & 1) != 0)  // USE ONLY 1ST EVEN ONE ENCOUNTERED
			{
				DWORD lkname = MAKEDWORD(itemw0, itemw1);
				int index;
				if (!symbol_table_lookup(lkname, SY_SEC, ~SY_SEC, &index))
					fatal_error("ERR31: Transfer address for '%s' undefined or in overlay.\n", buffer);
				//TODO
			}
			break;
		case 4: // 4 - GLOBAL SYMBOL, see LINK3\SYMNAM
			printf("      Item '%s' type 4 - GLOBAL SYMBOL flags %03o addr %06o\n", buffer, itemflags, itemw3);
			{
				DWORD lkname = MAKEDWORD(itemw0, itemw1);
				WORD lkwd = 0;
				WORD lkmsk = ~SY_SEC;
				//TODO: IS SYMBOL DEFINED HERE?
				if (Globals.SEGNUM == 0) // REFERENCE FROM ROOT?
				{
					int index;
					symbol_table_dlooke(lkname, lkwd, lkmsk, &index);
					SymbolTableEntry* entry = SymbolTable + index;
					//TODO: IS IT A DUP SYMBOL?
				}
				else // NORMAL LOOKUP
				{
					int index;
					symbol_table_looke(lkname, lkwd, lkmsk, &index);
				}
			}
			break;
		case 5: // 5 - PSECT NAME; see LINK3\PSECNM
			printf("      Item '%s' type 5 - PSECT NAME flags %03o maxlen %06o\n", buffer, itemflags, itemw3);
			{
				Globals.LRUNUM++; // COUNT SECTIONS IN A MODULE

				DWORD lkname = MAKEDWORD(itemw0, itemw1);
				WORD lkwd = SY_SEC;
				lkwd |= Globals.SEGNUM;
				WORD lkmsk = ~(SY_SEC+SY_SEG);

				// SECTION NAME LOOKUP
				int index;
				symbol_table_looke(lkname, lkwd, lkmsk, &index);
				SymbolTableEntry* entry = SymbolTable + index;
				Globals.BASE = entry->value;
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

void process_pass1()
{
	printf("Pass 1 started\n");
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
			if (blocktype == 7)  // See LINK3\LIBRA
			{
				printf("    Block type 7 - TITLIB at %06o size %06o\n", offset, blocksize);
				WORD eptsize = *(WORD*)(data + L_HEAB);
				printf("      EPT size %06o bytes, %d. records\n", eptsize, (int)(eptsize / 8));
				//TODO: Test /X switch library flag
				data += L_HEPT; offset += L_HEPT;  // Move to 1ST EPT ENTRY
				//TODO: Resolve undefined symbols using EPT
				break;  // Skip for now
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
		case 4:
			printf("      Item type 004 GLOBAL DISPLACED  %03o '%s'\n", (int)disbyte, unrad50(*((DWORD*)data)));
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

void process_pass2_init()
{
	printf("Pass 2 initialization\n");

	// Dump SymbolTable
	printf("  SymbolTable count=%d.\n", SymbolTableCount);
	for (int i = 0; i < SymbolTableSize; i++)
	{
		const SymbolTableEntry* entry = SymbolTable + i;
		if (entry->name == 0)
			continue;
		printf("    '%s' %06o %06o %06o\n", unrad50(entry->name), entry->flags, entry->value, entry->status);
	}

	// Allocate space for .SAV file image
	OutputBufferSize = 65536;
	OutputBuffer = (BYTE*) malloc(OutputBufferSize);
	if (OutputBuffer == NULL)
		fatal_error("ERR11: Failed to allocate memory for output buffer.\n");
	memset(OutputBuffer, 0, OutputBufferSize);

	// See LINK6\DOCASH
	*((WORD*)(OutputBuffer + SysCom_BEGIN)) = Globals.BEGBLK.value;
	//TODO: if (Globals.STKBLK != 0)
	//TODO: *((WORD*)(OutputBuffer + SysCom_STACK)) = ???
	//TODO: *((WORD*)(OutputBuffer + SysCom_HIGH)) = ???
	//TODO: For /K switch STORE IT AT LOC. 56 IN SYSCOM AREA

	//TODO: SYSCOM AREA FOR REL FILE

	assert(outfileobj == NULL);
	outfileobj = fopen("OUTPUT.SAV", "wb");
	if (outfileobj == NULL)
		fatal_error("ERR6: Failed to open output .SAV file, errno %d.\n", errno);

	// See LINK6\INITP2
	//TODO: Some code for REL
	Globals.VIRSIZ = 0;
	Globals.SEGBLK = 0;
	//TODO: INIT FOR LIBRARY PROCESSING
	//TODO: RESET NUMBER OF ENTRIES IN MODULE SECTION TBL
	Globals.LIBNB = 1;

	//.SBTTL	-	FORCE BASE OF ZERO FOR VSECT IF ANY
	//TODO
}

void process_pass2_dump_txtblk()  // See TDMP0
{
	if (Globals.TXTLEN == 0)
		return;

	WORD addr = *((WORD*)Globals.TXTBLK);
	WORD baseaddr = 512;  //TODO: Globals.BASE
	BYTE* dest = OutputBuffer + (baseaddr + addr);
	BYTE* src = Globals.TXTBLK + 2;
	memcpy(dest, src, Globals.TXTLEN);

	Globals.TXTLEN = 0;
}

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

	Globals.PAS1_5 = 0;
	process_pass1();

	//TODO: Check if we need Pass 1.5
	Globals.PAS1_5 = 0200;
	process_pass15();

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
