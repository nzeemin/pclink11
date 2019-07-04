
#define _CRT_SECURE_NO_WARNINGS

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <stdarg.h>
#include <time.h>
#include <errno.h>

char objfilename[256] = { 0 };
size_t objfilesize;
void* objfiledata = nullptr;


/////////////////////////////////////////////////////////////////////////////


#define MAKEDWORD(lo, hi)  ((uint32_t)(((uint16_t)((uint32_t)(lo) & 0xffff)) | ((uint32_t)((uint16_t)((uint32_t)(hi) & 0xffff))) << 16))

#define LOWORD(l)          ((uint16_t)(((uint32_t)(l)) & 0xffff))
#define HIWORD(l)          ((uint16_t)((((uint32_t)(l)) >> 16) & 0xffff))

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

// Macros used to mark and detect unimplemented but needed
#define NOTIMPLEMENTED { printf("*** NOT IMPLEMENTED, line %d\n", __LINE__); }

static char radtbl[] = " ABCDEFGHIJKLMNOPQRSTUVWXYZ$. 0123456789";

static char unrad50buffer[7];

// Decodes 3 chars of RAD50 into the given buffer
void unrad50(uint16_t word, char *cp)
{
    if (word < 0175000)              /* Is it legal RAD50? */
    {
        cp[0] = radtbl[word / 03100];
        cp[1] = radtbl[(word / 050) % 050];
        cp[2] = radtbl[word % 050];
    }
    else
        cp[0] = cp[1] = cp[2] = ' ';
}

// Decodes 6 chars of RAD50 into the temp buffer and returns buffer address
const char* unrad50(uint16_t loword, uint16_t hiword)
{
    memset(unrad50buffer, 0, sizeof(unrad50buffer));
    unrad50(loword, unrad50buffer);
    unrad50(hiword, unrad50buffer + 3);
    return unrad50buffer;
}

// Decodes 6 chars of RAD50 into the temp buffer and returns buffer address
const char* unrad50(uint32_t data)
{
    memset(unrad50buffer, 0, sizeof(unrad50buffer));
    unrad50(LOWORD(data), unrad50buffer);
    unrad50(HIWORD(data), unrad50buffer + 3);
    return unrad50buffer;
}

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


/////////////////////////////////////////////////////////////////////////////


void read_files()
{
    FILE* file = fopen(objfilename, "rb");
    if (file == nullptr)
        fatal_error("Failed to open input file: %s, errno %d.\n", objfilename, errno);

    fseek(file, 0L, SEEK_END);
    objfilesize = ftell(file);

    objfiledata = malloc(objfilesize);
    if (objfiledata == nullptr)
        fatal_error("Failed to allocate memory for input file %s.\n", objfilename);

    fseek(file, 0L, SEEK_SET);
    size_t bytesread = fread(objfiledata, 1, objfilesize, file);
    if (bytesread != objfilesize)
        fatal_error("Failed to read input file %s.\n", objfilename);

    fclose(file);
}

void dumpobj_gsd_item(const uint16_t* itemw)
{
    uint32_t itemnamerad50 = MAKEDWORD(itemw[0], itemw[1]);
    int itemtype = (itemw[2] >> 8) & 0xff;
    int itemflags = (itemw[2] & 0377);

    printf("      GSD '%s' type %d - %s", unrad50(itemnamerad50), itemtype, (itemtype > 7) ? "UNKNOWN" : GSDItemTypeNames[itemtype]);
    switch (itemtype)
    {
    case 0: // 0 - MODULE NAME FROM .TITLE
        printf("\n");
        break;
    case 2: // 2 - ISD ENTRY
        printf("\n");
        break;
    case 3: // 3 - TRANSFER ADDRESS
        printf(" %06ho\n", itemw[3]);
        break;
    case 4: // 4 - GLOBAL SYMBOL
        printf(" flags %03o addr %06ho\n", itemflags, itemw[3]);
        break;
    case 1: // 1 - CSECT NAME
        printf(" %06ho\n", itemw[3]);
        break;
    case 5: // 5 - PSECT NAME
        printf(" flags %03o maxlen %06ho\n", itemflags, itemw[3]);
        break;
    case 6: // 6 - IDENT DEFINITION
        printf(" '%s'\n", unrad50(itemnamerad50));
        break;
    case 7: // 7 - VIRTUAL SECTION
        printf("\n");
        break;
    default:
        printf("\n");
        fatal_error("Bad GSD type %d found in %s.\n", itemtype, objfilename);
    }
}

