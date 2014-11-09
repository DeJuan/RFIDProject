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

#define ttt_ KBD_
#include "kbdhid.h"
#include <devload.h>


static
PHID_KBD
GetContextFromReg(
    LPCTSTR  pszActivePath
    )
{
    SETFNAME(_T("GetContextFromReg"));

    PHID_KBD pHidKbd = NULL;
    HKEY hKey;
    LONG lStatus;

    if (pszActivePath != NULL) 
    {
        //
        // open the registry and read out our context pointer
        // since Dev Mgr doesn't pass it in.
        //
        lStatus = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                                pszActivePath,
                                0,
                                0,
                                &hKey);

        if (lStatus == ERROR_SUCCESS) 
        {
            DWORD dwVal;
            DWORD dwType = DEVLOAD_CLIENTINFO_VALTYPE;
            DWORD dwValLen = sizeof(dwVal);
            lStatus = RegQueryValueEx( hKey,
                                       DEVLOAD_CLIENTINFO_VALNAME,
                                       NULL,
                                       &dwType,
                                       (LPBYTE)(&dwVal),
                                       &dwValLen);

            if ((lStatus == ERROR_SUCCESS) && (dwType == REG_DWORD))
            {
                // check the signature
                pHidKbd = (PHID_KBD) dwVal;
                if (HID_KBD_SIG != pHidKbd->dwSig ) {
                    DEBUGMSG(ZONE_ERROR, (_T("%s: Invalid signature!!\r\n"), pszFname));
                    DEBUGCHK(FALSE);
                    pHidKbd = NULL;
                } else {
                    DEBUGMSG(ZONE_INIT, (_T("%s: ActivePath: %s\r\n"), pszFname, pszActivePath));
                }
            }

            RegCloseKey(hKey);

        }
        else {
            DEBUGMSG(ZONE_ERROR, (_T("%s: Open ActivePath failed\n"), pszFname));
        }
    }

   return pHidKbd;
}



extern "C" 
DWORD
KBD_Init (
    DWORD dwCtx
    )
{
    SETFNAME(_T("KBD_Init"));

    LPCTSTR pszActivePath = (LPCTSTR) dwCtx; // HKLM\Drivers\Active\xx
    PHID_KBD pHidKbd = NULL;
    DWORD dwRet = 0;

    DEBUGCHK(pszActivePath != NULL);

    DEBUGMSG(ZONE_FUNCTION, (_T("+%s"), pszFname));

    pHidKbd = GetContextFromReg(pszActivePath);

    if (VALID_HID_KBD(pHidKbd) == TRUE) {
        ValidateHidKbd(pHidKbd);
        dwRet = (DWORD) pHidKbd;
    }
    else {
        DEBUGCHK(FALSE); // Who called ActivateDevice with a bad value?
    }

    DEBUGMSG(ZONE_FUNCTION, (_T("-%s"), pszFname));
    
    return dwRet;
}


extern "C" 
BOOL
KBD_PreDeinit (DWORD /*dwCtx*/)
{
    SETFNAME(_T("KBD_PreDeinit"));

    // This function is necessary so that KBD_Deinit is called only 
    // after no threads are in KBD_IOControl.

    DEBUGMSG(ZONE_FUNCTION, (_T("+%s"), pszFname));
    DEBUGMSG(ZONE_FUNCTION, (_T("-%s"), pszFname));

    return TRUE;
}


extern "C" 
BOOL
KBD_Deinit (DWORD dwCtx)
{
    SETFNAME(_T("KBD_Deinit"));

    BOOL fRet = TRUE;

    PHID_KBD pHidKbd = (PHID_KBD) dwCtx;

    DEBUGMSG(ZONE_FUNCTION, (_T("+%s"), pszFname));

    if (VALID_HID_KBD(pHidKbd)) {
        SetEvent(pHidKbd->hevClosing);
    }
    else {
        fRet = FALSE;
    }

    DEBUGMSG(ZONE_FUNCTION, (_T("-%s"), pszFname));

    return fRet;
}


extern "C" 
DWORD
KBD_Open (DWORD dwCtx, DWORD /*dwAccMode*/, DWORD /*dwShrMode*/)
{
    SETFNAME(_T("KBD_Open"));

    PHID_KBD pHidKbd = (PHID_KBD) dwCtx;    
    DWORD dwRet = dwCtx;

    DEBUGMSG(ZONE_FUNCTION, (_T("+%s"), pszFname));

    if (VALID_HID_KBD(pHidKbd) == FALSE) {
        DEBUGMSG(ZONE_ERROR, (_T("%s: ERROR_FILE_NOT_FOUND\r\n"), pszFname));
        SetLastError(ERROR_FILE_NOT_FOUND);
        dwRet = 0;
    }
    
    DEBUGMSG(ZONE_FUNCTION, (_T("-%s"), pszFname));
    return dwRet;
}

