//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
//
// Use of this sample source code is subject to the terms of the Microsoft
// license agreement under which you licensed this sample source code. If
// you did not accept the terms of the license agreement, you are not
// authorized to use this sample source code. For the terms of the license,
// please see the license agreement between you and Microsoft or, if applicable,
// see the LICENSE.RTF on your install media or the root of your tools installation.
// THE SAMPLE SOURCE CODE IS PROVIDED "AS IS", WITH NO WARRANTIES OR INDEMNITIES.
//
/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

Module Name:

logging.cpp

Abstract:
 logging functions implementation.

Notes:

--*/
#include <windows.h>
#include "logging.h"


//------------------------------------------------------------------------------
// Local Functions
static VOID OpenLogFile(__in size_t ecount,WCHAR* LogFileName);
static VOID  CloseLogFile();
static int LogRoutine(const WCHAR *pFormat,va_list pArguments);

//------------------------------------------------------------------------------
// Global Variables
FILE *g_LogFileHandle = NULL;
volatile USBSER_LOG_LEVEL g_DebugLevel = LOG_LEVEL_INFO;
static DWORD g_dwLogToFile = 0;

//------------------------------------------------------------------------------
//
// Function: InitUSBSerFileLogging
//
// Determines the logging option for this driver. If file logging specified all
// the logs will be logged in to the FILE else all the logs will be printed on
// the platform builder debug output. The file logging options can be specified
// through registry.
//------------------------------------------------------------------------------
VOID InitUSBSerFileLogging()
{
    TCHAR szFilePath[MAX_PATH] = {0};
    TCHAR szFile[] = TEXT("usbstresslog.txt");
    HKEY  hKey = NULL;
    DWORD dwType  = REG_NONE;
    DWORD dwSize  = 0;

    // Default all logs goes to PB output
    g_dwLogToFile = 0x00;
    g_DebugLevel  = LOG_LEVEL_INFO;

    // Determine file logging and debug level options.
    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, USBSER_CLIENT_REGKEY_SZ, 0, 0, &hKey))
    {
        dwType = REG_DWORD;
        dwSize = sizeof(DWORD);

        // Determine if file logging is enabled.
        RegQueryValueEx(hKey, REGVAL_FILE_LOGGING, 0, &dwType, (LPBYTE)&g_dwLogToFile, &dwSize);

        // Determine debug level settings.
        dwType = REG_DWORD;
        dwSize = sizeof(g_DebugLevel);

        RegQueryValueEx(hKey, REGVAL_FILE_LOGLEVEL, 0, &dwType, (LPBYTE)&g_DebugLevel, &dwSize);

        // Default to LOG_LEVEL_INFO if invalid setting specified.
        if (g_DebugLevel > LOG_LEVEL_NONE)
        {
            g_DebugLevel = LOG_LEVEL_INFO;
        }

        RegCloseKey(hKey);
    }

    if (g_dwLogToFile)
    {
        // Log to file
        StringCchPrintf (szFilePath, _countof(szFilePath), _T("\\temp\\%s"), szFile);
        OpenLogFile(0,szFilePath);
    }
}

//------------------------------------------------------------------------------
//
// Function: DeInitUSBSerFileLogging
//------------------------------------------------------------------------------
VOID DeInitUSBSerFileLogging()
{
    // Close the log file when the device is disconnected
    CloseLogFile();
}   
//------------------------------------------------------------------------------
//
// Function: OpenLogFile
//------------------------------------------------------------------------------
static VOID OpenLogFile(
    __in size_t ecount,
    WCHAR* LogFileName)
{
    UNREFERENCED_PARAMETER(ecount);

    if (g_LogFileHandle == NULL)
    {
        g_LogFileHandle = _wfopen(LogFileName, TEXT("w"));
        if (g_LogFileHandle == NULL)
        {
            // Failure: logs goes to PB output
            g_dwLogToFile = 0x00;

            RETAILMSG(1, (TEXT("Log file handle open failed with GetLastError = 0x%08X\n"),
                 GetLastError()));
        }
    }

    return;
}

//------------------------------------------------------------------------------
//
// Function: CloseLogFile
//------------------------------------------------------------------------------
static VOID CloseLogFile(VOID)
{
    if (g_LogFileHandle != NULL)
    {
        if (fclose(g_LogFileHandle) != 0)
        {
            RETAILMSG(1, (TEXT("Log file close() failed with GetLastError = 0x%08X\n"),
                 GetLastError()));
        }
    }
    return;
}

//------------------------------------------------------------------------------
//
// Function: LogRoutine
//------------------------------------------------------------------------------
static int LogRoutine(
    const WCHAR *pFormat,
    va_list pArguments
    )
{
#define LOG_BUFFER_SIZE  1024

    WCHAR TempBuffer[LOG_BUFFER_SIZE] = {0};

    if (SUCCEEDED(StringCbVPrintf(TempBuffer, 
                                  LOG_BUFFER_SIZE * sizeof(WCHAR), 
                                  pFormat,
                                  pArguments)))
    {
        if (g_dwLogToFile == 0)
        {
             OutputDebugString(TempBuffer);
             return 0;
        }        
        else if (g_LogFileHandle != NULL)
        {
             return fwprintf(g_LogFileHandle,TempBuffer);
        }
        return 0;

    }
    return 0;
}

//------------------------------------------------------------------------------
//
// Function: LogVerbose
//------------------------------------------------------------------------------
int
LogVerbose(
    const WCHAR *pFormat,
    ...
    )
{
    int returnValue = 0;
    va_list pArguments = NULL;
    va_start(pArguments, pFormat);

    if (g_DebugLevel <= LOG_LEVEL_VERBOSE)
    { 
        returnValue = LogRoutine(pFormat, pArguments);
    }
    else
    {
        returnValue = -1;
    }

    va_end(pArguments);
    return returnValue;
}

//------------------------------------------------------------------------------
//
// Function: LogError
//------------------------------------------------------------------------------
int
LogError(
    const WCHAR *pFormat,
    ...
    )
{
    int returnValue = 0;
    va_list pArguments = NULL;
    va_start(pArguments, pFormat);

    if (g_DebugLevel <= LOG_LEVEL_ERROR)
    { 
        returnValue = LogRoutine(pFormat, pArguments);
    }
    else
    {
        returnValue = -1;
    }

    va_end(pArguments);
    return returnValue;
}

//------------------------------------------------------------------------------
//
// Function: LogInfo
//------------------------------------------------------------------------------
int
LogInfo(
    const WCHAR *pFormat,
    ...
    )
{
    int returnValue = 0;
    va_list pArguments = NULL;
    va_start(pArguments, pFormat);

    if (g_DebugLevel <= LOG_LEVEL_INFO)
    { 
        returnValue = LogRoutine(pFormat, pArguments);
    } 
    else 
    {
        returnValue = -1;
    }

    va_end(pArguments);
    return returnValue;
}

//------------------------------------------------------------------------------
//
// Function: Log
//------------------------------------------------------------------------------
int Log(const WCHAR *pFormat,  ... )
{
    int returnValue = 0;
    va_list pArguments = NULL;
    va_start(pArguments, pFormat);

    returnValue = LogRoutine(pFormat, pArguments);

    va_end(pArguments);
    return returnValue;
}