void dumpobj_gsd_block(uint8_t* data)
{
    assert(data != nullptr);

    uint16_t blocksize = ((uint16_t*)data)[1];
    int itemcount = (blocksize - 6) / 8;

    for (int i = 0; i < itemcount; i++)
    {
        const uint16_t* itemw = (const uint16_t*)(data + 6 + 8 * i);

        dumpobj_gsd_item(itemw);
    }
}

// COMPLEX RELOCATION STRING PROCESSING (GLOBAL ARITHMETIC), see LINK7\RLDCPX
uint16_t dumpobj_rld_complex(uint8_t* &data, uint16_t &offset, uint16_t blocksize)
{
    bool cpxbreak = false;
    uint16_t cpxresult = 0;
    const int cpxstacksize = 16;
    uint16_t cpxstack[cpxstacksize];  memset(cpxstack, 0, sizeof(cpxstack));
    while (!cpxbreak && offset < blocksize)
    {
        uint8_t cpxcmd = *data;  data += 1;  offset += 1;
        uint8_t cpxsect;
        uint16_t cpxval;
        uint32_t cpxname;
        printf("        Complex cmd %03ho %s", (uint16_t)cpxcmd, (cpxcmd > 020) ? "UNKNOWN" : CPXCommandNames[cpxcmd]);
        switch (cpxcmd)
        {
        case 000:  // NOP
        case 001:  // ADD -- ADD TOP 2 ITEMS
        case 002:  // SUBTRACT -- NEGATE TOP ITEM ON STACK
        case 003:  // MULTIPLY
        case 004:  // DIVIDE
        case 005:  // AND
        case 006:  // OR
        case 007:  // XOR
        case 010:  // NEGATE -- DECREMENT TOP ITEM
        case 011:  // COMPLEMENT -- COMPLEMENT TOP ITEM
            printf("\n");
            break;
        case 012:  // STORE NOT DISPLACED
        case 013:  // STORE DISPLACED
            printf("\n");
            cpxbreak = true;
            break;
        case 014:  // ILLEGAL FORMAT
        case 015:  // ILLEGAL FORMAT
            printf("\n");
            break;
        case 016:  // PUSH GLOBAL SYMBOL VALUE
            cpxname = MAKEDWORD(((uint16_t*)data)[0], ((uint16_t*)data)[1]);
            data += 4;  offset += 4;
            printf(" '%s'\n", unrad50(cpxname));
            break;
        case 017:  // PUSH RELOCATABLE VALUE
            // GET SECTION NUMBER
            cpxsect = *data;  data += 1;  offset += 1;
            cpxval = *((uint16_t*)data);  data += 2;  offset += 2;  // GET OFFSET WITHIN SECTION
            printf(" %03ho %06ho\n", (uint16_t)cpxsect, cpxval);
            break;
        case 020:  // PUSH CONSTANT
            cpxval = *((uint16_t*)data);  data += 2;  offset += 2;
            printf(" %06ho\n", cpxval);
            break;
        default:
            printf("\n");
            fatal_error("Unknown complex relocation command %03ho in %s\n", (uint16_t)cpxcmd, objfilename);
        }
    }
    return cpxresult;
}

