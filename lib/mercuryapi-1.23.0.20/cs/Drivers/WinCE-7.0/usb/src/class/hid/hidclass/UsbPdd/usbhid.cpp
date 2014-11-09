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

    usbhid.cpp

Abstract:  
    USB Client Driver for Human Interface Device (HID) Class.

Functions:

Notes: 

--*/

#include "usbhid.h"
#include <devload.h>


#define USB_HID_MAX_REPORT_LENGTH 0xFFFF // HID standard allows 2-byte transfer size


/*
 * DLL entry point
 */
extern "C" 
BOOL
DllEntry(
    HANDLE hDllHandle,
    DWORD dwReason, 
    LPVOID lpReserved
    )
{
    UNREFERENCED_PARAMETER(lpReserved);
    
    switch (dwReason) {
        case DLL_PROCESS_ATTACH:
            DEBUGREGISTER((HINSTANCE)hDllHandle);

            DEBUGMSG(ZONE_INIT, (_T("Hid DllEntry Attach\r\n")));
            DisableThreadLibraryCalls((HMODULE) hDllHandle);
            break;
            
        case DLL_PROCESS_DETACH:
            DEBUGMSG(ZONE_INIT, (_T("Hid DllEntry Detach\r\n")));
            break;
    }

    return HidMdd_DllEntry(dwReason);
}


/*
 * USBInstallDriver
 *
 *   USB driver install routine - set up client registry settings so we will be loaded
 *   by USBD for HID devices.  This function should not be called for systems in which the
 *   OEM ships the HID driver.
 *
 * Return value:
 *   Return TRUE if install succeeds, or FALSE if there is some error.
 */
extern "C" 
BOOL 
USBInstallDriver(
    LPCWSTR szDriverLibFile // Contains client driver DLL name
    )
{
    SETFNAME(_T("USBInstallDriver"));
    
    BOOL fRet = FALSE;

    USB_DRIVER_SETTINGS usbDriverSettings = { DRIVER_SETTINGS };

    REG_VALUE_DESCR rgUsbHidInstanceValues[] = {
        DEVLOAD_DLLNAME_VALNAME, DEVLOAD_DLLNAME_VALTYPE, 0, (PBYTE)(DRIVER_NAME),
        NULL, 0, 0, NULL
    };

    DWORD dwQueuedTransfers = DEFAULT_QUEUED_TRANSFERS;

    REG_VALUE_DESCR rgUsbHidPublicValues[] = {
        DEVLOAD_DLLNAME_VALNAME, DEVLOAD_DLLNAME_VALTYPE, 0, (PBYTE)(DRIVER_NAME),
        DEVLOAD_PREFIX_VALNAME,  DEVLOAD_PREFIX_VALTYPE,  0, (PBYTE)(DEVICE_PREFIX),
        QUEUED_TRANSFERS_SZ,     REG_DWORD,               0, (PBYTE)(&dwQueuedTransfers),
        NULL, 0, 0, NULL
    };

    DEBUGCHK(szDriverLibFile != NULL);
    
    DEBUGMSG(ZONE_INIT, (_T("%s: Install function called (driver: %s)\r\n"), 
        pszFname, szDriverLibFile));

    // register with USBD
    fRet = RegisterClientDriverID(CLASS_NAME_SZ);
    if (fRet == FALSE) {
        DEBUGMSG(ZONE_ERROR, (_T("%s: RegisterClientDriverID error:%d\r\n"), 
            pszFname, GetLastError()));
        goto EXIT;
    }

    fRet = RegisterClientSettings( szDriverLibFile,
                                   CLASS_NAME_SZ, 
                                   NULL, 
                                   &usbDriverSettings );
    
    if (fRet == FALSE) {
        UnRegisterClientDriverID(CLASS_NAME_SZ);
        DEBUGMSG(ZONE_ERROR, (_T("%s: RegisterClientSettings error:%d\r\n"), 
            pszFname, GetLastError()));
        goto EXIT;
    }
    
    // Add our default values to the reg
    if ( !GetSetKeyValues( HID_REGKEY_SZ,
                           rgUsbHidInstanceValues,
                           SET,
                           TRUE ) ) {
        DEBUGMSG(ZONE_ERROR, (_T("%s: GetSetKeyValues failed!\r\n"), pszFname));
        goto EXIT;
    }

    if ( !GetSetKeyValues( CLIENT_REGKEY_SZ,
                           rgUsbHidPublicValues,
                           SET,
                           TRUE ) ) {
        DEBUGMSG(ZONE_ERROR, (_T("%s: GetSetKeyValues failed!\r\n"), pszFname));
        goto EXIT;
    }

    fRet = TRUE;
 
EXIT:  
    return fRet;
}


