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
    hidmdd.h

Abstract:
    Header for Human Interface Device (HID) Class Driver MDD.

Functions:

Notes:

--*/

#ifndef _HIDMDD_H_
#define _HIDMDD_H_

#include <windows.h>

#include <hid.h>
#include <hidpddi.h>
#include <hiddi.h>

#include "queue.h"


#define UnusedParameter(x) UNREFERENCED_PARAMETER(x)
#pragma deprecated("UnusedParameter")

#ifndef dim
// Prefer to use _countof() directly, replacing dim()
#pragma deprecated("dim")
#define dim(x) _countof(x)
#endif

#define DEFAULT_SZ              _T("Default")
#define LOAD_CLIENTS_PATH_SZ    _T("Drivers\\HID\\LoadClients")
#define DLL_VALUE_SZ            _T("Dll")
#define REMOTE_WAKEUP_VALUE_SZ  _T("RemoteWakeup")
#define IDLE_RATE_VALUE_SZ      _T("IdleRate")


#define HID_SIG 'DdiH' // "HidD" tag
#define HID_CLIENT_SIG 'CdiH' // "HidC" tag

#define MANUAL_RESET_EVENT TRUE
#define AUTO_RESET_EVENT   FALSE

extern const HID_FUNCS g_HidFuncs;

struct _HID_CLIENT_HANDLE; // Forward declaration


typedef struct _HID_CONTEXT {
    // We use a Signature (defined above) for validation
    ULONG Sig;

    // The device handle received from the HID PDD.
    HID_PDD_HANDLE hPddDevice;

    // Our parsed report descriptor data
    HIDP_DEVICE_DESC hidpDeviceDesc;

    // The queues used to send data to HID clients
    HidTLCQueue *pQueues;

    // The structures passed to HID clients as a handle
    struct _HID_CLIENT_HANDLE *pClientHandles;

} HID_CONTEXT, *PHID_CONTEXT;
typedef HID_CONTEXT const * PCHID_CONTEXT;


// Is the pointer a valid PHID_CONTEXT
#define VALID_HID_CONTEXT( p ) \
   ( p && (HID_SIG == p->Sig) )
   

// This is what gets passed to HID clients as a handle
typedef struct _HID_CLIENT_HANDLE {
    // We use a Signature (defined above) for validation
    ULONG Sig;

    // The hInst of the client DLL
    HINSTANCE hInst;

    // A parameter filled in by the client
    LPVOID lpvNotifyParameter;

    // The HID_CONTEXT that this client refers to
    PHID_CONTEXT pHidContext;

    // The parsed HID report descriptor.
    PHIDP_PREPARSED_DATA phidpPreparsedData;

    // The queue for this Top Level Collection
    HidTLCQueue *pQueue;

    // The settings used to find the registry key for this client
    HID_DRIVER_SETTINGS driverSettings;
} HID_CLIENT_HANDLE, *PHID_CLIENT_HANDLE;

// Is the pointer a valid PHID_CLIENT_HANDLE
#define VALID_CLIENT_HANDLE( p ) \
   ( p && (HID_CLIENT_SIG == p->Sig) )


#define RegOpenKey(hkey, lpsz, phk) \
        RegOpenKeyEx((hkey), (lpsz), 0, 0, (phk))


#ifdef DEBUG

//
// ***** Debug utility functions *****
// 

void
ValidateHidContext(
    PCHID_CONTEXT pHidContext 
    );

void
ValidateClientHandle(
    PHID_CLIENT_HANDLE pHidClient
    );

void 
DumpHIDDeviceDescription(
    const HIDP_DEVICE_DESC *phidpDeviceDesc
    );

void
DumpHIDReportDescriptor(
    const BYTE *pbReportDescriptor,
    DWORD cbReportDescriptor
    );

#else

#define ValidateHidContext(ptr)
#define ValidateClientHandle(ptr)
#define DumpHIDDeviceDescription(ptr)
#define DumpHIDReportDescriptor(ptr, dw)

#endif // DEBUG


#endif // _HIDMDD_H_