void dumpobj_rld_block(uint8_t* data)
{
    assert(data != nullptr);

    uint16_t blocksize = ((uint16_t*)data)[1];
    uint16_t offset = 6;  data += 6;
    while (offset < blocksize)
    {
        uint8_t command = *data;  data++;  offset++;  // CMD BYTE OF RLD
        data++;  offset++;  // DISPLACEMENT BYTE

        printf("      RLD type %03ho %s",
               (uint16_t)(command & 0177), ((command & 0177) > 017) ? "UNKNOWN" : RLDCommandNames[command & 0177]);
        uint16_t constdata;
        switch (command & 0177)
        {
        case 001:  // INTERNAL REL, see LINK7\RLDIR
            // DIRECT POINTER TO AN ADDRESS WITHIN A MODULE. THE CURRENT SECTION BASE ADDRESS IS ADDED
            // TO A SPECIFIED CONSTANT ANT THE RESULT STORED IN THE IMAGE FILE AT THE CALCULATED ADDRESS
            // (I.E., DISPLACEMENT BYTE ADDED TO VALUE CALCULATED FROM THE LOAD ADDRESS OF THE PREVIOUS TEXT BLOCK).
            printf(" %06ho\n", *((uint16_t*)data));
            data += 2;  offset += 2;
            break;
        case 002:  // GLOBAL
        case 012:  // PSECT
            // RELOCATES A DIRECT POINTER TO A GLOBAL SYMBOL. THE VALUE OF THE GLOBAL SYMBOL IS OBTAINED & STORED.
            printf(" '%s'\n", unrad50(*((uint32_t*)data)));
            data += 4;  offset += 4;
            break;
        case 003:  // INTERNAL DISPLACED
            // RELATIVE REFERENCE TO AN ABSOLUTE ADDRESS FROM WITHIN A RELOCATABLE SECTION.
            // THE ADDRESS + 2 THAT THE RELOCATED VALUE IS TO BE WRITTEN INTO IS SUBTRACTED FROM THE SPECIFIED CONSTANT & RESULTS STORED.
            constdata = ((uint16_t*)data)[0];
            printf(" %06ho\n", constdata);
            data += 2;  offset += 2;
            break;
        case 004:  // GLOBAL DISPLACED, see LINK7\RLDGDR
        case 014:  // PSECT DISPLACED
            printf(" '%s'\n", unrad50(*((uint32_t*)data)));
            data += 4;  offset += 4;
            break;
        case 005:  // GLOBAL ADDITIVE REL
        case 015:  // PSECT ADDITIVE
            // RELOCATED A DIRECT POINTER TO A GLOBAL SYMBOL WITH AN ADDITIVE CONSTANT
            // THE SYMBOL VALUE IS ADDED TO THE SPECIFIED CONSTANT & STORED.
            constdata = ((uint16_t*)data)[2];
            printf(" '%s' %06ho\n", unrad50(*((uint32_t*)data)), constdata);
            data += 6;  offset += 6;
            break;
        case 006:  // GLOBAL ADDITIVE DISPLACED
        case 016:  // PSECT ADDITIVE DISPLACED
            // RELATIVE REFERENCE TO A GLOBAL SYMBOL WITH AN ADDITIVE CONSTANT.
            // THE GLOBAL VALUE AND THE CONSTANT ARE ADDED. THE ADDRESS + 2 THAT THE RELOCATED VALUE IS
            // TO BE WRITTEN INTO IS SUBTRACTED FROM THE RESULTANT ADDITIVE VALUE & STORED.
            constdata = ((uint16_t*)data)[2];
            printf(" '%s' %06ho\n", unrad50(*((uint32_t*)data)), constdata);
            data += 6;  offset += 6;
            break;
        case 007:  // LOCATION COUNTER DEFINITION, see LINK7\RLDLCD
            // DECLARES A CURRENT SECTION & LOCATION COUNTER VALUE
            constdata = ((uint16_t*)data)[2];
            printf(" '%s' %06ho\n", unrad50(*((uint32_t*)data)), constdata);
            data += 6;  offset += 6;
            break;
        case 010:  // LOCATION COUNTER MODIFICATION, see LINK7\RLDLCM
            // THE CURRENT SECTION BASE IS ADDED TO THE SPECIFIED CONSTANT & RESULT IS STORED AS THE CURRENT LOCATION CTR.
            constdata = ((uint16_t*)data)[0];
            printf(" %06ho\n", constdata);
            data += 2;  offset += 2;
            break;
        case 011:  // SET PROGRAM LIMITS
            printf("\n");
            break;
        case 017:  // COMPLEX RELOCATION STRING PROCESSING (GLOBAL ARITHMETIC)
            printf("\n");
            dumpobj_rld_complex(data, offset, blocksize);
            break;
        default:
            fatal_error("Unknown RLD command: %d\n", (int)command);
        }
    }
}

void dumpobj_titlib_block(uint8_t* data, uint16_t eptsize)
{
    int eptcount = eptsize / 8;
    for (int eptno = 0; eptno < eptcount; eptno++)
    {
        const uint16_t* itemw = (const uint16_t*)(data + 8 * eptno);
        uint32_t itemnamerad50 = MAKEDWORD(itemw[0], itemw[1]);
        if (itemnamerad50 == 0)
            continue;
        uint16_t block = itemw[2];
        uint16_t offset = itemw[3] & 0777;
        printf("      EPT '%s' block %06o offset %06o\n", unrad50(itemnamerad50), block, offset);
    }
}

