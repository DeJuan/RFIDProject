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
Module Name: 
    usbInstl.cpp

Abstract:  
    USB driver access class.

Functions:

Notes: 

--*/

#include <windows.h>
#include <UsbInstl.h>

BOOL USBDriverClass::CreateUsbAccessHandle(HINSTANCE hInst)
{
    lpOpenClientRegistyKey=(LPOPEN_CLIENT_REGISTRY_KEY)GetProcAddress(hInst,TEXT("OpenClientRegistryKey"));
    lpRegisterClientDriverID=(LPREGISTER_CLIENT_DRIVER_ID)GetProcAddress(hInst,TEXT("RegisterClientDriverID"));
    lpUnRegisterClientDriverID=(LPUN_REGISTER_CLIENT_DRIVER_ID)GetProcAddress(hInst,TEXT("UnRegisterClientDriverID"));
    lpRegisterClientSettings=(LPREGISTER_CLIENT_SETTINGS)GetProcAddress(hInst,TEXT("RegisterClientSettings"));
    lpUnRegisterClientSettings=(LPUN_REGISTER_CLIENT_SETTINGS)GetProcAddress(hInst,TEXT("UnRegisterClientSettings"));
    lpGetUSBDVersion=(LPGET_USBD_VERSION)GetProcAddress(hInst,TEXT("GetUSBDVersion"));
    if (lpOpenClientRegistyKey &&
            lpRegisterClientDriverID &&
            lpUnRegisterClientDriverID &&
            lpRegisterClientSettings &&
            lpUnRegisterClientSettings &&
            lpGetUSBDVersion ) {
        UsbDriverClassError=FALSE;
        return TRUE;
    }
    else {
        UsbDriverClassError=TRUE;
        return FALSE;
    };
};
USBDriverClass::USBDriverClass(DWORD dwDebugZone)
:   m_sDebugZone(dwDebugZone)
{
    CreateUsbAccessHandle(hInst=LoadLibrary(DEFAULT_USB_DRIVER));
}
USBDriverClass::USBDriverClass(LPCTSTR lpDrvName,DWORD dwDebugZone)
:   m_sDebugZone(dwDebugZone)
{
    CreateUsbAccessHandle(hInst=LoadLibrary(lpDrvName));
}
VOID USBDriverClass::GetUSBDVersion(LPDWORD lpdwMajorVersion, LPDWORD lpdwMinorVersion)
{
    if(lpGetUSBDVersion == NULL)
        return;

    (*lpGetUSBDVersion)(lpdwMajorVersion,lpdwMinorVersion);
};
BOOL USBDriverClass::RegisterClientDriverID(LPCWSTR szUniqueDriverId)
{
    if(lpRegisterClientDriverID == NULL)
        return FALSE;
    return (*lpRegisterClientDriverID)(szUniqueDriverId);
};
BOOL USBDriverClass::UnRegisterClientDriverID(LPCWSTR szUniqueDriverId)
{
    if(lpUnRegisterClientDriverID == NULL)
        return FALSE;
    return (*lpUnRegisterClientDriverID)(szUniqueDriverId);
};
BOOL USBDriverClass::RegisterClientSettings(LPCWSTR lpszDriverLibFile,
                            LPCWSTR lpszUniqueDriverId, LPCUSB_DRIVER_SETTINGS lpDriverSettings)
{
    if(lpRegisterClientSettings == NULL)
        return FALSE;
    return (*lpRegisterClientSettings)(lpszDriverLibFile,lpszUniqueDriverId,NULL,lpDriverSettings);
}
BOOL USBDriverClass::UnRegisterClientSettings(LPCWSTR lpszUniqueDriverId, LPCWSTR szReserved,
                              LPCUSB_DRIVER_SETTINGS lpDriverSettings)
{
    if(lpUnRegisterClientSettings == NULL)
        return FALSE;
    return (*lpUnRegisterClientSettings)(lpszUniqueDriverId,szReserved,lpDriverSettings);
}
HKEY USBDriverClass::OpenClientRegistryKey(LPCWSTR szUniqueDriverId)
{
    if(lpOpenClientRegistyKey == NULL)
        return NULL;
    return (*lpOpenClientRegistyKey)(szUniqueDriverId);
};

BOOL USBDriverClass::SetDefaultDriverRegistry(LPCWSTR szUniqueDriverId, int nCount, REGINI *pReg, BOOL fOverWrite )
{
    HKEY hRegKey;
    if (szUniqueDriverId && nCount && pReg) {
        hRegKey = OpenClientRegistryKey(szUniqueDriverId);
        if (!fOverWrite) { // The registry has exist. We can overwrite it. So Just return.
            RegCloseKey(hRegKey);
            return TRUE;
        }
        if (hRegKey == NULL) {
            RegisterClientDriverID(szUniqueDriverId); // This will create Registry if it does not exist.
            hRegKey = OpenClientRegistryKey(szUniqueDriverId);
        };
        if (hRegKey != NULL) {
            DWORD status = ERROR_SUCCESS ;
            while (nCount && status == ERROR_SUCCESS) {
                LPCWSTR pszName = pReg->lpszVal;
                LPVOID pvData =  pReg->pData;
                if(pszName == NULL || pvData == NULL) {
                    status = ERROR_INVALID_ACCESS;
                    break;
                } else {
                    status = RegSetValueEx(hRegKey, pszName, 0, pReg->dwType, (PBYTE)pvData, pReg->dwLen);
                }
                nCount -- ;
                pReg ++ ;
            }
            RegCloseKey(hRegKey);
            SetLastError( status );
            ASSERT( status== ERROR_SUCCESS ) ;
            return ( status== ERROR_SUCCESS ) ;
        }
       
    }
    SetLastError (ERROR_INVALID_PARAMETER);
    return FALSE;
}

