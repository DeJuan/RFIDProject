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

#include "conshid.h"
#include <windev.h>


#ifdef DEBUG

// Debug Zones
#define DBG_ERROR              0x0001
#define DBG_WARNING            0x0002
#define DBG_INIT               0x0004
#define DBG_FUNCTION           0x0008

#define DBG_USAGES             0x0010

DBGPARAM dpCurSettings = {
        TEXT("ConsHid"), {
        TEXT("Errors"), TEXT("Warnings"), TEXT("Init"), TEXT("Function"),
        TEXT("Usages"), TEXT(""), TEXT(""), TEXT(""),
        TEXT(""), TEXT(""), TEXT(""), TEXT(""),
        TEXT(""), TEXT(""), TEXT(""), TEXT("") },
        DBG_ERROR | DBG_WARNING };
        

static
void
ValidateHidConsumer(
    PHID_CONSUMER pHidConsumer
    );

#else

#define ValidateHidConsumer(ptr)

#endif // DEBUG


// MapVirtualKey(x, 3) converts scan codes to virtual keys
#define MAP_SC_TO_VK 3


// Describes an association from a Usage to an AT Scan Code
struct USAGE_TO_SC_ASSOCIATION {
    USAGE  usage;
    UINT16 uiSc;
};


// Usage to Virtual Key table
static const USAGE_TO_SC_ASSOCIATION g_rgUsageToScAssn[] = {
    { HID_CONSUMER_SCAN_NEXT_TRACK,             0xE04D },
    { HID_CONSUMER_SCAN_PREVIOUS_TRACK,         0xE015 },
    { HID_CONSUMER_STOP,                        0xE03B },
    { HID_CONSUMER_PLAY_PAUSE,                  0xE034 },

    { HID_CONSUMER_VOLUME_MUTE,                 0xE023 },
    { HID_CONSUMER_VOLUME_INCREMENT,            0xE032 },
    { HID_CONSUMER_VOLUME_DECREMENT,            0xE021 },

    { HID_CONSUMER_LAUNCH_CONFIGURATION,        0xE050 },
    { HID_CONSUMER_LAUNCH_EMAIL,                0xE048 },
    { HID_CONSUMER_LAUNCH_CALCULATOR,           0xE02B },
    { HID_CONSUMER_LAUNCH_BROWSER,              0xE040 },

    { HID_CONSUMER_APP_SEARCH,                  0xE010 },
    { HID_CONSUMER_APP_HOME,                    0xE03A },
    { HID_CONSUMER_APP_BACK,                    0xE038 },
    { HID_CONSUMER_APP_FORWARD,                 0xE030 },
    { HID_CONSUMER_APP_STOP,                    0xE028 },
    { HID_CONSUMER_APP_REFRESH,                 0xE020 },
    { HID_CONSUMER_APP_BOOKMARKS,               0xE018 },
};


static DWORD
SendKeyboardUsages(
    USAGE const* pUsages,
    DWORD dwMaxUsages,
    HIDP_KEYBOARD_DIRECTION keyEvent 
    );

static void 
KeyboardEvent(
    UINT vk,
    UINT sc,
    DWORD dwFlags
    );

static void
ProcessConsumerReport(
    PHID_CONSUMER pHidConsumer,
    PCHAR pbHidPacket,
    DWORD cbHidPacket
    );

static BOOL
AllocateUsageLists(
    PHID_CONSUMER pHidConsumer,
    size_t cbUsages
    );