extern "C" 
BOOL
KBD_Close (DWORD dwCtx)
{
    SETFNAME(_T("KBD_Close"));

    PHID_KBD pHidKbd = (PHID_KBD) dwCtx;    
    BOOL fRet = FALSE;

    DEBUGMSG(ZONE_FUNCTION, (_T("+%s"), pszFname));

    if (VALID_HID_KBD(pHidKbd) == FALSE) {
        DEBUGMSG(ZONE_ERROR, (_T("%s: ERROR_INVALID_HANDLE\r\n"), pszFname));
        SetLastError(ERROR_INVALID_HANDLE);
        goto EXIT;
    }

    fRet = TRUE;

EXIT:
    DEBUGMSG(ZONE_FUNCTION, (_T("-%s"), pszFname));
    return fRet;
}


extern "C" 
BOOL
KBD_IOControl(
    PHID_KBD pHidKbd,
    DWORD dwCode,
    PBYTE pBufIn,
    DWORD dwLenIn,
    PBYTE /*pBufOut*/,
    DWORD /*dwLenOut*/,
    PDWORD /*pdwActualOut*/
    )
{
    SETFNAME(_T("KBD_IOControl"));
    
    DWORD dwErr = ERROR_SUCCESS;

    DEBUGMSG(ZONE_FUNCTION, (_T("+%s"), pszFname));

    if (VALID_HID_KBD(pHidKbd) == FALSE) {
        DEBUGMSG(ZONE_ERROR, (_T("%s: ERROR_INVALID_HANDLE\r\n"), pszFname));
        dwErr = ERROR_INVALID_HANDLE;
        goto EXIT;
    }

    switch(dwCode) {
    case IOCTL_KBD_SET_MODIFIERS: {
        DEBUGMSG(ZONE_FUNCTION, (_T("%s: IOCTL_HID_SET_MODIFIERS\r\n"), 
            pszFname));

        if ( (dwLenIn != sizeof(KEY_STATE_FLAGS)) ||
             (pBufIn == NULL) ) {
            DEBUGMSG(ZONE_ERROR, (_T("%s: ERROR_INVALID_PARAMETER\r\n"),
                pszFname));
            dwErr = ERROR_INVALID_PARAMETER;

        }
        else {
            // Only take the values of the following flags, since we keep
            // our own per-keyboard shift, ctrl, and alt states.
            DWORD dwFlags = KeyShiftCapitalFlag | KeyShiftNumLockFlag | 
                KeyShiftScrollLockFlag;
            pHidKbd->KeyStateFlags &= ~dwFlags;
            pHidKbd->KeyStateFlags |= (*((KEY_STATE_FLAGS*) pBufIn) & dwFlags);
            SetLEDs(pHidKbd, pHidKbd->KeyStateFlags);
            DEBUGMSG(ZONE_FUNCTION, (_T("%s: New modifier set = 0x%08x\r\n"), 
                pszFname, pHidKbd->KeyStateFlags));
        }
        break;
    }

    case IOCTL_KBD_SET_AUTOREPEAT: {
        DEBUGMSG(ZONE_FUNCTION, (_T("%s: IOCTL_HID_SET_AUTOREPEAT\r\n"), 
            pszFname));

        if ( (dwLenIn != sizeof(KBDI_AUTOREPEAT_INFO)) ||
             (pBufIn == NULL) ) {
            DEBUGMSG(ZONE_ERROR, (_T("%s: ERROR_INVALID_PARAMETER\r\n"),
                pszFname));
            dwErr = ERROR_INVALID_PARAMETER;
        }
        else {
            // These global writes are not protected since the Layout Manager
            // serializes these IOCTLs.
            KBDI_AUTOREPEAT_INFO *pAutoRepeat = (KBDI_AUTOREPEAT_INFO*) pBufIn;
            
            INT32 iInitialDelay = pAutoRepeat->CurrentInitialDelay;
            INT32 iRepeatRate = pAutoRepeat->CurrentRepeatRate;
            
            iInitialDelay = max(KBD_AUTO_REPEAT_INITIAL_DELAY_MIN, iInitialDelay);
            iInitialDelay = min(KBD_AUTO_REPEAT_INITIAL_DELAY_MAX, iInitialDelay);
          
            if (iRepeatRate) { // Do not alter 0
                iRepeatRate = max(KBD_AUTO_REPEAT_KEYS_PER_SEC_MIN, iRepeatRate);
                iRepeatRate = min(KBD_AUTO_REPEAT_KEYS_PER_SEC_MAX, iRepeatRate);
            }
            
            g_dwAutoRepeatInitialDelay = iInitialDelay;
            g_dwAutoRepeatKeysPerSec   = iRepeatRate;

            DEBUGMSG(ZONE_FUNCTION, (_T("%s: AutoRepeat intial delay = %u\r\n"), 
                pszFname, iInitialDelay));
            DEBUGMSG(ZONE_FUNCTION, (_T("%s: AutoRepeat keys/sec = %u\r\n"), 
                pszFname, iRepeatRate));
        }
        break;
    }

    case IOCTL_KBD_SET_LOCALE_FLAGS: {
        DEBUGMSG(ZONE_FUNCTION, (_T("%s: IOCTL_HID_SET_LOCALE_FLAGS\r\n"), 
            pszFname));

        if ( (dwLenIn != sizeof(DWORD)) || (pBufIn == NULL) ) {
            DEBUGMSG(ZONE_ERROR, (_T("%s: ERROR_INVALID_PARAMETER\r\n"),
                pszFname));
            dwErr = ERROR_INVALID_PARAMETER;
        }
        else {
            DWORD const*const pdwLocaleFlags = (DWORD const*const) pBufIn;
            g_dwLocaleFlags = *pdwLocaleFlags;
            DEBUGMSG(ZONE_FUNCTION, (_T("%s: Local flags = 0x%02x\r\n"), 
                pszFname, *pdwLocaleFlags));
        }
        break;
    }

    default:
        DEBUGMSG(ZONE_ERROR, (_T("%s(0x%x) ERROR_NOT_SUPPORTED\r\n"), 
            pszFname, dwCode));
        dwErr = ERROR_NOT_SUPPORTED;
    }

EXIT:
    if (dwErr != ERROR_SUCCESS) {
        SetLastError(dwErr);
    }

    DEBUGMSG(ZONE_FUNCTION, (_T("-%s"), pszFname));
    return (dwErr == ERROR_SUCCESS);
}


