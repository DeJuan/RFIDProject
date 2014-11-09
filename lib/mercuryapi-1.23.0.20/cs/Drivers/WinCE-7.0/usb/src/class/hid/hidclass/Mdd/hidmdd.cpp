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

    hidmdd.cpp

Abstract:  
    MDD for Human Interface Device (HID) Class Driver.

Functions:

Notes: 

--*/


#include "hidmdd.h"
#include <pm.h>
#include <devload.h>
#include <intsafe.h>

// Globals that keep track of the reference count for the unnamed stream
// driver.
static CRITICAL_SECTION g_csHidRefCount;
static DWORD            g_dwHidRefCount;
static HANDLE           g_hMainHidDriver;


#ifdef DEBUG

// Print the report.
static
void
OutputReport(
    BYTE const*const pData,
    DWORD cbData,
    DWORD dwReportID,
    USAGE UsagePage,
    USAGE Usage
    )
{
    TCHAR szBuf[512];
    TCHAR *szCurr = szBuf;
    DWORD cbCurr = _countof(szBuf);
    
    if (_countof(szBuf) > cbData * 3) 
    {
        for (DWORD dwByte = 0; dwByte < cbData; ++dwByte) {
            VERIFY(SUCCEEDED(StringCchPrintf(szCurr, cbCurr, _T("%02X "), pData[dwByte])));
            szCurr += 3;
            cbCurr -= 3;
        }
    }
    else {
        DEBUGCHK(FALSE); // Why does the buffer need to be bigger? 512 is plenty!
        szBuf[0] = 0;
    }
    
    DEBUGMSG(ZONE_HID_DATA, (_T("HID Report ID %u (TLC %X-%X): %s\r\n"), dwReportID, 
        UsagePage, Usage, szBuf));
}


// Prints the available device strings.
static
void
DumpDeviceStrings(
    PCHID_CONTEXT pHidContext
    )
{
    SETFNAME(_T("DumpDeviceStrings"));

    struct DESC_TYPE_PAIR {
        LPCTSTR pszDesc;
        HID_STRING_TYPE stringType;
    };

    static const DESC_TYPE_PAIR rgList[] = {
        { _T("Manufacturer"), HID_STRING_ID_IMANUFACTURER },
        { _T("Product"), HID_STRING_ID_IPRODUCT},
        { _T("Serial Number"), HID_STRING_ID_ISERIALNUMBER},
    };            
    
    TCHAR szBuf[256];
    DWORD cchBuf;
    DWORD dwIdx;
    DWORD dwErr;

    for (dwIdx = 0; dwIdx < _countof(rgList); ++dwIdx) {
        dwErr = HidPdd_GetString(pHidContext->hPddDevice, 
            rgList[dwIdx].stringType, 0, szBuf, _countof(szBuf), &cchBuf);
        
        if (dwErr == ERROR_SUCCESS) {
            DEBUGMSG(ZONE_INIT, (_T("%s: %s: %s\r\n"), pszFname, 
                rgList[dwIdx].pszDesc, szBuf));
        }       
    }
}

#else

#define OutputReport(ptr, dw1, dw2, usage1, usage2)
#define DumpDeviceStrings(ptr)

#endif // DEBUG



// Generic routine to return the subkey of the form 
// pdwVals[0]_pdwVals[1]_...pdwVals[cdwVals-1].
static
HKEY 
OpenSpecificClientRegKey(
    HKEY hKeyRoot,
    const DWORD *pdwVals,
    DWORD cdwVals
    )
{
    HKEY hKeyRet = NULL;
    TCHAR szBuf[40];
    TCHAR *pszCur = szBuf;
    const TCHAR *pszKey;
    DWORD dwIdx;
    int ichWritten;
    int ichLeft = _countof(szBuf)-1;
    

    DEBUGCHK(cdwVals * 11 + 1 < _countof(szBuf));

    if (cdwVals == 0) {
        // Try default
        pszKey = DEFAULT_SZ;
    }
    else 
    {
        // Build key string
        for (dwIdx = 0; dwIdx < cdwVals; ++dwIdx) {
            ichWritten = _stprintf_s(pszCur, ichLeft, _T("%u_"), pdwVals[dwIdx]);
            pszCur += ichWritten;
            ichLeft -= ichWritten;
        }

        *(pszCur - 1) = 0; // Remove the last _
        pszKey = szBuf;
    }

    RegOpenKey(hKeyRoot, pszKey, &hKeyRet);

    return hKeyRet;    
}


