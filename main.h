

/////////////////////////////////////////////////////////////////////////////


typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned long DWORD;

#define MAKEDWORD(lo, hi)  ((DWORD)(((WORD)((DWORD)(lo) & 0xffff)) | ((DWORD)((WORD)((DWORD)(hi) & 0xffff))) << 16))

#define LOWORD(l)          ((WORD)(((DWORD)(l)) & 0xffff))
#define HIWORD(l)          ((WORD)((((DWORD)(l)) >> 16) & 0xffff))
#define LOBYTE(w)          ((BYTE)(((DWORD)(w)) & 0xff))
#define HIBYTE(w)          ((BYTE)((((DWORD)(w)) >> 8) & 0xff))

#define NULL 0


/////////////////////////////////////////////////////////////////////////////


const char* unrad50(DWORD data);
void unrad50(WORD word, char *cp);

WORD rad50(const char *cp, const char **endp);
DWORD rad50x2(const char *cp);


/////////////////////////////////////////////////////////////////////////////
