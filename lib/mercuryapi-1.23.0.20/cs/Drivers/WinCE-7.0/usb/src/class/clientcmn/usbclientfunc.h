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
#ifndef __USBCLIENTFUNC_H_
#define __USBCLIENTFUNC_H_
#define LEGGACY_CLIENTDRIVER_PATH TEXT("Drivers\\USB\\ClientDrivers\\")
#include <usbdi.h>
class USBDFunction {
public:
    USBDFunction(LPCUSB_FUNCS pUsbFuncsPtr)
    :   m_pUsbFuncsPtr(pUsbFuncsPtr)
    {; };
    LPCUSB_FUNCS GetUsbFunctionPtr () { return m_pUsbFuncsPtr; };
    VOID GetUSBDVersion(LPDWORD lpdwMajorVersion, LPDWORD lpdwMinorVersion) {
        m_pUsbFuncsPtr->lpGetUSBDVersion(lpdwMajorVersion,lpdwMinorVersion) ; 
    };
    HKEY OpenClientRegistryKey(LPCWSTR szUniqueDriverId) {
        return m_pUsbFuncsPtr->lpOpenClientRegistyKey(szUniqueDriverId) ;
    }
    BOOL GetClientRegistryPath(__out_ecount(dwRegPathUnit) LPWSTR szRegistryPath, DWORD dwRegPathUnit, LPCWSTR szUniqueDriverId) {
        DWORD dwMajorVersion;
        DWORD dwMinorVersion;
        GetUSBDVersion(&dwMajorVersion, &dwMinorVersion) ;
        if (dwMajorVersion>1 || (dwMajorVersion == 1 && dwMinorVersion>=3))
            return m_pUsbFuncsPtr->lpGetClientRegistyPath(szRegistryPath,dwRegPathUnit,szUniqueDriverId);
        else {
            VERIFY(SUCCEEDED(StringCchCopy (szRegistryPath, dwRegPathUnit, LEGGACY_CLIENTDRIVER_PATH)));
            VERIFY(SUCCEEDED(StringCchCat(szRegistryPath, dwRegPathUnit, szUniqueDriverId)));
            return TRUE;
        }
    }
    BOOL RegisterNotificationRoutine(USB_HANDLE hDevice, LPDEVICE_NOTIFY_ROUTINE lpNotifyRoutine, LPVOID lpvNotifyParameter) {
        return m_pUsbFuncsPtr->lpRegisterNotificationRoutine(hDevice, lpNotifyRoutine, lpvNotifyParameter) ;
    }
                                 
    BOOL UnRegisterNotificationRoutine(USB_HANDLE hDevice, LPDEVICE_NOTIFY_ROUTINE lpNotifyRoutine, LPVOID lpvNotifyParameter) {
        return m_pUsbFuncsPtr->lpUnRegisterNotificationRoutine(hDevice, lpNotifyRoutine, lpvNotifyParameter) ;
    }


    BOOL LoadGenericInterfaceDriver(USB_HANDLE hDevice,LPCUSB_INTERFACE lpInterface) {
        return m_pUsbFuncsPtr->lpLoadGenericInterfaceDriver(hDevice,lpInterface);
    }


    BOOL TranslateStringDescr(LPCUSB_STRING_DESCRIPTOR lpStringDescr, LPWSTR szString, DWORD cchStringLength) {
        return m_pUsbFuncsPtr->lpTranslateStringDesc(lpStringDescr,szString,cchStringLength) ;
    }
    LPCUSB_INTERFACE FindInterface(LPCUSB_DEVICE lpDeviceInfo, UCHAR bInterfaceNumber, UCHAR bAlternateSetting) {
        return m_pUsbFuncsPtr->lpFindInterface(lpDeviceInfo, bInterfaceNumber, bAlternateSetting) ;
    }

    BOOL GetFrameNumber(USB_HANDLE hDevice, LPDWORD lpdwFrameNumber) {
        return m_pUsbFuncsPtr->lpGetFrameNumber(hDevice, lpdwFrameNumber) ;
    }
    