// Contains most of the logic to find and open the proper HID client key.
// The algorithm searches for the most specific vendor key of the form
// idVendor_idProduct_bcdDevice. Then its subkeys are searched for the most
// specific interface key of the form bInterface_bCollection. Then its
// subkeys are searched for the most specific top level collection key 
// of the form UsagePage_Usage.
//
// The order of vendor precedence is described below.
//
// idVendor_idProduct_bcdDevice
// idVendor_idProduct
// idVendor
// Default
//
// The order of interface precedence is described below.
//
// bInterface_bCollection
// bInterface
// Default
//
// The order of TLC precedence is 
//
// UsagePage_Usage
// UsagePage
// Default
//
// So the order of checking would be
// 1. idVendor_idProduct_bcdDevice\bInterface_bCollection\UsagePage_Usage
// 2. idVendor_idProduct_bcdDevice\bInterface_bCollection\UsagePage
// 3. idVendor_idProduct_bcdDevice\bInterface_bCollection\Default
// 4. idVendor_idProduct_bcdDevice\bInterface\UsagePage_Usage
// ...
// 7. idVendor_idProduct_bcdDevice\Default\UsagePage_Usage
// ...
// 10. idVendor_idProduct\bInterface_bCollection\UsagePage_Usage
// ...
// 42. Default\bInterface_bCollection\Default
// 43. Default\bInterface\UsagePage_Usage
// 44. Default\bInterface\UsagePage
// 45. Default\bInterface\Default
// 46. Default\Default\UsagePage_Usage
// 47. Default\Default\UsagePage
// 48. Default\Default\Default
//
static
HKEY
OpenClientRegKey(
    HKEY hKeyRoot,
    const DWORD *pdwVendorVals, 
    DWORD cdwVendorVals,
    const DWORD *pdwInterfaceVals,
    DWORD cdwInterfaceVals,
    const DWORD *pdwTLCVals,
    DWORD cdwTLCVals,
    PHID_DRIVER_SETTINGS pDriverSettingsUsed
    )
{
    HKEY hKeyRet = NULL;
    HKEY hKeyVendor;
    HKEY hKeyInterface;
    INT cCurrVendor = -1;
    INT cCurrInterface = -1;
    INT cCurrTLC = -1;
    INT iIdx;

    DEBUGCHK(pdwVendorVals != NULL);
    DEBUGCHK(pdwInterfaceVals != NULL);
    DEBUGCHK(pdwTLCVals != NULL);

    for (cCurrVendor = (INT) cdwVendorVals; cCurrVendor >= 0; --cCurrVendor)
    {
        hKeyVendor = OpenSpecificClientRegKey(hKeyRoot, pdwVendorVals, cCurrVendor);
        if (hKeyVendor != NULL)
        {
            for (cCurrInterface = (INT) cdwInterfaceVals; 
                 cCurrInterface >= 0; 
                 --cCurrInterface)
            {
                hKeyInterface = OpenSpecificClientRegKey(hKeyVendor, pdwInterfaceVals, cCurrInterface);
                if (hKeyInterface != NULL)
                {
                    for (cCurrTLC = (INT) cdwTLCVals; cCurrTLC >= 0; --cCurrTLC)
                    {
                        hKeyRet = OpenSpecificClientRegKey(hKeyInterface, pdwTLCVals, cCurrTLC);
                        if (hKeyRet != NULL) {
                            // We found our key
                            break;
                        }
                    }
                    
                    RegCloseKey(hKeyInterface);

                    if (hKeyRet != NULL) {
                        // We found our key
                        break;
                    }
                } 
            }

            RegCloseKey(hKeyVendor);

            if (hKeyRet != NULL) {
                // We found our key.
                break;
            }
        }
    }

    if (hKeyRet != NULL) {
        // We found a match. Fill in the driver settings used.
        PDWORD rgpdwDriverSettings[] = {
            &pDriverSettingsUsed->dwVendorId,
            &pDriverSettingsUsed->dwProductId,
            &pDriverSettingsUsed->dwReleaseNumber,
            &pDriverSettingsUsed->dwInterfaceNumber,
            &pDriverSettingsUsed->dwCollection,
            &pDriverSettingsUsed->dwUsagePage,
            &pDriverSettingsUsed->dwUsage,
        };

        // Fill in the matching vendor info
        for (iIdx = 0; iIdx < cCurrVendor; ++iIdx) {
            *rgpdwDriverSettings[iIdx] = pdwVendorVals[iIdx];
        }

        // Fill in the matching interface info
        for (iIdx = 0; iIdx < cCurrInterface; ++iIdx) {
            *rgpdwDriverSettings[iIdx + cdwVendorVals] = pdwInterfaceVals[iIdx];
        }

        // Fill in the matching TLC info
        for (iIdx = 0; iIdx < cCurrTLC; ++iIdx) {
            *rgpdwDriverSettings[iIdx + cdwVendorVals + cdwInterfaceVals] 
                = pdwTLCVals[iIdx];
        }
    }

    return hKeyRet;
}


// Get the registry key for the HID client that will best service the device
// descripted in pDriverSettings.
// Returns the registry key and the actual driver settings that matched the key.
static
HKEY
FindClientRegKey(
    const HID_DRIVER_SETTINGS *pDriverSettings,
    PHID_DRIVER_SETTINGS pDriverSettingsUsed
    )
{
    SETFNAME(_T("FindClientRegKey"));

    const DWORD rgdwVendorVals[] = { 
        pDriverSettings->dwVendorId, 
        pDriverSettings->dwProductId, 
        pDriverSettings->dwReleaseNumber 
        };
    const DWORD rgdwInterfaceVals[] = { 
        pDriverSettings->dwInterfaceNumber,
        pDriverSettings->dwCollection
        };
    const DWORD rgdwTLCVals[] = { 
        pDriverSettings->dwUsagePage, 
        pDriverSettings->dwUsage 
        };

    HKEY hKeyRoot = NULL;
    HKEY hKeyRet = NULL;
    LONG iErr;

    DEBUGCHK(pDriverSettings != NULL);
    DEBUGCHK(pDriverSettingsUsed != NULL);
#pragma warning(suppress:4310) //cast truncates constant value
    memset(pDriverSettingsUsed, (BYTE) HID_NO_INFO, sizeof(HID_DRIVER_SETTINGS));

    iErr = RegOpenKey(HKEY_LOCAL_MACHINE, LOAD_CLIENTS_PATH_SZ, &hKeyRoot);
    if (iErr != ERROR_SUCCESS) {
        DEBUGMSG(ZONE_ERROR, (_T("%s: Failed opening HID client key [HKLM\\%s]\r\n"),
            pszFname, LOAD_CLIENTS_PATH_SZ));
        goto EXIT;
    }

    // Now find the most specific match.
    hKeyRet = OpenClientRegKey(hKeyRoot, 
        rgdwVendorVals, _countof(rgdwVendorVals),
        rgdwInterfaceVals, _countof(rgdwInterfaceVals),
        rgdwTLCVals, _countof(rgdwTLCVals), 
        pDriverSettingsUsed);
    
    if (hKeyRet == NULL) {
        DEBUGMSG(ZONE_COMMENT, (_T("%s: Could not find a suitable HID client driver\r\n"),
            pszFname));
        goto EXIT;
    }

EXIT:
    if (hKeyRoot != NULL) RegCloseKey(hKeyRoot);

    return hKeyRet;
}