void dumpobj()
{
    printf("Processing %s size %06o\n", objfilename, (unsigned int)objfilesize);
    size_t offset = 0;
    while (offset < objfilesize)
    {
        uint8_t* data = (uint8_t*)(objfiledata) + offset;
        uint16_t* dataw = (uint16_t*)(data);
        if (*dataw != 1)
        {
            // Possibly that is filler at the end of the block
            size_t offsetnext = (offset + 511) & ~511;
            size_t shift = offsetnext - offset;
            data += shift; offset += shift;
            if (offset >= objfilesize)
                break;  // End of file
            dataw = (uint16_t*)(data);
            if (*dataw != 1)
                fatal_error("Unexpected word %06ho at %06ho in %s\n", *dataw, offset, objfilename);
        }

        uint16_t blocksize = ((uint16_t*)data)[1];
        uint16_t blocktype = ((uint16_t*)data)[2];

        if (blocktype == 0 || blocktype > 8)
            fatal_error("Illegal record type at %06ho in %s\n", offset, objfilename);
        else if (blocktype == 1)  // 1 - START GSD RECORD
        {
            int itemcount = (blocksize - 6) / 8;
            printf("  Block type 1 - GSD at %06ho size %06ho itemcount %d\n", (uint16_t)offset, blocksize, itemcount);
            dumpobj_gsd_block(data);
        }
        else if (blocktype == 2)  // 2 - ENDGSD
        {
            printf("  Block type 2 - ENDGSD at %06ho size %06ho\n", (uint16_t)offset, blocksize);
        }
        else if (blocktype == 3)  // 3 - TXT
        {
            uint16_t addr = ((uint16_t*)data)[3];
            uint16_t datasize = blocksize - 8;
            printf("  Block type 3 - TXT at %06ho size %06ho addr %06ho len %06ho\n", (uint16_t)offset, blocksize, addr, datasize);
            //dumpobj_txt_block(data);
        }
        else if (blocktype == 4)  // 4 - RLD
        {
            printf("  Block type 4 - RLD at %06ho size %06ho\n", (uint16_t)offset, blocksize);
            dumpobj_rld_block(data);
        }
        else if (blocktype == 5)  // 5 - ISD
        {
            printf("  Block type 5 - ISD at %06ho size %06ho\n", (uint16_t)offset, blocksize);
            //dumpobj_isd_block(data);
        }
        else if (blocktype == 6)  // 6 - MODULE END
        {
            printf("  Block type 6 - ENDMOD at %06ho size %06ho\n", (uint16_t)offset, blocksize);
        }
        else if (blocktype == 7)  // 7 - TITLIB
        {
            printf("  Block type 7 - TITLIB at %06ho size %06ho\n", (uint16_t)offset, blocksize);
            uint16_t eptsize = *(uint16_t*)(data + 24/*L_HEAB*/);  // EPT SIZE IN LIBRARY HEADER
            printf("      EPT size %06ho bytes, %d. records\n", eptsize, (int)(eptsize / 8));
            //data += 32/*L_HEPT*/; offset += 32/*L_HEPT*/;  // Move to 1ST EPT ENTRY
            dumpobj_titlib_block(data + 32, eptsize);
            data += 32 + eptsize; offset += 32 + eptsize;
            continue;
        }
        else if (blocktype == 8)
        {
            printf("  Block type 10 - ENDLIB at %06ho size %06ho\n", (uint16_t)offset, blocksize);
        }

        data += blocksize; offset += blocksize;
        data += 1; offset += 1;  // Skip checksum
    }
}

void parse_commandline(int argc, char **argv)
{
    for (int arg = 1; arg < argc; arg++)
    {
        const char* argvcur = argv[arg];

        strcpy(objfilename, argvcur);
    }

    //TODO: Check if objfilename specified
}

int main(int argc, char *argv[])
{
    if (argc <= 1)
    {
        printf("\n");
        printf("dumpobj utility  by Nikita Zimin  [%s %s]\n", __DATE__, __TIME__);
        printf(
            "Usage: objdump <objfile>\n");

        exit(EXIT_FAILURE);
    }

    parse_commandline(argc, argv);

    read_files();

    dumpobj();
}

/////////////////////////////////////////////////////////////////////////////