    LPCUSB_DEVICE GetDeviceInfo(USB_HANDLE hDevice) {
        return m_pUsbFuncsPtr->lpGetDeviceInfo(hDevice);
    }


    USB_TRANSFER IssueVendorTransfer(USB_HANDLE hDevice,
                                 LPTRANSFER_NOTIFY_ROUTINE lpStartAddress,
                                 LPVOID lpvNotifyParameter, DWORD dwFlags,
                                 LPCUSB_DEVICE_REQUEST lpControlHeader,
                                 LPVOID lpvBuffer,
                                 ULONG uBufferPhysicalAddress) {
        return m_pUsbFuncsPtr->lpIssueVendorTransfer(hDevice,
                                 lpStartAddress,
                                 lpvNotifyParameter, dwFlags,
                                 lpControlHeader,
                                 lpvBuffer,
                                 uBufferPhysicalAddress) ;
    }
    USB_TRANSFER GetInterface(USB_HANDLE hDevice,
                          LPTRANSFER_NOTIFY_ROUTINE lpStartAddress,
                          LPVOID lpvNotifyParameter, DWORD dwFlags,
                          UCHAR bInterfaceNumber, PUCHAR lpbAlternateSetting) {
        return m_pUsbFuncsPtr->lpGetInterface(hDevice,
                          lpStartAddress,
                          lpvNotifyParameter,dwFlags,
                          bInterfaceNumber, lpbAlternateSetting);
    }
    USB_TRANSFER SetInterface(USB_HANDLE hDevice,
                          LPTRANSFER_NOTIFY_ROUTINE lpStartAddress,
                          LPVOID lpvNotifyParameter, DWORD dwFlags,
                          UCHAR bInterfaceNumber, UCHAR bAlternateSetting) {
        return m_pUsbFuncsPtr->lpSetInterface(hDevice,
                          lpStartAddress,
                          lpvNotifyParameter,dwFlags,
                          bInterfaceNumber,  bAlternateSetting) ;
    }
                          
    USB_TRANSFER GetDescriptor(USB_HANDLE hDevice,
                           LPTRANSFER_NOTIFY_ROUTINE lpStartAddress,
                           LPVOID lpvNotifyParameter, DWORD dwFlags,
                           UCHAR bType, UCHAR bIndex, WORD wLanguage,
                           WORD wLength, LPVOID lpvBuffer) {
        return m_pUsbFuncsPtr->lpGetDescriptor(hDevice,
                           lpStartAddress,
                           lpvNotifyParameter, dwFlags,
                           bType, bIndex, wLanguage,
                           wLength, lpvBuffer);
    }
    USB_TRANSFER SetDescriptor(USB_HANDLE hDevice,
                           LPTRANSFER_NOTIFY_ROUTINE lpStartAddress,
                           LPVOID lpvNotifyParameter, DWORD dwFlags,
                           UCHAR bType, UCHAR bIndex, WORD wLanguage,
                           WORD wLength, LPVOID lpvBuffer) {
        return m_pUsbFuncsPtr->lpSetDescriptor(hDevice,
                           lpStartAddress,
                           lpvNotifyParameter, dwFlags,
                           bType, bIndex, wLanguage,
                           wLength, lpvBuffer) ;
    }
    USB_TRANSFER SetFeature(USB_HANDLE hDevice,
                        LPTRANSFER_NOTIFY_ROUTINE lpStartAddress,
                        LPVOID lpvNotifyParameter, DWORD dwFlags,
                        WORD wFeature, UCHAR bIndex) {
        return m_pUsbFuncsPtr->lpSetFeature(hDevice,
                        lpStartAddress,
                        lpvNotifyParameter, dwFlags,
                        wFeature, bIndex) ;
    }
    USB_TRANSFER ClearFeature(USB_HANDLE hDevice,
                          LPTRANSFER_NOTIFY_ROUTINE lpStartAddress,
                          LPVOID lpvNotifyParameter, DWORD dwFlags,
                          WORD wFeature, UCHAR bIndex) {
        return m_pUsbFuncsPtr->lpClearFeature(hDevice,
                          lpStartAddress,
                          lpvNotifyParameter,dwFlags,
                          wFeature, bIndex) ;
    }
    USB_TRANSFER GetStatus(USB_HANDLE hDevice,
                       LPTRANSFER_NOTIFY_ROUTINE lpStartAddress,
                       LPVOID lpvNotifyParameter, DWORD dwFlags, UCHAR bIndex,
                       LPWORD lpwStatus) {
        return m_pUsbFuncsPtr->lpGetStatus(hDevice,
                       lpStartAddress,
                       lpvNotifyParameter, dwFlags, bIndex,
                       lpwStatus) ;
    }
    USB_TRANSFER SyncFrame(USB_HANDLE hDevice,
                       LPTRANSFER_NOTIFY_ROUTINE lpStartAddress,
                       LPVOID lpvNotifyParameter, DWORD dwFlags,
                       UCHAR bEndpoint, LPWORD lpwFrame) {
        return m_pUsbFuncsPtr->lpSyncFrame(hDevice,
                       lpStartAddress,
                       lpvNotifyParameter,dwFlags,
                       bEndpoint, lpwFrame)  ;
    }