// Check this collection's registry key to see if remote wakeup needs
// to be enabled for this device.
static
void
SetRemoteWakeup(
    PHID_CONTEXT pHidContext,
    HKEY hKeyClient
    )
{
    SETFNAME(_T("SetRemoteWakeup"));
    
    DWORD dwRemoteWakeup;
    DWORD cbdwRemoteWakeup;
    BOOL fEnable;
    DWORD dwType;
    INT iErr;
    DWORD dwErr;

    DEBUGCHK(pHidContext != NULL);
    PREFAST_DEBUGCHK(hKeyClient != NULL);
    
    // Check to see if we should enable remote wakeup for this device.
    // If any TLC client wishes to be a remote wakeup source, the device
    // will be set as a remote wakeup source.
    cbdwRemoteWakeup = sizeof(dwRemoteWakeup);
    iErr = RegQueryValueEx(hKeyClient, REMOTE_WAKEUP_VALUE_SZ, NULL, &dwType, 
        (PBYTE) &dwRemoteWakeup, &cbdwRemoteWakeup);
    if ( (iErr == ERROR_SUCCESS) && (dwType == REG_DWORD) && 
         (cbdwRemoteWakeup == sizeof(dwRemoteWakeup)) && (dwRemoteWakeup != 0) ) {
        // Enable remote wakeup
        fEnable = TRUE;
        dwErr = HidPdd_IssueCommand(pHidContext->hPddDevice, 
            HID_PDD_REMOTE_WAKEUP, fEnable);
        
        if (dwErr != ERROR_SUCCESS) {
            DEBUGMSG(ZONE_WARNING, 
                (_T("%s: Unable to set device as remote wakeup source\r\n"),
                pszFname));
        }
    }
}


// Check this collection's registry key to see if the idle rate needs 
// to be adjusted.
static
void
SetIdleRate(
    PHID_CONTEXT pHidContext,
    HKEY hKeyClient,
    DWORD dwCollection
    )
{
    SETFNAME(_T("SetIdleRate"));
    
    DWORD dwDuration;
    DWORD cbdwDuration;
    DWORD dwType;
    INT iErr;
    DWORD dwIdx;
    DWORD dwErr;

    DEBUGCHK(pHidContext != NULL);
    PREFAST_DEBUGCHK(hKeyClient != NULL);
    
    // Check to see if we should set an idle rate for the device.
    cbdwDuration = sizeof(dwDuration);
    iErr = RegQueryValueEx(hKeyClient, IDLE_RATE_VALUE_SZ, NULL, &dwType, 
        (PBYTE) &dwDuration, &cbdwDuration);
    if ( (iErr == ERROR_SUCCESS) && (dwType == REG_DWORD) && 
         (cbdwDuration == sizeof(dwDuration)) ) {         
        // Set idle rate
        DEBUGCHK(pHidContext->hidpDeviceDesc.ReportIDs != NULL);
        
        for (dwIdx = 0; dwIdx < pHidContext->hidpDeviceDesc.ReportIDsLength; ++dwIdx) {
            PHIDP_REPORT_IDS phidpReport = &pHidContext->hidpDeviceDesc.ReportIDs[dwIdx];
            
            if (phidpReport->CollectionNumber == dwCollection) {
                // This report falls under this collection.
                DEBUGMSG(ZONE_COMMENT, (_T("%s: Setting report %u to idle rate of %u\r\n"),
                    pszFname, phidpReport->ReportID, dwDuration));
                dwErr = HidPdd_SetIdle(pHidContext->hPddDevice,
                    dwDuration, phidpReport->ReportID);
            
                if (dwErr != ERROR_SUCCESS) {
                    DEBUGMSG(ZONE_WARNING, 
                        (_T("%s: Unable to set idle rate for device\r\n"),
                        pszFname));
                }
            }
        }
    }
}


