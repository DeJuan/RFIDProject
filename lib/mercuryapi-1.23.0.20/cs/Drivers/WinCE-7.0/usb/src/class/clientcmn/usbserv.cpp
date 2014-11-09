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
// Use of this source code is subject to the terms of the Microsoft end-user
// license agreement (EULA) under which you licensed this SOFTWARE PRODUCT.
// If you did not accept the terms of the EULA, you are not authorized to use
// this source code. For a copy of the EULA, please see the LICENSE.RTF on your
// install media.
//
/*++

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

Module Name:
    usbser.cpp

Abstract:
    USB Client Driver for Frame Work.

Functions:

Notes:

--*/

#include <windows.h>
#include "UsbServ.h"
// Attach.
BOOL UsbClassInterface::Init()
{
    ASSERT(m_pEndpointList == NULL);
    BOOL bReturn = FALSE;
    if ( m_usbInterface.lpEndpoints && m_usbInterface.Descriptor.bNumEndpoints ) { // We have Endpoint 
        for (DWORD dwIndex = 0; dwIndex< m_usbInterface.Descriptor.bNumEndpoints; dwIndex++) {
            USB_ENDPOINT curEndpoint = m_usbInterface.lpEndpoints[dwIndex];
            UsbEndpoint * pNewPipe = new UsbEndpoint(curEndpoint,m_pEndpointList);
            if (pNewPipe && pNewPipe->Init()) {
                m_pEndpointList = pNewPipe ;
            }
            else {
                if (pNewPipe)
                    delete pNewPipe;
                break;
            }
        }
        bReturn = TRUE;
    
    }
    ASSERT(bReturn == TRUE);
    return bReturn;
};
UsbClassInterface::~UsbClassInterface()
{
    while (m_pEndpointList) {
        UsbEndpoint * pNext = m_pEndpointList->GetNextEndpoint();
        delete m_pEndpointList;
        m_pEndpointList = pNext;
    }
}
UsbClientDevice::UsbClientDevice(USB_HANDLE hUsb, LPCUSB_FUNCS UsbFuncsPtr,LPCUSB_INTERFACE lpInputInterface, LPCWSTR szUniqueDriverId,LPCUSB_DRIVER_SETTINGS /*lpDriverSettings*/,DWORD dwDebugZone)
:   m_dwDebugZone(dwDebugZone)
,   m_lpInputInterface(lpInputInterface)
,   m_hUsb (hUsb)
,   USBDFunction(UsbFuncsPtr)
{
    // Copy the szUniqueDriverId;
    m_szUniqueDriverId = NULL;
    m_pClassInterfaceList = NULL;
    m_hDriver = NULL;
    m_hSetupComleteEvent = CreateEvent(NULL,TRUE,TRUE,NULL);
    m_hSetupTransfer = NULL;
    m_fCallbackRegistered = FALSE;
    if (szUniqueDriverId) {
        size_t dwMaxSize = 0;
        VERIFY(SUCCEEDED(StringCchLength(szUniqueDriverId,MAX_PATH, &dwMaxSize)));
        m_szUniqueDriverId = new WCHAR[dwMaxSize+1];
        if (m_szUniqueDriverId)
            VERIFY(SUCCEEDED(StringCchCopy(m_szUniqueDriverId, dwMaxSize+1,szUniqueDriverId)));
    }
};
UsbClientDevice::~UsbClientDevice()
{
    Lock();
    ASSERT(m_hDriver == NULL) ;// If we up to here, Driver should be Detached already.
    CloseTransfer();
    if (m_pClassInterfaceList) { // Delete Interface 
        UsbClassInterface * pNext = m_pClassInterfaceList->GetNextClassInterface();
        delete m_pClassInterfaceList;
        m_pClassInterfaceList = pNext;
    }
    if (m_fCallbackRegistered) {
        UnRegisterNotificationRoutine(m_hUsb, UsbDeviceNotifyStub, this);
    }
    if (m_szUniqueDriverId)
        delete [] m_szUniqueDriverId;
    if (m_hSetupComleteEvent != NULL) {
        CloseHandle(m_hSetupComleteEvent);
    }
    Unlock();
}
BOOL UsbClientDevice::Init()
{
    BOOL bReturn = FALSE;
    ASSERT(m_hDriver==NULL);
    DEBUGMSG(m_dwDebugZone,(TEXT("+UsbClientDevice::Init")));
    Lock();
    if (m_szUniqueDriverId &&m_hSetupComleteEvent!=NULL && Attach()) {
        m_fCallbackRegistered=RegisterNotificationRoutine(m_hUsb,UsbDeviceNotifyStub, this);
        if (m_fCallbackRegistered) {
            ASSERT(m_hDriver == NULL);
            TCHAR ClientRegistryPath[MAX_PATH];
            if (GetClientRegistryPath(ClientRegistryPath,MAX_PATH,m_szUniqueDriverId))
                m_hDriver = ActivateDevice(ClientRegistryPath,(DWORD) this);
        }
    }
    Unlock();
    ASSERT(m_hDriver!=NULL);
    bReturn = (m_hDriver!=NULL);
    DEBUGMSG(m_dwDebugZone,(TEXT("-UsbClientDevice::Init return %d"),bReturn));
    return bReturn;
}
BOOL UsbClientDevice::CloseTransfer()
{
    BOOL bReturn = TRUE;
    Lock();
    if (m_hSetupTransfer!=NULL) {
        m_hSetupTransfer = NULL;
        USBDFunction::CloseTransfer(m_hSetupTransfer);
        SetEvent(m_hSetupComleteEvent);
        m_hSetupTransfer = NULL;
    }
    Unlock();
    return bReturn;
}

