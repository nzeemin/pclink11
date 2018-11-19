
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "main.h"


/////////////////////////////////////////////////////////////////////////////


static char radtbl[] = " ABCDEFGHIJKLMNOPQRSTUVWXYZ$. 0123456789";

static char unrad50buffer[7];

// Decodes 6 chars of RAD50 into the temp buffer and returns buffer address
const char* unrad50(DWORD data)
{
    memset(unrad50buffer, 0, sizeof(unrad50buffer));
    unrad50(LOWORD(data), unrad50buffer);
    unrad50(HIWORD(data), unrad50buffer + 3);
    return unrad50buffer;
}

// Decodes 3 chars of RAD50 into the given buffer
void unrad50(WORD word, char *cp)
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
DWORD rad50x2(const char *cp)
{
    WORD lo = rad50(cp, &cp);
    WORD hi = 0;
    if (*cp)
        hi = rad50(cp, &cp);
    return MAKEDWORD(lo, hi);
}

// Encode 3 chars of RAD50 into 2 bytes
WORD rad50(const char *cp, const char **endp)
{
    DWORD	acc = 0;
    char	*rp;

    if (endp)
        *endp = cp;

    if (!*cp)                          /* Got to check for end-of-string manually, because strchr will call it a hit.  :-/ */
        return (WORD)acc;

    rp = strchr(radtbl, toupper(*cp));
    if (rp == NULL)                    /* Not a RAD50 character */
        return (WORD)acc;
    acc = ((int) (rp - radtbl)) * 03100;        /* Convert */
    cp++;

    /* Now, do the same thing two more times... */

    if (endp)
        *endp = cp;
    if (!*cp)
        return (WORD)acc;
    rp = strchr(radtbl, toupper(*cp));
    if (rp == NULL)
        return (WORD)acc;
    acc += ((int) (rp - radtbl)) * 050;

    cp++;
    if (endp)
        *endp = cp;
    if (!*cp)
        return (WORD)acc;
    rp = strchr(radtbl, toupper(*cp));
    if (rp == NULL)
        return (WORD)acc;
    acc += (int) (rp - radtbl);

    cp++;
    if (endp)
        *endp = cp;

    return (WORD)acc;  // Done
}

/////////////////////////////////////////////////////////////////////////////
