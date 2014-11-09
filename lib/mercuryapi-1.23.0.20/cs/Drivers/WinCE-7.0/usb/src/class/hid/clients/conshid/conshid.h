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

#ifndef _CONSHID_H_
#define _CONSHID_H_

#include "hidpi.h"
#include "hiddi.h"


#define HID_CONSUMER_COLLECTION_CONSUMER_CONTROL ((USAGE) 0x0001)

// Transport controls
#define HID_CONSUMER_SCAN_NEXT_TRACK            ((USAGE) 0x00B5)
#define HID_CONSUMER_SCAN_PREVIOUS_TRACK        ((USAGE) 0x00B6)
#define HID_CONSUMER_STOP                       ((USAGE) 0x00B7)
#define HID_CONSUMER_PLAY_PAUSE                 ((USAGE) 0x00CD)

// Audio controls 
#define HID_CONSUMER_VOLUME_MUTE                ((USAGE) 0x00E2)
#define HID_CONSUMER_VOLUME_INCREMENT           ((USAGE) 0x00E9)
#define HID_CONSUMER_VOLUME_DECREMENT           ((USAGE) 0x00EA)


// Application launch controls
#define HID_CONSUMER_LAUNCH_CONFIGURATION       ((USAGE) 0x0183)
#define HID_CONSUMER_LAUNCH_EMAIL               ((USAGE) 0x018A)
#define HID_CONSUMER_LAUNCH_CALCULATOR          ((USAGE) 0x0192)
#define HID_CONSUMER_LAUNCH_BROWSER             ((USAGE) 0x0194)


// Generic GUI aplication controls
#define HID_CONSUMER_APP_SEARCH                 ((USAGE) 0x0221)
#define HID_CONSUMER_APP_HOME                   ((USAGE) 0x0223)
#define HID_CONSUMER_APP_BACK                   ((USAGE) 0x0224)
#define HID_CONSUMER_APP_FORWARD                ((USAGE) 0x0225)
#define HID_CONSUMER_APP_STOP                   ((USAGE) 0x0226)
#define HID_CONSUMER_APP_REFRESH                ((USAGE) 0x0227)
#define HID_CONSUMER_APP_BOOKMARKS              ((USAGE) 0x022A)



#define HID_CONSUMER_SIG 'noCH' // "HKbd"



// Hid consumer data structure. One for each consumer TLC.
typedef struct _HID_CONSUMER {
    DWORD dwSig;
    HID_HANDLE hDevice;
    PCHID_FUNCS pHidFuncs;
    PHIDP_PREPARSED_DATA phidpPreparsedData;
    HIDP_CAPS hidpCaps;
    HANDLE hThread;
    
    DWORD dwMaxUsages;
    HIDP_KEYBOARD_MODIFIER_STATE Modifiers;
    PUSAGE puPrevUsages;
    PUSAGE puCurrUsages;
    PUSAGE puBreakUsages;
    PUSAGE puMakeUsages;

#ifdef DEBUG
    BOOL fhThreadInited;
#endif
} HID_CONSUMER, *PHID_CONSUMER;

#define VALID_HID_CONSUMER( p ) \
   ( p && (HID_CONSUMER_SIG == p->dwSig) )


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