BOOL UsbClientDevice::IssueVendorTransfer(DWORD dwFlags,LPCUSB_DEVICE_REQUEST lpControlHeader, LPVOID lpvBuffer, ULONG uBufferPhysicalAddress,DWORD dwTimeout )
{
    BOOL bReturn = FALSE;
    if (lpControlHeader) {
        DEBUGMSG(m_dwDebugZone,(TEXT("+UsbClientDevice::IssueVendorTransfer Setup(%x,%x,%x,%x,%x), length=%d,dwTimeout=%d"),
            lpControlHeader->bmRequestType,lpControlHeader->bRequest,lpControlHeader->wValue,lpControlHeader->wIndex,lpControlHeader->wLength,
            uBufferPhysicalAddress,dwTimeout));
        Lock();
        CloseTransfer();
        ResetEvent(m_hSetupComleteEvent);
        dwFlags |= USB_NO_WAIT;// Always Async.
        m_hSetupTransfer = USBDFunction::IssueVendorTransfer(m_hUsb,TransferNotifyStub,this,dwFlags,lpControlHeader,lpvBuffer,uBufferPhysicalAddress);
        if (m_hSetupTransfer) { 
            bReturn = (WaitForSingleObject( m_hSetupComleteEvent,dwTimeout) == WAIT_OBJECT_0);
        }
        Unlock();
    }
    DEBUGMSG(m_dwDebugZone,(TEXT("-UsbClientDevice::IssueVendorTransfer return %d"),bReturn));
    return bReturn;
}

BOOL UsbClientDevice::Attach()
{
    DEBUGMSG(m_dwDebugZone,(TEXT("UsbClientDevice::Attach")));
    BOOL bReturn = FALSE;
    if (m_lpInputInterface!=NULL) { // This driver is loaded by Interface So.
        if (IsThisInterfaceSupported(m_lpInputInterface) && IsClientDriverSatisfied()) {
            DEBUGMSG(m_dwDebugZone,(TEXT("UsbClientDevice::Attach(InterfaceOnly): Found Supported Interface (bInterfaceNumber=%d,bAlternateSetting=%d"),
                m_lpInputInterface->Descriptor.bInterfaceNumber,m_lpInputInterface->Descriptor.bAlternateSetting));
            if (m_lpInputInterface->Descriptor.bAlternateSetting!=0) { // We need set interface.
                SetInterface(m_lpInputInterface->Descriptor.bInterfaceNumber,m_lpInputInterface->Descriptor.bAlternateSetting);
            }
            UsbClassInterface * pNewInterface = CreateClassInterface(*m_lpInputInterface,m_pClassInterfaceList ) ;
            if (pNewInterface && pNewInterface->Init()) {
                m_pClassInterfaceList = pNewInterface ;
            }
            else {
                ASSERT(FALSE);
                if (pNewInterface)
                    delete pNewInterface;
                return FALSE;
            }
            return TRUE;
        }
    }
    else { // We don't have inteface. So We have to exam the diescriptor.
        LPCUSB_DEVICE  pDevice = GetDeviceInfo();
        if (pDevice) {
            LPCUSB_INTERFACE pUsbInterface   = pDevice->lpActiveConfig->lpInterfaces;
            DWORD dwNumInterfaces = pDevice->lpActiveConfig->dwNumInterfaces;
            LPCUSB_INTERFACE *pSupportedInterface = new LPCUSB_INTERFACE[dwNumInterfaces];
            DWORD dwNumInterfaceSupported = 0;
            LPCUSB_INTERFACE *pUnsupportedInterface = new LPCUSB_INTERFACE[dwNumInterfaces];
            DWORD dwNumInterfaceUnsupported = 0;
            
            while (dwNumInterfaces && pUsbInterface && pSupportedInterface && pUnsupportedInterface ) {
                if (IsThisInterfaceSupported(pUsbInterface)) {
                    DEBUGMSG(m_dwDebugZone,(TEXT("UsbClientDevice::Attach: Found Supported Interface (bInterfaceNumber=%d,bAlternateSetting=%d"),
                        pUsbInterface->Descriptor.bInterfaceNumber,pUsbInterface->Descriptor.bAlternateSetting));
                    pSupportedInterface[dwNumInterfaceSupported] = pUsbInterface ;
                    dwNumInterfaceSupported ++;
                }
                else {
                    pUnsupportedInterface[dwNumInterfaceUnsupported] = pUsbInterface;
                    dwNumInterfaceUnsupported ++;
                }
                pUsbInterface ++ ;
                dwNumInterfaces --;
            }
            if (IsClientDriverSatisfied() && pSupportedInterface && dwNumInterfaceSupported ) { // We at least support one. So, We accept device and enumerate them.
                bReturn = TRUE;
                for (DWORD dwNumIndex = 0;bReturn && pSupportedInterface && dwNumIndex< dwNumInterfaceSupported; dwNumIndex++) {
                    UsbClassInterface * pNewInterface = CreateClassInterface(*pSupportedInterface[dwNumIndex], m_pClassInterfaceList ) ;
                    if (pNewInterface && pNewInterface->Init()) {
                        m_pClassInterfaceList = pNewInterface ;
                    }
                    else {
                        ASSERT(FALSE);
                        if (pNewInterface)
                            delete pNewInterface;
                        bReturn = FALSE;
                        break;
                    }
                }
                for (dwNumIndex = 0;bReturn && pUnsupportedInterface && dwNumIndex< dwNumInterfaceUnsupported; dwNumIndex++) {
                    // We only load the driver that has same bAlternateSetting. (which can work with primaly interface)
                    if (pUnsupportedInterface[dwNumIndex]!=NULL && pUnsupportedInterface[dwNumIndex]->Descriptor.bAlternateSetting==0){
                        if (!LoadGenericInterfaceDriver(m_hUsb,pUnsupportedInterface[dwNumIndex])) { 
                            // We failed to load other driver for unsupported interface. We just ignore them
                            DEBUGMSG(m_dwDebugZone,(TEXT("Unsupported interface(bInterfaceNumber=%d) founded on attached device\n"),pUnsupportedInterface[dwNumIndex]->Descriptor.bInterfaceNumber));
                        }
                    }
                }
            }
            if (pSupportedInterface)
                delete[] pSupportedInterface;
            if (pUnsupportedInterface)
                delete [] pUnsupportedInterface;
        }
    }
    return bReturn;
}
BOOL UsbClientDevice::Detach()
{
    DEBUGMSG(m_dwDebugZone,(TEXT("UsbClientDevice::Detach")));
    Lock();
    if (m_hDriver) {
        DeactivateDevice(m_hDriver);
        m_hDriver = NULL;
    }
    else
        ASSERT(FALSE);
    Unlock();
    return TRUE;
}
BOOL WINAPI UsbClientDevice::UsbDeviceNotifyStub( LPVOID lpvNotifyParameter, DWORD dwCode,LPDWORD * dwInfo1, LPDWORD * dwInfo2,
           LPDWORD * dwInfo3,LPDWORD * dwInfo4)
{
    return ((UsbClientDevice *)lpvNotifyParameter)->UsbDeviceNotify(dwCode,dwInfo1, dwInfo2,dwInfo3,dwInfo4);
}
BOOL UsbClientDevice::UsbDeviceNotify(DWORD dwCode,LPDWORD * /*dwInfo1*/, LPDWORD * /*dwInfo2*/,LPDWORD * /*dwInfo3*/,LPDWORD * /*dwInfo4*/)
{
    switch (dwCode ) {
      case USB_CLOSE_DEVICE:
        UsbClientDeviceMsg(TEXT("USB_CLOSE_DEVICE\n")) ;
        Detach();
        delete this;
    };
    return TRUE;
}


