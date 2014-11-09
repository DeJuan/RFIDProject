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

#ifndef _MOUHID_H_
#define _MOUSEHID_H_

#include "hidpi.h"
#include "hiddi.h"
#include <cedrv_guid.h>

#define HID_MOUSE_SIG 'uoMH' // "HKbd"

// Hid mouse data structure. One for each mouse TLC.
typedef struct _HID_MOUSE {
    DWORD dwSig;
    HID_HANDLE hDevice;
    PCHID_FUNCS pHidFuncs;
    PHIDP_PREPARSED_DATA phidpPreparsedData;
    HIDP_CAPS hidpCaps;
    HANDLE hThread;

    DWORD dwMaxUsages;
    DWORD dwInstanceId;
    PUSAGE puPrevUsages;
    PUSAGE puCurrUsages;
    PUSAGE puBreakUsages;
    PUSAGE puMakeUsages;

    USAGE usageWheel; // The usage used to get the wheel movement.
    
#ifdef DEBUG
    BOOL fhThreadInited;
#endif
} HID_MOUSE, *PHID_MOUSE;

#define VALID_HID_MOUSE( p ) \
   ( p && (HID_MOUSE_SIG == p->dwSig) )

#define HAS_NO_WHEEL 0

#define HID_MOUSE_NAME TEXT("HID MOUSE")

#ifndef dim
// Prefer to use _countof() directly, replacing dim()
#pragma deprecated("dim")
#define dim(x) _countof(x)
#endif

#ifdef DEBUG

extern DBGPARAM dpCurSettings;

#define SETFNAME(name)          const TCHAR * const pszFname = (name)

#define ZONE_ERROR              DEBUGZONE(0)
#define ZONE_WARNING            DEBUGZONE(1)
#define ZONE_INIT               DEBUGZONE(2)
#define ZONE_FUNCTION           DEBUGZONE(3)

#define ZONE_USAGES             DEBUGZONE(4)

#else

#define SETFNAME(name)

#endif // DEBUG


#endif