    USB_PIPE OpenPipe(USB_HANDLE hDevice,LPCUSB_ENDPOINT_DESCRIPTOR lpEndpointDescriptor) {
        return m_pUsbFuncsPtr->lpOpenPipe(hDevice,lpEndpointDescriptor);
    }
    BOOL AbortPipeTransfers(USB_PIPE hPipe, DWORD dwFlags) {
        return m_pUsbFuncsPtr->lpAbortPipeTransfers(hPipe,dwFlags) ;
    }
    BOOL ResetPipe(USB_PIPE hPipe) {
        return m_pUsbFuncsPtr->lpResetPipe(hPipe) ;
    }
    BOOL ResetDefaultPipe(USB_HANDLE hDevice){
        return m_pUsbFuncsPtr->lpResetDefaultPipe(hDevice);
    }
    BOOL ClosePipe(USB_PIPE hPipe) {
        return m_pUsbFuncsPtr->lpClosePipe(hPipe) ;
    }
    BOOL IsPipeHalted(USB_PIPE hPipe, LPBOOL lpfHalted) {
        return m_pUsbFuncsPtr->lpIsPipeHalted(hPipe, lpfHalted) ;
    }
    BOOL IsDefaultPipeHalted(USB_HANDLE hDevice, LPBOOL lpbHalted) {
        return m_pUsbFuncsPtr->lpIsDefaultPipeHalted(hDevice, lpbHalted);
    }

    USB_TRANSFER IssueControlTransfer(USB_PIPE hPipe,
                                  LPTRANSFER_NOTIFY_ROUTINE lpStartAddress,
                                  LPVOID lpvNotifyParameter, DWORD dwFlags,
                                  LPCVOID lpvControlHeader, DWORD dwBufferSize,
                                  LPVOID lpvBuffer,
                                  ULONG uBufferPhysicalAddress) {
        return m_pUsbFuncsPtr->lpIssueControlTransfer(hPipe,
                                  lpStartAddress,
                                  lpvNotifyParameter, dwFlags,
                                  lpvControlHeader, dwBufferSize,
                                  lpvBuffer,
                                  uBufferPhysicalAddress);
    }
    USB_TRANSFER IssueBulkTransfer(USB_PIPE hPipe,
                               LPTRANSFER_NOTIFY_ROUTINE lpStartAddress,
                               LPVOID lpvNotifyParameter, DWORD dwFlags,
                               DWORD dwBufferSize, LPVOID lpvBuffer,
                               ULONG uBufferPhysicalAddress) {
        return m_pUsbFuncsPtr->lpIssueBulkTransfer(hPipe,
                               lpStartAddress,
                               lpvNotifyParameter, dwFlags,
                               dwBufferSize, lpvBuffer,
                               uBufferPhysicalAddress) ;
    }
    USB_TRANSFER IssueInterruptTransfer(USB_PIPE hPipe,
                                    LPTRANSFER_NOTIFY_ROUTINE lpStartAddress,
                                    LPVOID lpvNotifyParameter, DWORD dwFlags,
                                    DWORD dwBufferSize, LPVOID lpvBuffer,
                                    ULONG uBufferPhysicalAddress) {
        return m_pUsbFuncsPtr->lpIssueInterruptTransfer(hPipe,
                                    lpStartAddress,
                                    lpvNotifyParameter, dwFlags,
                                    dwBufferSize, lpvBuffer,
                                    uBufferPhysicalAddress);
    }
    