//---------------------------------Usb Transfer--------------------------------
LONG UsbTransfer::m_lCurrentID=0;
UsbTransfer::UsbTransfer(UsbClassPipe *lpClassPipe,UsbTransfer * pNext,DWORD dwDebugZone) 
:   USBDFunction(lpClassPipe->GetClientDevice()->GetUsbFunctionPtr())
,   m_dwDebugZone (dwDebugZone)
,   m_lpClassPipe (lpClassPipe)
,   m_pNextTransfer(pNext)
{
    PREFAST_ASSERT(lpClassPipe!=NULL);
    m_hUsbTransfer = NULL;
    m_hCompleteEvent = CreateEvent(NULL,TRUE,TRUE,NULL);
    m_lpAttachedBuffer = NULL;
    m_dwAttachedSize = 0 ;
    m_dwAttachedPhysicalAddr = 0;
    m_lTransferID = -1;
    m_lpStartAddress = NULL;
    m_lpvNotifyParameter = NULL;
    
}
UsbTransfer::~UsbTransfer()
{
    Lock();
    if (m_hUsbTransfer!=NULL) {
        USBDFunction::CloseTransfer(m_hUsbTransfer) ;
        m_hUsbTransfer = NULL;
        m_lpStartAddress = NULL;
        m_lpvNotifyParameter = NULL;
    }
    if (m_hCompleteEvent!=NULL)
        CloseHandle(m_hCompleteEvent);
    Unlock();
}
void UsbTransfer::Lock()
{   (m_lpClassPipe->GetTransferLock())->Lock(); 
};
void UsbTransfer::Unlock()
{   (m_lpClassPipe->GetTransferLock())->Unlock(); 
};

BOOL UsbTransfer::IsTransferComplete ()
{
    Lock();
    BOOL bReturn = TRUE;
    if (m_hUsbTransfer!=NULL) {
        bReturn = USBDFunction::IsTransferComplete(m_hUsbTransfer);
    }
    Unlock();
    return bReturn;
}
BOOL UsbTransfer::IsCompleteNoError()
{
    BOOL bReturn = FALSE;
    Lock();
    if (m_hUsbTransfer && IsTransferComplete()) {
        DWORD dwBytesTransfered=0;
        DWORD dwError;
        if (GetTransferStatus(&dwBytesTransfered,&dwError) && dwError==USB_NO_ERROR) 
            bReturn = TRUE;
        else {
            DEBUGMSG(m_dwDebugZone,(TEXT("GetTransferStatus return %d!"), dwError));
        }
    }
    Unlock();
    return bReturn;
}
BOOL UsbTransfer::WaitForTransferComplete(DWORD dwTicks, DWORD nCount , CONST HANDLE* lpHandles )
{
    //return TRUE if m_hUsbTransfer is null so that no one waits.
    if(m_hUsbTransfer == NULL)
        return TRUE;
    if (nCount== 0 || lpHandles == NULL)
        return ( WaitForSingleObject(m_hCompleteEvent,dwTicks)==WAIT_OBJECT_0);
    else {
        HANDLE hArray[ MAXIMUM_WAIT_OBJECTS ];
        DWORD dwCurIndex = 0;
        hArray[dwCurIndex++] = m_hCompleteEvent;
        while (dwCurIndex<MAXIMUM_WAIT_OBJECTS && nCount!=0) {
            hArray[dwCurIndex++] = *(lpHandles++);
            nCount -- ;
        }
        return (WaitForMultipleObjects(dwCurIndex,hArray,FALSE,dwTicks) == WAIT_OBJECT_0);
    }
};

