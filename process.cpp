
#define _CRT_SECURE_NO_WARNINGS

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cassert>
#include <cstdarg>
#include <ctime>
#include <cerrno>

#include "main.h"


/////////////////////////////////////////////////////////////////////////////


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

static const char* GSDItemTypeNames[] =
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

static const char* RLDCommandNames[] =
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

static const char* CPXCommandNames[] =
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


void prepare_filename(char* buffer, const char* basefilename, const char* extension)
{
    assert(buffer != nullptr);
    assert(basefilename != nullptr);
    assert(*basefilename != 0);  // make sure we have the base filename
    assert(extension != nullptr);

    strcpy(buffer, basefilename);

    char* pext = strrchr(buffer, '.');
    if (pext == nullptr)
    {
        pext = buffer + strlen(buffer);
        *pext = '.';
    }
    pext++;  // skip the dot

    strcpy(pext, extension);
}


/////////////////////////////////////////////////////////////////////////////


// FORCE0 IS CALLED TO GENERATE THE FOLLOWING WARNING MESSAGE, see LINK3\FORCE0
// ?LINK-W-DUPLICATE SYMBOL "SYMBOL" IS FORCED TO THE ROOT
void pass1_force0(const SymbolTableEntry* entry)
{
    assert(entry != nullptr);

    printf("DUPLICATE SYMBOL \"%s\" IS FORCED TO THE ROOT\n", entry->unrad50name());
}

