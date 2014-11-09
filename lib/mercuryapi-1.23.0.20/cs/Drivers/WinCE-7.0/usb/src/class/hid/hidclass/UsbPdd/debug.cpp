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

    debug.cpp

Abstract:  
    USB Client Driver for Human Interface Device (HID) Class.

Functions:

Notes: 

--*/

#include "usbhid.h"


#ifdef DEBUG


// Debug Zones
#define DBG_ERROR              0x0001
#define DBG_WARNING            0x0002
#define DBG_INIT               0x0004
#define DBG_FUNCTION           0x0008

#define DBG_HID_DATA           0x0010
#define DBG_COMMENT            0x0020

#define DBG_PARSE              0x0100

#define DBG_USB_CLIENT         0x8000

DBGPARAM dpCurSettings = {
    _T("UsbHid"), 
    {
        _T("Errors"), _T("Warnings"), _T("Init"), _T("Function"),
        _T("HID Data"), _T("Comments"), _T(""), _T(""),
        _T("Parsing"), _T(""), _T(""), _T(""),
        _T(""), _T(""), _T(""), _T("USB Client") 
    },
    DBG_ERROR | DBG_WARNING 
};


//
// ***** Debug utility functions *****
//


// Returns true if the Hid context is locked
BOOL
IsUsbHidContextLocked(
    const USBHID_CONTEXT *pUsbHid
    )
{
    PREFAST_DEBUGCHK(pUsbHid != NULL);
    return DebugIsLocked(pUsbHid->cInCS);
}


// Take the Hid context lock
void
LockUsbHidContext(
    USBHID_CONTEXT *pUsbHid
    )
{
    DEBUGCHK(pUsbHid != NULL);
    DebugLock(&pUsbHid->csLock, &pUsbHid->cInCS);
}


// Release the Hid context lock
void
UnlockUsbHidContext(
    USBHID_CONTEXT *pUsbHid
    )
{
    DEBUGCHK(pUsbHid != NULL);
    DebugUnlock(&pUsbHid->csLock, &pUsbHid->cInCS);
}


// Verify the integrity of a device context.
void
ValidateUsbHidContext(
    PUSBHID_CONTEXT pUsbHid 
    )
{
    PREFAST_DEBUGCHK(pUsbHid != NULL);

    LockUsbHidContext(pUsbHid);

    DEBUGCHK(pUsbHid->Sig == USB_HID_SIG);
    DEBUGCHK(pUsbHid->hEP0Event != NULL);
    DEBUGCHK(pUsbHid->hUsbDevice != NULL);
    DEBUGCHK(pUsbHid->InterruptIn.hPipe != NULL);
    DEBUGCHK(pUsbHid->InterruptIn.hEvent != NULL);
    DEBUGCHK(pUsbHid->pUsbFuncs != NULL);
    DEBUGCHK(pUsbHid->pUsbInterface != NULL);
    if (pUsbHid->fhThreadInited == TRUE) {
        DEBUGCHK(pUsbHid->hThread != NULL);
    }

    UnlockUsbHidContext(pUsbHid);
}


#endif // DEBUG