BOOL UsbTransfer::GetTransferStatus (LPDWORD lpdwBytesTransfered, LPDWORD lpdwError)
{
    BOOL bReturn = FALSE;
    Lock();
    if (m_hUsbTransfer) 
        bReturn = USBDFunction::GetTransferStatus(m_hUsbTransfer,lpdwBytesTransfered,lpdwError);
    Unlock();
    return bReturn;
}
BOOL UsbTransfer::GetIsochResults(DWORD cFrames, LPDWORD lpdwBytesTransfered, LPDWORD lpdwErrors)
{
    BOOL bReturn = FALSE;
    Lock();
    if (m_hUsbTransfer) 
        bReturn = USBDFunction::GetIsochResults(m_hUsbTransfer,cFrames,lpdwBytesTransfered, lpdwErrors);
    Unlock();
    return bReturn;

}
// transfer maniuplators
BOOL UsbTransfer::AbortTransfer(DWORD dwFlags)
{
    BOOL bReturn = FALSE;
    Lock();
    if (m_hUsbTransfer) 
        bReturn = USBDFunction::AbortTransfer(m_hUsbTransfer,dwFlags);
    Unlock();
    return bReturn;
}
BOOL UsbTransfer::CloseTransfer()
{
    BOOL bReturn = FALSE;
    Lock();
    if (m_hUsbTransfer)  {
        bReturn = USBDFunction::CloseTransfer(m_hUsbTransfer);
        m_dwClientInfo = 0 ;
        SetEvent(m_hCompleteEvent);
        m_hUsbTransfer=NULL;
        m_lpStartAddress = NULL;
        m_lpvNotifyParameter = NULL;
    }
    Unlock();
    return bReturn;
}
BOOL UsbTransfer::IssueControlTransfer(DWORD dwFlags , LPCVOID lpvControlHeader,DWORD dwBufferSize,LPVOID lpvBuffer,ULONG uBufferPhysicalAddress,
        DWORD dwClientInfo,LPTRANSFER_NOTIFY_ROUTINE lpStartAddress,LPVOID lpvNotifyParameter )
{
    BOOL bReturn = FALSE;
    Lock();
    // If you decide Issue Another transfer, I assume you want to close the previous transfer if it hasn't complete
    if (m_hUsbTransfer)  {
        CloseTransfer();
    }
    
    ASSERT(m_hUsbTransfer == NULL );
    PREFAST_ASSERT(m_lpClassPipe!=NULL) ;
    ResetEvent(m_hCompleteEvent );
    
    // Correct dwFlags according what we know.
    if (m_lpClassPipe->IsInPipe())
        dwFlags |= USB_IN_TRANSFER;
    else
        dwFlags &= ~USB_IN_TRANSFER;
    dwFlags |= USB_NO_WAIT;// Always Async.
    
    m_hUsbTransfer = USBDFunction::IssueControlTransfer(m_lpClassPipe->GetPipeHandle(),
            TransferNotifyStub,this,
            dwFlags,lpvControlHeader,dwBufferSize,lpvBuffer,uBufferPhysicalAddress);
    if (m_hUsbTransfer) { 
        m_dwClientInfo = dwClientInfo;
        m_lpStartAddress = lpStartAddress ;
        m_lpvNotifyParameter = lpvNotifyParameter ;
        // For debugging.
        m_lTransferID = ++m_lCurrentID ;
        m_lpAttachedBuffer = lpvBuffer;
        m_dwAttachedSize = dwBufferSize;
        m_dwAttachedPhysicalAddr = uBufferPhysicalAddress ;
    }
    bReturn = (m_hUsbTransfer!=NULL);
    
    Unlock();
    return bReturn;
}
BOOL UsbTransfer::IssueBulkTransfer(DWORD dwFlags , DWORD dwBufferSize, LPVOID lpvBuffer,ULONG uBufferPhysicalAddress,
        DWORD dwClientInfo,LPTRANSFER_NOTIFY_ROUTINE lpStartAddress,LPVOID lpvNotifyParameter )
{
    BOOL bReturn = FALSE;
    Lock();
    // If you decide Issue Another transfer, I assume you want to close the previous transfer if it hasn't complete
    if (m_hUsbTransfer)  {
        CloseTransfer();
    }
    
    ASSERT(m_hUsbTransfer==NULL);
    PREFAST_ASSERT(m_lpClassPipe!=NULL) ;
    ResetEvent(m_hCompleteEvent );
    
    // Correct dwFlags according what we know.
    if (m_lpClassPipe->IsInPipe())
        dwFlags |= USB_IN_TRANSFER;
    else
        dwFlags &= ~USB_IN_TRANSFER;
    dwFlags |= USB_NO_WAIT;// Always Async.
    
    m_hUsbTransfer = USBDFunction::IssueBulkTransfer(m_lpClassPipe->GetPipeHandle(),
            TransferNotifyStub,this,
            dwFlags,dwBufferSize,lpvBuffer,uBufferPhysicalAddress);
    if (m_hUsbTransfer) { 
        m_dwClientInfo = dwClientInfo;
        m_lpStartAddress = lpStartAddress ;
        m_lpvNotifyParameter = lpvNotifyParameter ;
        // For debugging.
        m_lTransferID = ++m_lCurrentID ;
        m_lpAttachedBuffer = lpvBuffer;
        m_dwAttachedSize = dwBufferSize;
        m_dwAttachedPhysicalAddr = uBufferPhysicalAddress ;
    }
    bReturn = (m_hUsbTransfer!=NULL);
    
    Unlock();
    return bReturn;
}
BOOL UsbTransfer::IssueInterruptTransfer(DWORD dwFlags ,DWORD dwBufferSize, LPVOID lpvBuffer,ULONG uBufferPhysicalAddress,
        DWORD dwClientInfo,LPTRANSFER_NOTIFY_ROUTINE lpStartAddress,LPVOID lpvNotifyParameter )
{
    BOOL bReturn = FALSE;
    Lock();
    // If you decide Issue Another transfer, I assume you want to close the previous transfer if it hasn't complete
    if (m_hUsbTransfer)  {
        CloseTransfer();
    }
    
    ASSERT(m_hUsbTransfer == NULL);
    PREFAST_ASSERT(m_lpClassPipe!=NULL) ;
    ResetEvent(m_hCompleteEvent );
    
    // Correct dwFlags according what we know.
    if (m_lpClassPipe->IsInPipe())
        dwFlags |= USB_IN_TRANSFER;
    else
        dwFlags &= ~USB_IN_TRANSFER;
    dwFlags |= USB_NO_WAIT;// Always Async.
    
    m_hUsbTransfer = USBDFunction::IssueInterruptTransfer(m_lpClassPipe->GetPipeHandle(),
            TransferNotifyStub,this,
            dwFlags,dwBufferSize,lpvBuffer,uBufferPhysicalAddress);
    if (m_hUsbTransfer) {
        m_dwClientInfo = dwClientInfo;
        m_lpStartAddress = lpStartAddress ;
        m_lpvNotifyParameter = lpvNotifyParameter ;
        // For debugging.
        m_lTransferID = ++m_lCurrentID ;
        m_lpAttachedBuffer = lpvBuffer;
        m_dwAttachedSize = dwBufferSize;
        m_dwAttachedPhysicalAddr = uBufferPhysicalAddress ;
    }
    bReturn = (m_hUsbTransfer!=NULL);
    
    Unlock();
    return bReturn;
}
BOOL UsbTransfer::IssueIsochTransfer(DWORD dwFlags ,DWORD dwStartingFrame,DWORD dwFrames,LPCDWORD lpdwLengths,LPVOID lpvBuffer,ULONG uBufferPhysicalAddress,
        DWORD dwClientInfo,LPTRANSFER_NOTIFY_ROUTINE lpStartAddress,LPVOID lpvNotifyParameter )
{
    BOOL bReturn = FALSE;
    Lock();
    // If you decide Issue Another transfer, I assume you want to close the previous transfer if it hasn't complete
    if (m_hUsbTransfer)  {
        CloseTransfer();
    }
    
    ASSERT(m_hUsbTransfer == NULL );
    PREFAST_ASSERT(m_lpClassPipe!=NULL) ;
    ResetEvent(m_hCompleteEvent );
    
    // Correct dwFlags according what we know.
    if (m_lpClassPipe->IsInPipe())
        dwFlags |= USB_IN_TRANSFER;
    else
        dwFlags &= ~USB_IN_TRANSFER;
    dwFlags |= USB_NO_WAIT;// Always Async.
    
    m_hUsbTransfer = USBDFunction::IssueIsochTransfer(m_lpClassPipe->GetPipeHandle(),
            TransferNotifyStub,this,
            dwFlags,dwStartingFrame,dwFrames,lpdwLengths,lpvBuffer,uBufferPhysicalAddress);
    if (m_hUsbTransfer) { 
        m_dwClientInfo = dwClientInfo;
        m_lpStartAddress = lpStartAddress ;
        m_lpvNotifyParameter = lpvNotifyParameter ;
        // For debugging.
        m_lTransferID = ++m_lCurrentID ;
        m_lpAttachedBuffer = lpvBuffer; 
        m_dwAttachedSize = dwFrames;
        m_dwAttachedPhysicalAddr = uBufferPhysicalAddress ;
    }
    bReturn = (m_hUsbTransfer!=NULL);
    
    Unlock();
    return bReturn;
}