// LINK3\ORDER, LINK3\ALPHA
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
    bool alphabetize = (Globals.SWITCH & SW_A) != 0;
    for (;;)
    {
        int nextindex = preventry->nextindex();
        if (nextindex == 0)  // end of chain
            break;
        SymbolTableEntry* nextentry = SymbolTable + nextindex;
        if (nextentry->flagseg & SY_SEC)  // next entry is a section
            break;

        if (alphabetize)  // Compare 6-char RAD50 strings
        {
            if (LOWORD(nextentry->name) > LOWORD(entry->name))
                break;
            if (LOWORD(nextentry->name) == LOWORD(entry->name) && HIWORD(nextentry->name) > HIWORD(entry->name))
                break;
        }
        else
        {
            if (nextentry->value > entry->value)
                break;
        }

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

    if ((entry->flags() & 040/*CS$REL*/) == 0 ||
        (entry->flags() & 4/*CS$ALO*/) != 0 ||
        entry->value == 0)  // ABS section or OVERLAY SECTION, or CURRENT SIZE IS 0
    {
        Globals.BEGBLK.symbol = entry->name;  // NAME OF THE SECTION
        Globals.BEGBLK.flags = entry->flags();
        Globals.BEGBLK.code = entry->seg();
        Globals.BEGBLK.value = itemw[3];
        return;
    }

    // MUST SCAN CURRENT MODULE TO FIND SIZE CONTRIBUTION TO
    // TRANSFER ADDR SECTION TO CALCULATE PROPER OFFSET
    size_t offset = 0;
    while (offset < sscur->filesize)
    {
        const uint8_t* data = sscur->data + offset;
        uint16_t blocksize = ((uint16_t*)data)[1];
        if (blocksize == 0)
            break;
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

        if ((Globals.PAS1_5 & 128) != 0 && Globals.SEGNUM == 0) // ROOT DEF?
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
            if ((entry->status & SY_UDF) == 0)  // DEFINED BEFORE?
            {
                // SYMBOL WAS DEFINED BEFORE, ABSOLUTE SYMBOLS WITH SAME VALUE NOT MULTIPLY DEFINED
                if ((itemw[2] & 040/*SY$REL*/) != 0 || entry->value != itemw[3])
                    warning_message("ERR24: Multiple definition of symbol '%s'.\n", unrad50(lkname));
            }
            else  // DEFINES A REFERENCED SYMBOL, see LINK3\DEFREF\130$
            {
                symbol_table_remove_undefined(index);  // REMOVE ENTRY FROM UNDEFINED LIST
                entry->status &= ~(SY_UDF | SY_WK | 07777);  // CLR SY.UDF+SY.WK+^CSY.ENB
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

            SymbolTableEntry* entry = SymbolTable + index;
            if ((itemw[2] & 1/*SY$WK*/) != 0)  // IS REFERENCE WEAK?
                entry->status |= SY_WK;  // SET WEAK BIT
        }
        else // Symbol is found
        {
            SymbolTableEntry* entry = SymbolTable + index;
            if ((itemw[2] & 1/*SY$WK*/) == 0)  // IS THIS A STRONG REFERENCE?
                entry->status &= ~SY_WK;  // YES, SO MAKE SURE WEAK BIT IS CLEARED
            //TODO: GET SYMBOL FLAG WD & ISOLATE SEGMENT #
            //TODO: IF INPUT SEG # .NE. SYM SEG # THEN SET EXTERNAL REFERENCE BIT
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
    //uint16_t lkwd = (uint16_t)SY_SEC;
    //uint16_t lkmsk = (uint16_t)~SY_SEC;
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
void process_pass1_gsd_item_psecnm(uint32_t sectname, int itemflags, uint16_t sectsize)
{
    Globals.LRUNUM++; // COUNT SECTIONS IN A MODULE
    uint32_t lkname = sectname;
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
        entry->status |= SY_SAV; // INDICATE SAV ATTRIBUTE IN PSECT ENTRY
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
            fatal_error("ERR10: Conflicting section '%s' attributes. R=0x%x flags=0x%x\n", entry->unrad50name(), R2, itemflags);
    }

    // CHKRT
    if ((entry->flags() & 0400/*CS$GBL*/) != 0 && entry->seg() != 0)
    {
        NOTIMPLEMENTED;
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
    //if ((itemflags & 0040/*CS$REL*/) == 0 || (Globals.SWITCH & SW_X) != 0)
    //    sectsize = 0;
    if ((itemflags & 0200/*CS$TYP*/) == 0 || (itemflags & 0004/*CS$ALO*/) != 0)  // INSTRUCTION SECTION? CON SECTION ?
        sectsize = (sectsize + 1) & ~1;  // ROUND SECTION SIZE TO WORD BOUNDARY
    if ((itemflags & 0040/*CS$REL*/) == 0)  // ASECT
    {
        if (sectsize > entry->value)  // Size is maximum from all ASECT sections
            entry->value = sectsize;
        Globals.BASE = 0;
    }
    else if (itemflags & 4/*CS$ALO*/)  // OVERLAYED SECTION?
    {
        Globals.BASE = 0;  // OVR PSECT, GBL SYM OFFSET IS FROM START OF SECTION
        if (sectsize > entry->value)  // Size is maximum from all such sections
            entry->value = sectsize;
    }
    else
    {
        Globals.BASE = entry->value;
        entry->value += sectsize;
    }
}
void process_pass1_gsd_item_psecnm(const uint16_t* itemw, int itemflags)
{
    assert(itemw != nullptr);

    uint32_t sectname = MAKEDWORD(itemw[0], itemw[1]);
    uint16_t sectsize = itemw[3];
    process_pass1_gsd_item_psecnm(sectname, itemflags, sectsize);
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
        printf(", base %06ho\n", Globals.BASE);
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
        break;
    case 5: // 5 - PSECT NAME; see LINK3\PSECNM
        printf(" flags %03o maxlen %06ho\n", itemflags, itemw[3]);
        process_pass1_gsd_item_psecnm(itemw, itemflags);
        break;
    case 6: // 6 - IDENT DEFINITION; see LINK3\PGMIDN
        println();
        if (Globals.IDENT == 0)
            Globals.IDENT = itemnamerad50;
        break;
    case 7: // 7 - VIRTUAL SECTION; see LINK3\VSECNM
        {
            printf(" size %06ho\n", itemw[3]);
            uint16_t sectsize = itemw[3];
            // A VIRTUAL SECTION HAS BEEN FOUND WHICH IS PROCESSED SIMILAR TO
            //     .PSECT	. VIR.,RW,D,GBL,REL,CON
            // THE SIZE IS THE NUMBER OF 32 WORD AREAS REQUIRED.  THE SEGMENT # IS DEFINED AS THE ROOT (ZERO).
            // THERE WILL NEVER BE ANY GLOBALS UNDER THIS SECTION AND THE SECTION STARTS AT A BASE OF ZERO.
            itemflags = 0200/*CS$TYP*/ | 0100/*CS$GBL*/ | 040/*CS$REL*/;
            process_pass1_gsd_item_psecnm(RAD50_VSEC, itemflags, 0);
            // IF $VIRSZ ALREADY IN SYM TBL THEN JUST ADD SIZE OF CURRENT VSECT TO IT
            // ELSE ENTER NEW SYMBOL "$VIRSZ" WITH VALUE EQUAL TO CURRENT SIZE.
            uint32_t lkname = RAD50_VIRSZ;
            uint16_t lkmsk = (uint16_t)~SY_SEC;
            uint16_t lkwd = 0;
            int index;
            bool isnewentry = !symbol_table_looke(lkname, lkwd, lkmsk, &index);
            SymbolTableEntry* entry = SymbolTable + index;
            if (isnewentry)
            {
                entry->flagseg = 010/*SY$DEF*/ | (4/*GLOBAL SYMBOL*/ << 8);
                entry->value = sectsize;

                pass1_insert_entry_into_ordered_list(index, entry, true);
            }
            else
            {
                entry->value += Globals.BASE;
            }
        }
        break;
    default:
        println();
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

void process_pass1_file(SaveStatusEntry* sscur)
{
    assert(sscur != nullptr);
    assert(sscur->data != nullptr);

    printf("  Processing %s\n", sscur->filename);
    size_t offset = 0;
    while (offset < sscur->filesize - 1)
    {
        uint8_t* data = sscur->data + offset;
        uint16_t* dataw = (uint16_t*)(data);
        if (*dataw != 1)
        {
            if (*dataw == 0)  // Possibly that is filler at the end of the block
            {
                size_t offsetnext = (offset + 511) & ~511;
                while (offset < sscur->filesize && offset < offsetnext)
                {
                    if (*data != 0) break;
                    data++; offset++;
                }
                if (offset >= sscur->filesize)
                    break;  // End of file
            }
            dataw = (uint16_t*)(data);
            if (*dataw != 1)
                fatal_error("Unexpected word %06ho at %06o in %s\n", *dataw, (unsigned int)offset, sscur->filename);
        }

        uint16_t blocksize = ((uint16_t*)data)[1];
        uint16_t blocktype = ((uint16_t*)data)[2];

        if (blocktype == 0 || blocktype > 8)
            fatal_error("Illegal record type at %06o in %s\n", (unsigned int)offset, sscur->filename);
        else if (blocktype == 1)  // 1 - START GSD RECORD, see LINK3\GSD
        {
            printf("    Block type 1 - GSD at %06o size %06ho\n", (unsigned int)offset, blocksize);
            process_pass1_gsd_block(sscur, data);
        }
        else if (blocktype == 6)  // 6 - MODULE END, see LINK3\MODND
        {
            //printf("    Block type 6 - ENDMOD at %06o size %06ho\n", (unsigned int)offset, blocksize);
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

// PASS1: GSD PROCESSING, see LINK3\PASS1
void process_pass1()
{
    // PROCESS FORMATTED BINARY RECORDS, see LINK3\PA1
    for (int i = 0; i < SaveStatusCount; i++)
    {
        SaveStatusEntry* sscur = SaveStatusArea + i;

        process_pass1_file(sscur);
    }
}

// SEARCH THE ENTRY POINT TABLE FOR A MATCH OF THE INDICATED SYMBOL. See LINK3\EPTSER
const uint16_t* process_pass15_eptsearch(const uint8_t* data, uint16_t eptsize, uint32_t symbol)
{
    assert(data != nullptr);

    for (uint16_t eptno = 0; eptno < eptsize; eptno++)
    {
        const uint16_t* itemw = (uint16_t*)(data + 8 * eptno);
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
    lmlentry->passno = 0;
    lmlentry->segmentno = 0;  // SET SEGMNT NUMBER TO ZERO=ROOT
    //TODO

    LibraryModuleCount++;
}

// ORDER LIBRARY MODULE LIST. See LINK3\ORLIB
void process_pass15_lmlorder()
{
    if (LibraryModuleCount < 2)
        return;  // NOTHING TO ORDER.  RETURN

    // Find where to start - first not processed
    int kbase = LibraryModuleCount;
    for (int k = 0; k < LibraryModuleCount; k++)
    {
        const LibraryModuleEntry* ek = LibraryModuleList + k;
        if (ek->passno == 0)
        {
            kbase = k;
            break;
        }
    }

    if (kbase == LibraryModuleCount)
        return;  // Nothing to order

    // Sort from kbase to the end of the array
    for (int k = kbase + 1; k < LibraryModuleCount; k++)
    {
        const LibraryModuleEntry* ek = LibraryModuleList + k;
        for (int j = kbase; j < k; j++)
        {
            LibraryModuleEntry* ej = LibraryModuleList + j;
            if (ek->libfileno < ej->libfileno ||
                (ek->libfileno == ej->libfileno && ek->offset() < ej->offset()))
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
        LibraryModuleEntry* lmlentry = LibraryModuleList + i;
        if (lmlentry->libfileno != Globals.LIBNB)
            continue;  // not this library file
        if (lmlentry->offset() == offset)
            continue;  // same offset
        if (lmlentry->passno > 0)
            continue;  // already passed

        offset = lmlentry->offset();
        printf("      Module #%d offset %06o\n", i, (unsigned int)offset);
        while (offset < sscur->filesize)
        {
            uint8_t* data = sscur->data + offset;
            //uint16_t* dataw = (uint16_t*)(data);
            uint16_t blocksize = ((uint16_t*)data)[1];
            uint16_t blocktype = ((uint16_t*)data)[2];

            if (blocktype == 0 || blocktype > 8)
                fatal_error("Illegal record type at %06o in %s\n", (unsigned int)offset, sscur->filename);
            else if (blocktype == 1)  // 1 - START GSD RECORD, see LINK3\GSD
            {
                printf("    Block type 1 - GSD at %06o size %06ho\n", (unsigned int)offset, blocksize);
                process_pass1_gsd_block(sscur, data);
            }
            else if (blocktype == 6)  // 6 - MODULE END, see LINK3\MODND
            {
                //printf("    Block type 6 - ENDMOD at %06o size %06ho\n", (unsigned int)offset, blocksize);
                if (Globals.HGHLIM < Globals.LRUNUM)
                    Globals.HGHLIM = Globals.LRUNUM;
                Globals.LRUNUM = 0;
                break;
            }

            data += blocksize; offset += blocksize;
            data += 1; offset += 1;  // Skip checksum
        }
        lmlentry->passno++;  // Mark as passed
    }
}

void process_pass15_library(const SaveStatusEntry* sscur)
{
    size_t offset = 0;
    while (offset < sscur->filesize - 1)
    {
        uint8_t* data = sscur->data + offset;
        uint16_t* dataw = (uint16_t*)(data);
        if (*dataw != 1)
        {
            // Possibly that is filler at the end of the block
            size_t offsetnext = (offset + 511) & ~511;
            size_t shift = offsetnext - offset;
            data += shift; offset += shift;
            if (offset >= sscur->filesize)
                break;  // End of file
            dataw = (uint16_t*)(data);
            if (*dataw != 1)
#define IGNORE_BROKEN_ENTRIES 1
#ifndef IGNORE_BROKEN_ENTRIES
                fatal_error("Unexpected word %06ho at %06o in %s\n", *dataw, (unsigned int)offset, sscur->filename);
#else
            {
                warning_message("Unexpected word %06ho at %06o in %s. Trying skip to ENDLIB\n", *dataw, (unsigned int)offset, sscur->filename);
                while ((dataw[0] != 1 || dataw[1] != 6 || dataw[2] != 6) && offset < sscur->filesize)
                {
                    dataw++;
                    data += 2;
                    offset += 2;
                }
                if (*dataw != 1)
                {
                    fatal_error("Unable to find ENDLIB till offset %06o in %s\n", *dataw, (unsigned int)offset, sscur->filename);
                }
            }
#endif
        }

        uint16_t blocksize = ((uint16_t*)data)[1];
        uint16_t blocktype = ((uint16_t*)data)[2];

        if (blocktype == 0 || blocktype > 8)
            fatal_error("Illegal record type %03ho at %06o in %s\n", blocktype, (unsigned int)offset, sscur->filename);
        if (blocktype == 7)  // See LINK3\LIBRA, WE ARE ON PASS 1.5 , SO PROCESS LIBRARIES
        {
            printf("    Block type 7 - TITLIB at %06o size %06ho\n", (unsigned int)offset, blocksize);
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
            data += L_HEPT; offset += L_HEPT;  // Move to 1ST EPT ENTRY

            // Resolve undefined symbols using EPT
            if (is_any_undefined())  // IF NO UNDEFS, THEN END LIBRARY
            {
                int index = Globals.UNDLST;
                while (index > 0)
                {
                    SymbolTableEntry* entry = SymbolTable + index;

                    if ((entry->status & SY_WK) != 0)  // IS THIS A WEAK SYMBOL? SKIP WEAK SYMBOL
                    {
                        index = entry->nextindex();
                        continue;
                    }

                    if ((entry->status & SY_DUP) != 0)  // IS SYMBOL A DUP SYMBOL?
                    {
                        //TODO: IS SEG NUM FIELD ZERO?
                        index = entry->nextindex();
                        continue;
                    }

                    const uint16_t* itemw = process_pass15_eptsearch(data, eptsize, entry->name);
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

                    const uint16_t* itemwmod = itemw;
                    for (;;)  // GO SEARCH FOR MODULE NAME
                    {
                        itemwmod -= 4;  // BACK ONE EPT
                        if (itemwmod <= (uint16_t*)data)
                            break;
                        if (itemwmod[2] & 0x8000)  // MODULE NAME?
                            break;
                    }

                    const uint16_t* itemw1 = itemwmod;
                    itemw1 += 4;  // GO TO NEXT SYMBOL
                    //TODO

                    index = entry->nextindex();  // CONTINUE THRU UNDEF LIST
                }
            }

            data += eptsize; offset += eptsize;
            continue;
        }
        else if (blocktype == 8)  // LINK3\ENDLIB
        {
            printf("    Block type 10 - ENDLIB at %06o size %06ho\n", (unsigned int)offset, blocksize);

            process_pass15_lmlorder();  // ORDER THIS LIBRARY LML
            //print_lml_table();//DEBUG

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
    Globals.LIBNB = 0;  // RESET LIBRARY FILE #

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

            printf("  Processing %s (%d), %06ho\n", sscur->filename, j, Globals.BASE);
            process_pass15_library(sscur);

            if ((Globals.FLGWD & AD_LML) == 0)  // NEW UNDEF'S ADDED WHILE PROCESSING LIBR ?
                break;
        }
    }

    // Process weak symbols, see LINK3\PASS1\30$
    if (is_any_undefined())  // ARE THERE ANY UNDEFINED GLOBALS LEFT? IF NO then NO WEAK SYMBOLS LEFT
    {
        int index = Globals.UNDLST;
        while (index > 0)
        {
            SymbolTableEntry* entry = SymbolTable + index;
            uint16_t nextindex = entry->nextindex();
            if ((entry->status & SY_WK) != 0)  // GET A WEAK SYMBOL FROM THE UNDEFINED SYMBOL TABLE LIST
            {
                // WE HAVE A WEAK SYMBOL. NOW DEFINE IT WITH ABS VALUE OF 0
                Globals.CSECT = 0;  // ALL WEAK SYMBOLS GO IN . ABS. PSECT
                Globals.BASE = 0;  // CLEAR BASE SINCE WE ARE IN .ABS PSECT
                symbol_table_remove_undefined(index);  // REMOVE ENTRY FROM UNDEFINED LIST
                entry->status &= ~(SY_UDF | SY_WK | 07777);  // CLR SY.UDF+SY.WK+^CSY.ENB
                entry->flagseg = 4 * 0400 | 010/*SY$DEF*/;  // PUT ENTRY TYPE AND FLAG BYTES IN ENTRY
                //TODO: IF INPUT SEG # .NE. SYM SEG # THEN SET EXTERNAL REFERENCE BIT
                //TODO: CLEAR SEGMENT # BITS & SET SEGMENT # WHERE DEFINED
                entry->value = 0;  // VALUE WORD IS 0
                pass1_insert_entry_into_ordered_list(index, entry, true);
                //print_symbol_table();//DEBUG
            }
            index = nextindex;
        }
    }
}

// END PASS ONE PROCESSING; see LINK4\ENDP1
void process_pass1_endp1()
{
    //TODO

    {
        // NOW LOOKUP IN SYMBOL TABLE FOR THE VIRTUAL SECTION, IF IT IS THERE
        // ZERO IT'S VALUE WORD(INDICATES SIZE OF SECTION) SO DOES NOT
        // CONTRIBUTE TO SIZE OF PROGRAM DURING MAP PASS
        uint16_t lkwd = (uint16_t)SY_SEC;  // SECTION NAME LOOKUP
        uint16_t lkmsk = (uint16_t)~SY_SEC;  // CARE ABOUT SECTION FLG
        int index;
        if (symbol_table_lookup(RAD50_VSEC, lkwd, lkmsk, &index))
        {
            SymbolTableEntry* entry = SymbolTable + index;
            entry->value = 0;
        }
    }

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

    // Prepare output file name
    if (*outfilename == 0)
    {
        const char* extension;
        if (Globals.SWITCH & SW_R)
            extension = "REL";
        else if (Globals.SWITCH & SW_L)
            extension = "LDA";
        else
            extension = "SAV";

        prepare_filename(outfilename, SaveStatusArea[0].filename, extension);
    }

    if (Globals.FlagSTB) // IS THERE AN STB FILE?
    {
        // Prepare STB file name
        char stbfilename[PATH_MAX + 1];
        prepare_filename(stbfilename, outfilename, "STB");

        // Open STB file
        assert(stbfileobj == nullptr);
        stbfileobj = fopen(stbfilename, "wb");
        if (stbfileobj == nullptr)
            fatal_error("ERR5: Failed to open %s file, error %d: %s.\n", stbfilename, errno, strerror(errno));

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
    if (Globals.SWITCH & SW_H)
    {
        //TODO
        ASECTentry->value = Globals.HSWVAL;  //TODO: very dumb version
    }

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
    if (stbalign != 512)
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

void process_pass_map_output_headers()
{
    fprintf(mapfileobj, "PCLINK11 %-8s", APP_VERSION_STRING);
    fprintf(mapfileobj, "\tLoad Map \t");

    time_t curtime;  time(&curtime); // DETERMINE DATE & TIME; see LINK5\MAPHDR
    struct tm * timeptr = localtime(&curtime);
    fprintf(mapfileobj, "%s %.2d-%s-%d %.2d:%.2d\n",
            weekday_names[timeptr->tm_wday],
            timeptr->tm_mday, month_names[timeptr->tm_mon], 1900 + timeptr->tm_year,
            timeptr->tm_hour, timeptr->tm_min);

    char savname[PATH_MAX + 1];
    strcpy(savname, outfilename);
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
    fprintf(mapfileobj, "%s\t", unrad50(Globals.IDENT));
    if (Globals.SWITCH & SW_H)
        fprintf(mapfileobj, "/H:%06ho", Globals.HSWVAL);

    fprintf(mapfileobj, "\n\n");
    fprintf(mapfileobj, "Section  Addr\tSize");
    //printf("  Section  Addr   Size ");
    for (uint8_t i = 0; i < Globals.NUMCOL; i++)
    {
        fprintf(mapfileobj, "\tGlobal\tValue");
        printf("   Global  Value");
    }
    fprintf(mapfileobj, "\n\n");
    println();
}

// PRINT UNDEFINED GLOBALS IF ANY, see LINK5\DOUDFS
void process_pass_map_output_print_undefined_globals()
{
    process_pass_map_endstb();  // CLOSE OUT THE STB FILE

    int index = Globals.UNDLST;
    if (index == 0)
        return;
    printf("  Undefined globals:\n  ");
    if (Globals.FlagMAP)
        fprintf(mapfileobj, "\nUndefined globals:\n");
    int count = 0;
    while (index != 0)
    {
        if (count > 0 && count % 8 == 0) printf("\n  ");
        SymbolTableEntry* entry = SymbolTable + index;
        printf(" '%s'", entry->unrad50name());
        if (Globals.FlagMAP)
            fprintf(mapfileobj, "%s\n", entry->unrad50name());
        index = entry->nextindex();
        count++;
    }
    println();
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
    if (Globals.FlagMAP)
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
    if (Globals.FlagMAP)
    {
        // Prepare MAP file name
        char mapfilename[PATH_MAX + 1];
        prepare_filename(mapfilename, outfilename, "MAP");

        // Open MAP file
        assert(mapfileobj == nullptr);
        mapfileobj = fopen(mapfilename, "wt");
        if (mapfileobj == nullptr)
            fatal_error("ERR5: Failed to open file %s, error %d: %s.\n", mapfilename, errno, strerror(errno));
    }

    //Globals.LINLFT = LINPPG;

    if (Globals.FlagMAP) // OUTPUT THE HEADERS
    {
        process_pass_map_output_headers();
    }

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
                if (Globals.FlagMAP)
                    fprintf(mapfileobj, "\n");
                println();
                tabcount = 0;
            }
            for (int i = 0; i < Globals.QSWCNT; i++)
                if (Globals.QSWVAL[i].name == entry->name)
                {
                    baseaddr = Globals.QSWVAL[i].addr;
                    break;
                }
            entry->value = baseaddr;
            // Check whether next section set fixed adddres via /Q option
            SymbolTableEntry* nextentry = entry;
            while (nextentry->nextindex())
            {
                nextentry = SymbolTable + nextentry->nextindex();
                if (!(nextentry->flagseg & SY_SEC)) continue;

                for (int i = 0; i < Globals.QSWCNT; i++)
                    if (Globals.QSWVAL[i].name == nextentry->name)
                    {
                        sectsize = Globals.QSWVAL[i].addr - baseaddr;
                        break;
                    }

                break;
            }
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
                if (Globals.FlagMAP)
                    fprintf(mapfileobj, "\t\t\t");
                printf("                        ");
            }
            if (Globals.FlagMAP)
                fprintf(mapfileobj, "%s\t%06ho\t", entry->unrad50name(), entry->value);
            printf("  %s  %06ho", entry->unrad50name(), entry->value);
            tabcount++;
            if (tabcount >= Globals.NUMCOL)
            {
                if (Globals.FlagMAP)
                    fprintf(mapfileobj, "\n");
                println();
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
                if (Globals.FlagMAP)
                    fprintf(mapfileobj, "\n");
                println();
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

    // Print undefined globals, see LINK5\DOUDFS
    process_pass_map_output_print_undefined_globals();

    // OUTPUT TRANSFER ADR & CHECK ITS VALIDITY, see LINK5\DOTADR
    uint16_t lkmsk = (uint16_t) ~(SY_SEC + SY_SEG);  // LOOK AT SECTION & SEGMENT # BITS
    //uint16_t segnum = 0;  // MUST BE IN ROOT SEGMENT
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
    if (Globals.FlagMAP)
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
uint16_t process_pass2_rld_complex(const SaveStatusEntry* sscur, const uint8_t* &data, uint16_t &offset, uint16_t blocksize, uint16_t addr)
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
            println();
            break;
        case 001:  // ADD -- ADD TOP 2 ITEMS
            println();
            if (cpxstacktop > 0)
            {
                cpxstacktop--;
                cpxstack[cpxstacktop] = cpxstack[cpxstacktop] + cpxstack[cpxstacktop + 1];
            }
            break;
        case 002:  // SUBTRACT -- NEGATE TOP ITEM ON STACK
            println();
            if (cpxstacktop > 0)
            {
                cpxstacktop--;
                cpxstack[cpxstacktop] = cpxstack[cpxstacktop] - cpxstack[cpxstacktop + 1];
            }
            break;
        case 003:  // MULTIPLY
            println();
            if (cpxstacktop > 0)
            {
                cpxstacktop--;
                cpxstack[cpxstacktop] = cpxstack[cpxstacktop] * cpxstack[cpxstacktop + 1];
            }
            break;
        case 004:  // DIVIDE, see LINK7\CPXDIV
            println();
            if (cpxstacktop > 0)
            {
                if (cpxstack[cpxstacktop] == 0)
                    fatal_error("ERR33: Complex relocation divide by zero in %s\n", sscur->filename);
                cpxstacktop--;
                cpxstack[cpxstacktop] = cpxstack[cpxstacktop] / cpxstack[cpxstacktop + 1];
            }
            break;
        case 005:  // AND
            println();
            if (cpxstacktop > 0)
            {
                cpxstacktop--;
                cpxstack[cpxstacktop] = cpxstack[cpxstacktop] & cpxstack[cpxstacktop + 1];
            }
            break;
        case 006:  // OR
            println();
            if (cpxstacktop > 0)
            {
                cpxstacktop--;
                cpxstack[cpxstacktop] = cpxstack[cpxstacktop] | cpxstack[cpxstacktop + 1];
            }
            break;
        case 007:  // XOR
            println();
            if (cpxstacktop > 0)
            {
                cpxstacktop--;
                cpxstack[cpxstacktop] = cpxstack[cpxstacktop] ^ cpxstack[cpxstacktop + 1];
            }
            break;
        case 010:  // NEGATE TOP ITEM
            println();
            cpxstack[cpxstacktop] = 0 - cpxstack[cpxstacktop];
            break;
        case 011:  // COMPLEMENT -- COMPLEMENT TOP ITEM
            println();
            cpxstack[cpxstacktop] = ~cpxstack[cpxstacktop];
            break;
        case 012:  // STORE NOT DISPLACED
            cpxresult = cpxstack[cpxstacktop];
            printf(" %06ho\n", cpxresult);
            cpxbreak = true;
            break;
        case 013:  // STORE DISPLACED
            cpxresult = cpxstack[cpxstacktop];
            cpxresult -= addr + 2;
            printf(" %06ho\n", cpxresult);
            cpxbreak = true;
            break;
        case 014:  // ILLEGAL FORMAT
        case 015:  // ILLEGAL FORMAT
            println();
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
            println();
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
            //curlocation = Globals.BASE;  // ADD SECTION BASE
            constdata = *((uint16_t*)data);
            printf(" %06ho\n", constdata);
            *((uint16_t*)dest) = constdata + Globals.BASE;
            if (Globals.SWITCH & SW_R)
            {
                RelocationTable[RelocationTableCount].addr = (disbyte - 4) >> 1;
                RelocationTable[RelocationTableCount].value = *((uint16_t*)dest);
                RelocationTableCount++;
            }
            data += 2;  offset += 2;
            break;
        case 002:  // GLOBAL
        case 012:  // PSECT
            // RELOCATES A DIRECT POINTER TO A GLOBAL SYMBOL. THE VALUE OF THE GLOBAL SYMBOL IS OBTAINED & STORED.
            printf(" '%s'\n", unrad50(*((uint32_t*)data)));
            {
                SymbolTableEntry* entry = process_pass2_rld_lookup(data, (command & 010) == 0);
                //printf("        Entry '%s' value = %06ho %04X dest = %06ho\n", entry->unrad50name(), entry->value, entry->value, *((uint16_t*)dest));
                if ((command & 0177) == 012)
                    *((uint16_t*)dest) = entry->value;
                else if ((command & 0177) == 002)
                    *((uint16_t*)dest) += entry->value;
            }
            data += 4;  offset += 4;
            break;
        case 003:  // INTERNAL DISPLACED REL, see LINK7\RLDIDR
            // RELATIVE REFERENCE TO AN ABSOLUTE ADDRESS FROM WITHIN A RELOCATABLE SECTION.
            // THE ADDRESS + 2 THAT THE RELOCATED VALUE IS TO BE WRITTEN INTO IS SUBTRACTED FROM THE SPECIFIED CONSTANT & RESULTS STORED.
            constdata = *((uint16_t*)data);
            printf(" %06ho\n", constdata);
            *((uint16_t*)dest) = constdata - (addr + 2);
            if (Globals.SWITCH & SW_R)
            {
                RelocationTable[RelocationTableCount].addr = (disbyte - 4) >> 1;
                RelocationTable[RelocationTableCount].value = *((uint16_t*)dest);
                RelocationTableCount++;
            }
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
            //curlocation = 0;  // ADD 0 AS CONSTANT
            constdata = ((uint16_t*)data)[2];
            printf(" '%s' %06ho\n", unrad50(*((uint32_t*)data)), constdata);
            {
                SymbolTableEntry* entry = process_pass2_rld_lookup(data, (command & 010) == 0);
                *((uint16_t*)dest) = entry->value + constdata;
                if (Globals.SWITCH & SW_R)
                {
                    RelocationTable[RelocationTableCount].addr = (disbyte - 4) >> 1;
                    RelocationTable[RelocationTableCount].value = *((uint16_t*)dest);
                    RelocationTableCount++;
                }
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
                if ((entry->flags() & 0040/*SY$REL*/) == 0)  // IS SYMBOL ABSOLUTE ?
                {
                    if (entry->name != RAD50_ABS)  // ARE WE LOOKING AT THE ASECT?
                        Globals.MBPTR = 1;  // SAY NOT TO STORE TXT FOR ABS SECTION
                }
                Globals.BASE = entry->value;  // SET UP NEW SECTION BASE
                //TODO: ARE WE DOING I-D SPACE?
                //curlocation = constdata + Globals.BASE;  // BASE OF SECTION + OFFSET = CURRENT LOCATION COUNTER
            }
            data += 6;  offset += 6;
            break;
        case 010:  // LOCATION COUNTER MODIFICATION, see LINK7\RLDLCM
            // THE CURRENT SECTION BASE IS ADDED TO THE SPECIFIED CONSTANT & RESULT IS STORED AS THE CURRENT LOCATION CTR.
            constdata = ((uint16_t*)data)[0];
            printf(" %06ho\n", constdata);
            //curlocation = constdata + Globals.BASE;  // BASE OF SECTION + OFFSET = CURRENT LOCATION COUNTER
            data += 2;  offset += 2;
            break;
        case 011:  // SET PROGRAM LIMITS, see LINK7\RLDSPL
            println();
            *((uint16_t*)dest) = Globals.BOTTOM;
            *((uint16_t*)dest + 1) = Globals.HGHLIM;
            break;
        case 017:  // COMPLEX RELOCATION STRING PROCESSING (GLOBAL ARITHMETIC)
            println();
            *((uint16_t*)dest) = process_pass2_rld_complex(sscur, data, offset, blocksize, addr);
            break;
        default:
            fatal_error("ERR36: Unknown RLD command: %d in %s\n", (int)command, sscur->filename);
        }
    }
}

// Prapare SYSCOM area, pass 2 initialization; see LINK6
void process_pass2_init()
{
    mst_table_clear();

    // Allocate space for .SAV file image
    OutputBufferSize = 65536;
    OutputBuffer = (uint8_t*) calloc(OutputBufferSize, 1);
    if (OutputBuffer == nullptr)
        fatal_error("ERR11: Failed to allocate memory for output buffer.\n");

    // FORCE BASE OF ZERO FOR VSECT IF ANY
    {
        uint16_t lkwd = (uint16_t)SY_SEC;  // SECTION NAME LOOKUP
        uint16_t lkmsk = (uint16_t)~SY_SEC;  // CARE ABOUT SECTION FLG
        int index;
        if (symbol_table_lookup(RAD50_VSEC, lkwd, lkmsk, &index))
        {
            SymbolTableEntry* entry = SymbolTable + index;
            entry->value = 0;
        }
    }

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

    // SYSCOM AREA FOR REL FILE
    if (Globals.SWITCH & SW_R)
    {
//        *((uint16_t*)(OutputBuffer + 052)) = Globals.BOTTOM;  // LESS BASE TO GET ACTUAL CODE SIZE
        *((uint16_t*)(OutputBuffer + 052)) = highlim - Globals.BEGBLK.value;  // Hack ???

        *((uint16_t*)(OutputBuffer + 054)) = Globals.KSWVAL;  // SIZE OF STACK IN BYTES
        //TODO: SIZE OF /O OVERLAY REGIONS IN BYTES, 0 IF NO OVERLAYS
        *((uint16_t*)(OutputBuffer + 060)) = rad50("REL", nullptr);  // REL FILE ID
        //TODO: RELATIVE BLOCK NUMBER OF START OF RELOCATION INFORMATION
        *((uint16_t*)(OutputBuffer + 062)) = ((highlim +  511) / 512);
    }

    //TODO: BINOUT REQUESTED?

    // Open output file for writing
    assert(outfileobj == nullptr);
    outfileobj = fopen(outfilename, "wb");
    if (outfileobj == nullptr)
        fatal_error("ERR6: Failed to open %s file, error %d: %s.\n", outfilename, errno, strerror(errno));

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

    uint16_t addr = *((uint16_t*)Globals.TXTBLK);  //NOTE: Should be absolute address
    uint8_t* dest = OutputBuffer + addr;
    uint8_t* src = Globals.TXTBLK + 2;
    memcpy(dest, src, Globals.TXTLEN);
    printf("    process_pass2_dump_txtblk() at %04x len %04x data %02x %02x %02x %02x\n", addr, Globals.TXTLEN, src[0], src[1], src[2], src[3]);

    mark_bitmap_bits(addr, Globals.TXTLEN);
    //TODO: if ((Globals.SWITCH & SW_X) != 0)

    if (addr < 0400 && addr + Globals.TXTLEN > 0360 && (Globals.SWITCH & SW_X) != 0)
        Globals.FLGWD |= 02000/*FG.XX*/;  // YES->SET FLAG NOT TO OUTPUT BITMAP

    if (Globals.MBPTR)  // SHOULD THE TXT BE STORED?
        Globals.TXTLEN = 0;  // MARK TXT BLOCK EMPTY, see LINK7\CLRTXL
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
            uint16_t lkmsk = (uint16_t)~SY_SEC;  // CARE ABOUT SECTION FLAG
            uint16_t lkwd = (uint16_t)SY_SEC;  // SECTION BIT FOR MATCH
            //TODO: DOES PSECT HAVE SAV ATTRIBUTE?
            if ((itemflags & 0100/*CS$GBL*/) == 0)  // LOCAL OR GLOBAL SECTION ?
                lkmsk = (uint16_t)~(SY_SEC | SY_SEG);  // CARE ABOUT SECTION & SEGMENT #
            int index;
            bool isfound = symbol_table_lookup(lkname, lkwd, lkmsk, &index);
            if (!isfound)
                fatal_error("ERR21: Invalid GSD in %s\n", sscur->filename);

            ModuleSectionEntry* msentry = ModuleSectionTable + ModuleSectionCount;
            msentry->stindex = (uint16_t)index;
            msentry->size = 0;  // ASSUME CONTRIBUTION OF 0 FOR THIS SECTION
            ModuleSectionCount++;
            SymbolTableEntry* entry = SymbolTable + index;
            if ((entry->flags() & 4/*CS$ALO*/) != 0)
                continue;  // DON'T UPDATE SECTION BASE IF OVR SECT
            uint16_t sectsize = itemw[3];
            //TODO: ROUND ALL SECTIONS EXCEPT "CON" DATA SECTIONS TO WORD BOUNDARIES
            if ((entry->flags() & 0200/*CS$TYP*/) == 0)
                sectsize = (sectsize + 1) & ~1;
            msentry->size = sectsize;
        }
    }
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

        printf("  proccess_pass2_libpa2() #%04d for offset %06o\n", i, (unsigned int)offset);
        while (offset < sscur->filesize)
        {
            uint8_t* data = sscur->data + offset;
            //uint16_t* dataw = (uint16_t*)(data);
            uint16_t blocksize = ((uint16_t*)data)[1];
            uint16_t blocktype = ((uint16_t*)data)[2];

            if (blocktype == 0 || blocktype > 8)
                fatal_error("Illegal record type at %06o in %s\n", (unsigned int)offset, sscur->filename);
            else if (blocktype == 1)  // START GSD RECORD, see LINK7\GSD
            {
                printf("    Block type 1 - GSD at %06o size %06ho\n", (unsigned int)offset, blocksize);
                process_pass2_gsd_block(sscur, data);
            }
            else if (blocktype == 3)  // See LINK7\DMPTXT
            {
                process_pass2_dump_txtblk();

                uint16_t addr = ((uint16_t*)data)[3];
                uint16_t destaddr = addr + Globals.BASE;
                uint16_t datasize = blocksize - 8;
                printf("    Block type 3 - TXT at %06o size %06ho addr %06ho base %06ho dest %06ho len %06ho\n",
                       (unsigned int)offset, blocksize, addr, Globals.BASE, destaddr, datasize);
                Globals.TXTLEN = datasize;
                assert(datasize <= sizeof(Globals.TXTBLK));
                memcpy(Globals.TXTBLK, data + 6, blocksize - 6);
                //printf(" data %02x %02x %02x %02x\n", data[8 + 0], data[8 + 1], data[8 + 2], data[8 + 3]);

                *((uint16_t*)Globals.TXTBLK) = destaddr;  // ADD BASE TO GIVE ABS LOAD ADDR
                if (Globals.SWITCH & SW_L)
                {
                    if (LdaTable[destaddr >> 1].value == 0)
                    {
                        LdaTable[destaddr >> 1].addr = destaddr;
                        LdaTable[destaddr >> 1].value = Globals.TXTLEN;
                    }
                }
            }
            else if (blocktype == 4)  // See LINK7\RLD
            {
                uint16_t base = *((uint16_t*)Globals.TXTBLK);
                printf("    Block type 4 - RLD at %06o size %06ho base %06ho\n", (unsigned int)offset, blocksize, base);
                process_pass2_rld(sscur, data);
            }
            else if (blocktype == 6)  // MODULE END RECORD, See LINK7\MODND
            {
                //printf("    Block type 6 - ENDMOD at %06o size %06ho\n", (unsigned int)offset, blocksize);
                process_pass2_dump_txtblk();

                // AT THE END OF EACH MODULE THE BASE ADR OF EACH SECTION IS UPDATED AS DETERMINED BY THE MST.
                for (int m = 0; m < ModuleSectionCount; m++)
                {
                    ModuleSectionEntry* mstentry = ModuleSectionTable + m;
                    if (mstentry->size == 0)
                        continue;
                    SymbolTableEntry* entry = SymbolTable + mstentry->stindex;
                    entry->value += mstentry->size;
                }
                mst_table_clear();

                break;
            }

            data += blocksize; offset += blocksize;
            data += 1; offset += 1;  // Skip checksum
        }
    }
}

void process_pass2_file(const SaveStatusEntry* sscur)
{
    assert(sscur != nullptr);
    assert(sscur->data != nullptr);

    size_t offset = 0;
    while (offset < sscur->filesize - 1)
    {
        uint8_t* data = sscur->data + offset;
        uint16_t* dataw = (uint16_t*)(data);
        if (*dataw != 1)
        {
            if (*dataw == 0)  // Possibly that is filler at the end of the block
            {
                size_t offsetnext = (offset + 511) & ~511;
                while (offset < sscur->filesize && offset < offsetnext)
                {
                    if (*data != 0) break;
                    data++; offset++;
                }
                if (offset >= sscur->filesize)
                    break;  // End of file
            }
            dataw = (uint16_t*)(data);
            if (*dataw != 1)
                fatal_error("Unexpected word %06ho at %06o in %s\n", *dataw, (unsigned int)offset, sscur->filename);
        }

        uint16_t blocksize = ((uint16_t*)data)[1];
        uint16_t blocktype = ((uint16_t*)data)[2];

        if (blocktype == 0 || blocktype > 8)
            fatal_error("ERR4: Illegal record type at %06o in %s\n", (unsigned int)offset, sscur->filename);
        else if (blocktype == 1)  // START GSD RECORD, see LINK7\GSD
        {
            printf("    Block type 1 - GSD at %06o size %06ho\n", (unsigned int)offset, blocksize);
            process_pass2_gsd_block(sscur, data);
        }
        else if (blocktype == 3)  // See LINK7\DMPTXT
        {
            process_pass2_dump_txtblk();

            uint16_t addr = ((uint16_t*)data)[3];
            uint16_t destaddr = addr + Globals.BASE;
            uint16_t datasize = blocksize - 8;
            printf("    Block type 3 - TXT at %06o size %06ho addr %06ho dest %06ho len %06ho\n",
                   (unsigned int)offset, blocksize, addr, destaddr, datasize);
            Globals.TXTLEN = datasize;
            assert(datasize <= sizeof(Globals.TXTBLK));
            memcpy(Globals.TXTBLK, data + 6, blocksize - 6);
            //printf(" data %02x %02x %02x %02x\n", data[8 + 0], data[8 + 1], data[8 + 2], data[8 + 3]);

            *((uint16_t*)Globals.TXTBLK) = destaddr;  // ADD BASE TO GIVE ABS LOAD ADDR
            if (Globals.SWITCH & SW_L)
            {
                if (LdaTable[destaddr >> 1].value == 0)
                {
                    LdaTable[destaddr >> 1].addr = destaddr;
                    LdaTable[destaddr >> 1].value = Globals.TXTLEN;
                }
            }
        }
        else if (blocktype == 4)  // See LINK7\RLD
        {
            uint16_t base = *((uint16_t*)Globals.TXTBLK);
            printf("    Block type 4 - RLD at %06o size %06ho base %06ho\n", (unsigned int)offset, blocksize, base);
            process_pass2_rld(sscur, data);
        }
        else if (blocktype == 6)  // MODULE END RECORD, See LINK7\MODND
        {
            printf("    Block type 6 - ENDMOD at %06o size %06ho\n", (unsigned int)offset, blocksize);

            process_pass2_dump_txtblk();  // DUMP TXT BLK IF ANY

            // AT THE END OF EACH MODULE THE BASE ADR OF EACH SECTION IS UPDATED AS DETERMINED BY THE MST.
            for (int m = 0; m < ModuleSectionCount; m++)
            {
                ModuleSectionEntry* mstentry = ModuleSectionTable + m;
                if (mstentry->size == 0)
                    continue;
                SymbolTableEntry* entry = SymbolTable + mstentry->stindex;
                entry->value += mstentry->size;
            }
            mst_table_clear();
        }
        else if (blocktype == 7)  // See LINK7\LIBPA2
        {
            printf("    Block type 7 - TITLIB at %06o size %06ho\n", (unsigned int)offset, blocksize);
            //TODO
            proccess_pass2_libpa2(sscur);
            break;
        }

        data += blocksize; offset += blocksize;
        data += 1; offset += 1;  // Skip checksum
    }
}

// PRODUCE SAVE IMAGE FILE, see LINK7\PASS2
void process_pass2()
{
    Globals.TXTLEN = 0;
    Globals.LIBNB = 0;
    for (int i = 0; i < SaveStatusCount; i++)
    {
        SaveStatusEntry* sscur = SaveStatusArea + i;

        if (sscur->islibrary)
        {
            if ((Globals.PAS1_5 & 128) == 0)
            {
                Globals.PAS1_5 |= 1;  // LIBRARY PASS REQUIRED
                continue;  // Skip libraries on non-library pass
            }
            Globals.LIBNB++;
        }
        else
        {
            if ((Globals.PAS1_5 & 128) != 0)
                continue;  // Skip non-library on library pass
        }

        printf("  Processing %s %s\n", sscur->filename, sscur->islibrary ? "library" : "");
        process_pass2_file(sscur);
    }
}

void process_pass2_done()
{
    uint16_t highlim = (Globals.SWIT1 & SW_J) ? Globals.DHGHLM : Globals.HGHLIM;
    // Write to the output file
    if (Globals.SWITCH & SW_L)  // LDA file
    {
        uint16_t baseaddr = Globals.BOTTOM;
        uint16_t total = 0;

        uint16_t ldaheader[3];
        uint8_t* ldaheaderbp = (uint8_t*)ldaheader;
        uint8_t checksum;
        size_t bytestowrite;
        size_t byteswrit;
        for (int l = 0; l < 0x800; l++)
        {
            if (LdaTable[l].value == 0) continue;
            printf("LDA: @0x%x = 0x%x\n", LdaTable[l].addr, LdaTable[l].value);
            ldaheader[0] = 1;
            ldaheader[1] = LdaTable[l].value + 6;
            ldaheader[2] = LdaTable[l].addr;
            bytestowrite = sizeof(ldaheader);
            byteswrit = fwrite(ldaheader, 1, bytestowrite, outfileobj);
            if (byteswrit != bytestowrite)
                fatal_error("ERR6: Failed to write output file.\n");
            total += byteswrit;
            bytestowrite = LdaTable[l].value;
            byteswrit = fwrite(OutputBuffer + LdaTable[l].addr, 1, bytestowrite, outfileobj);
            if (byteswrit != bytestowrite)
                fatal_error("ERR6: Failed to write output file.\n");
            total += byteswrit;

            checksum = 0;
            for (int i = 0; i < 6; i++)
                checksum += ldaheaderbp[i];
            for (int i = 0; i < LdaTable[l].value; i++)
                checksum += OutputBuffer[LdaTable[l].addr + i];
            checksum = (255 - checksum) + 1;

            bytestowrite = 1;
            byteswrit = fwrite(&checksum, 1, bytestowrite, outfileobj);
            if (byteswrit != bytestowrite)
                fatal_error("ERR6: Failed to write output file.\n");
            total += byteswrit;
        }
        // Final block
        ldaheader[0] = 1;
        ldaheader[1] = 6;
        ldaheader[2] = baseaddr;
        bytestowrite = sizeof(ldaheader);
        byteswrit = fwrite(ldaheader, 1, bytestowrite, outfileobj);
        if (byteswrit != bytestowrite)
            fatal_error("ERR6: Failed to write output file.\n");
        total += byteswrit;

        checksum = 0xf7;
        bytestowrite = 1;
        byteswrit = fwrite(&checksum, 1, bytestowrite, outfileobj);
        if (byteswrit != bytestowrite)
            fatal_error("ERR6: Failed to write output file.\n");
        total += byteswrit;

        // Zero padding to block size
        checksum = 0;
        while ( total & 0x1ff)
        {
            byteswrit = fwrite(&checksum, 1, bytestowrite, outfileobj);
            if (byteswrit != bytestowrite)
                fatal_error("ERR6: Failed to write output file.\n");
            total += byteswrit;
        }
    }
    else if (Globals.SWITCH & SW_R) // REL file
    {
        Globals.BITMAP[0] |= 128;  // We always use block 1

        // Copy the bitmap to block 0
        if ((Globals.FLGWD & 02000) == 0)
        {
            int bitstowrite = ((int)highlim + 511) / 512;
            int bytestowrite = (bitstowrite + 7) / 8;
            memcpy(OutputBuffer + SysCom_BITMAP, Globals.BITMAP, bytestowrite);
        }

        size_t bytestowrite = OutputBlockCount == 0 ? 65536 : OutputBlockCount * 512;
        size_t byteswrit = fwrite(OutputBuffer, 1, bytestowrite, outfileobj);
        if (byteswrit != bytestowrite)
            fatal_error("ERR6: Failed to write output file.\n");

        // Add relocation list end marker
        RelocationTable[RelocationTableCount].addr = 0177776;
        RelocationTable[RelocationTableCount].value = 0;
        RelocationTableCount++;
        // Write relocation list
        bytestowrite = RelocationTableCount * sizeof(RELEntry);
        bytestowrite = (bytestowrite + 511) / 512 * 512; // round to block size
        byteswrit = fwrite(RelocationTable, 1, bytestowrite, outfileobj);
        if (byteswrit != bytestowrite)
            fatal_error("ERR6: Failed to write output file.\n");
    }
    else  // SAV file
    {
        Globals.BITMAP[0] |= 128;  // We always use block 1

        // Copy the bitmap to block 0
        if ((Globals.FLGWD & 02000) == 0)
        {
            int bitstowrite = ((int)highlim + 511) / 512;
            int bytestowrite = (bitstowrite + 7) / 8;
            memcpy(OutputBuffer + SysCom_BITMAP, Globals.BITMAP, bytestowrite);
        }

        size_t bytestowrite = OutputBlockCount == 0 ? 65536 : OutputBlockCount * 512;
        size_t byteswrit = fwrite(OutputBuffer, 1, bytestowrite, outfileobj);
        if (byteswrit != bytestowrite)
            fatal_error("ERR6: Failed to write output file.\n");
    }

    // Done with the output file, closing
    fclose(outfileobj);  outfileobj = nullptr;
}


/////////////////////////////////////////////////////////////////////////////
