#include <stdarg.h>
#include <stdio.h>
#include <wchar.h>

#include "Misc/Types.h"
#include "WindowsOutputDevices.h"

// ----
// Platform specific globals variables
// ----

OutputDevice*       GLog = new WindowsOutputDevice();
OutputDevice*       GWarn = GLog;
OutputDevice*       GError = GLog;

// ----
// Platform specific functions
// ----

int appGetVarArgs( tchar* InOutDest, uint32 InDestSize, uint32 InCount, const tchar*& InFormat, va_list InArgPtr )
{
    return vswprintf( InOutDest, InCount, InFormat, InArgPtr );
}

int appGetVarArgsAnsi( achar* InOutDest, uint32 InDestSize, uint32 InCount, const achar*& InFormat, va_list InArgPtr )
{
    return vsnprintf( InOutDest, InCount, InFormat, InArgPtr );
}