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
//
//

#include "mouhid.h"
#include <windev.h>
#include "MouHidLib.h"
#include <svsutil.hxx>

#define REG_GUID_FORMAT_W   L"{%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}"
// Global variables
static BOOL g_fUseMouseHook = FALSE;

#ifdef DEBUG

// Debug Zones
#define DBG_ERROR              0x0001
#define DBG_WARNING            0x0002
#define DBG_INIT               0x0004
#define DBG_FUNCTION           0x0008

#define DBG_USAGES             0x0010

DBGPARAM dpCurSettings = {
        TEXT("MouHid"), {
        TEXT("Errors"), TEXT("Warnings"), TEXT("Init"), TEXT("Function"),
        TEXT("Usages"), TEXT(""), TEXT(""), TEXT(""),
        TEXT(""), TEXT(""), TEXT(""), TEXT(""),
        TEXT(""), TEXT(""), TEXT(""), TEXT("") },
        DBG_ERROR | DBG_WARNING};
        
#endif // DEBUG


// Conversions from button usages to mouse_event flags
static const DWORD g_rgdwUsageToDownButton[] = {
    0, // Undefined
    MOUSEEVENTF_LEFTDOWN,
    MOUSEEVENTF_RIGHTDOWN,
    MOUSEEVENTF_MIDDLEDOWN,
};

static const DWORD g_rgdwUsageToUpButton[] = {
    0, // Undefined
    MOUSEEVENTF_LEFTUP,
    MOUSEEVENTF_RIGHTUP,
    MOUSEEVENTF_MIDDLEUP,
};


#ifdef DEBUG

static
void
ValidateHidMouse(
    PHID_MOUSE pHidMouse
    );

#else

#define ValidateHidMouse(ptr)

#endif // DEBUG


static BOOL
AllocateUsageLists(
    PHID_MOUSE pHidMouse,
    size_t cbUsages
    );

static void
DetermineWheelUsage(
    PHID_MOUSE pHidMouse
    );

static void
SetButtonFlags(
    PDWORD pdwFlags,
    USAGE const*const pUsages,
    DWORD dwMaxUsages,
    const DWORD *pdwUsageMappings,
    DWORD cdwUsageMappings
    );

static void
ProcessMouseReport(
    PHID_MOUSE pHidMouse,
    PCHAR pbHidPacket,
    DWORD cbHidPacket
    );

static void
MouseEvent(
    DWORD dwFlags,
    DWORD dx,
    DWORD dy,
    DWORD dwData
    );

VOID
FreeHidMouse(
    PHID_MOUSE pHidMouse
    );


VOID 
AdvertiseHidMouseInterface(
    PHID_MOUSE pHidMouse, 
    BOOL fAdd
    );

// Dll entry function.
extern "C" 
BOOL
DllEntry(
    HANDLE hDllHandle,
    DWORD dwReason, 
    LPVOID lpReserved
    )
{
    SETFNAME(_T("MOUHID DllEntry"));
    
    UNREFERENCED_PARAMETER(lpReserved);
    
    switch (dwReason) {
        case DLL_PROCESS_ATTACH:
            DEBUGREGISTER((HINSTANCE)hDllHandle);
            DEBUGMSG(ZONE_INIT, (_T("%s: Attach\r\n"), pszFname));
            DisableThreadLibraryCalls((HMODULE) hDllHandle);
            g_fUseMouseHook = DriverMouseHookInitialize(hDllHandle);
            break;
            
        case DLL_PROCESS_DETACH:
            DEBUGMSG(ZONE_INIT, (_T("%s: Detach\r\n"), pszFname));
            break;
            
        default:
            break;
    }
    
    return TRUE ;
}