/*
 * USBUnInstallDriver
 *
 *   This function can be called by a client driver to deregister itself
 *   with USBD.
 */
extern "C"
BOOL 
USBUnInstallDriver(
    VOID
    )
{
    SETFNAME(_T("USBUnInstallDriver"));
    
    BOOL fRet = FALSE;
    const WCHAR szUsbDeviceID[] = CLASS_NAME_SZ;
    USB_DRIVER_SETTINGS usbDriverSettings = { DRIVER_SETTINGS };

    DEBUGMSG(ZONE_INIT, (_T("%s: Uninstall function called\r\n"), pszFname));

    fRet = UnRegisterClientSettings( szUsbDeviceID,
                                     NULL,
                                     &usbDriverSettings );

    if (fRet == TRUE) {
        fRet = UnRegisterClientDriverID(szUsbDeviceID);
    }

    return fRet;
}



/*
 * USBDeviceNotifications
 *
 *    Process notifications from USBD.  Currently, the only notification
 *    supported is a device removal.  Unused parameters are reserved for
 *    future use.
 */
extern "C" 
BOOL 
USBDeviceNotifications(
    LPVOID    lpvNotifyParameter,
    DWORD     dwCode,
    LPDWORD * dwInfo1,
    LPDWORD * dwInfo2,
    LPDWORD * dwInfo3,
    LPDWORD * dwInfo4
    )
{
    SETFNAME(_T("USBDeviceNotifications"));
    
    BOOL fRet = FALSE;
    PUSBHID_CONTEXT pUsbHid = (PUSBHID_CONTEXT) lpvNotifyParameter;
    
    UNREFERENCED_PARAMETER(dwInfo1);
    UNREFERENCED_PARAMETER(dwInfo2);
    UNREFERENCED_PARAMETER(dwInfo3);
    UNREFERENCED_PARAMETER(dwInfo4);

    DEBUGCHK(pUsbHid != NULL);

    ValidateUsbHidContext(pUsbHid);
    
    switch(dwCode)
    {
        case USB_CLOSE_DEVICE:
            DEBUGMSG(ZONE_INIT, (_T("%s: USB_CLOSE_DEVICE\r\n"), pszFname));
            // Remove and free the device context;
            RemoveDeviceContext(pUsbHid);            
            fRet = TRUE;
            break;

        default:
            DEBUGMSG(ZONE_ERROR, (_T("%s: Unhandled code:%d\n"), 
                pszFname, dwCode));
            break;
    }
    
    return fRet;
}


/*
 *  USBDeviceAttach 
 * 
 *    USB device attach routine.  This function is called by USBD when a device is attached
 *    to the USB, and a matching registry key is found off the LoadClients registry key. 
 *    We must determine whether the device may be controlled by this driver, and load 
 *    drivers for any uncontrolled interfaces.
 *
 *  Return Value:
 *    Return TRUE upon success, or FALSE if an error occurs.
 */