// Load a HID client for each top level collection, if possible.
// Returns FALSE if there was a major error. Returns TRUE even
// if certain TLC's do not have clients. This is not an error
// because internal processing will still be performed on unhandled
// reports (like power events).
static
BOOL
LoadHidClients(
    PHID_CONTEXT pHidContext,
    DWORD dwVendorId,
    DWORD dwProductId,
    DWORD dwReleaseNumber,
    DWORD dwInterface
    )
{
    SETFNAME(_T("LoadHidClients"));

    BOOL fRet = FALSE;
    DWORD cCollections;
    DWORD dwIdx;
    DWORD dwType;
    LONG iErr;
    HID_DRIVER_SETTINGS driverSettings;
    
    PREFAST_DEBUGCHK(pHidContext != NULL);
    ValidateHidContext(pHidContext);

    // Initially set all reports on the interface to infinite idle
    DEBUGMSG(ZONE_COMMENT, (_T("%s: Setting all reports to infinite idle\r\n"),
        pszFname));
    HidPdd_SetIdle(pHidContext->hPddDevice, 0, 0);

    cCollections = pHidContext->hidpDeviceDesc.CollectionDescLength;
    
    driverSettings.dwVendorId = dwVendorId;
    driverSettings.dwProductId = dwProductId;
    driverSettings.dwReleaseNumber = dwReleaseNumber;
    driverSettings.dwInterfaceNumber = dwInterface;

    for (dwIdx = 0; dwIdx < cCollections; ++dwIdx)
    {
        PHIDP_COLLECTION_DESC pCollection = &pHidContext->hidpDeviceDesc.CollectionDesc[dwIdx];
        PHID_CLIENT_HANDLE pClientHandle = &pHidContext->pClientHandles[dwIdx];
        TCHAR szBuf[MAX_PATH];
        DWORD cchBuf = _countof(szBuf);
        HKEY hKey;
        HKEY hSubKey = NULL;
        HINSTANCE hInst = NULL;
        LPHID_CLIENT_ATTACH pfnAttach;
        BOOL fSuccess = FALSE;

        DEBUGCHK(pClientHandle->hInst == NULL);
        DEBUGCHK(pClientHandle->pQueue != NULL);

        driverSettings.dwCollection = pCollection->CollectionNumber;
        driverSettings.dwUsagePage = pCollection->UsagePage;
        driverSettings.dwUsage = pCollection->Usage;

        hKey = FindClientRegKey(&driverSettings, &pClientHandle->driverSettings);

        if (hKey == NULL) {
            // No match for top level collection. Just continue.
            goto CONTINUE;
        }
        
        iErr = RegEnumKeyEx(hKey, 0, szBuf, &cchBuf, NULL, NULL, NULL, NULL);
        if (iErr != ERROR_SUCCESS) {
            DEBUGMSG(ZONE_ERROR, (_T("%s: Could not enumerate subkey of LoadClient key. Error %i\r\n"),
                pszFname, iErr));
            goto CONTINUE;
        }

        iErr = RegOpenKey(hKey, szBuf, &hSubKey);
        if (iErr != ERROR_SUCCESS) {
            DEBUGMSG(ZONE_ERROR, (_T("%s: Could not open subkey \"%s\"of LoadClient key. Error %i\r\n"),
                pszFname, szBuf, iErr));
            goto CONTINUE;
        }

        // ** At this point we have our client key **

        // Set various device features.
        SetRemoteWakeup(pHidContext, hSubKey);
        SetIdleRate(pHidContext, hSubKey, pCollection->CollectionNumber);

        cchBuf = _countof(szBuf);
        iErr = RegQueryValueEx(hSubKey, DLL_VALUE_SZ, NULL, &dwType, 
            (PBYTE) szBuf, &cchBuf);
        if (iErr != ERROR_SUCCESS || dwType != REG_SZ) {
            DEBUGMSG(ZONE_ERROR, (_T("s: Could not get %s value in LoadClient key.\r\n"),
                pszFname, DLL_VALUE_SZ));
            goto CONTINUE;
        }
        szBuf[_countof(szBuf) - 1] = 0; // Force NULL-termination.

        hInst = LoadDriver(szBuf);
        if (hInst == NULL) {
            DEBUGMSG(ZONE_ERROR, (_T("%s: Could not load client DLL %s\r\n"), 
                pszFname, szBuf));
            goto CONTINUE;
        }
        
        pfnAttach = (LPHID_CLIENT_ATTACH) GetProcAddress(hInst, SZ_HID_CLIENT_ATTACH);
        if (pfnAttach == NULL) {
            DEBUGMSG(ZONE_ERROR, (_T("%s: Could not get the address of %s from %s\r\n"),
                pszFname, SZ_HID_CLIENT_ATTACH, szBuf));
            goto CONTINUE;
        }

        __try {
            fSuccess = (*pfnAttach)(
                (HID_HANDLE) pClientHandle,
                &g_HidFuncs, 
                &pClientHandle->driverSettings,
                pCollection->PreparsedData,
                &pClientHandle->lpvNotifyParameter,
                0);
        }
        __except(EXCEPTION_EXECUTE_HANDLER) {
            DEBUGMSG(ZONE_ERROR, (_T("%s: Exception in attach procedure %s in %s\r\n"),
                pszFname, SZ_HID_CLIENT_ATTACH, szBuf));
        }

        if (fSuccess != TRUE) {
            DEBUGMSG(ZONE_INIT, (_T("%s: Failure in attach procedure %s in %s\r\n"),
                pszFname, SZ_HID_CLIENT_ATTACH, szBuf));
            goto CONTINUE;
        }

        // Turn on this client's queue.
        pClientHandle->pQueue->AcceptNewReports(TRUE);

        // Finally this client has been initialized. Save our hInst.
        pClientHandle->hInst = hInst;
        
CONTINUE:
        if (hKey != NULL) RegCloseKey(hKey);
        if (hSubKey != NULL) RegCloseKey(hSubKey);
        if (pClientHandle->hInst == NULL) {
            // We did not successfully initialize the client.
            if (hInst != NULL) FreeLibrary(hInst);
            
            DEBUGMSG(ZONE_COMMENT, (_T("%s: No client found for HID top level collection 0x%X-0x%X on interface 0x%X\r\n"),
                pszFname, pCollection->UsagePage, pCollection->Usage, 
                dwInterface));
        }
    }

    fRet = TRUE;

    return fRet;
}


