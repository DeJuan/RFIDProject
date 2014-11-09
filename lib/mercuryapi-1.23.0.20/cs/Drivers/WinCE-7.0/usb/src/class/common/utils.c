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

    utils.c

Abstract:

    Common USB Client Driver utils

--*/

#include "usbclient.h"

//
// GetSetKeyValues
// Get or Set the specified KeyName and its Values to the registry under HKLM
//
// KeyName: HKLM\KeyName
// ValDesc: pointer to a Reg Value Descriptor array. Note the last array entry must be NULL.
// bSet: TRUE to Set, else Get
// bOverwrite: TRUE to overwrite it the key already exists
//
BOOL
GetSetKeyValues(
   LPCTSTR          KeyName,
   PREG_VALUE_DESCR ValDesc,
   BOOL             bSet,
   BOOL             bOverwrite
   )
{
    HKEY   hKey;
    DWORD  dwStatus;
    DWORD  dwDisp;
    DWORD  dwVal;    
    DWORD  dwValLen = 0;
    LPCWSTR pwStr;
    PREG_VALUE_DESCR pValueDesc = NULL;

    if (!KeyName || !ValDesc) {
        return FALSE;
    }

    dwStatus = RegOpenKeyEx( HKEY_LOCAL_MACHINE, KeyName, 0, 0, &hKey);

    if ( SET == bSet ) {

        if ( dwStatus == ERROR_SUCCESS ) {
            // Registry Key already exist, do not overwrite.
            // Assumes the values are correct.
            RegCloseKey( hKey); 
            if (!bOverwrite)
                return TRUE;
        };

        dwStatus = RegCreateKeyEx( HKEY_LOCAL_MACHINE,
                                   KeyName,
                                   0,
                                   NULL,
                                   REG_OPTION_NON_VOLATILE,
                                   0,
                                   NULL,
                                   &hKey,
                                   &dwDisp);

        if (dwStatus != ERROR_SUCCESS) {
            return FALSE;
        }

        pValueDesc = ValDesc;
        while (pValueDesc->Name) {
            switch (pValueDesc->Type) {
                case REG_DWORD:
                    dwValLen = sizeof(DWORD);
                    break;

                case REG_SZ:
                    dwValLen = (wcslen((LPWSTR)pValueDesc->Data) + 1) * sizeof(WCHAR);
                    break;

                case REG_MULTI_SZ:
                    pwStr = (LPWSTR)pValueDesc->Data;
                    dwVal = 0;
                    while (*pwStr) {
                        dwDisp = wcslen(pwStr) + 1;
                        dwVal += dwDisp;
                        pwStr += dwDisp;
                    }
                    dwVal++;

                    dwValLen = dwVal*sizeof(WCHAR);
                    break;
        
                default:
                    ASSERT(0);
                    RegCloseKey(hKey);
                    return FALSE;
                    break;
            }

            __try {
                
                dwStatus = RegSetValueEx( hKey,
                                          pValueDesc->Name,
                                          0,
                                          pValueDesc->Type,
                                          pValueDesc->Data,
                                          dwValLen );

            } __except ( EXCEPTION_EXECUTE_HANDLER ) {
                //
                // did you forgot to allocate space for pValueDesc->Data
                //
                RegCloseKey(hKey);
                return FALSE;
            }

            if (dwStatus != ERROR_SUCCESS ) {
                RegCloseKey(hKey);
                return FALSE;
            }

            pValueDesc++;
        }
    
    } else {

        if ( dwStatus != ERROR_SUCCESS ) {
            return FALSE;
        }

        pValueDesc = ValDesc;
        while (pValueDesc->Name) {
        
            dwStatus = RegQueryValueEx( hKey,
                                        pValueDesc->Name,
                                        NULL,
                                        &pValueDesc->Type,
                                        pValueDesc->Data,
                                        &pValueDesc->Size);

            if (dwStatus != ERROR_SUCCESS ) {
                RegCloseKey(hKey);
                return FALSE;
            }

            pValueDesc++;
        }

    }

    RegCloseKey(hKey);

    return (ERROR_SUCCESS == dwStatus);
}