void UsbTransfer::OutputTransferInfo()
{
    Lock();
    DEBUGMSG(m_dwDebugZone, (TEXT("No transfer ID = %d "),m_lTransferID));
    if (m_hUsbTransfer) {
        if (IsTransferComplete ()) {
            DEBUGMSG(m_dwDebugZone,(TEXT("Transfer Is completet")));
            DWORD dwByteTransfered = 0;
            DWORD dwError = USB_NO_ERROR; 
            BOOL bResult = GetTransferStatus (&dwByteTransfered, &dwError) ;
            DEBUGMSG(m_dwDebugZone,(TEXT("Transfer GetTransferStatus (%d,%d) return %d"),dwByteTransfered,dwError,bResult));
            DBG_UNREFERENCED_LOCAL_VARIABLE(bResult);
        }
        else {
            DEBUGMSG(m_dwDebugZone,(TEXT("Transfer Is Not complete: callback %x,%x"),m_lpStartAddress,m_lpvNotifyParameter));
        }
    }
    else {
        DEBUGMSG(m_dwDebugZone,(TEXT("No transfer Issued")));
    }
    Unlock();
}
UsbClassPipe::UsbClassPipe(USB_ENDPOINT usbEndpoint,UsbClientDevice *lpClientDevice)
:   m_lpClientDevice(lpClientDevice)
{
    Descriptor = usbEndpoint.Descriptor;
    m_hPipeHandle = m_lpClientDevice->OpenPipe(m_lpClientDevice->GetUsbDeviceHandle(),&Descriptor);
}
BOOL UsbClassPipe::IsPipeHalted()
{
    BOOL bHalted = FALSE;
    if (m_lpClientDevice->IsPipeHalted(m_hPipeHandle, &bHalted))
        return bHalted;
    else
        return TRUE;
}
UsbSyncClassPipe::UsbSyncClassPipe(USB_ENDPOINT usbEndpoint,UsbClientDevice *lpClientDevice,DWORD dwZone)
:   UsbClassPipe(usbEndpoint,lpClientDevice)
,   m_dwDebugZone(dwZone)
{
    m_lpTransfer = NULL;
}

UsbSyncClassPipe::~UsbSyncClassPipe()
{
    if (m_lpTransfer)
        delete m_lpTransfer;
}
    