// We have data structures for each top level collection. This initializes them.
static
BOOL
InitializeTLCStructures(
    PHID_CONTEXT pHidContext
    )
{
    SETFNAME(_T("InitializeTLCStructures"));

    BOOL fRet = FALSE;
    DWORD cCollections;
    DWORD dwIdx;
    DWORD dwAllocSize = 0;
    
    PREFAST_DEBUGCHK(pHidContext != NULL);
    
    // Allocate our HID queues. One for each top level collection.
    cCollections = pHidContext->hidpDeviceDesc.CollectionDescLength;
    pHidContext->pQueues = new HidTLCQueue[cCollections];

    if (pHidContext->pQueues == NULL) {
        DEBUGMSG(ZONE_ERROR, (_T("%s: Memory allocation error\r\n"), pszFname));
        goto EXIT;
    }

    if(S_OK != DWordMult(sizeof(HID_CLIENT_HANDLE), cCollections, &dwAllocSize))
    {
        DEBUGMSG(ZONE_ERROR, (_T("%s: Too many collections.") 
                                _T("Integer overflow occured\r\n."), pszFname));
        dwAllocSize = 0;
        goto EXIT;
    }

    // Allocate our HID client handles. One for each TLC.
    pHidContext->pClientHandles = (PHID_CLIENT_HANDLE) HidAlloc(dwAllocSize);
    if (pHidContext->pClientHandles == NULL) {
        DEBUGMSG(ZONE_ERROR, (_T("%s: LocalAlloc error:%d\r\n"), pszFname, GetLastError()));
        goto EXIT;
    }

    // Initialize our HID queues and client handles.
    for (dwIdx = 0; dwIdx < cCollections; ++dwIdx) 
    {        
        BOOL fSucceeded;
        PHIDP_COLLECTION_DESC phidpCollection = 
            &pHidContext->hidpDeviceDesc.CollectionDesc[dwIdx];
        PHID_CLIENT_HANDLE pClientHandle = &pHidContext->pClientHandles[dwIdx];
        
        fSucceeded = pHidContext->pQueues[dwIdx].Initialize(
            HID_TLC_QUEUE_DEFAULT_CAPACITY, 
            phidpCollection->InputLength);

        if (fSucceeded == FALSE) {
            DEBUGMSG(ZONE_ERROR, (_T("%s: Failed initializing queue\r\n"), pszFname));
            goto EXIT;
        }

        pClientHandle->Sig = HID_CLIENT_SIG;
        pClientHandle->hInst = NULL;
        pClientHandle->lpvNotifyParameter = NULL;
        pClientHandle->pHidContext = pHidContext;
        pClientHandle->pQueue = &pHidContext->pQueues[dwIdx];
        pClientHandle->phidpPreparsedData = phidpCollection->PreparsedData;
    }

    fRet = TRUE;

EXIT:
    return fRet;
}


// Frees the Hid context for a specific PDD-defined device. Calls any
// client's notification routine and releases the library.
//
// Note that synchronization is unnecessary because the only threads
// that will call into the MDD at this point are the client's. We do
// not free the data structures until the client is completely closed.
// If HID_IOControl ever does anything specific to a Hid context, then
// synchronization will need to be added.
static
void
FreeHidContext(
    PHID_CONTEXT pHidContext
    )
{
    SETFNAME(_T("FreeHidContext"));
    
    DWORD cCollections = 0;
    DWORD dwIdx;
    
    DEBUGCHK(pHidContext != NULL);

    if (VALID_HID_CONTEXT(pHidContext)) {
        cCollections = pHidContext->hidpDeviceDesc.CollectionDescLength;

        if (pHidContext->pQueues != NULL)
        {
            // Tell each queue that it is closing.
            for (dwIdx = 0; dwIdx < cCollections; ++dwIdx) 
            {
                HidTLCQueue *pQueue = &pHidContext->pQueues[dwIdx];
                if (pQueue->IsInitialized() == TRUE) {
                    pQueue->Close();
                }
            }
        }

        if (pHidContext->pClientHandles != NULL) {
            // Notify each client that its device has been removed.
            for (dwIdx = 0; dwIdx < cCollections; ++dwIdx) {
                PHID_CLIENT_HANDLE pHidClient = &pHidContext->pClientHandles[dwIdx];
                if (pHidClient->hInst != NULL) {
                    LPHID_CLIENT_NOTIFICATIONS lpNotifications;

                    lpNotifications = (LPHID_CLIENT_NOTIFICATIONS) 
                        GetProcAddress(pHidClient->hInst, SZ_HID_CLIENT_NOTIFICATIONS);

                    if (lpNotifications == NULL) {
                        DEBUGMSG(ZONE_ERROR, (_T("%s: Could not get client address of %s\r\n"),
                            pszFname, SZ_HID_CLIENT_NOTIFICATIONS));
                    }
                    else 
                    {
                        __try {
                            (*lpNotifications)(HID_CLOSE_DEVICE, 0, 
                                pHidClient->lpvNotifyParameter);
                        } 
                        __except(EXCEPTION_EXECUTE_HANDLER) {
                              DEBUGMSG(ZONE_ERROR,(_T("%s: Exception in notification proc\r\n"), 
                                pszFname));
                        }
                    }

                    FreeLibrary(pHidClient->hInst);
                }
            }

            HidFree(pHidContext->pClientHandles);
        }

        if (pHidContext->pQueues != NULL) delete [] pHidContext->pQueues;
        
        HidP_FreeCollectionDescription(&pHidContext->hidpDeviceDesc);

        HidFree(pHidContext);
    }    
    else {
        DEBUGMSG(ZONE_ERROR, (_T("%s: Invalid Parameter\r\n"), pszFname));
        DEBUGCHK(FALSE);
    }
}


// Determine the length of the longest input report.
static
BOOL
DetermineMaxInputReportLength(
    PHIDP_DEVICE_DESC phidpDeviceDesc,
    PDWORD pcbMaxReport
    )
{
    PREFAST_DEBUGCHK(phidpDeviceDesc);
    PREFAST_DEBUGCHK(pcbMaxReport);

    *pcbMaxReport = 0;
    
    for (DWORD dwReport = 0; dwReport < phidpDeviceDesc->ReportIDsLength; ++dwReport) {
        PHIDP_REPORT_IDS phidpReportIds = &phidpDeviceDesc->ReportIDs[dwReport];
        *pcbMaxReport = max(*pcbMaxReport, phidpReportIds->InputLength);
    }

    return TRUE;
}