extern "C" 
BOOL 
USBDeviceAttach(
    USB_HANDLE hDevice,           // USB device handle
    LPCUSB_FUNCS UsbFuncs,        // Pointer to USBDI function table.
    LPCUSB_INTERFACE UsbInterface,// If client is being loaded as an interface driver, contains a pointer to the USB_INTERFACE
                                  // structure that contains interface information. If client is not loaded for a specific interface,
                                  // this parameter will be NULL.
    LPCWSTR UniqueDriverId,       // Contains client driver id string.
    LPBOOL AcceptControl,         // Filled in with TRUE if we accept control of the device, 
                                  // or FALSE if USBD should continue to try to load client drivers.
    LPCUSB_DRIVER_SETTINGS UsbDriverSettings,// Contains pointer to USB_DRIVER_SETTINGS struct that indicates how we were loaded.
    DWORD Unused                  // Reserved for use with future versions of USBD
    )
{
    SETFNAME(_T("USBDeviceAttach"));

    BOOL fRet = FALSE;
    LPCUSB_INTERFACE pUsbInterface = NULL;
    PUSBHID_CONTEXT pUsbHid = NULL;
    
    DEBUGMSG(ZONE_INIT, (_T("+%s: 0x%x, %s\r\n"), pszFname, hDevice, 
        UniqueDriverId));

    UNREFERENCED_PARAMETER(UniqueDriverId);
    UNREFERENCED_PARAMETER(UsbDriverSettings);
    UNREFERENCED_PARAMETER(Unused);

    PREFAST_DEBUGCHK(UsbFuncs != NULL);
    DEBUGCHK(UniqueDriverId != NULL);
    PREFAST_DEBUGCHK(AcceptControl != NULL);
    DEBUGCHK(UsbDriverSettings != NULL);

    // Determine if we control this USB peripheral...
    *AcceptControl = FALSE;

    pUsbInterface = ParseUsbDescriptors(UsbInterface);

    if (pUsbInterface == NULL) {
        goto EXIT;
    }

    // Tell USBD to stop looking for drivers
    *AcceptControl = TRUE;

    // We found a device and interface we control, so create our device context.
    pUsbHid = CreateUsbHidDevice(hDevice, UsbFuncs, pUsbInterface);
    if (pUsbHid == NULL) {
        goto EXIT;
    }

    // Register to receive notifications regarding this device from USB.
    (*UsbFuncs->lpRegisterNotificationRoutine)(hDevice, 
        USBDeviceNotifications, pUsbHid);

    fRet = TRUE;
    
EXIT:
    if (fRet == FALSE) {
        if (pUsbHid != NULL) {
            RemoveDeviceContext(pUsbHid);
        }
    }
    
    DEBUGMSG(ZONE_INIT, (_T("-%s\r\n"), pszFname));
    return fRet;
}



// Regarding fSendToInterface.  The original HID spec said that the Hid
// descriptor would come after the interface and endpoint descriptors.
// It also said that class specific commands should be sent to the endpoint.
// The next spec said that the HID descriptor would come after the interface
// descriptor (not at the end) and that commands should be sent to the
// interface, not to the endpoint.  So, I'm assuming that if I find the
// Hid descriptor after the interface, the device is following the new spec
// and I should send commands to the interface.  Otherwise, I'll send them
// to the endpoint, as stated in the old spec.
void
DetermineDestination(
    PUSBHID_CONTEXT pUsbHid,
    BYTE *pbmRequestType,
    USHORT *pwIndex
    )
{
    PREFAST_DEBUGCHK(pUsbHid != NULL);
    PREFAST_DEBUGCHK(pbmRequestType != NULL);
    PREFAST_DEBUGCHK(pwIndex != NULL);

    // Do we send this to the endpoint or the interface?
    if (pUsbHid->fSendToInterface == TRUE) {
        *pbmRequestType = USB_REQUEST_FOR_INTERFACE;
        *pwIndex = pUsbHid->pUsbInterface->Descriptor.bInterfaceNumber;
    }
    else {
        *pbmRequestType = USB_REQUEST_FOR_ENDPOINT;
        *pwIndex = pUsbHid->InterruptIn.bIndex;
    }
}