BOOL UsbSyncClassPipe::Init()
{
    if ( UsbClassPipe::Init()) {
        m_lpTransfer = new UsbTransfer(this,NULL,m_dwDebugZone )  ;
        BOOL bReturn = (m_lpClientDevice!=NULL&& m_hPipeHandle!=NULL && m_lpTransfer!=NULL);
        ASSERT(bReturn);
        return bReturn;
    }
    return FALSE;
}

BOOL UsbSyncClassPipe::ResetPipe(BOOL fForce)
{   
    BOOL bReturn = TRUE;
    if (fForce || IsPipeHalted()) { // We only Reset Pipe When Pipe Halted.
        if (m_lpTransfer)
            m_lpTransfer->CloseTransfer(); // Close Any Outstatnding Transfer
        bReturn = m_lpClientDevice->ResetPipe(m_hPipeHandle);
        bReturn = (bReturn && m_lpClientDevice->ClearFeature(USB_FEATURE_ENDPOINT_STALL,Descriptor.bEndpointAddress,2000));
        ASSERT(bReturn);
    }
    return bReturn ;
}

BOOL UsbSyncClassPipe::BulkOrIntrTransfer(DWORD dwFlags,DWORD dwBufferSize, LPVOID lpvBuffer,ULONG uBufferPhysicalAddress,DWORD dwTimeout)
{
    BOOL bReturn = FALSE;
    DEBUGMSG(m_dwDebugZone,(TEXT("+UsbSyncClassPipe::BulkOrIntrTransfer (%d,%x, %d)"),dwBufferSize,lpvBuffer,dwTimeout));
    if ( m_lpTransfer!=NULL && 
            (GetPipeType () == USB_ENDPOINT_TYPE_BULK || GetPipeType () == USB_ENDPOINT_TYPE_INTERRUPT)) {
        if (GetPipeType () == USB_ENDPOINT_TYPE_BULK )
            m_lpTransfer->IssueBulkTransfer(dwFlags,dwBufferSize,lpvBuffer,uBufferPhysicalAddress);
        else
            m_lpTransfer->IssueInterruptTransfer(dwFlags,dwBufferSize,lpvBuffer,uBufferPhysicalAddress);
        if (!m_lpTransfer->IsTransferComplete()) {
            m_lpTransfer->WaitForTransferComplete(dwTimeout);
        }
        bReturn = m_lpTransfer->IsCompleteNoError();
    }
    DEBUGMSG(m_dwDebugZone,(TEXT("-UsbSyncClassPipe::BulkOrIntrTransfer (%d,%x) return %d"),dwBufferSize,lpvBuffer,bReturn));
    return bReturn;
}
BOOL UsbSyncClassPipe::VendorTransfer(DWORD dwFlags,LPCVOID lpvControlHeader,DWORD dwBufferSize, LPVOID lpvBuffer,ULONG uBufferPhysicalAddress,DWORD dwTimeout )
{
    BOOL bReturn = FALSE;
    if ( m_lpTransfer!=NULL && GetPipeType () == USB_ENDPOINT_TYPE_CONTROL) {
            m_lpTransfer->IssueControlTransfer(dwFlags,lpvControlHeader,dwBufferSize,lpvBuffer,uBufferPhysicalAddress);
        if (!m_lpTransfer->IsTransferComplete()) {
            m_lpTransfer->WaitForTransferComplete(dwTimeout);
        }
        bReturn = m_lpTransfer->IsCompleteNoError();
    }
    return bReturn;
}
BOOL UsbSyncClassPipe::IsochTransfer(DWORD dwFlags,DWORD dwStartingFrame,DWORD dwFrames,LPCDWORD lpdwLengths,LPVOID lpvBuffer,ULONG uBufferPhysicalAddress,DWORD dwTimeout)
{
    DEBUGMSG(m_dwDebugZone,(TEXT("+UsbSyncClassPipe::IsochTransfer (0x%x, %d, %d)"),dwFlags,dwStartingFrame,dwTimeout));
    BOOL bReturn = FALSE;
    if ( m_lpTransfer!=NULL && GetPipeType () == USB_ENDPOINT_TYPE_ISOCHRONOUS ) {
            m_lpTransfer->IssueIsochTransfer(dwFlags,dwStartingFrame,dwFrames,lpdwLengths,lpvBuffer,uBufferPhysicalAddress);
        if (!m_lpTransfer->IsTransferComplete()) {
            m_lpTransfer->WaitForTransferComplete(dwTimeout);
        }
        bReturn = m_lpTransfer->IsCompleteNoError();
    }
    DEBUGMSG(m_dwDebugZone,(TEXT("-UsbSyncClassPipe::IsochTransfer (%x,%x) return %d"),lpvBuffer,uBufferPhysicalAddress,bReturn));
    return bReturn;
}