// Called with that report descriptor and ID structure of a newly-connected
// device.
BOOL
WINAPI
HidMdd_Attach(
    HID_PDD_HANDLE           hPddDevice,
    PHIDP_REPORT_DESCRIPTOR  pbReportDescriptor,
    DWORD                    cbReportDescriptor,
    DWORD                    dwVendorId,
    DWORD                    dwProductId,
    DWORD                    dwReleaseNumber,
    DWORD                    dwInterface,
    PVOID                   *ppvNotifyParameter,
    PDWORD                   pcbMaxInputReport
    )
{
    SETFNAME(_T("HidMdd_Attach"));

    BOOL fRet = FALSE;
    PHID_CONTEXT pHidContext = NULL;
    NTSTATUS status;
    
    DEBUGMSG(ZONE_INIT, (_T("+%s"), pszFname));
    
    DEBUGCHK(pbReportDescriptor != NULL);
    PREFAST_DEBUGCHK(ppvNotifyParameter != NULL);
    PREFAST_DEBUGCHK(pcbMaxInputReport);

    DumpHIDReportDescriptor(pbReportDescriptor, cbReportDescriptor);
    
    pHidContext = (PHID_CONTEXT) HidAlloc(sizeof(HID_CONTEXT));
    if (pHidContext == NULL) {
        DEBUGMSG(ZONE_ERROR, (_T("%s: LocalAlloc error:%d\r\n"), pszFname, 
            GetLastError()));
        goto EXIT;
    }
    ZeroMemory(pHidContext, sizeof(HID_CONTEXT));
    
    pHidContext->Sig = HID_SIG;
    pHidContext->hPddDevice = hPddDevice;

    // Now that we've allocated space for it, let's parse the report descriptor.
    status = HidP_GetCollectionDescription(
       pbReportDescriptor,
       cbReportDescriptor,
       0,
       &pHidContext->hidpDeviceDesc
       );

    if (NT_SUCCESS(status) == FALSE) {
        DEBUGMSG(ZONE_ERROR, (_T("%s: Problem parsing report descriptor Error: 0x%x\r\n"),
            pszFname, status));
        goto EXIT;
    }
    
    DumpHIDDeviceDescription(&pHidContext->hidpDeviceDesc);

    // Set up our data structures for each top level collection.
    if (InitializeTLCStructures(pHidContext) == FALSE) {
        goto EXIT;
    }

    // Determine how long the longest input report may be.
    if ( DetermineMaxInputReportLength(&pHidContext->hidpDeviceDesc,
            pcbMaxInputReport) == FALSE ) {
        goto EXIT;
    }

    if (LoadHidClients(pHidContext, dwVendorId, dwProductId, 
            dwReleaseNumber, dwInterface) == FALSE) {
        goto EXIT;
    }

    *ppvNotifyParameter = (PVOID) pHidContext;

    DumpDeviceStrings(pHidContext);
    
    fRet = TRUE;
    
EXIT:
    if (fRet == FALSE) {
        if (pHidContext != NULL) {
            FreeHidContext(pHidContext);
        }
        
        *ppvNotifyParameter = NULL;
    }

    DEBUGMSG(ZONE_INIT, (_T("-%s"), pszFname));

    return fRet;
}


// Notifies the MDD of device changes.
BOOL
WINAPI
HidMdd_Notifications(
    DWORD  dwMsg,
    WPARAM wParam,    
    PVOID  pvNotifyParameter
    )
{
    SETFNAME(_T("HidMdd_Notifications"));
    
    BOOL fRet = FALSE;
    PHID_CONTEXT pHidContext = (PHID_CONTEXT) pvNotifyParameter;

    UNREFERENCED_PARAMETER(wParam);

    ValidateHidContext(pHidContext);
    
    switch(dwMsg) {
        case HID_MDD_CLOSE_DEVICE:
            FreeHidContext(pHidContext);
            fRet = TRUE;
            break;

        default:
            DEBUGMSG(ZONE_ERROR, (_T("%s: Unhandled message %u\r\n"), pszFname));
            break;
    };

    return fRet;
}



// Return a report descriptor given the ID.
static
PHIDP_REPORT_IDS
GetReportDesc(
    PHIDP_DEVICE_DESC phidpDeviceDesc,
    DWORD dwReportID
    )
{
    PHIDP_REPORT_IDS pReport = NULL;
    DWORD dwIdx;

    PREFAST_DEBUGCHK(phidpDeviceDesc != NULL);
    DEBUGCHK(phidpDeviceDesc->ReportIDs != NULL);
    
    for (dwIdx = 0; dwIdx < phidpDeviceDesc->ReportIDsLength; ++dwIdx)
    {
        PHIDP_REPORT_IDS pCurrReport = &phidpDeviceDesc->ReportIDs[dwIdx];
        PREFAST_DEBUGCHK(pCurrReport != NULL);
        
        if (pCurrReport->ReportID == dwReportID) {
            pReport = pCurrReport;
            break;
        }
    }

    DEBUGCHK(pReport != NULL);

    return pReport;        
}

    