// Get an Input, Output, or Feature report from the device. This is not to be
// be used for recurring interrupt reports. The PDD calls into the MDD with 
// HidMdd_ProcessReport for that.
DWORD
WINAPI
HidPdd_GetReport(
    HID_PDD_HANDLE   hPddDevice,
    HIDP_REPORT_TYPE type,
    PCHAR            pbBuffer,
    DWORD            cbBuffer,
    PDWORD           pcbTransferred,
    DWORD            dwTimeout,
    BYTE             bReportID
    )
{
    SETFNAME(_T("HidPdd_GetReport"));

    PUSBHID_CONTEXT pUsbHid = (PUSBHID_CONTEXT) hPddDevice;

    USB_DEVICE_REQUEST udr;
    DWORD dwErr = ERROR_SUCCESS;
    LPTRANSFER_NOTIFY_ROUTINE pfnNotify = NULL;
    HANDLE hEvent = NULL;
    USB_ERROR usbErr;

    PREFAST_DEBUGCHK(VALID_CONTEXT(pUsbHid));
    ValidateUsbHidContext(pUsbHid);

    // Mdd guarantees the following
    DEBUGCHK(pbBuffer != NULL);
    DEBUGCHK(pcbTransferred != NULL);
    DEBUGCHK(IS_HID_REPORT_TYPE_VALID(type) == TRUE);

    if (cbBuffer > USB_HID_MAX_REPORT_LENGTH) {
        // Reduce to maximum possible value.
        cbBuffer = USB_HID_MAX_REPORT_LENGTH;
    }

    if (dwTimeout != INFINITE) {
        // Set up the wait variables.
        hEvent = CreateEvent(NULL, MANUAL_RESET_EVENT, FALSE, NULL);
        if (hEvent == NULL) {
            dwErr = GetLastError();
            DEBUGMSG(ZONE_ERROR, (_T("%s: CreateEvent error:%d\r\n"), 
                pszFname, dwErr));
            goto EXIT;
        }

        pfnNotify = DefaultTransferComplete;

        if (dwTimeout == 0) {
            // IssueVendorTransfer has a special case for 0 so change it to 1.
            dwTimeout = 1; 
        }
    }

    DetermineDestination(pUsbHid, &udr.bmRequestType, &udr.wIndex);
    udr.bmRequestType |= USB_REQUEST_DEVICE_TO_HOST | USB_REQUEST_CLASS;

    udr.bRequest = USB_REQUEST_HID_GET_REPORT;
    udr.wValue   = USB_DESCRIPTOR_MAKE_TYPE_AND_INDEX(type, bReportID);
    udr.wLength  = (USHORT) cbBuffer;

    dwErr = IssueVendorTransfer(
        pUsbHid->pUsbFuncs,
        pUsbHid->hUsbDevice,
        pfnNotify, 
        hEvent,
        USB_IN_TRANSFER,
        &udr,
        pbBuffer,
        0,
        pcbTransferred,
        dwTimeout,
        &usbErr);

    if (dwErr == ERROR_SUCCESS) {
        if (usbErr == USB_STALL_ERROR) {
            // GET_REPORT is not required according to the HID spec. If it is not 
            // present, the device may return a stall handshake.
            dwErr = ERROR_NOT_SUPPORTED;
        }
        else if (usbErr != USB_NO_ERROR) {
            dwErr = ERROR_GEN_FAILURE;
        }
    }

EXIT:
    if (hEvent != NULL) CloseHandle(hEvent);

    return dwErr;
}


// Set an Input, Output, or Feature report on the device.
DWORD
WINAPI
HidPdd_SetReport(
    HID_PDD_HANDLE   hPddDevice,
    HIDP_REPORT_TYPE type,
    PCHAR            pbBuffer,
    DWORD            cbBuffer,
    DWORD            dwTimeout,
    BYTE             bReportID
    )
{
    SETFNAME(_T("HidPdd_SetReport"));

    PUSBHID_CONTEXT pUsbHid = (PUSBHID_CONTEXT) hPddDevice;
    USB_DEVICE_REQUEST udr;
    DWORD dwBytesTransferred;
    DWORD dwErr = ERROR_SUCCESS;
    LPTRANSFER_NOTIFY_ROUTINE pfnNotify = NULL;
    HANDLE hEvent = NULL;
    USB_ERROR usbErr;

    PREFAST_DEBUGCHK(VALID_CONTEXT(pUsbHid));
    ValidateUsbHidContext(pUsbHid);

    // Mdd guarantees the following
    DEBUGCHK(pbBuffer != NULL);
    DEBUGCHK(IS_HID_REPORT_TYPE_VALID(type) == TRUE);

    if (cbBuffer > USB_HID_MAX_REPORT_LENGTH) {
        DEBUGMSG(ZONE_ERROR, (_T("%s: Buffer is too large\r\n"), pszFname));
        dwErr = ERROR_MESSAGE_EXCEEDS_MAX_SIZE;
        goto EXIT;
    }

    if (dwTimeout != INFINITE) {
        // Set up the wait variables.
        hEvent = CreateEvent(NULL, MANUAL_RESET_EVENT, FALSE, NULL);
        if (hEvent == NULL) {
            dwErr = GetLastError();
            DEBUGMSG(ZONE_ERROR, (_T("%s: CreateEvent error:%d\r\n"), 
                pszFname, dwErr));
            goto EXIT;
        }

        pfnNotify = DefaultTransferComplete;

        if (dwTimeout == 0) {
            // IssueVendorTransfer has a special case for 0 so change it to 1.
            dwTimeout = 1; 
        }
    }


    DetermineDestination(pUsbHid, &udr.bmRequestType, &udr.wIndex);
    udr.bmRequestType |= USB_REQUEST_HOST_TO_DEVICE | USB_REQUEST_CLASS;

    udr.bRequest = USB_REQUEST_HID_SET_REPORT;
    udr.wValue   = USB_DESCRIPTOR_MAKE_TYPE_AND_INDEX(type, bReportID);
    udr.wLength  = (USHORT) cbBuffer;

    dwErr = IssueVendorTransfer(
        pUsbHid->pUsbFuncs,
        pUsbHid->hUsbDevice,
        pfnNotify, 
        hEvent,
        USB_OUT_TRANSFER,
        &udr,
        pbBuffer,
        0,
        &dwBytesTransferred,
        dwTimeout,
        &usbErr);


    if (dwErr == ERROR_SUCCESS) {
        if (usbErr == USB_STALL_ERROR) {
            // SET_REPORT is not required according to the HID spec. If it is not 
            // present, the device may return a stall handshake.
            dwErr = ERROR_NOT_SUPPORTED;
        }
        else if (usbErr != USB_NO_ERROR) {
            dwErr = ERROR_GEN_FAILURE;
        }
    }

EXIT:
    if (hEvent != NULL) CloseHandle(hEvent);
    
    return dwErr;
}