// Get interrupt reports from the device. Thread exits when the device has
// been removed.
static
DWORD
WINAPI
MouseThreadProc(
    LPVOID lpParameter
    )
{
    SETFNAME(_T("MouseThreadProc"));

    PHID_MOUSE pHidMouse = (PHID_MOUSE) lpParameter;
    
    PREFAST_DEBUGCHK(pHidMouse != NULL);

    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);
    
    const DWORD cbMaxHidPacket = pHidMouse->hidpCaps.InputReportByteLength;
    PCHAR pbHidPacket = (PCHAR) LocalAlloc(LMEM_FIXED, cbMaxHidPacket);
    if (pbHidPacket == NULL) {
        DEBUGMSG(ZONE_ERROR, (TEXT("%s: LocalAlloc error:%d\r\n"), pszFname, GetLastError()));
        goto EXIT;
    }

    for (;;) {
        ValidateHidMouse(pHidMouse);

        DWORD cbHidPacket;
    
        // Get an interrupt report from the device.
        DWORD dwErr = pHidMouse->pHidFuncs->lpGetInterruptReport(
            pHidMouse->hDevice,
            pbHidPacket,
            cbMaxHidPacket,
            &cbHidPacket,
            NULL,
            INFINITE);

        if (dwErr != ERROR_SUCCESS) {
            if ( (dwErr != ERROR_DEVICE_REMOVED) && (dwErr != ERROR_CANCELLED) ) {
                DEBUGMSG(ZONE_ERROR, 
                    (_T("%s: GetInterruptReport returned unexpected error %u\r\n"),
                    pszFname, dwErr));
            }

            // Exit thread
            break;
        }
        else {
            DEBUGCHK(cbHidPacket <= cbMaxHidPacket);
            ProcessMouseReport(pHidMouse, pbHidPacket, cbHidPacket);
        }
    }

EXIT:
    if (pbHidPacket != NULL) LocalFree(pbHidPacket);
    DEBUGMSG(ZONE_FUNCTION, (_T("%s: Exiting thread\r\n"), pszFname));
    
    return 0;
}


// Entry point for the HID driver. Initializes the structures for this 
// keyboard and starts the thread that will receive interrupt reports.
extern "C"
BOOL
HIDDeviceAttach(
    HID_HANDLE                 hDevice, 
    PCHID_FUNCS                pHidFuncs,
    const HID_DRIVER_SETTINGS * /*pDriverSettings*/,
    PHIDP_PREPARSED_DATA       phidpPreparsedData,
    PVOID                     *ppvNotifyParameter,
    DWORD                      /*dwUnused*/
    )
{
    SETFNAME(_T("HIDDeviceAttach"));

    BOOL fRet = FALSE;
    PHID_MOUSE pHidMouse;
    size_t cbUsages;

    DEBUGCHK(hDevice != NULL);
    DEBUGCHK(pHidFuncs != NULL);
    DEBUGCHK(phidpPreparsedData != NULL);
    PREFAST_DEBUGCHK(ppvNotifyParameter != NULL);

    // Allocate this keyboard's data structure and fill it.
    pHidMouse = (PHID_MOUSE) LocalAlloc(LPTR, sizeof(HID_MOUSE));
    if (pHidMouse == NULL) {
        DEBUGMSG(ZONE_ERROR, (TEXT("%s: LocalAlloc error:%d\r\n"), pszFname, GetLastError()));
        goto EXIT;
    }

    pHidMouse->dwSig = HID_MOUSE_SIG;
    pHidMouse->hDevice = hDevice;
    pHidMouse->pHidFuncs = pHidFuncs;
    pHidMouse->phidpPreparsedData = phidpPreparsedData;
    HidP_GetCaps(pHidMouse->phidpPreparsedData, &pHidMouse->hidpCaps);

    // Get the total number of usages that can be returned in an input packet.
    // This could be zero, like if the device was just a scroll wheel.
    pHidMouse->dwMaxUsages = HidP_MaxUsageListLength(HidP_Input, 
        HID_USAGE_PAGE_BUTTON, phidpPreparsedData);
    cbUsages = pHidMouse->dwMaxUsages * sizeof(USAGE);

    if (AllocateUsageLists(pHidMouse, cbUsages) == FALSE) {
        goto EXIT;
    }

    // Do we have a mouse wheel?
    DetermineWheelUsage(pHidMouse);

    // Create the thread that will receive reports from this device
    pHidMouse->hThread = CreateThread(NULL, 0, MouseThreadProc, pHidMouse, 0, NULL);
    if (pHidMouse->hThread == NULL) {
        DEBUGMSG(ZONE_ERROR, (_T("%s: Failed creating mouse thread\r\n"), 
            pszFname));
        goto EXIT;
    }
#ifdef DEBUG
    pHidMouse->fhThreadInited = TRUE;
#endif

    *ppvNotifyParameter = pHidMouse;
    ValidateHidMouse(pHidMouse);
    fRet = TRUE;

    //Advertise HID mouse interface attach.
    AdvertiseHidMouseInterface(pHidMouse, TRUE);
    
EXIT:
    if ((fRet == FALSE) && (pHidMouse != NULL)) {
        FreeHidMouse(pHidMouse);
    }
    
    return fRet;
}
#ifdef DEBUG
// Match function with typedef.
static LPHID_CLIENT_ATTACH g_pfnDeviceAttach = HIDDeviceAttach;
#endif