// Return a collection decriptor given the collection number. Also returns the
// index of the collection in the device context's array.
static
PHIDP_COLLECTION_DESC
GetCollectionDesc(
    PHIDP_DEVICE_DESC phidpDeviceDesc,
    DWORD dwCollectionNumber,
    PDWORD pdwCollectionIndex
    )
{
    PHIDP_COLLECTION_DESC pColl = NULL;
    DWORD dwIdx;

    PREFAST_DEBUGCHK(phidpDeviceDesc != NULL);
    DEBUGCHK(phidpDeviceDesc->CollectionDesc != NULL);
    
    for (dwIdx = 0; dwIdx < phidpDeviceDesc->CollectionDescLength; ++dwIdx) 
    {
        PHIDP_COLLECTION_DESC pCurrColl = &phidpDeviceDesc->CollectionDesc[dwIdx];
        PREFAST_DEBUGCHK(pCurrColl != NULL);
        
        if (pCurrColl->CollectionNumber == dwCollectionNumber) {
            pColl = pCurrColl;
            break;
        }
    }

    DEBUGCHK(pColl != NULL);

    if (pdwCollectionIndex != NULL) {
        *pdwCollectionIndex = dwIdx;
    }

    return pColl;        
}    



// If this report includes power events, perform them.
static
void
PerformPowerEvents(
    PHIDP_COLLECTION_DESC phidpCollection,
    BYTE *pbHidPacket,
    DWORD cbHidPacket
    )
{
    SETFNAME(_T("PerformPowerEvents"));
    
    NTSTATUS status;
    ULONG uPowerEvents = 0;
    
    PREFAST_DEBUGCHK(phidpCollection != NULL);
    DEBUGCHK(pbHidPacket != NULL);

    status = HidP_SysPowerEvent((PCHAR) pbHidPacket, (USHORT) cbHidPacket, 
        phidpCollection->PreparsedData, &uPowerEvents);
    
    if (NT_SUCCESS(status)) 
    {
        DEBUGMSG(ZONE_HID_DATA, (_T("%s: Received power event 0x%08x\r\n"), 
            pszFname, uPowerEvents));
        
        if ((uPowerEvents & (SYS_BUTTON_SLEEP | SYS_BUTTON_POWER)) != 0) {
            SetSystemPowerState(NULL, POWER_STATE_SUSPEND, FALSE);
        }
    }    
}


// Called by the PDD when a new report arrives. The report is supplied
// in its raw format (ie. the report ID of 0 is not prepended).
DWORD
WINAPI
HidMdd_ProcessInterruptReport(
    PBYTE pData,
    DWORD cbData,
    PVOID pvNotifyParameter
    )
{
    SETFNAME(_T("HidMdd_ProcessInterruptReport"));

    PHID_CONTEXT pHidContext = (PHID_CONTEXT) pvNotifyParameter;
    PHIDP_REPORT_IDS phidpReport;
    PHIDP_COLLECTION_DESC phidpCollection;
    DWORD dwReportID;
    DWORD dwCollectionIndex;
    PHID_CLIENT_HANDLE pClientHandle;
    HidTLCQueue *pQueue;
    DWORD dwRet = ERROR_INVALID_PARAMETER;
   
    PREFAST_DEBUGCHK(pData != NULL);
    ValidateHidContext(pHidContext);

    if (pHidContext->hidpDeviceDesc.ReportIDs[0].ReportID == 0) {
        // The report ID is not returned in this report
        dwReportID = 0;
    }
    else {
        // We got a report ID.
        dwReportID = *pData;
    }

    // Get the report and collection data structures
    phidpReport = GetReportDesc(&pHidContext->hidpDeviceDesc, dwReportID);
    if (phidpReport != NULL) {
        phidpCollection = GetCollectionDesc(&pHidContext->hidpDeviceDesc, 
            phidpReport->CollectionNumber, &dwCollectionIndex);

        if (phidpCollection != NULL) {
            DEBUGCHK(dwCollectionIndex < pHidContext->hidpDeviceDesc.CollectionDescLength);

            OutputReport(pData, cbData, dwReportID, phidpCollection->UsagePage,
                phidpCollection->Usage);
            
            // Perform any power events listed in this report
            PerformPowerEvents(phidpCollection, pData, cbData);

            // Send this HID packet to the proper client
            pClientHandle = &pHidContext->pClientHandles[dwCollectionIndex];

            // Only queue this report if we have a client that will receive it.
            if (pClientHandle->hInst != NULL) 
            {
                pQueue = pClientHandle->pQueue;
                pQueue->Lock();
                if (pQueue->IsAccepting() == TRUE) {
                    if (pQueue->IsFull() == FALSE) {
                        BOOL fRet;
                        fRet = pQueue->Enqueue((PCHAR) pData, cbData, dwReportID);
                        DEBUGCHK(fRet);
                    }
                    else {
                        DEBUGMSG(ZONE_ERROR, 
                            (_T("%s: Error: Queue is full. Dropping packet.\r\n"), pszFname));
                    }
                }
                else {
                    DEBUGMSG(ZONE_HID_DATA, 
                        (_T("%s: Queue not accepting input. Dropping packet.\r\n"), pszFname));
                }
                pQueue->Unlock();
            }

            dwRet = ERROR_SUCCESS;
        }
    }

    DEBUGMSG(ZONE_ERROR && dwRet != ERROR_SUCCESS, 
        (_T("%s: Error: Received an invalid report.\r\n"), pszFname));

    return dwRet;
}




#define HID_DEVICE_CONTEXT_HANDLE 0xCAFE

#ifdef DEBUG
static DWORD g_dwInitCount = 0;
#endif


// Called by the PDD during XXX_Init. The PDD is responsible for 
// making sure the return value is stored and passed into
// HidMdd_Deinit.
DWORD
WINAPI
HidMdd_Init (
    LPCWSTR lpszDevKey
    )
{
    DEBUGCHK(lpszDevKey);
    UNREFERENCED_PARAMETER(lpszDevKey);
    
#ifdef DEBUG
    // Be sure that only one instance of this DLL is initialized.
    DEBUGCHK(g_dwInitCount == 0);
    ++g_dwInitCount;
#endif

    return HID_DEVICE_CONTEXT_HANDLE;
}