// Retrieves the requested string descriptor and validates it.
static
DWORD
GetStringDescriptor(
    PUSBHID_CONTEXT pUsbHid,
    BYTE bIdx,
    WORD wLangId,
    PBYTE pbBuffer,
    WORD  cbBuffer,
    PDWORD pbTransferred
    )
{
    SETFNAME(_T("GetStringDescriptor"));

    PUSB_STRING_DESCRIPTOR pStringDesc = (PUSB_STRING_DESCRIPTOR) pbBuffer;
    USB_ERROR usbErr;
    USB_TRANSFER hTransfer;
    DWORD dwErr;

    PREFAST_DEBUGCHK(VALID_CONTEXT(pUsbHid));
    ValidateUsbHidContext(pUsbHid);
    
    PREFAST_DEBUGCHK(pbBuffer != NULL);
    DEBUGCHK(cbBuffer >= sizeof(USB_STRING_DESCRIPTOR));
    PREFAST_DEBUGCHK(pbTransferred != NULL);
    
    hTransfer = pUsbHid->pUsbFuncs->lpGetDescriptor(pUsbHid->hUsbDevice, NULL, NULL,
        USB_SHORT_TRANSFER_OK, USB_STRING_DESCRIPTOR_TYPE, bIdx, 
        wLangId, cbBuffer, pbBuffer);

    if (hTransfer != NULL) {
        GetTransferStatus(pUsbHid->pUsbFuncs, hTransfer, 
            pbTransferred, &usbErr);
        CloseTransferHandle(pUsbHid->pUsbFuncs, hTransfer);

        if (usbErr != USB_NO_ERROR) {
            if (usbErr == USB_STALL_ERROR) {
                dwErr = ERROR_NOT_SUPPORTED;
            }        
            else {
                dwErr = ERROR_GEN_FAILURE;
            }

            goto EXIT;
        }
    }
    else {
        dwErr = GetLastError();
        goto EXIT;
    }
    
    // We've got a descriptor. Is it valid?
    if ( (*pbTransferred < (sizeof(USB_STRING_DESCRIPTOR) - sizeof(pStringDesc->bString))) || 
         (pStringDesc->bDescriptorType != USB_STRING_DESCRIPTOR_TYPE) ) { 
        DEBUGCHK(FALSE); // The device returned something strange.
        dwErr = ERROR_GEN_FAILURE;
        goto EXIT;
    }

    dwErr = ERROR_SUCCESS;

EXIT:
    if (dwErr != ERROR_SUCCESS) {
        DEBUGMSG(ZONE_WARNING, (_T("%s: Error getting string descriptor %u, %u. Err=%u\r\n"),
            pszFname, bIdx, wLangId, dwErr));
    }
    
    return dwErr;    
}