VOID
FreeHidConsumer(
    PHID_CONSUMER pHidConsumer
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
    SETFNAME(_T("CONSHID DllEntry"));
    
    UNREFERENCED_PARAMETER(lpReserved);
    
    switch (dwReason) {
        case DLL_PROCESS_ATTACH:
            DEBUGREGISTER((HINSTANCE)hDllHandle);

            DEBUGMSG(ZONE_INIT, (_T("%s: Attach\r\n"), pszFname));
            DisableThreadLibraryCalls((HMODULE) hDllHandle);
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
ConsumerThreadProc(
    LPVOID lpParameter
    )
{
    SETFNAME(_T("ConsumerThreadProc"));

    PHID_CONSUMER pHidConsumer = (PHID_CONSUMER) lpParameter;
    
    PREFAST_DEBUGCHK(pHidConsumer != NULL);

    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);

    const DWORD cbMaxHidPacket = pHidConsumer->hidpCaps.InputReportByteLength;
    PCHAR pbHidPacket = (PCHAR) LocalAlloc(LMEM_FIXED, cbMaxHidPacket);
    if (pbHidPacket == NULL) {
        DEBUGMSG(ZONE_ERROR, (TEXT("%s: LocalAlloc error: %d\r\n"), pszFname, 
            GetLastError()));
        goto EXIT;
    }

    for (;;) {
        DWORD cbHidPacket;
        
        ValidateHidConsumer(pHidConsumer);
        
        // Get an interrupt report from the device.
        DWORD dwErr = pHidConsumer->pHidFuncs->lpGetInterruptReport(
            pHidConsumer->hDevice,
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
            ProcessConsumerReport(pHidConsumer, pbHidPacket, cbHidPacket);
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
    size_t cbUsages;
    PHID_CONSUMER pHidConsumer;

    DEBUGCHK(hDevice != NULL);
    DEBUGCHK(pHidFuncs != NULL);
    DEBUGCHK(phidpPreparsedData != NULL);
    DEBUGCHK(ppvNotifyParameter != NULL);

    // Allocate this device's data structure and fill it.
    pHidConsumer = (PHID_CONSUMER) LocalAlloc(LPTR, sizeof(HID_CONSUMER));
    if (pHidConsumer == NULL) {
        DEBUGMSG(ZONE_ERROR, (TEXT("%s: LocalAlloc error:%d\r\n"), pszFname, GetLastError()));
        goto EXIT;
    }

    pHidConsumer->dwSig = HID_CONSUMER_SIG;
    pHidConsumer->hDevice = hDevice;
    pHidConsumer->pHidFuncs = pHidFuncs;
    pHidConsumer->phidpPreparsedData = phidpPreparsedData;
    HidP_GetCaps(pHidConsumer->phidpPreparsedData, &pHidConsumer->hidpCaps);
    
    // Get the total number of usages that can be returned in an input packet.
    pHidConsumer->dwMaxUsages = HidP_MaxUsageListLength(HidP_Input, 
        HID_USAGE_PAGE_CONSUMER, phidpPreparsedData);
    if (pHidConsumer->dwMaxUsages == 0) {
        DEBUGMSG(ZONE_WARNING, (_T("%s: This collection does not have any ")
            _T("understood usages and will be ignored\r\n"), pszFname));
        goto EXIT;
    }
    
    cbUsages = pHidConsumer->dwMaxUsages * sizeof(*pHidConsumer->puPrevUsages);

    if (AllocateUsageLists(pHidConsumer, cbUsages) == FALSE) {
        goto EXIT;
    }
    
    // Create the thread that will receive reports from this device
    pHidConsumer->hThread = CreateThread(NULL, 0, ConsumerThreadProc, pHidConsumer, 0, NULL);
    if (pHidConsumer->hThread == NULL) {
        DEBUGMSG(ZONE_ERROR, (_T("%s: Failed creating keyboard thread\r\n"), 
            pszFname));
        goto EXIT;
    }
#ifdef DEBUG
    pHidConsumer->fhThreadInited = TRUE;
#endif


    *ppvNotifyParameter = pHidConsumer;
    ValidateHidConsumer(pHidConsumer);
    fRet = TRUE;

EXIT:
    if ((fRet == FALSE) && (pHidConsumer != NULL)) {
        FreeHidConsumer(pHidConsumer);
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
    PHID_CONSUMER pHidConsumer = (PHID_CONSUMER) pvNotifyParameter;

    UNREFERENCED_PARAMETER(wParam);

    if (VALID_HID_CONSUMER(pHidConsumer) == FALSE) {
        DEBUGMSG(ZONE_ERROR, (_T("%s: Received invalid structure pointer\r\n"), pszFname));
        goto EXIT;
    }

    ValidateHidConsumer(pHidConsumer);
    
    switch(dwMsg) {
        case HID_CLOSE_DEVICE:
            // Free all of our resources.
            WaitForSingleObject(pHidConsumer->hThread, INFINITE);
            CloseHandle(pHidConsumer->hThread);
            pHidConsumer->hThread = NULL;
            
            // Key up all keys that are still down.
            SendKeyboardUsages(pHidConsumer->puPrevUsages, pHidConsumer->dwMaxUsages,
                HidP_Keyboard_Break);
            
            FreeHidConsumer(pHidConsumer);
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


// Allocate the usage lists for this device. The lists are allocated in 
// a single block and then divided up.
//
// Note: There may not be any usages for this device so cbUsages may be 0.
static
BOOL
AllocateUsageLists(
    PHID_CONSUMER pHidConsumer,
    size_t cbUsages
    )
{
    SETFNAME(_T("AllocateUsageLists"));

    BOOL fRet = FALSE;
    size_t cbTotal;
    PBYTE pbStart;
    DWORD dwIdx;
    PUSAGE *rgppUsage[] = {
        &pHidConsumer->puPrevUsages,
        &pHidConsumer->puCurrUsages,
        &pHidConsumer->puBreakUsages,
        &pHidConsumer->puMakeUsages
    };
    
    DEBUGCHK(pHidConsumer != NULL);

    // Allocate a block of memory for all the usage lists
    if (cbUsages<MAXDWORD/_countof(rgppUsage)) {
        cbTotal = cbUsages * _countof(rgppUsage);
        pbStart = (PBYTE) LocalAlloc(LPTR, cbTotal);
    }
    else {
        pbStart = NULL;
        ASSERT(FALSE);
    }
    
    if (pbStart == NULL) {
       DEBUGMSG(ZONE_ERROR, (TEXT("%s: LocalAlloc error:%d\r\n"), pszFname, GetLastError()));
       goto EXIT;
    }

    // Divide the block.
    for (dwIdx = 0; dwIdx < _countof(rgppUsage); ++dwIdx) {
        *rgppUsage[dwIdx] = (PUSAGE) (pbStart + (cbUsages * dwIdx));
    }

    fRet = TRUE;

EXIT:
    return fRet;
}


// Free the memory used by pHidConsumer
void
FreeHidConsumer(
    PHID_CONSUMER pHidConsumer
    )
{
    PREFAST_DEBUGCHK(pHidConsumer != NULL);
    DEBUGCHK(pHidConsumer->hThread == NULL); // We do not close the thread handle

    if (pHidConsumer->puPrevUsages != NULL) LocalFree(pHidConsumer->puPrevUsages);
    LocalFree(pHidConsumer);
}


// Process a report from this device.
static
void
ProcessConsumerReport(
    PHID_CONSUMER pHidConsumer,
    PCHAR pbHidPacket,
    DWORD cbHidPacket
    )
{
    ULONG uCurrUsages;
    DWORD cbUsageList;
    NTSTATUS status;
    
    PREFAST_DEBUGCHK(pHidConsumer != NULL);
    DEBUGCHK(pbHidPacket != NULL);

    uCurrUsages = pHidConsumer->dwMaxUsages;
    cbUsageList = uCurrUsages * sizeof(*pHidConsumer->puPrevUsages);

    status = HidP_GetUsages(
        HidP_Input,
        HID_USAGE_PAGE_CONSUMER,
        0,
        pHidConsumer->puCurrUsages,
        &uCurrUsages, // IN OUT parameter
        pHidConsumer->phidpPreparsedData,
        pbHidPacket,
        cbHidPacket
        );
    DEBUGCHK(NT_SUCCESS(status));
    DEBUGCHK(uCurrUsages <= pHidConsumer->dwMaxUsages);

    // Determine what keys went down and up.
    status = HidP_UsageListDifference(
        pHidConsumer->puPrevUsages,
        pHidConsumer->puCurrUsages,
        pHidConsumer->puBreakUsages,
        pHidConsumer->puMakeUsages,
        pHidConsumer->dwMaxUsages
        );
    DEBUGCHK(NT_SUCCESS(status));

    // Send key ups
    SendKeyboardUsages(pHidConsumer->puBreakUsages, pHidConsumer->dwMaxUsages,
        HidP_Keyboard_Break);
    
    // Send key downs
    SendKeyboardUsages(pHidConsumer->puMakeUsages, pHidConsumer->dwMaxUsages,
        HidP_Keyboard_Make);

    // Save current usages
    memcpy(pHidConsumer->puPrevUsages, pHidConsumer->puCurrUsages, cbUsageList);
}


// Converts the usages to virtual keys and sends them off.
// Returns the number of usages converted.
static
DWORD
SendKeyboardUsages(
    USAGE const* pUsages,
    DWORD dwMaxUsages,
    HIDP_KEYBOARD_DIRECTION hidpKeyEvent
    )
{    
    SETFNAME(_T("SendKeyboardUsages"));

    DWORD dwUsage;
    DWORD dwAssn;
    
    PREFAST_DEBUGCHK(pUsages != NULL);

    for (dwUsage = 0; dwUsage < dwMaxUsages; ++dwUsage) {
        // Determine virtual key mapping
        const USAGE usage = pUsages[dwUsage];
        UINT uiSc = 0;
        DWORD dwFlags = 0;
        UINT uiVk;

        if (usage == 0) {
            // No more usages
            break;
        }

        // Find what virtual key matches this usage
        for (dwAssn = 0; dwAssn < _countof(g_rgUsageToScAssn); ++dwAssn) {
            if (g_rgUsageToScAssn[dwAssn].usage == usage) {
                uiSc = g_rgUsageToScAssn[dwAssn].uiSc;
                break;
            }
        }

        if (uiSc != 0) {
            DEBUGMSG(ZONE_USAGES, (_T("%s: Usage Page: 0x%04x Usage: 0x%04x -> SC: 0x%06x\r\n"),
                pszFname, HID_USAGE_PAGE_CONSUMER, usage, uiSc));
            
            // Set the key flags

            if (hidpKeyEvent == HidP_Keyboard_Break) {
                dwFlags |= KEYEVENTF_KEYUP;
            }

            uiVk = MapVirtualKey(uiSc, MAP_SC_TO_VK);
            
            // The key is extended if it matches 0x0000e0xx
            if ((uiSc & 0xFFFFFF00) == 0xE000) {
                dwFlags |= KEYEVENTF_EXTENDEDKEY;
            }
            
            KeyboardEvent(uiVk, uiSc, dwFlags);
        }
        else {
            DEBUGMSG(ZONE_WARNING, (_T("%s: No scan code for Usage Page: 0x%04x Usage: 0x%04x\r\n"),
                pszFname, HID_USAGE_PAGE_CONSUMER, usage));
        }
    }

    return dwUsage;
}


static
void
KeyboardEvent(
    UINT vk,
    UINT sc,
    DWORD dwFlags
    )
{
    SETFNAME(_T("KeyboardEvent"));

    UINT8 uiVk = (UINT8) vk;
    UINT8 uiSc = (UINT8) sc;

    DEBUGCHK((uiVk != 0) || (uiSc != 0));

    DEBUGMSG(ZONE_USAGES, (_T("%s: Keybd event: vk: 0x%02x sc: 0x%02x flags: 0x%08x\r\n"),
        pszFname, uiVk, uiSc, dwFlags));
    keybd_event(uiVk, uiSc, dwFlags, 0);
}


#ifdef DEBUG

// Validate a PHID_CONSUMER structure
static
void
ValidateHidConsumer(
    PHID_CONSUMER pHidConsumer
    )
{
    DWORD cbUsageList;
    
    PREFAST_DEBUGCHK(pHidConsumer != NULL);
    DEBUGCHK(pHidConsumer->dwSig == HID_CONSUMER_SIG);
    DEBUGCHK(pHidConsumer->hDevice != NULL);
    DEBUGCHK(pHidConsumer->pHidFuncs != NULL);
    DEBUGCHK(pHidConsumer->phidpPreparsedData != NULL);
    DEBUGCHK(pHidConsumer->hidpCaps.UsagePage == HID_USAGE_PAGE_CONSUMER);
    DEBUGCHK(pHidConsumer->hidpCaps.Usage == HID_CONSUMER_COLLECTION_CONSUMER_CONTROL);
    if (pHidConsumer->fhThreadInited == TRUE) {
        DEBUGCHK(pHidConsumer->hThread != NULL);
    }

    cbUsageList = pHidConsumer->dwMaxUsages * sizeof(*pHidConsumer->puPrevUsages);
    DEBUGCHK(pHidConsumer->puPrevUsages != NULL);
    DEBUGCHK(LocalSize(pHidConsumer->puPrevUsages) >= cbUsageList * 4);
    DEBUGCHK((DWORD) pHidConsumer->puCurrUsages == ((DWORD) pHidConsumer->puPrevUsages) + cbUsageList);
    DEBUGCHK((DWORD) pHidConsumer->puBreakUsages == ((DWORD) pHidConsumer->puCurrUsages) + cbUsageList);
    DEBUGCHK((DWORD) pHidConsumer->puMakeUsages == ((DWORD) pHidConsumer->puBreakUsages) + cbUsageList);
}

#endif // DEBUG