// Entry point for the HID driver to give us notifications.
extern "C" 
BOOL 
WINAPI
HIDDeviceNotifications(
    DWORD  dwMsg,
    WPARAM wParam, // Message parameter
    PVOID  pvNotifyParameter
    )
{
    SETFNAME(_T("HIDDeviceNotifications"));

    BOOL fRet = FALSE;
    PHID_MOUSE pHidMouse = (PHID_MOUSE) pvNotifyParameter;
    DWORD dwFlags = 0;

    UNREFERENCED_PARAMETER(wParam);

    if (VALID_HID_MOUSE(pHidMouse) == FALSE) {
        DEBUGMSG(ZONE_ERROR, (_T("%s: Received invalid structure pointer\r\n"), pszFname));
        goto EXIT;
    }

    ValidateHidMouse(pHidMouse);
    
    switch(dwMsg) {
        case HID_CLOSE_DEVICE:
            // Free all of our resources.
            WaitForSingleObject(pHidMouse->hThread, INFINITE);
            CloseHandle(pHidMouse->hThread);
            pHidMouse->hThread = NULL;

            // Send ups for each button that is still down.
            SetButtonFlags(&dwFlags, pHidMouse->puPrevUsages, pHidMouse->dwMaxUsages,
                g_rgdwUsageToUpButton, _countof(g_rgdwUsageToUpButton));
            MouseEvent(dwFlags, 0, 0, 0);

            //Advertise HID Mouse interface detach
            AdvertiseHidMouseInterface(pHidMouse, FALSE);
            
            FreeHidMouse(pHidMouse);
            fRet = TRUE;
            break;

        default:
            DEBUGMSG(ZONE_ERROR, (_T("%s: Unhandled message %u\r\n"), pszFname));
            break;
    };

EXIT:
    return fRet;
}
#ifdef DEBUG
// Match function with typedef.
static LPHID_CLIENT_NOTIFICATIONS g_pfnDeviceNotifications = HIDDeviceNotifications;
#endif


// Allocate the usage lists for this mouse. The lists are allocated in 
// a single block and then divided up.
static
BOOL
AllocateUsageLists(
    PHID_MOUSE pHidMouse,
    size_t cbUsages
    )
{
    SETFNAME(_T("AllocateUsageLists"));

    BOOL fRet = FALSE;
    size_t cbTotal;
    PBYTE pbStart;
    DWORD dwIdx;
    PUSAGE *rgppUsages[] = {
        &pHidMouse->puPrevUsages,
        &pHidMouse->puCurrUsages,
        &pHidMouse->puBreakUsages,
        &pHidMouse->puMakeUsages
    };
    
    DEBUGCHK(pHidMouse != NULL);

    // Allocate a block of memory for all the usage lists
    if (cbUsages < MAXDWORD/_countof(rgppUsages)) {
        cbTotal = cbUsages * _countof(rgppUsages);
        pbStart = (PBYTE) LocalAlloc(LPTR, cbTotal);
    }
    else {
        pbStart = NULL;
        ASSERT(FALSE);
    };
    
    if (pbStart == NULL) {
       DEBUGMSG(ZONE_ERROR, (TEXT("%s: LocalAlloc error:%d\r\n"), pszFname, GetLastError()));
       goto EXIT;
    }

    // Divide the block.
    for (dwIdx = 0; dwIdx < _countof(rgppUsages); ++dwIdx) {
        *rgppUsages[dwIdx] = (PUSAGE) (pbStart + (cbUsages * dwIdx));
    }

    fRet = TRUE;

EXIT:
    return fRet;
}