UsbAsyncClassPipe::UsbAsyncClassPipe(USB_ENDPOINT usbEndpoint,UsbClientDevice *lpClientDevice, DWORD dwNumOfTransfer,DWORD dwZone)
:   m_dwNumOfTransfer(dwNumOfTransfer)
,   UsbClassPipe(usbEndpoint,lpClientDevice)
,   m_dwDebugZone(dwZone)
{
    m_lpTransferList = NULL;
    m_lpEmptyTranfser = NULL;
    m_lpArmedTranfser = NULL;
}
UsbAsyncClassPipe:: ~UsbAsyncClassPipe()
{
    Lock();
    while (m_lpTransferList!=NULL) {
        UsbTransfer * lpCreatedTransfer = m_lpTransferList->GetNextTransfer();
        delete m_lpTransferList;
        m_lpTransferList = lpCreatedTransfer;
    }
    Unlock();
}
BOOL UsbAsyncClassPipe::Init()
{
    Lock();
    BOOL bReturn = FALSE;
    if (UsbClassPipe::Init() && m_lpTransferList==NULL) {
        DWORD dwCount = m_dwNumOfTransfer;
        while (dwCount) {
            UsbTransfer * lpCreatedTransfer = new UsbTransfer(this,m_lpTransferList,m_dwDebugZone) ;
            if (lpCreatedTransfer && lpCreatedTransfer->Init()) { 
                m_lpTransferList = lpCreatedTransfer;
            }
            else {
                if (lpCreatedTransfer)
                    delete lpCreatedTransfer;
                break;
            }
            dwCount -- ;
        }
        if (dwCount== 0 && m_lpTransferList!=NULL) {
            bReturn = TRUE;
            m_lpEmptyTranfser = m_lpArmedTranfser = m_lpTransferList;
            
        }
    }
    Unlock();
    return bReturn;
}
BOOL UsbAsyncClassPipe::ResetTransferQueue()
{
    Lock();
    UsbTransfer * pCurTranfer = m_lpTransferList;
    while (pCurTranfer) {
        if (!pCurTranfer->IsTransferEmpty()) {
            pCurTranfer->CloseTransfer();
        }
        pCurTranfer = pCurTranfer->GetNextTransfer();
    }
    m_lpArmedTranfser = m_lpEmptyTranfser = m_lpTransferList ;
    Unlock();
    return TRUE;
}
BOOL UsbAsyncClassPipe::ResetPipe(BOOL fForce)
{
    BOOL bReturn  = TRUE;
    DEBUGMSG(m_dwDebugZone, (TEXT("UsbAsyncClassPipe::ResetPipe(fForce=%d"),fForce));
    if (fForce || IsPipeHalted()) { // We only Reset Pipe When Pipe Halted.
        Lock();
        ResetTransferQueue();
        bReturn = m_lpClientDevice->ResetPipe(m_hPipeHandle);
        bReturn = (bReturn && m_lpClientDevice->ClearFeature(USB_FEATURE_ENDPOINT_STALL,Descriptor.bEndpointAddress,2000));
        Unlock();
        ASSERT(bReturn);
    }
    return bReturn;
}

BOOL UsbAsyncClassPipe::IsEmptyTransferAvailable()
{
    BOOL bReturn = FALSE;
    Lock();
    PREFAST_ASSERT(m_lpEmptyTranfser);
    bReturn = (m_lpEmptyTranfser && m_lpEmptyTranfser->IsTransferEmpty());
    Unlock();
    return bReturn;
        
}
BOOL UsbAsyncClassPipe::WaitForTransferComplete(DWORD dwTicks, DWORD nCount  , CONST HANDLE* lpHandles )
{
    // The Transfer does not delete until the Destructor. So we can wait event on transfer.
    Lock();
    UsbTransfer * lpWaitTransfer =m_lpArmedTranfser;
    Unlock();

    if (lpWaitTransfer) {
        DEBUGMSG(m_dwDebugZone, (TEXT("+UsbAsyncClassPipe::WaitForTransferComplete:%d,dwClientInfo=%x"),dwTicks,lpWaitTransfer->GetClientInfo()));
        return lpWaitTransfer->WaitForTransferComplete(dwTicks,nCount,lpHandles);
    }
    else 
        return FALSE;
}
HANDLE UsbAsyncClassPipe::GetCurrentWaitObjectHandle()
{
    HANDLE hReturn = NULL;
    Lock();
    if (m_lpArmedTranfser) {
        hReturn = m_lpArmedTranfser->GetWaitObjectHandle();
    }
    Unlock();
    return hReturn;
}

BOOL UsbAsyncClassPipe::IsFrontArmedTransferComplete()
{
    BOOL bReturn = FALSE;
    Lock();
    if ( !m_lpArmedTranfser->IsTransferEmpty()){
        bReturn = m_lpArmedTranfser->IsTransferComplete();
    }
    Unlock();
    return bReturn;
}
BOOL UsbAsyncClassPipe::GetFrontArmedTransferStatus(LPDWORD lpdwBytesTransfered, LPDWORD lpdwError)
{
    BOOL bReturn = FALSE;
    Lock();
    if (m_lpArmedTranfser != m_lpEmptyTranfser && m_lpArmedTranfser->IsTransferEmpty()) { // Something Wrong here. We tru to recover.
        ASSERT(FALSE);
        m_lpArmedTranfser = IncTransfer(m_lpArmedTranfser);
    }
    else if ( !m_lpArmedTranfser->IsTransferEmpty()){
        bReturn = m_lpArmedTranfser->GetTransferStatus(lpdwBytesTransfered,lpdwError);
    }
    Unlock();
    return bReturn;    
}
BOOL UsbAsyncClassPipe::GetFrontArmedIsochResults(DWORD cFrames, LPDWORD lpdwBytesTransfered, LPDWORD lpdwErrors)
{
    BOOL bReturn = FALSE;
    Lock();
    if (m_lpArmedTranfser != m_lpEmptyTranfser && m_lpArmedTranfser->IsTransferEmpty()) { // Something Wrong here. We tru to recover.
        ASSERT(FALSE);
        m_lpArmedTranfser = IncTransfer(m_lpArmedTranfser);
    }
    else if ( !m_lpArmedTranfser->IsTransferEmpty()){
        bReturn = m_lpArmedTranfser->GetIsochResults(cFrames, lpdwBytesTransfered, lpdwErrors);
    }
    Unlock();
    return bReturn;    
}

