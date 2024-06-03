
#define _CRT_SECURE_NO_WARNINGS

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cctype>
#include <cassert>
#include <cstdarg>
#include <cerrno>

#include "main.h"

#if defined(_DEBUG) && defined(_MSC_VER)
#include <crtdbg.h>
#endif


/////////////////////////////////////////////////////////////////////////////


static const char* FORLIB = "FORLIB.OBJ";  // FORTRAN LIBRARY FILENAME
//static const char* SYSLIB = "SYSLIB.OBJ";  // DEFAULT SYSTEM LIBRARY FILENAME

uint8_t* OutputBuffer = nullptr;
size_t OutputBufferSize = 0;
int OutputBlockCount = 0;

FILE* outfileobj = nullptr;
FILE* mapfileobj = nullptr;
FILE* stbfileobj = nullptr;

struct tagGlobals Globals;

char outfilename[PATH_MAX + 1] = { 0 };

void println()
{
    putchar('\n');
}


/////////////////////////////////////////////////////////////////////////////


const uint32_t RAD50_ABS = rad50x2(". ABS.");  // ASECT


/////////////////////////////////////////////////////////////////////////////


// Initialize arrays and variables
void initialize()
{
    memset(&Globals, 0, sizeof(Globals));

    memset(SaveStatusArea, 0, SaveStatusAreaSize * sizeof(SaveStatusEntry));
    SaveStatusCount = 0;

    SymbolTable = (SymbolTableEntry*) ::calloc(SymbolTableSize, sizeof(SymbolTableEntry));
    SymbolTableCount = 0;

    RelocationTable = (RELEntry*) :: calloc(RelocationTableSize, sizeof(RELEntry));
    RelocationTableCount = 0;

    memset(LibraryModuleList, 0, LibraryModuleListSize * sizeof(LibraryModuleEntry));

    // Set globals defaults, see LINK1\START1
    Globals.NUMCOL = 3; // 3-COLUMN MAP IS NORMAL
    Globals.NUMBUF = 3; // NUMBER OF AVAILABLE CACHING BLOCKS (DEF=3)
    Globals.BEGBLK.symbol = RAD50_ABS;
    Globals.BEGBLK.code = 5;
    Globals.BEGBLK.flags = 0100/*CS$GBL*/ | 04/*CS$ALO*/;
    Globals.BEGBLK.value = 000001;  // INITIAL TRANSFER ADR TO 1
    //TODO
    Globals.DBOTTM = 01000;  // INITIAL BOTTOM ADR
    Globals.BOTTOM = 01000;  // INITIAL D-SPACE BOTTOM ADR
    Globals.KSWVAL = 128/*RELSTK*/;  // DEFAULT REL FILE STACK SIZE
}