// Find out if this mouse has a wheel or Z axis
static
void
DetermineWheelUsage(
    PHID_MOUSE pHidMouse
    )
{
    static const USAGE rgWheelUsages[] = {
        HID_USAGE_GENERIC_WHEEL,
        HID_USAGE_GENERIC_Z,
    };
    
    NTSTATUS status;
    HIDP_VALUE_CAPS hidpValueCaps;
    USHORT cValueCaps;
    DWORD dwIdx;    
    
    PREFAST_DEBUGCHK(pHidMouse != NULL);

    for (dwIdx = 0; dwIdx < _countof(rgWheelUsages); ++dwIdx) 
    {
        USAGE usageCurr = rgWheelUsages[dwIdx];
        cValueCaps = 1;  // Only one structure to fill
        
        status = HidP_GetSpecificValueCaps(
            HidP_Input,
            HID_USAGE_PAGE_GENERIC, 
            0, 
            usageCurr, 
            &hidpValueCaps, 
            &cValueCaps, 
            pHidMouse->phidpPreparsedData);

        if (NT_SUCCESS(status)) {
            // We found our wheel axis
            pHidMouse->usageWheel = usageCurr;
            break;
        }
    }

    if (dwIdx == _countof(rgWheelUsages)) {
        // We did not find a wheel axis
        pHidMouse->usageWheel = HAS_NO_WHEEL;
    }    
}


// Set the mouse_event button flags for this list of usages.
static
void
SetButtonFlags(
    PDWORD pdwFlags,
    USAGE const*const pUsages,
    DWORD dwMaxUsages,
    const DWORD *pdwUsageMappings, // Array of usage->flags to use
    DWORD cdwUsageMappings // Count of usage->flags mappings
    )
{
    DWORD dwIdx;

    DEBUGCHK(pdwFlags != NULL);
    PREFAST_DEBUGCHK(pUsages != NULL);
    DEBUGCHK(pdwUsageMappings != NULL);
    
    for (dwIdx = 0; dwIdx < dwMaxUsages; ++dwIdx) {
        const USAGE usage = pUsages[dwIdx];

        if (usage == 0) {
            // No more set usages
            break;
        }
        else if (usage < cdwUsageMappings) {
            *pdwFlags |= pdwUsageMappings[usage];
        }
        // else this is a button we do not handle.
    }    
}


// Determine the movement on the axis defined by usage. Store it in pDelta.
static
NTSTATUS
GetAxisMovement(
    PHID_MOUSE pHidMouse,
    USAGE usage, // X, Y, Z, or Wheel
    PCHAR pbHidPacket,
    DWORD cbHidPacket,
    PLONG pDelta
    )
{
    SETFNAME(_T("GetAxisMovement"));
    
    NTSTATUS status;
    
    PREFAST_DEBUGCHK(pHidMouse != NULL);
    DEBUGCHK(pbHidPacket != NULL);
    PREFAST_DEBUGCHK(pDelta != NULL);
    
    status = HidP_GetScaledUsageValue(
        HidP_Input,
        HID_USAGE_PAGE_GENERIC,
        0,
        usage,
        pDelta,
        pHidMouse->phidpPreparsedData,
        pbHidPacket,
        cbHidPacket);

    if (status == HIDP_STATUS_BAD_LOG_PHY_VALUES) {
        // Bad physical max/min detected.
        DEBUGMSG(ZONE_ERROR, (_T("%s: Bad physical max/min for mouse axis usage 0x%x\r\n"),
            pszFname, usage));
        *pDelta = 0;
    }

    return status;
}


