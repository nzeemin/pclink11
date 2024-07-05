
#define _CRT_SECURE_NO_WARNINGS

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cassert>

#include "main.h"


/////////////////////////////////////////////////////////////////////////////


SaveStatusEntry SaveStatusArea[SaveStatusAreaSize];
int SaveStatusCount = 0;

SymbolTableEntry* SymbolTable = nullptr;
SymbolTableEntry* ASECTentry = nullptr;
int SymbolTableCount = 0;  // STCNT -- SYMBOL TBL ENTRIES COUNTER

RELEntry* RelocationTable = nullptr;
int RelocationTableCount = 0;

RELEntry* LdaTable = nullptr;

LibraryModuleEntry LibraryModuleList[LibraryModuleListSize];
int LibraryModuleCount = 0;  // Count of records in LibraryModuleList, see LMLPTR

ModuleSectionEntry ModuleSectionTable[ModuleSectionTableSize];
int ModuleSectionCount = 0;


/////////////////////////////////////////////////////////////////////////////


void symbol_table_enter(int* pindex, uint32_t lkname, uint16_t lkwd)
{
    assert(pindex != nullptr);
    assert(SymbolTable != nullptr);

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
    assert(SymbolTable != nullptr);

    //TODO
    NOTIMPLEMENTED
}

// ADD A REFERENCED SYMBOL TO THE UNDEFINED LIST, see LINK3\ADDUDF
void symbol_table_add_undefined_head(int index)
{
    assert(index > 0 && index < SymbolTableSize);
    assert(SymbolTable != nullptr);

    if (Globals.UNDLST != 0)
    {
        SymbolTableEntry* oldentry = SymbolTable + Globals.UNDLST;
        oldentry->value = (uint16_t)index;  // set back reference
    }else
    {
        Globals.UNDEND = index;
    }

    SymbolTableEntry* entry = SymbolTable + index;
    entry->status |= SY_UDF;  // MAKE CUR SYM UNDEFINED
    entry->flagseg = (entry->flagseg & ~SY_SEG) | Globals.SEGNUM;  // SET SEGMENT # WHERE INITIAL REF
    entry->status |= (uint16_t)Globals.UNDLST;

    Globals.UNDLST = index;

    Globals.FLGWD |= AD_LML;  // IND TO ADD TO LML LATER
}
void symbol_table_add_undefined_tail(int index)
{
    assert(index > 0 && index < SymbolTableSize);
    assert(SymbolTable != nullptr);

    SymbolTableEntry* entry = SymbolTable + index;
    entry->status |= SY_UDF;  // MAKE CUR SYM UNDEFINED
    entry->flagseg = (entry->flagseg & ~SY_SEG) | Globals.SEGNUM;  // SET SEGMENT # WHERE INITIAL REF
    if (Globals.UNDLST == 0)
    {
        entry->value = 0;
        Globals.UNDLST = index;
        Globals.UNDEND = index;
    }else{
        uint16_t oldindex = Globals.UNDEND;
        SymbolTableEntry* oldentry = SymbolTable + Globals.UNDEND;

        oldentry->status |= (uint16_t)index;
        entry->value = oldindex;  // set back reference
        Globals.UNDEND = index;
    }
    Globals.FLGWD |= AD_LML;  // IND TO ADD TO LML LATER
}
void symbol_table_add_undefined(int index)
{
  symbol_table_add_undefined_tail(index);
}
// REMOVE A ENTRY FROM THE UNDEFINED LIST, see LINK3\REMOVE
void symbol_table_remove_undefined(int index)
{
    assert(index > 0 && index < SymbolTableSize);
    assert(SymbolTable != nullptr);

    SymbolTableEntry* entry = SymbolTable + index;
    uint16_t previndex = entry->value;
    uint16_t nextindex = entry->nextindex();
    if (previndex == 0)
        Globals.UNDLST = nextindex;
    else
    {
        SymbolTableEntry* preventry = SymbolTable + previndex;
        preventry->status = (preventry->status & 0170000) | (entry->status & 07777);
    }
    if (nextindex != 0)
    {
        SymbolTableEntry* nextentry = SymbolTable + nextindex;
        nextentry->value = previndex;
    }else{
        Globals.UNDEND = previndex;
    }
    entry->value = 0;
    entry->status &= 0170000;
}

// ANY UNDEFINED SYMBOLS IN LIST ? See LINK3\ANYUND
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
        printf("    #%04d file %02d block %03ho offset %03ho pass %d\n", (uint16_t)i, lmlentry->libfileno, lmlentry->relblockno, lmlentry->byteoffset, (int)lmlentry->passno/*, lmlentry->segmentno*/);
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
        if (entry->nextindex() == 0) printf("(eol) ");  // show end-of-list node
        printf("\n");
    }

    printf("  BEGBLK '%s' %06ho\n", unrad50(Globals.BEGBLK.symbol), Globals.BEGBLK.value);

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
        printf("  ASECT-started list has loops.\n");
    else
        printf("  ASECT-started list has %d. entries.\n", count);

    if (Globals.UNDLST == 0)
        printf("  UNDLST = %06ho\n", (uint16_t)Globals.UNDLST);
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
            printf("  UNDLST = %06ho; the list has loops.\n", (uint16_t)Globals.UNDLST);
        else
            printf("  UNDLST = %06ho; the list has %d. entries.\n", (uint16_t)Globals.UNDLST, count);
    }
}

// Clear Module Section Table (MST)
void mst_table_clear()
{
    memset(ModuleSectionTable, 0, ModuleSectionTableSize * sizeof(ModuleSectionEntry));
    ModuleSectionCount = 0;
}

// Print Module Section Table (MST), for DEBUG only
void print_mst_table()
{
    printf("  ModuleSectionTable count = %d.\n", ModuleSectionCount);
    for (int m = 0; m < ModuleSectionCount; m++)
    {
        const ModuleSectionEntry* mstentry = ModuleSectionTable + m;
        const SymbolTableEntry* entry = SymbolTable + mstentry->stindex;
        printf("    #%04d index %06ho '%s' size %06ho flagseg %06ho value %06ho\n",
               (uint16_t)m, mstentry->stindex,
               mstentry->stindex == 0 ? "" : entry->unrad50name(),
               mstentry->size,
               entry->flagseg, entry->value);
    }
}


/////////////////////////////////////////////////////////////////////////////
