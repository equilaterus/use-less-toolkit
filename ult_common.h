#ifndef NO_INCLUDES
#include "ult_globals.h"
#include <cstdio>
#include <cstring>
#include <cctype>
#include <type_traits>
#endif

struct buffer
{
    uint8* Data;
    int32 Size;
};
typedef buffer string;

#define BundleStringC(S) MakeString((uint8*)S, strlen(S))
#define BundleString(S) MakeString(S, strlen((char*)S))
inline function string
MakeString(uint8* Address, int32 Size)
{
    string NewString;
    NewString.Data = Address;
    NewString.Size = Size;
    return(NewString);
}

inline function void
StringToChar(string* S, char* C) 
{
    snprintf(C, S->Size + 1, "%s", S->Data);
}

inline internal uint32
GenerateHash(void* MemoryBlock, uint32 SizeInBytes)
{
    uint8* Ptr = (uint8*)MemoryBlock;
    uint32 Result = 0;
    for(int i = 0; i < SizeInBytes; ++i)
	{
		Result = Ptr[i] + (Result * 31);
	}
    return(Result);
}

inline function bool
IsLittleEndian()
{
    int32 One = 1;
    return (*((uint8*)&One) == 1);
}

#define ChangeEndianness16(n) n<<8 | n >>8;
#define ChangeEndianness32(n) ((n>>24)&0xff) | ((n<<8)&0xff0000) | ((n>>8)&0xff00) | ((n<<24)&0xff000000)

#define UNLIKELY(x) __builtin_expect((bool)x, 0)
#define Max(a,b) a < b ? b : a
#define Mod(a,b) a >= b ? a % b : a
#define UnlikelyMod(a,b) UNLIKELY((a >= b)) ? a % b : a

function void
SystemCall(const char * Command, char* Result)
{
    fprintf(stdout, "System call: %s\n", Command);
    FILE *cmd = popen(Command, "r");
    while (fgets(Result, sizeof(Result), cmd) != 0) {
        fprintf(stdout, "System call [out]: %s\n", Result);
    }
    pclose(cmd);
}

function void
ReplaceCharInLine(char* S, char Find, char Replace)
{    
    while (1) {
        S = strchr(S, Find);
        if (!S)
            break;
        *S = Replace;
    }
}

function void
RemoveExtensionInLine(char* S) {
    char* LastPoint = 0;
    while (1) {
        S = strchr(S, '.');
        if (!S)
            break;
        
        LastPoint = S;
        ++S;
    }

    if (LastPoint)
        LastPoint[0] = '\0';
}

// Note: This function returns a pointer to a substring of the original string.
// If the given string was allocated dynamically, the caller must not overwrite
// that pointer with the returned value, since the original pointer must be
// deallocated using the same allocator with which it was allocated.  The return
// value must NOT be deallocated using free() etc.
function char*
TrimInLine(char *S)
{
    // Trim leading space
    while(isspace((unsigned char)*S)) S++;

    if (*S == 0)  // All spaces?
    return S;

    // Trim trailing space
    char *End = S + strlen(S) - 1;
    while (End > S && isspace((unsigned char)*End)) End--;

    // Write new null terminator character
    End[1] = '\0';

    return S;
}