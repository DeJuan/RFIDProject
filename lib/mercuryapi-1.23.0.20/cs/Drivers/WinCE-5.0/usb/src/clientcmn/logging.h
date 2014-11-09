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

logging.h

Abstract:  

Notes: 

--*/

#ifndef __USBSER_LOGGING_H__
#define __USBSER_LOGGING_H__

// Macros to create Unicode function name
#define WIDEN2(x) L ## x
#define WIDEN(x) WIDEN2(x)
#define __WFUNCTION__ WIDEN(__FUNCTION__)

#define _countof( x ) ( sizeof( x )/sizeof( x[0] ) )

typedef enum _USBSER_LOG_LEVEL {
    LOG_LEVEL_VERBOSE     = 0,
    LOG_LEVEL_INFO        = 1,
    LOG_LEVEL_ERROR       = 2,
    LOG_LEVEL_NONE        = 3,
}USBSER_LOG_LEVEL;

#define USBSER_CLIENT_REGKEY_SZ TEXT("Drivers\\USB\\ClientDrivers\\USBSER_CLASS")
#define REGVAL_FILE_LOGGING   TEXT("FileLogging")
#define REGVAL_FILE_LOGLEVEL  TEXT("DebugLogLevel")

VOID InitUSBSerFileLogging();
VOID DeInitUSBSerFileLogging();
int  Log(const WCHAR *pFormat,  ... );
int  LogInfo(const WCHAR *pFormat, ...);
int  LogError(const WCHAR *pFormat,...);
int  LogVerbose(const WCHAR *pFormat,...);

#endif //__USBSER_LOGGING_H__