// Take a report generated by the device and convert it to a mouse_event
static
void
ProcessMouseReport(
    PHID_MOUSE pHidMouse,
    PCHAR pbHidPacket,
    DWORD cbHidPacket
    )
{
    NTSTATUS status;
    ULONG uCurrUsages;
    DWORD cbUsageList;
    DWORD dwFlags;
    LONG dx, dy, dz;
    
    PREFAST_DEBUGCHK(pHidMouse != NULL);
    DEBUGCHK(pbHidPacket != NULL);

    DEBUGCHK(_countof(g_rgdwUsageToDownButton) == _countof(g_rgdwUsageToUpButton));

    ValidateHidMouse(pHidMouse);

    dwFlags = dx = dy = dz = 0;
    
    uCurrUsages = pHidMouse->dwMaxUsages;
    cbUsageList = uCurrUsages * sizeof(USAGE);

    //        
    // Handle mouse buttons
    //

    // Get the usage list from this packet.
    status = HidP_GetUsages(
        HidP_Input,
        HID_USAGE_PAGE_BUTTON,
        0,
        pHidMouse->puCurrUsages,
        &uCurrUsages, // IN OUT parameter
        pHidMouse->phidpPreparsedData,
        pbHidPacket,
        cbHidPacket);
    
    if (NT_SUCCESS(status)) {
        status = HidP_UsageListDifference(
            pHidMouse->puPrevUsages,
            pHidMouse->puCurrUsages,
            pHidMouse->puBreakUsages,
            pHidMouse->puMakeUsages,
            pHidMouse->dwMaxUsages);

        if (NT_SUCCESS(status)) {
            // Determine which buttons went up and down.
            SetButtonFlags(&dwFlags, pHidMouse->puBreakUsages, pHidMouse->dwMaxUsages,
                g_rgdwUsageToUpButton, _countof(g_rgdwUsageToUpButton));
            SetButtonFlags(&dwFlags, pHidMouse->puMakeUsages, pHidMouse->dwMaxUsages,
                g_rgdwUsageToDownButton, _countof(g_rgdwUsageToDownButton));
        }
    }
    // else maybe we do not have any buttons (HIDP_STATUS_USAGE_NOT_FOUND)

    //
    // Handle mouse axis movement
    //
    GetAxisMovement(pHidMouse, HID_USAGE_GENERIC_X, pbHidPacket, cbHidPacket, &dx);
    GetAxisMovement(pHidMouse, HID_USAGE_GENERIC_Y, pbHidPacket, cbHidPacket, &dy);

    if ((dx != 0) || (dy != 0)) {
        dwFlags |= MOUSEEVENTF_MOVE;
    }

    if (pHidMouse->usageWheel != HAS_NO_WHEEL) {
        GetAxisMovement(pHidMouse, pHidMouse->usageWheel, pbHidPacket, cbHidPacket, &dz);

        if (dz != 0) {
            dz *= WHEEL_DELTA;
            dwFlags |= MOUSEEVENTF_WHEEL;
        }
    }

    if (dwFlags) {
        // Send this mouse event
        MouseEvent(dwFlags, dx, dy, dz);
    }

    // Save the current usages.
    memcpy(pHidMouse->puPrevUsages, pHidMouse->puCurrUsages, cbUsageList);
}


// Send the mouse event to GWES
static
void
MouseEvent(
    DWORD dwFlags,
    DWORD dx,
    DWORD dy,
    DWORD dwData
    )
{
    SETFNAME(_T("MouseEvent"));
    BOOL fSendEventToSystem = TRUE;

    DEBUGMSG(ZONE_USAGES, (_T("%s: HID: dx %4d, dy %4d, dwData %4d, flags 0x%04x\r\n"), pszFname,
        dx, dy, dwData, dwFlags));
    
    if (g_fUseMouseHook) {
       // See if any mouse hook app, such as Transcriber wants the mouse event
       fSendEventToSystem = !DriverMouseHook(dwFlags, dx, dy, dwData);
    }
    
    if (fSendEventToSystem) {
        mouse_event(dwFlags, dx, dy, dwData, 0);
    }
}


