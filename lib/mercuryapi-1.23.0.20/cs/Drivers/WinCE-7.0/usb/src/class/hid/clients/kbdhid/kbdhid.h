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

#ifndef _KBDHID_H_
#define _KBDHID_H_

#include "hiddi.h"
#include "hidpi.h"
#include <windev.h>


#define CLASS_NAME_SZ           _T("Keyboard")
#define CLIENT_REGKEY_SZ        _T("Drivers\\HID\\ClientDrivers\\") CLASS_NAME_SZ


#define HID_KBD_SIG 'dbKH' // "HKbd"


enum AutoRepeatState {
    AR_WAIT_FOR_ANY = 0,
    AR_INITIAL_DELAY,
    AR_AUTOREPEATING,
};

// Hid keyboard data structure. One for each keyboard TLC.
typedef struct _HID_KBD {
    DWORD dwSig;
    HID_HANDLE hDevice;
    PCHID_FUNCS pHidFuncs;
    PHIDP_PREPARSED_DATA phidpPreparsedData;
    HIDP_CAPS hidpCaps;
    HANDLE hThread;
    HANDLE hOSDevice;
    HANDLE hevClosing;
    
    DWORD dwMaxUsages;
    KEY_STATE_FLAGS KeyStateFlags;
    PUSAGE_AND_PAGE puapPrevUsages;
    PUSAGE_AND_PAGE puapCurrUsages;
    PUSAGE_AND_PAGE puapBreakUsages;
    PUSAGE_AND_PAGE puapMakeUsages;
    PUSAGE_AND_PAGE puapOldMakeUsages;

    PCHAR  pbOutputBuffer;
    USHORT cbOutputBuffer;

    AutoRepeatState ARState;

#ifdef DEBUG
    BOOL fhThreadInited;
    BOOL fhOSDeviceInited;
#endif
} HID_KBD, *PHID_KBD;

#define VALID_HID_KBD( p ) \
   ( p && (HID_KBD_SIG == p->dwSig) )

extern DWORD g_dwAutoRepeatInitialDelay;
extern DWORD g_dwAutoRepeatKeysPerSec;

extern DWORD g_dwLocaleFlags;

BOOL
SetLEDs(
    PHID_KBD pHidKbd,
    KEY_STATE_FLAGS KeyStateFlags
    );


#ifdef DEBUG

extern DBGPARAM dpCurSettings;

#define SETFNAME(name)          const TCHAR * const pszFname = (name)

#define ZONE_ERROR              DEBUGZONE(0)
#define ZONE_WARNING            DEBUGZONE(1)
#define ZONE_INIT               DEBUGZONE(2)
#define ZONE_FUNCTION           DEBUGZONE(3)

#define ZONE_USAGES             DEBUGZONE(4)

void
ValidateHidKbd(
    PHID_KBD pHidKbd
    );

#else

#define SETFNAME(name)
#define ValidateHidKbd(ptr)

#endif // DEBUG


#endif