// Free all memory, close all files
void finalize()
{
    //printf("Finalization\n");

    if (SymbolTable != nullptr)
    {
        free(SymbolTable);  SymbolTable = nullptr;
    }

    if (RelocationTable != nullptr)
    {
        free(RelocationTable); RelocationTable = nullptr;
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
void prepare_read_files()
{
    const size_t MAX_INPUTFILE_SIZE = 512 * 1024;  // 512 KB max

    for (int i = 0; i < SaveStatusCount; i++)
    {
        SaveStatusEntry* sscur = SaveStatusArea + i;
        assert(sscur->filename[0] != 0);

        FILE* file = fopen(sscur->filename, "rb");
        if (file == nullptr)
            fatal_error("ERR2: Failed to open input file: %s, error %d: %s.\n", sscur->filename, errno, strerror(errno));

        fseek(file, 0L, SEEK_END);
        size_t filesize = ftell(file);
        if (filesize > MAX_INPUTFILE_SIZE)
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

void parse_commandline_option(const char* cur)
{
    assert(cur != nullptr);

    uint16_t param1, param2;

    // /EXECUTE:filespec - Specifies the name of the memory image file
    if (strncmp(cur, "EXECUTE:", 8) == 0) //TODO: or /SAV
    {
        strcpy(outfilename, cur + 8);
        //TODO: Validate the name as a proper filename
        return;
    }

    // /WIDE /W - SPECIFY WIDE MAP LISTING
    if (strcmp(cur, "WIDE") == 0 || strcmp(cur, "W") == 0)
    {
        Globals.NUMCOL = 6; // 6 COLUMNS
        //Globals.LSTFMT--; // WIDE CREF
        return;
    }

    // /NOBITMAP /X - DO NOT EMIT BIT MAP
    if (strcmp(cur, "NOBITMAP") == 0 || strcmp(cur, "X") == 0)
    {
        Globals.SWITCH |= SW_X;
        return;
    }

    // /SYMBOLTABLE /STB - Generates a symbol table file
    if (strcmp(cur, "SYMBOLTABLE") == 0 || strcmp(cur, "STB") == 0)
    {
        Globals.FlagSTB = true;
        return;
    }

    // /MAP - Generates map file
    if (strcmp(cur, "MAP") == 0)
    {
        Globals.FlagMAP = true;
        return;
    }

    // /ALPHABETIZE /A - ALPHABETIZE MAP
    if (strcmp(cur, "ALPHABETIZE") == 0 || strcmp(cur, "A") == 0)
    {
        Globals.SWITCH |= SW_A;
        return;
    }

    // /FOREGROUND /R[:stacksize] - INDICATE FOREGROUND LINK
    if (strcmp(cur, "FOREGROUND") == 0 || strcmp(cur, "R") == 0)
    {
        //result = sscanf(cur, ":%ho", &param1);
        if ((Globals.SWITCH & (SW_B | SW_H | SW_K | SW_L)) != 0)
            fatal_error("FOREGROUND mode is illegal with -B, -H, -K or LDA mode");
        Globals.SWITCH |= SW_R;
        //TODO: stacksize
        return;
    }

    // /L - INDICATE LDA OUTPUT
    if (strcmp(cur, "LDA") == 0 || strcmp(cur, "L") == 0)
    {
        // /L IS ILLEGAL FOR FOREGROUND LINKS
        if ((Globals.SWITCH & SW_R) != 0)
            fatal_error("LDA mode is illegal for FOREGROUND links");
        //TODO: IS /V SET? -> YES, ILLEGAL COMBINATION
        Globals.SWITCH |= SW_L;
        return;
    }

    // /F - INCLUDE FORLIB.OBJ IN LINK
    if (strcmp(cur, "FORLIB") == 0 || strcmp(cur, "F") == 0)
    {
        Globals.SWITCH |= SW_F;
        return;
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
        //TODO: Ability to assign a symbol like /M:SYMBOL
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
        if (param1 & 1)
            fatal_error("Invalid /B option value, use even address\n");
        Globals.SWITCH |= SW_B;
        Globals.BOTTOM = param1;
        Globals.DBOTTM = param1;
        break;

    case 'H':  // /H:addr - SPECIFY TOP ADR FOR LINK
        result = sscanf(cur, ":%ho", &param1);
        if (result < 1)
            fatal_error("Invalid /H option, use /H:addr\n");
        if (param1 & 1)
            fatal_error("Invalid /H option value, use even address\n");
        Globals.SWITCH |= SW_H;
        Globals.HSWVAL = param1;
        Globals.DHSWVL = param1;
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

        //case 'V':  // /XM, OR /V ON 1ST LINE
        //    result = sscanf(cur, ":%ho", &param1);
        //    //TODO
        //    break;

        //case 'I':  // /I - INCLUDE MODULES FROM LIBRARY
        //    Globals.SWITCH |= SW_I;
        //    break;

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

        //case 'J':  // /J - USE SEPARATED I-D SPACE
        //    if (Globals.SWITCH & SW_R)
        //        fatal_error("Invalid option: /R illegal with /J\n"); //TODO: Should be warning only
        //    else
        //        Globals.SWIT1 |= SW_J;
        //    break;

    default:
        fatal_error("Unknown command line option '%c'\n", option);
    }
}

void parse_commandline_filename(const char * filename)
{
    if (SaveStatusCount == SaveStatusAreaSize)
        fatal_error("Too many files specified.\n");

    SaveStatusEntry* sscur = SaveStatusArea + SaveStatusCount;

    // Parse filename
    char* filenamecur = sscur->filename;
    int filenamelen = 0;
    const char* cur = filename;
    while (*cur != 0 && (isalnum(*cur) || *cur == '.' || *cur == '_' || *cur == '-') || *cur == PATH_SEPARATOR_CHAR)
    {
        *filenamecur = *cur;
        filenamecur++;  cur++;
        filenamelen++;
        if (filenamelen >= sizeof(sscur->filename) - 1)
            fatal_error("Too long filename: %s\n", filename);
    }
    SaveStatusCount++;
}

bool g_okHelpRequested = false;
bool g_okVersionRequested = false;

// PROCESS COMMAND STRING SWITCHES, see LINK1\SWLOOP
void parse_commandline(int argc, char **argv)
{
    assert(argv != nullptr);

    //uint16_t TMPIDD = 0;  // D BIT TO TEST FOR /J PROCESSING
    //uint16_t TMPIDI = 0;  // I BIT TO TEST FOR /J PROCESSING

    for (int arg = 1; arg < argc; arg++)
    {
        const char* argvcur = argv[arg];

        if (strcmp(argvcur, "--help") == 0)
        {
            g_okHelpRequested = true;
            break;
        }
        if (strcmp(argvcur, "--version") == 0)
        {
            g_okVersionRequested = true;
            break;
        }

        if (*argvcur == '-'
#ifdef _MSC_VER
            || *argvcur == '/'
#endif
           )  // Parse global arguments
        {
            //TODO: Parse arguments like Command String Interpreter
            const char* cur = argvcur + 1;
            if (*cur != 0)
            {
                parse_commandline_option(cur);
            }
        }
        else  // Parse filename and arguments
        {
            parse_commandline_filename(argvcur);
        }
    }

    if (g_okHelpRequested || g_okVersionRequested)
        return;

    if (Globals.SWITCH & SW_F)
    {
        parse_commandline_filename(FORLIB); // INCLUDE FORLIB
    }

    // Validate command line params
    if (SaveStatusCount == 0)
        fatal_error("Input file not specified.\n");
}

void print_help()
{
    printf("\n"
           "Usage: pclink11 <input files and options, space-separated>\n"
           "Options:\n"
           "  -EXECUTE:filespec  Specifies the name of the memory image file\n"
           "  -T:addr       Specifies the starting address of the linked program\n"
           "  -M:addr       Specifies the stack address for the linked program\n"
           "  -B:addr       Specifies the lowest address to be used by the linked program\n"
           "  -H:addr       Specifies the highest address to be used by the linked program\n"
           "  -NOBITMAP    -X    Do not emit bit map\n"
           "  -WIDE        -W    Produces a load map that is 132 columns wide\n"
           "  -ALPHABETIZE -A    Lists global symbols on the link map in alphabetical order\n"
           "  -SYMBOLTABLE -STB  Generates a symbol table file (.STB file)\n"
           "  -MAP               Generates map file\n"
           "  -FORLIB      -F    Include FORLIB.OBJ\n"
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
    if (g_okHelpRequested)
    {
        print_help();
        exit(EXIT_SUCCESS);
    }
    if (g_okVersionRequested)
    {
        printf(
            "Cross-linker, porting PDP-11 LINK to C/C++, WIP\n"
            "Ported in 2019-2022 by Nikita Zimin\n"
            "License LGPLv3: GNU Lesser General Public License v3.0 https://www.gnu.org/licenses/lgpl-3.0.html\n"
            "Source code: https://github.com/nzeemin/pclink11\n");
        exit(EXIT_SUCCESS);
    }

    prepare_read_files();

    printf("PASS 1\n");
    Globals.PAS1_5 = 0;  // PASS 1 PHASE INDICATOR
    process_pass1();
    if (Globals.PAS1_5 & 1)  // BIT 0 SET IF TO DO 1.5 (we have library files)
    {
        printf("PASS 1.5\n");
        Globals.PAS1_5 = 128;
        process_pass15();  // SCANS ONLY LIBRARIES
    }
    process_pass1_endp1();

    //print_symbol_table();//DEBUG
    printf("PASS MAP\n");
    assert(mapfileobj == nullptr);
    process_pass_map_init();
    process_pass_map_output();
    process_pass_map_done();
    assert(mapfileobj == nullptr);
    assert(stbfileobj == nullptr);
    assert(outfileobj == nullptr);
    assert(OutputBuffer == nullptr);

    //print_symbol_table();//DEBUG
    printf("PASS 2\n");
    process_pass2_init();
    assert(outfileobj != nullptr);
    assert(OutputBuffer != nullptr);
    Globals.PAS1_5 = 0;
    process_pass2();  // Non-library pass
    if (Globals.PAS1_5 & 1)  // BIT 0 SET IF TO DO 1.5 (we have library files)
    {
        printf("PASS 2.5\n");
        Globals.PAS1_5 |= 128;  // MARK BEGINING OF 2ND HALF OF PASS
        process_pass2();  // Library pass
    }
    process_pass2_done();
    assert(outfileobj == nullptr);

    printf("SUCCESS\n");
    finalize();
    assert(mapfileobj == nullptr);
    assert(stbfileobj == nullptr);
    assert(outfileobj == nullptr);
    assert(OutputBuffer == nullptr);

#if defined(_DEBUG) && defined(_MSC_VER)
    if (_CrtDumpMemoryLeaks())
        printf("ERROR: Memory leak detected\n");
#endif

    return EXIT_SUCCESS;
}


/////////////////////////////////////////////////////////////////////////////