// Get a device string. For predefined types see hiddi.h.
// Call with pszBuffer == NULL to get the character count required 
// (then add 1 for the NULL terminator).
DWORD
WINAPI
HidPdd_GetString(
    HID_PDD_HANDLE  hPddDevice,
    HID_STRING_TYPE stringType,
    DWORD           dwIdx,     // Only used with stringType == HID_STRING_INDEXED
    __out_ecount(cchBuffer) LPWSTR pszBuffer, // Set to NULL to get character count
    DWORD           cchBuffer, // Count of chars that will fit into pszBuffer
                               // including the NULL terminator.
    PDWORD          pcchActual // Count of chars in the string NOT including
                               // the NULL terminator
    )
{
    SETFNAME(_T("HidPdd_GetString"));

    const DWORD CB_STRING_DESCRIPTOR_MAX = 0xff;

    PUSBHID_CONTEXT pUsbHid = (PUSBHID_CONTEXT) hPddDevice;

    union {
        BYTE rgbBuffer[CB_STRING_DESCRIPTOR_MAX];
        USB_STRING_DESCRIPTOR StringDesc;
    } StringDescriptor;
    
    PCUSB_DEVICE pDeviceInfo = NULL;
    DWORD dwErr;
    WORD wLangId;
//    DWORD cchToCopy;
    DWORD dwBytesTransferred;
    BYTE bIdx;

    PREFAST_DEBUGCHK(VALID_CONTEXT(pUsbHid));
    ValidateUsbHidContext(pUsbHid);

    // Mdd guarantees the following
    DEBUGCHK(stringType < HID_STRING_MAX);
    DEBUGCHK( ((stringType == HID_STRING_INDEXED) && (dwIdx != 0)) == FALSE );
    PREFAST_DEBUGCHK(pcchActual != NULL);

    if (stringType != HID_STRING_INDEXED) {
        pDeviceInfo = 
            (*pUsbHid->pUsbFuncs->lpGetDeviceInfo)(pUsbHid->hUsbDevice);
        if (pDeviceInfo == NULL) {
            DEBUGMSG(ZONE_ERROR, (_T("%s: Failure getting USB device info\r\n"), 
                pszFname));
            dwErr = ERROR_GEN_FAILURE;
            goto EXIT;
        }

        switch (stringType) {
            case HID_STRING_ID_IMANUFACTURER:
                bIdx = pDeviceInfo->Descriptor.iManufacturer;
                break;
            case HID_STRING_ID_IPRODUCT:
                bIdx = pDeviceInfo->Descriptor.iProduct;
                break;
            case HID_STRING_ID_ISERIALNUMBER:
                bIdx = pDeviceInfo->Descriptor.iSerialNumber;
                break;

            default:
                bIdx = 0;
        }

        if (bIdx == 0) {
            DEBUGMSG(ZONE_COMMENT, (_T("%s: String type %u does not exist\r\n"), 
                pszFname, stringType));
            dwErr = ERROR_NOT_FOUND;
            goto EXIT;
        }
    }
    else {
        // USB indexes strings with a byte. Make sure dwIdx can be represented
        // in a byte.
        if (dwIdx > UCHAR_MAX) {
            DEBUGMSG(ZONE_ERROR, (_T("%s: String index 0x%x is too large\r\n"),
                pszFname, dwIdx));
            dwErr = ERROR_INVALID_PARAMETER;
            goto EXIT;
        }

        bIdx = (BYTE) dwIdx;
    }


    // Get the Zero string descriptor to determine which LANGID to use.
    // We just use the first LANGID listed.
    dwErr = GetStringDescriptor(pUsbHid, 0, 0, (PBYTE) &StringDescriptor,
        sizeof(StringDescriptor), &dwBytesTransferred);
    if (dwErr != ERROR_SUCCESS) {
        goto EXIT;
    }

    DEBUGCHK(StringDescriptor.StringDesc.bLength >= sizeof(USB_STRING_DESCRIPTOR));
    DEBUGCHK(StringDescriptor.StringDesc.bDescriptorType == USB_STRING_DESCRIPTOR_TYPE);
    wLangId = StringDescriptor.StringDesc.bString[0];

    // Get the string descriptor for the first LANGID
    dwErr = GetStringDescriptor(pUsbHid, bIdx, wLangId, (PBYTE) &StringDescriptor,
        sizeof(StringDescriptor), &dwBytesTransferred);
    if (dwErr != ERROR_SUCCESS) {
        goto EXIT;
    }

    __try {
        // Copy the character count and string into the user's buffer
        *pcchActual = (StringDescriptor.StringDesc.bLength - 2) / sizeof(WCHAR); // Does not include NULL
        if (pszBuffer != NULL) {
            VERIFY(SUCCEEDED(StringCchCopyN(pszBuffer, cchBuffer, 
                StringDescriptor.StringDesc.bString, *pcchActual)));
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        DEBUGMSG(ZONE_ERROR, (_T("%s: Exception writing to user buffer\r\n"),
            pszFname));
        dwErr = ERROR_INVALID_PARAMETER;
    }

EXIT:
    return dwErr;    
}


// Get the idle rate for a specific report.
DWORD
WINAPI
HidPdd_GetIdle(
    HID_PDD_HANDLE hPddDevice,
    PDWORD         pdwIdle,
    DWORD          dwReportID
    )
{
    SETFNAME(_T("HidPdd_GetIdle"));

    PUSBHID_CONTEXT pUsbHid = (PUSBHID_CONTEXT) hPddDevice;
    USB_DEVICE_REQUEST udr;
    DWORD dwBytesTransferred;
    DWORD dwErr;
    BYTE bIdle;
    USB_ERROR usbErr;
    
    PREFAST_DEBUGCHK(VALID_CONTEXT(pUsbHid));
    ValidateUsbHidContext(pUsbHid);
    
    PREFAST_DEBUGCHK(pdwIdle != NULL);

    if (dwReportID > UCHAR_MAX) {
        DEBUGMSG(ZONE_ERROR, (_T("%s: Report ID of %u is too large.\r\n"),
            pszFname, dwReportID));
        dwErr = ERROR_INVALID_PARAMETER;
        goto EXIT;
    }
    
    // Do we send to the endpoint or interface?
    DetermineDestination(pUsbHid, &udr.bmRequestType, &udr.wIndex);
    udr.bmRequestType |= USB_REQUEST_DEVICE_TO_HOST | USB_REQUEST_CLASS;

    udr.bRequest = USB_REQUEST_HID_GET_IDLE;
    udr.wValue   = USB_DESCRIPTOR_MAKE_TYPE_AND_INDEX(0, dwReportID);
    udr.wLength  = 1;

    dwErr = IssueVendorTransfer(
        pUsbHid->pUsbFuncs,
        pUsbHid->hUsbDevice,
        NULL, 
        NULL,
        USB_IN_TRANSFER,
        &udr,
        &bIdle,
        0,
        &dwBytesTransferred,
        INFINITE,
        &usbErr);

    if (dwErr == ERROR_SUCCESS) {
        if (usbErr == USB_STALL_ERROR) {
            // GET_IDLE is not required according to the HID spec. If it is not 
            // present, the device may return a stall handshake.
            dwErr = ERROR_NOT_SUPPORTED;
        }
        else if (usbErr != USB_NO_ERROR) {
            dwErr = ERROR_GEN_FAILURE;
        }
    }
    else {
        // Success. Write value to user buffer.
        __try {
            *pdwIdle = (DWORD) bIdle;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            DEBUGMSG(ZONE_ERROR, (_T("%s: Exception writing to user buffer\r\n"),
                pszFname));
            dwErr = ERROR_INVALID_PARAMETER;
        }
    }

EXIT:
    return dwErr;
}


// Set the idle rate for a specific report. The idle rate is expressed in
// 4 ms increments, so to set idle rate of 20 ms, bDuration should be 5.
// bDuration of 0 means infinite. bReportID of 0 means to apply to all
// reports.
DWORD
WINAPI
HidPdd_SetIdle(
    HID_PDD_HANDLE hPddDevice,
    DWORD          dwDuration,
    DWORD          dwReportID
    )
{
    SETFNAME(_T("HidPdd_SetIdle"));

    PUSBHID_CONTEXT pUsbHid = (PUSBHID_CONTEXT) hPddDevice;
    USB_DEVICE_REQUEST udr;
    DWORD dwBytesTransferred;
    DWORD dwErr;
    USB_ERROR usbErr;
        
    PREFAST_DEBUGCHK(VALID_CONTEXT(pUsbHid));
    ValidateUsbHidContext(pUsbHid);

    if (dwReportID > UCHAR_MAX) {
        DEBUGMSG(ZONE_ERROR, (_T("%s: Report ID of %u is too large.\r\n"),
            pszFname, dwReportID));
        dwErr = ERROR_INVALID_PARAMETER;
        goto EXIT;
    }

    if (dwDuration > UCHAR_MAX) {
        // Use the maximum possible value.
        DEBUGMSG(ZONE_WARNING, (_T("%s: Reducing duration to max BYTE value\r\n"),
            pszFname));
        dwDuration = UCHAR_MAX;
    }

    // Do we send to the endpoint or interface?
    DetermineDestination(pUsbHid, &udr.bmRequestType, &udr.wIndex);
    udr.bmRequestType |= USB_REQUEST_HOST_TO_DEVICE | USB_REQUEST_CLASS;

    udr.bRequest = USB_REQUEST_HID_SET_IDLE;
    udr.wValue   = USB_DESCRIPTOR_MAKE_TYPE_AND_INDEX(dwDuration, dwReportID);
    udr.wLength  = 0;

    dwErr = IssueVendorTransfer(
        pUsbHid->pUsbFuncs,
        pUsbHid->hUsbDevice,
        NULL, 
        NULL,
        USB_OUT_TRANSFER,
        &udr,
        NULL,
        0,
        &dwBytesTransferred,
        INFINITE,
        &usbErr);

    if (dwErr == ERROR_SUCCESS) {
        if (usbErr == USB_STALL_ERROR) {
            // SET_IDLE is not required according to the HID spec. If it is not 
            // present, the device may return a stall handshake.
            dwErr = ERROR_NOT_SUPPORTED;
        }
        else if (usbErr != USB_NO_ERROR) {
            dwErr = ERROR_GEN_FAILURE;
        }
    }

EXIT:
    return dwErr;
}


// Enable remote wakeup if possible.
static
DWORD
EnableRemoteWakeup(
    PUSBHID_CONTEXT pUsbHid,
    BOOL fEnable
    )
{
    SETFNAME(_T("EnableRemotWakeup"));
    
    DWORD dwErr;
    
    PREFAST_DEBUGCHK(pUsbHid != NULL);
    ValidateUsbHidContext(pUsbHid);
        
    dwErr = ClearOrSetFeature(
        pUsbHid->pUsbFuncs,
        pUsbHid->hUsbDevice,
        NULL,
        NULL,
        USB_SEND_TO_DEVICE,
        USB_FEATURE_REMOTE_WAKEUP,
        0,
        0,
        fEnable
        );

    if (dwErr != ERROR_SUCCESS) {
        DEBUGMSG(ZONE_ERROR, (_T("%s: Set Remote Wakeup Feature failure. Error: 0x%x\r\n"), 
            pszFname, dwErr));
    }

    return dwErr;
}


// Commands the PDD to initiate some activity or configuration.
DWORD
WINAPI
HidPdd_IssueCommand(
    HID_PDD_HANDLE hPddDevice,        // PDD function parameter
    DWORD          dwMsg,             // Notification message
    WPARAM         wParam             // Message parameter
    )
{
    PUSBHID_CONTEXT pUsbHid = (PUSBHID_CONTEXT) hPddDevice;
    DWORD dwErr = ERROR_SUCCESS;

    ValidateUsbHidContext(pUsbHid);

    if (dwMsg == HID_PDD_REMOTE_WAKEUP) {
        dwErr = ERROR_SUCCESS;
        if (pUsbHid->Flags.RemoteWakeup == TRUE) {
            BOOL fEnable = wParam;
            dwErr = EnableRemoteWakeup(pUsbHid, fEnable);
        }
    }
    else {
        DEBUGCHK(FALSE);
        dwErr = ERROR_NOT_SUPPORTED;
    }

    return dwErr;
}


//
// ***** HID Stream Interface *****
//

// We do not expose any PDD IOCTLs, so these all pass through to the MDD.

extern "C" 
DWORD
HID_Init(
    LPCWSTR lpszDevKey
    )
{
    return HidMdd_Init(lpszDevKey);
}


extern "C" 
BOOL
HID_Deinit(
    DWORD dwContext
    )
{
    return HidMdd_Deinit(dwContext);
}


extern "C" 
DWORD
HID_Open(
    DWORD dwContext,
    DWORD dwAccMode,
    DWORD dwShrMode
    )
{
    return HidMdd_Open(dwContext, dwAccMode, dwShrMode);
}


extern "C" 
BOOL
HID_Close(
    DWORD dwContext
    )
{
    return HidMdd_Close(dwContext);
}


extern "C" 
BOOL
HID_IOControl (
    DWORD  dwContext,
    DWORD  dwCode,
    __in_ecount(dwInpLen) PBYTE  pInpBuf,
    DWORD  dwInpLen,
    PBYTE  pOutBuf,
    DWORD  dwOutLen,
    PDWORD pdwActualOutLen
    )
{
    // We do not have any PDD IOCTLs, so pass everything on to the MDD.
    return HidMdd_IOControl(dwContext, dwCode, pInpBuf, 
        dwInpLen, pOutBuf, dwOutLen, pdwActualOutLen);
}