DWORD UsbAsyncClassPipe::GetClientInfo()
{
    DWORD dwClientInfo = 0 ;
    Lock();
    if (m_lpArmedTranfser != m_lpEmptyTranfser && m_lpArmedTranfser->IsTransferEmpty()) { // Something Wrong here. We tru to recover.
        ASSERT(FALSE);
        m_lpArmedTranfser = IncTransfer(m_lpArmedTranfser);
    }
    else if ( !m_lpArmedTranfser->IsTransferEmpty()){
        dwClientInfo = m_lpArmedTranfser->GetClientInfo();
    }
    Unlock();
    DEBUGMSG(m_dwDebugZone, (TEXT("UsbAsyncClassPipe::GetClientInfo return 0x%x"),dwClientInfo)) ;
    return dwClientInfo ;
}
BOOL UsbAsyncClassPipe::CloseAllArmedTransfer()
{
    BOOL bReturn = TRUE;
    Lock();
    while (m_lpArmedTranfser && !m_lpArmedTranfser->IsTransferEmpty()) { // It is not empty
        DEBUGMSG(m_dwDebugZone, (TEXT("UsbAsyncClassPipe::CloseAllArmedTransfer:dwClientInfo=%x"),m_lpArmedTranfser->GetClientInfo()));    
        m_lpArmedTranfser->CloseTransfer();
        m_lpArmedTranfser = IncTransfer(m_lpArmedTranfser);
    }
    Unlock();
    return bReturn;
}
BOOL UsbAsyncClassPipe::CloseFrontArmedTransfer()
{
    BOOL bReturn = TRUE;
    Lock();
    if  (m_lpArmedTranfser && !m_lpArmedTranfser->IsTransferEmpty()) { // It is not empty
        DEBUGMSG(m_dwDebugZone, (TEXT("UsbAsyncClassPipe::CloseFrontArmedTransfer:dwClientInfo=%x"),m_lpArmedTranfser->GetClientInfo()));    
        m_lpArmedTranfser->CloseTransfer();
        m_lpArmedTranfser = IncTransfer(m_lpArmedTranfser);
    }
    Unlock();
    return bReturn;
}
BOOL UsbAsyncClassPipe::BulkOrIntrTransfer(DWORD dwFlags,DWORD dwBufferSize, LPVOID lpvBuffer,ULONG uBufferPhysicalAddress,DWORD dwClientInfo)
{
    DEBUGMSG(m_dwDebugZone, (TEXT("+UsbAsyncClassPipe::BulkOrIntrTransfer:0x%x,%d,0x%x, dwClientInfo=%x"),dwFlags,dwBufferSize,lpvBuffer,dwClientInfo));
    BOOL bReturn = FALSE;
    Lock();
    if ( IsEmptyTransferAvailable() &&
            (GetPipeType () == USB_ENDPOINT_TYPE_BULK || GetPipeType () == USB_ENDPOINT_TYPE_INTERRUPT)) {
        if (GetPipeType () == USB_ENDPOINT_TYPE_BULK )
            m_lpEmptyTranfser->IssueBulkTransfer(dwFlags,dwBufferSize,lpvBuffer,uBufferPhysicalAddress,dwClientInfo);
        else
            m_lpEmptyTranfser->IssueInterruptTransfer(dwFlags,dwBufferSize,lpvBuffer,uBufferPhysicalAddress,dwClientInfo);
        if (!m_lpEmptyTranfser->IsTransferEmpty()) { // Success.
            m_lpEmptyTranfser = m_lpEmptyTranfser->GetNextTransfer();
            if (m_lpEmptyTranfser == NULL)
                m_lpEmptyTranfser = m_lpTransferList ;
            bReturn = TRUE;
        }
    }
    Unlock();
    DEBUGMSG(m_dwDebugZone, (TEXT("-UsbAsyncClassPipe::BulkOrIntrTransfer:0x%x,0x%x, return=%x"),lpvBuffer,uBufferPhysicalAddress,bReturn));
    return bReturn;
}
BOOL UsbAsyncClassPipe::VendorTransfer(DWORD dwFlags,LPCVOID lpvControlHeader,DWORD dwBufferSize, LPVOID lpvBuffer,ULONG uBufferPhysicalAddress,DWORD dwClientInfo )
{
    BOOL bReturn = FALSE;
    Lock();
    if ( IsEmptyTransferAvailable() && GetPipeType () == USB_ENDPOINT_TYPE_CONTROL) {
            m_lpEmptyTranfser->IssueControlTransfer(dwFlags,lpvControlHeader,dwBufferSize,lpvBuffer,uBufferPhysicalAddress,dwClientInfo);            
        if (!m_lpEmptyTranfser->IsTransferEmpty()) { // Success.
            m_lpEmptyTranfser = m_lpEmptyTranfser->GetNextTransfer();
            if (m_lpEmptyTranfser == NULL)
                m_lpEmptyTranfser = m_lpTransferList ;
            bReturn = TRUE;
        }
    }
    Unlock();
    return bReturn;
}
BOOL UsbAsyncClassPipe::IsochTransfer(DWORD dwFlags,DWORD dwStartingFrame,DWORD dwFrames,LPCDWORD lpdwLengths,LPVOID lpvBuffer,ULONG uBufferPhysicalAddress,DWORD dwClientInfo)
{
    BOOL bReturn = FALSE;
    Lock();
    if ( IsEmptyTransferAvailable()  && GetPipeType () == USB_ENDPOINT_TYPE_ISOCHRONOUS ) {
            m_lpEmptyTranfser->IssueIsochTransfer(dwFlags,dwStartingFrame,dwFrames,lpdwLengths,lpvBuffer,uBufferPhysicalAddress,dwClientInfo);
        if (!m_lpEmptyTranfser->IsTransferEmpty()) { // Success.
            m_lpEmptyTranfser = m_lpEmptyTranfser->GetNextTransfer();
            if (m_lpEmptyTranfser == NULL)
                m_lpEmptyTranfser = m_lpTransferList ;
            bReturn = TRUE;
        }
    }
    Unlock();
    return bReturn;
}

