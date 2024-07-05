
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cctype>
#include <cassert>
#include <cstdarg>

#include "main.h"


/////////////////////////////////////////////////////////////////////////////


NORETURN
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

void warning_message(const char* message, ...)
{
    assert(message != nullptr);

    printf("WARNING: ");
    {
        va_list ptr;
        va_start(ptr, message);
        vprintf(message, ptr);
        va_end(ptr);
    }
}


/////////////////////////////////////////////////////////////////////////////


static char radtbl[] = " ABCDEFGHIJKLMNOPQRSTUVWXYZ$. 0123456789";

static char unrad50buffer[7];

// Decodes 6 chars of RAD50 into the temp buffer and returns buffer address
const char* unrad50(uint32_t data)
{
    memset(unrad50buffer, 0, sizeof(unrad50buffer));
    unrad50(LOWORD(data), unrad50buffer);
    unrad50(HIWORD(data), unrad50buffer + 3);
    return unrad50buffer;
}

// Decodes 6 chars of RAD50 into the temp buffer and returns buffer address
const char* unrad50(uint16_t loword, uint16_t hiword)
{
    memset(unrad50buffer, 0, sizeof(unrad50buffer));
    unrad50(loword, unrad50buffer);
    unrad50(hiword, unrad50buffer + 3);
    return unrad50buffer;
}

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

// Encode 6 chars of RAD50 into 4 bytes
uint32_t rad50x2(const char *cp)
{
    uint16_t lo = rad50(cp, &cp);
    uint16_t hi = 0;
    if (*cp)
        hi = rad50(cp, &cp);
    return MAKEDWORD(lo, hi);
}

// Encode 3 chars of RAD50 into 2 bytes
uint16_t rad50(const char *cp, const char **endp)
{
    uint32_t acc = 0;
    char *rp;

    if (endp)
        *endp = cp;

    if (!*cp)                          /* Got to check for end-of-string manually, because strchr will call it a hit.  :-/ */
        return (uint16_t)acc;

    rp = strchr(radtbl, toupper(*cp));
    if (rp == nullptr)                    /* Not a RAD50 character */
        return (uint16_t)acc;
    acc = (rp - radtbl) * 03100;        /* Convert */
    cp++;

    /* Now, do the same thing two more times... */

    if (endp)
        *endp = cp;
    if (!*cp)
        return (uint16_t)acc;
    rp = strchr(radtbl, toupper(*cp));
    if (rp == nullptr)
        return (uint16_t)acc;
    acc += (rp - radtbl) * 050;

    cp++;
    if (endp)
        *endp = cp;
    if (!*cp)
        return (uint16_t)acc;
    rp = strchr(radtbl, toupper(*cp));
    if (rp == nullptr)
        return (uint16_t)acc;
    acc += rp - radtbl;

    cp++;
    if (endp)
        *endp = cp;

    return (uint16_t)acc;  // Done
}


/////////////////////////////////////////////////////////////////////////////