// Set the keyboard's LEDs.
BOOL
SetLEDs(
    PHID_KBD pHidKbd,
    KEY_STATE_FLAGS KeyStateFlags
    )
{   
    struct SHIFT_FLAG_TO_USAGE {
        DWORD dwShiftFlag;
        USAGE usage;
    };

    static const SHIFT_FLAG_TO_USAGE rgShiftToUsage[] = {
        { KeyShiftCapitalFlag, HID_USAGE_LED_CAPS_LOCK },
        { KeyShiftNumLockFlag, HID_USAGE_LED_NUM_LOCK },
        { KeyShiftScrollLockFlag, HID_USAGE_LED_SCROLL_LOCK },
    };

    USAGE rgUsages[_countof(rgShiftToUsage)];
    DWORD cUsages = 0;
    DWORD dwShift;
    DWORD dwErr;
    BOOL fRet;

    NTSTATUS (__stdcall *lpSetUnsetUsages) (
       IN       HIDP_REPORT_TYPE      ReportType,
       IN       USAGE                 UsagePage,
       IN       USHORT                LinkCollection,
       IN       PUSAGE                UsageList,
       IN OUT   PULONG                UsageLength,
       IN       PHIDP_PREPARSED_DATA  PreparsedData,
       IN OUT   PCHAR                 Report,
       IN       ULONG                 ReportLength
       );    

    PREFAST_DEBUGCHK(pHidKbd != NULL);
    DEBUGCHK(pHidKbd->pbOutputBuffer != NULL);

    if (pHidKbd->cbOutputBuffer == 0) {
        // This device does not support output reports.
        return TRUE;
    }

    for (dwShift = 0; dwShift < _countof(rgShiftToUsage); ++dwShift) 
    {
        const SHIFT_FLAG_TO_USAGE *pShiftToUsage = &rgShiftToUsage[dwShift];
        
        if ((pShiftToUsage->dwShiftFlag & KeyStateFlags) != 0) {
            rgUsages[cUsages++] = pShiftToUsage->usage;
        }
    }
    
    // Clear the output report
    DEBUGCHK(pHidKbd->cbOutputBuffer != 0);
    ZeroMemory(pHidKbd->pbOutputBuffer, pHidKbd->cbOutputBuffer);

    // Set up the report.
    if (cUsages == 0) {
        // Set up the report for the proper report ID.
        // In order to do this, we unset a usage.
        rgUsages[0] = HID_USAGE_LED_CAPS_LOCK;
        cUsages = 1;
        lpSetUnsetUsages = HidP_UnsetUsages;
    }
    else {
        lpSetUnsetUsages = HidP_SetUsages;
    }

    (*lpSetUnsetUsages)(
        HidP_Output,
        HID_USAGE_PAGE_LED,
        0,
        rgUsages,
        &cUsages,
        pHidKbd->phidpPreparsedData,
        pHidKbd->pbOutputBuffer,
        pHidKbd->cbOutputBuffer
        );

    // Now send the report.
    dwErr = pHidKbd->pHidFuncs->lpSetReport(pHidKbd->hDevice, HidP_Output, 
        pHidKbd->pbOutputBuffer, pHidKbd->cbOutputBuffer, INFINITE);
    fRet = (dwErr == ERROR_SUCCESS);

    return fRet;
}