// Free the memory used by pHidKbd
void
FreeHidMouse(
    PHID_MOUSE pHidMouse
    )
{
    PREFAST_DEBUGCHK(pHidMouse != NULL);
    DEBUGCHK(pHidMouse->hThread == NULL); // We do not close the thread handle
    
    if (pHidMouse->puPrevUsages != NULL) LocalFree(pHidMouse->puPrevUsages);
    LocalFree(pHidMouse);    
}

void
AdvertiseHidMouseInterface(
    PHID_MOUSE pHidMouse,
    BOOL fAdd
    )
{
    union {
        BYTE rgbGuidBuffer[sizeof(GUID) + 4]; // +4 since scanf writes longs
        GUID guidBus;
    } u = { 0 };
    LPGUID pguidBus = &u.guidBus;
    LPCTSTR pszBusGuid = CE_DRIVER_HID_MOUSE_GUID;
    WCHAR szName[50] = { 0 };
    DWORD dwIndex = 1;
        
    // Parse the GUID
    int iErr = _stscanf_s(pszBusGuid, REG_GUID_FORMAT_W, SVSUTIL_PGUID_ELEMENTS(&pguidBus));
    DEBUGCHK(iErr != 0 && iErr != EOF);

    //All we care about is that the szName is unique. AdvertiseInterface is 
    //thread-safe. Keep whacking away at index until we find a unique name.
    do 
    {
        if(fAdd)
        {
            pHidMouse->dwInstanceId = dwIndex++;
        }
        VERIFY(SUCCEEDED(StringCchPrintf(szName,
                        _countof(szName),
                        TEXT("%s_%03u"), HID_MOUSE_NAME, pHidMouse->dwInstanceId)));

        iErr = AdvertiseInterface(pguidBus, szName, fAdd);
    }while (!iErr);
    
    DEBUGMSG(ZONE_FUNCTION, (L"MOUHID:%S::InterfaceName= %s", 
                                __FUNCTION__, szName));
    return;
}


#ifdef DEBUG

// Validate a PHID_KBD structure
static
void
ValidateHidMouse(
    PHID_MOUSE pHidMouse
    )
{    
    DWORD cbUsageList;
    
    PREFAST_DEBUGCHK(pHidMouse != NULL);
    DEBUGCHK(pHidMouse->dwSig == HID_MOUSE_SIG);
    DEBUGCHK(pHidMouse->hDevice != NULL);
    DEBUGCHK(pHidMouse->pHidFuncs != NULL);
    DEBUGCHK(pHidMouse->phidpPreparsedData != NULL);
    DEBUGCHK(pHidMouse->hidpCaps.UsagePage == HID_USAGE_PAGE_GENERIC);
    DEBUGCHK(pHidMouse->hidpCaps.Usage == HID_USAGE_GENERIC_MOUSE);
    if (pHidMouse->fhThreadInited == TRUE) {
        DEBUGCHK(pHidMouse->hThread != NULL);
    }

    cbUsageList = pHidMouse->dwMaxUsages * sizeof(*pHidMouse->puPrevUsages);
    DEBUGCHK(pHidMouse->puPrevUsages != NULL);
    DEBUGCHK(LocalSize(pHidMouse->puPrevUsages) >= cbUsageList * 4);
    DEBUGCHK((DWORD) pHidMouse->puCurrUsages == ((DWORD) pHidMouse->puPrevUsages) + cbUsageList);
    DEBUGCHK((DWORD) pHidMouse->puBreakUsages == ((DWORD) pHidMouse->puCurrUsages) + cbUsageList);
    DEBUGCHK((DWORD) pHidMouse->puMakeUsages == ((DWORD) pHidMouse->puBreakUsages) + cbUsageList);

    DEBUGCHK( (pHidMouse->usageWheel == HID_USAGE_GENERIC_WHEEL) ||
              (pHidMouse->usageWheel == HID_USAGE_GENERIC_Z)     ||
              (pHidMouse->usageWheel == HAS_NO_WHEEL) );
}

#endif // DEBUG