// Called by the PDD during XXX_Deinit. The PDD is responsible for 
// making sure dwContext is the same value returned from
// HidMdd_Init.
BOOL
WINAPI
HidMdd_Deinit(
    DWORD dwContext
    )
{
#ifdef DEBUG
    // Be sure that only one instance of this DLL is initialized.
    DEBUGCHK(g_dwInitCount == 1);
    --g_dwInitCount;
#endif

    DEBUGCHK(dwContext == HID_DEVICE_CONTEXT_HANDLE);
    UNREFERENCED_PARAMETER(dwContext);

    return TRUE;
}


// Called by the PDD during XXX_Open. The PDD is responsible for 
// making sure the return value is stored and passed into
// HidMdd_Close.
DWORD
WINAPI
HidMdd_Open(
    DWORD dwContext,
    DWORD /*dwAccMode*/,
    DWORD /*dwShrMode*/
    )
{
    DEBUGCHK(dwContext == HID_DEVICE_CONTEXT_HANDLE);
    UNREFERENCED_PARAMETER(dwContext);
    return HID_DEVICE_CONTEXT_HANDLE;
}


// Called by the PDD during XXX_Close. The PDD is responsible for 
// making sure dwContext is the same value returned from
// HidMdd_Open.
BOOL
WINAPI
HidMdd_Close(
    DWORD dwContext
    )
{
    DEBUGCHK(dwContext == HID_DEVICE_CONTEXT_HANDLE);
    UNREFERENCED_PARAMETER(dwContext);
    return TRUE;
}


// Called by the PDD during XXX_IOControl. The PDD is responsible for 
// making sure dwContext is the same value returned from
// HidMdd_Open.
//
// IOCTLs with a Function value between 0x300 and 0x3FF are reserved 
// for the MDD.
BOOL
WINAPI
HidMdd_IOControl(
    DWORD          /*dwContext*/, 
    DWORD          /*dwIoControlCode*/, 
    LPVOID         /*lpInBuffer*/, 
    DWORD          /*nInBufferSize*/, 
    LPVOID         /*lpOutBuffer*/, 
    DWORD          /*nOutBufferSize*/, 
    LPDWORD        /*lpBytesReturned*/
    )
{
    return ERROR_NOT_SUPPORTED;
}


// Called by the PDD from its DLL entry function. This allows the MDD
// to initialize any MDD global variables.
BOOL 
WINAPI
HidMdd_DllEntry(
    DWORD dwReason
    )
{
    switch (dwReason) {
        case DLL_PROCESS_ATTACH:
            InitializeCriticalSection(&g_csHidRefCount);
            break;
            
        case DLL_PROCESS_DETACH:
            DeleteCriticalSection(&g_csHidRefCount);
            break;
    }

    return TRUE;
}


// The Init entry point for the public interface to the driver.
// Its only purpose is to keep a reference count so that the underlying
// named stream interface is only activated one time.
// dwClientInfo should be the registry path to the stream driver to load.
extern "C"
DWORD Init(
    LPCWSTR /*lpszDevKey*/,
    DWORD   dwClientInfo
    )
{
    TCHAR szHidDriverKey[DEVKEY_LEN];
    DWORD dwRet = 0;
    DWORD dwErr = ERROR_SUCCESS;
    
    EnterCriticalSection(&g_csHidRefCount);

    LPCTSTR pszHidDriverKey = (LPCTSTR) dwClientInfo;

    if (pszHidDriverKey == NULL) {
        dwErr = ERROR_INVALID_PARAMETER;
        goto EXIT;
    }

    BOOL fException = FALSE;
    __try {
        VERIFY(SUCCEEDED(StringCchCopy(szHidDriverKey, _countof(szHidDriverKey) - 1, pszHidDriverKey)));
    }
    __except(EXCEPTION_EXECUTE_HANDLER) {
        fException = TRUE;
    }

    if (fException) {
        dwErr = ERROR_INVALID_PARAMETER;
        goto EXIT;
    }

    if (g_dwHidRefCount == 0) {
        DEBUGCHK(g_hMainHidDriver == NULL);
        g_hMainHidDriver = ActivateDeviceEx(szHidDriverKey, NULL, 0, NULL);
        if (g_hMainHidDriver == NULL) {
            dwErr = ERROR_DEVICE_NOT_AVAILABLE;
        }
    }

    dwRet = (DWORD) g_hMainHidDriver;

EXIT:
    if (dwErr != ERROR_SUCCESS) {
        SetLastError(dwErr);
        DEBUGCHK(dwRet == 0);
    }
    else {
        DEBUGCHK(dwRet != 0);
        ++g_dwHidRefCount;
    }
    
    LeaveCriticalSection(&g_csHidRefCount);

    return dwRet;
}


// The Init entry point for the public interface to the driver.
// Its only purpose is to keep a reference count so that the underlying
// named stream interface is only deactivated one time.
extern "C"
BOOL Deinit(DWORD hDeviceContext)
{
    EnterCriticalSection(&g_csHidRefCount);

    DEBUGCHK(g_dwHidRefCount > 0);
    DEBUGCHK(hDeviceContext == (DWORD) g_hMainHidDriver);
    UNREFERENCED_PARAMETER(hDeviceContext);
        
    --g_dwHidRefCount;
    
    if (g_dwHidRefCount == 0) {
        DEBUGCHK(g_hMainHidDriver);
        DeactivateDevice(g_hMainHidDriver);
        g_hMainHidDriver = NULL;
    }
    
    LeaveCriticalSection(&g_csHidRefCount);
    
    return TRUE;
}