    USB_TRANSFER IssueIsochTransfer(USB_PIPE hPipe,
                                LPTRANSFER_NOTIFY_ROUTINE lpStartAddress,
                                LPVOID lpvNotifyParameter, DWORD dwFlags,
                                DWORD dwStartingFrame, DWORD dwFrames,
                                LPCDWORD lpdwLengths, LPVOID lpvBuffer,
                                ULONG uBufferPhysicalAddress) {
        return m_pUsbFuncsPtr->lpIssueIsochTransfer(hPipe,
                                lpStartAddress,
                                lpvNotifyParameter, dwFlags,
                                dwStartingFrame, dwFrames,
                                lpdwLengths, lpvBuffer,
                                uBufferPhysicalAddress) ;
    }


    BOOL IsTransferComplete(USB_TRANSFER hTransfer) {
        return m_pUsbFuncsPtr->lpIsTransferComplete(hTransfer) ;
    }
    BOOL GetTransferStatus(USB_TRANSFER hTransfer, LPDWORD lpdwBytesTransfered,
                       LPDWORD lpdwError) {
        return m_pUsbFuncsPtr->lpGetTransferStatus(hTransfer, lpdwBytesTransfered, lpdwError) ;
    }
    BOOL GetIsochResults(USB_TRANSFER hTransfer, DWORD dwFrames,
                     LPDWORD lpdwBytesTransfered, LPDWORD lpdwErrors) {
        return m_pUsbFuncsPtr->lpGetIsochResults(hTransfer, dwFrames,
                     lpdwBytesTransfered,lpdwErrors) ;
    }
    BOOL AbortTransfer(USB_TRANSFER hTransfer, DWORD dwFlags) {
        return m_pUsbFuncsPtr->lpAbortTransfer(hTransfer, dwFlags) ;
    }
    BOOL CloseTransfer(USB_TRANSFER hTransfer) {
        return m_pUsbFuncsPtr->lpCloseTransfer(hTransfer) ;
    }

    
    BOOL DisableDevice(USB_HANDLE hDevice, BOOL    fResetDevice, BYTE    bInterfaceNumber ) {
        DWORD dwMajorVersion;
        DWORD dwMinorVersion;
        GetUSBDVersion(&dwMajorVersion,&dwMinorVersion) ;
        if (dwMajorVersion>1 || (dwMajorVersion == 1 && dwMinorVersion>=2))
            return m_pUsbFuncsPtr->lpDisableDevice(hDevice,fResetDevice,bInterfaceNumber );
        else
            return FALSE;
    }
    BOOL SuspendDevice(    USB_HANDLE hDevice, BYTE    bInterfaceNumber  ) {
        DWORD dwMajorVersion;
        DWORD dwMinorVersion;
        GetUSBDVersion(&dwMajorVersion,&dwMinorVersion);
        if (dwMajorVersion>1 || (dwMajorVersion == 1 && dwMinorVersion>=2))
            return m_pUsbFuncsPtr->lpSuspendDevice( hDevice, bInterfaceNumber  ) ;
        else
            return FALSE;
    }
    BOOL ResumeDevice( USB_HANDLE hDevice, BYTE    bInterfaceNumber ) {
        DWORD dwMajorVersion;
        DWORD dwMinorVersion;
        GetUSBDVersion(&dwMajorVersion,&dwMinorVersion);
        if (dwMajorVersion>1 || (dwMajorVersion == 1 && dwMinorVersion>=2))
            return m_pUsbFuncsPtr->lpResumeDevice( hDevice, bInterfaceNumber ) ;
        else
            return FALSE;
    }
private:
    LPCUSB_FUNCS m_pUsbFuncsPtr ;
};

#endif

