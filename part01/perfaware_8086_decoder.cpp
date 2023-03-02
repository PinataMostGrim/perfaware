#include "Windows.h"
#include <stdio.h>


int main(int argc, char const *argv[])
{
    char AssemblyFilename[] = "listing_0037_single_register_mov";
    printf("opening assembly '%s'\n", AssemblyFilename);

    HANDLE AssemblyHandle = CreateFileA(AssemblyFilename,
                                        GENERIC_READ,
                                        FILE_SHARE_READ,
                                        0,
                                        OPEN_EXISTING,
                                        0,
                                        0);

    if(AssemblyHandle == INVALID_HANDLE_VALUE)
    {
        printf("Invalid file handle");
        return 1;
    }
    printf("opened successfully\n");

    // read file

    // parse line
    // produce disassembly

    printf("closing %s\n", AssemblyFilename);
    CloseHandle(AssemblyHandle);

    return 0;
}
