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
// Copyright (c) Microsoft Corporation.  All rights reserved.
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

--*/
/*++

Module Name:  
    usbserv.h

Abstract:  
    USB driver service class.
    
Notes: 

--*/

#ifndef __USBSERV_H_
#define __USBSERV_H_
#include <usbdi.h>
#include <Csync.h>
#include <usbclientfunc.h>

class UsbClientDevice ;
class UsbClassInterface ;
class UsbClassPipe ;
class UsbClassPipe ;
class UsbEndpoint;
class UsbTransfer ;

// Attach.
class UsbEndpoint {
public:
    UsbEndpoint(USB_ENDPOINT usbEndpoint,UsbEndpoint * pNext) 
    :   m_UsbEndpoint(usbEndpoint)
    ,   m_pNext(pNext)
    {; }
    virtual BOOL Init() { return TRUE; };
    UsbEndpoint *GetNextEndpoint() { return m_pNext;};
    USB_ENDPOINT_DESCRIPTOR GetDescriptor() { return m_UsbEndpoint.Descriptor; };
    DWORD GetPipeType () { return (m_UsbEndpoint.Descriptor.bmAttributes &  USB_ENDPOINT_TYPE_MASK); };
    BOOL  IsInPipe() { return (USB_ENDPOINT_DIRECTION_IN(m_UsbEndpoint.Descriptor.bEndpointAddress)!=0); };
protected:
    UsbEndpoint&operator=(UsbEndpoint&){ASSERT(FALSE);}
    UsbEndpoint * m_pNext;
    const USB_ENDPOINT  m_UsbEndpoint;
};
class UsbClassInterface {
public:
    UsbClassInterface(USB_INTERFACE usbIntface,UsbClassInterface * pNext = NULL )
    :   m_usbInterface(usbIntface)
    ,   m_pNextInterface(pNext)
    {   m_pEndpointList = NULL;   };
    virtual ~UsbClassInterface();
    virtual BOOL Init();
    USB_INTERFACE_DESCRIPTOR GetDescriptor() { return m_usbInterface.Descriptor; };
    UsbClassInterface * GetNextClassInterface() { return m_pNextInterface; };
protected:
    UsbClassInterface&operator=(UsbClassInterface&){ASSERT(FALSE);}
    const USB_INTERFACE     m_usbInterface;
    UsbEndpoint *           m_pEndpointList;
    UsbClassInterface *     m_pNextInterface;
};

#define DEFAULT_SETUP_TIMEOUT 2000 
class UsbClientDevice :  public USBDFunction, public CLockObject {
public:
    UsbClientDevice(USB_HANDLE hUsb, LPCUSB_FUNCS UsbFuncsPtr,LPCUSB_INTERFACE lpInputInterface, LPCWSTR szUniqueDriverId,LPCUSB_DRIVER_SETTINGS lpDriverSettings,DWORD dwDebugZone  );
    virtual ~UsbClientDevice();
    virtual BOOL Init() ;
    LPCUSB_INTERFACE GetInterCurface() { return m_lpInputInterface; };
    LPCWSTR GetUniqueDriverId() { return m_szUniqueDriverId; };
    #define UsbClientDeviceMsg(x) DEBUGMSG(m_dwDebugZone,(x));
// Virtual Function need to be fill up by Client driver.
    virtual BOOL IsThisInterfaceSupported(LPCUSB_INTERFACE usbInterface) = 0 ;
    virtual BOOL IsClientDriverSatisfied() = 0 ;
    USB_HANDLE  GetUsbDeviceHandle() { return m_hUsb; };
protected:
    const DWORD     m_dwDebugZone;
    LPCUSB_INTERFACE m_lpInputInterface;
    LPWSTR          m_szUniqueDriverId;
    USB_HANDLE      m_hUsb;
    HANDLE          m_hDriver ;
    UsbClassInterface * m_pClassInterfaceList;
    BOOL            m_fCallbackRegistered;
    LPCUSB_DEVICE   GetDeviceInfo() { return USBDFunction::GetDeviceInfo(m_hUsb); };
    BOOL IsSameInterface(USB_INTERFACE_DESCRIPTOR& Interface1,USB_INTERFACE_DESCRIPTOR& lpInterface2 )  {
        if (Interface1.bInterfaceNumber == lpInterface2.bInterfaceNumber &&
                Interface1.bAlternateSetting == lpInterface2.bAlternateSetting ) {
            return TRUE;
        }
        return FALSE;
    }
    virtual UsbClassInterface *CreateClassInterface(USB_INTERFACE usbIntface, UsbClassInterface * pNext = NULL ) {
        return new UsbClassInterface(usbIntface,pNext);
    }
    virtual BOOL Attach();
    virtual BOOL Detach();
    virtual BOOL UsbDeviceNotify(DWORD dwCode,LPDWORD * dwInfo1, LPDWORD * dwInfo2,LPDWORD * dwInfo3,LPDWORD * dwInfo4) ;
private:
    static BOOL WINAPI UsbDeviceNotifyStub( LPVOID lpvNotifyParameter, DWORD dwCode,LPDWORD * dwInfo1, LPDWORD * dwInfo2,
           LPDWORD * dwInfo3,LPDWORD * dwInfo4);
    
    
// Endpoint Zero Traffic Handling.
public:
    BOOL CloseTransfer() ;
    BOOL IssueVendorTransfer(DWORD dwFlags,LPCUSB_DEVICE_REQUEST lpControlHeader, LPVOID lpvBuffer, ULONG uBufferPhysicalAddress,DWORD dwTimeout = DEFAULT_SETUP_TIMEOUT);
    BOOL GetInterface(UCHAR bInterfaceNumber, PUCHAR lpbAlternateSetting,DWORD dwTimeout = DEFAULT_SETUP_TIMEOUT ) {
        BOOL bReturn = FALSE;
        Lock();
        CloseTransfer();
        ResetEvent(m_hSetupComleteEvent);
        DWORD dwFlags = USB_IN_TRANSFER|USB_NO_WAIT;// Always Async.
        m_hSetupTransfer = USBDFunction::GetInterface(m_hUsb,TransferNotifyStub,this,dwFlags,bInterfaceNumber,lpbAlternateSetting);
        if (m_hSetupTransfer) { 
            bReturn = (WaitForSingleObject( m_hSetupComleteEvent,dwTimeout) == WAIT_OBJECT_0);
            CloseTransfer();
        }
        Unlock();
        return bReturn;
    }
    BOOL SetInterface(UCHAR bInterfaceNumber, UCHAR bAlternateSetting ,DWORD dwTimeout = DEFAULT_SETUP_TIMEOUT ) {
        BOOL bReturn = FALSE;
        Lock();
        CloseTransfer();
        ResetEvent(m_hSetupComleteEvent);
        DWORD dwFlags = USB_NO_WAIT;// Always Async.
        m_hSetupTransfer = USBDFunction::SetInterface(m_hUsb,TransferNotifyStub,this,dwFlags,bInterfaceNumber,bAlternateSetting);
        if (m_hSetupTransfer) { 
            bReturn = (WaitForSingleObject( m_hSetupComleteEvent,dwTimeout) == WAIT_OBJECT_0);
            CloseTransfer();
        }
        Unlock();
        return bReturn;
    }
    BOOL GetDescriptor(UCHAR bType, UCHAR bIndex, WORD wLanguage,WORD wLength, LPVOID lpvBuffer,DWORD dwTimeout = DEFAULT_SETUP_TIMEOUT ) {
        BOOL bReturn = FALSE;
        Lock();
        CloseTransfer();
        ResetEvent(m_hSetupComleteEvent);
        DWORD dwFlags = USB_IN_TRANSFER|USB_NO_WAIT|USB_SHORT_TRANSFER_OK;// Always Async.
        m_hSetupTransfer = USBDFunction::GetDescriptor(m_hUsb,TransferNotifyStub,this,dwFlags,bType,bIndex,wLanguage,wLength,lpvBuffer);
        if (m_hSetupTransfer) { 
            bReturn = (WaitForSingleObject( m_hSetupComleteEvent,dwTimeout) == WAIT_OBJECT_0);
            CloseTransfer();
        }
        Unlock();
        return bReturn;
    }
    BOOL SetFeature( WORD wFeature, UCHAR bIndex,DWORD dwTimeout = DEFAULT_SETUP_TIMEOUT ) {
        BOOL bReturn = FALSE;
        Lock();
        CloseTransfer();
        ResetEvent(m_hSetupComleteEvent);
        DWORD dwFlags = USB_NO_WAIT;// Always Async.
        m_hSetupTransfer = USBDFunction::SetFeature(m_hUsb,TransferNotifyStub,this,dwFlags,wFeature,bIndex);
        if (m_hSetupTransfer) { 
            bReturn = (WaitForSingleObject( m_hSetupComleteEvent,dwTimeout) == WAIT_OBJECT_0);
            CloseTransfer();
        }
        Unlock();
        return bReturn;
    }
    BOOL ClearFeature( WORD wFeature, UCHAR bIndex,DWORD dwTimeout = DEFAULT_SETUP_TIMEOUT ) {
        BOOL bReturn = FALSE;
        Lock();
        CloseTransfer();
        ResetEvent(m_hSetupComleteEvent);
        DWORD dwFlags = USB_NO_WAIT | (wFeature == USB_FEATURE_ENDPOINT_STALL? USB_SEND_TO_ENDPOINT: USB_SEND_TO_DEVICE) ;
        m_hSetupTransfer = USBDFunction::ClearFeature(m_hUsb,TransferNotifyStub,this,dwFlags,wFeature,bIndex);
        if (m_hSetupTransfer) { 
            bReturn = (WaitForSingleObject( m_hSetupComleteEvent,dwTimeout) == WAIT_OBJECT_0);
            CloseTransfer();
        }
        Unlock();
        return bReturn;
    }
    BOOL GetStatus(UCHAR bIndex,LPWORD lpwStatus,DWORD dwTimeout = DEFAULT_SETUP_TIMEOUT ) {
        BOOL bReturn = FALSE;
        Lock();
        CloseTransfer();
        ResetEvent(m_hSetupComleteEvent);
        DWORD dwFlags = USB_NO_WAIT;// Always Async.
        m_hSetupTransfer = USBDFunction::GetStatus(m_hUsb,TransferNotifyStub,this,dwFlags,bIndex,lpwStatus);
        if (m_hSetupTransfer) { 
            bReturn = (WaitForSingleObject( m_hSetupComleteEvent,dwTimeout) == WAIT_OBJECT_0);
            CloseTransfer();
        }
        Unlock();
        return bReturn;
    }
protected:
    virtual void CompleteNotify() { 
        SetEvent(m_hSetupComleteEvent);
    };
    HANDLE m_hSetupComleteEvent ;
    USB_TRANSFER m_hSetupTransfer;
private:
    UsbClientDevice&operator=(UsbClientDevice&){ASSERT(FALSE);}
    static DWORD WINAPI TransferNotifyStub(LPVOID lpvNotifyParameter) {
        ((UsbClientDevice*)lpvNotifyParameter)->CompleteNotify();
        return 0;
    }
};
// Operation.
class UsbTransfer: USBDFunction {
public:
    UsbTransfer(UsbClassPipe *lpClassPipe,UsbTransfer * pNext , DWORD dwDebugZone = 0) ;
    virtual ~UsbTransfer();
    virtual BOOL Init() { return m_hCompleteEvent!=NULL && m_lpClassPipe!=NULL ; };
    //
    // get info on Transfers
    BOOL IsTransferComplete ();
    BOOL IsCompleteNoError();
    BOOL GetTransferStatus ( LPDWORD, LPDWORD);
    BOOL GetIsochResults(DWORD, LPDWORD, LPDWORD);
    BOOL IsTransferEmpty () { return m_hUsbTransfer == NULL; };
    UsbTransfer * GetNextTransfer() { return m_pNextTransfer; };
    //
    // transfer maniuplators
    BOOL AbortTransfer(DWORD dwFlags=0);
    BOOL CloseTransfer();
    USB_TRANSFER GetTransferHandle() { return m_hUsbTransfer; };
    USB_TRANSFER SetTransferHandle(USB_TRANSFER usbHandle){ return (m_hUsbTransfer=usbHandle); }
    //
    // Transfer Completion callback;
    virtual void CompleteNotify() { 
        SetEvent(m_hCompleteEvent);
        if (m_lpStartAddress) {
            __try {
                (*m_lpStartAddress)(m_lpvNotifyParameter);
            } __except( EXCEPTION_EXECUTE_HANDLER ) {
                DEBUGMSG(m_dwDebugZone,(TEXT("exception on callback %x,%x \n"),m_lpStartAddress,m_lpvNotifyParameter));
            }
        }
    };
    virtual BOOL WaitForTransferComplete(DWORD dwTicks=INFINITE, DWORD nCount = 0 , CONST HANDLE* lpHandles = NULL);
    virtual HANDLE GetWaitObjectHandle() { return m_hCompleteEvent; };
    //
    // Transfer Function.
    virtual BOOL IssueControlTransfer(DWORD dwFlags,LPCVOID lpvControlHeader,DWORD dwBufferSize,LPVOID lpvBuffer,ULONG uBufferPhysicalAddress,
        DWORD dwClientInfo= 0,LPTRANSFER_NOTIFY_ROUTINE lpStartAddress = NULL,LPVOID lpvNotifyParameter = NULL);
    virtual BOOL IssueBulkTransfer(DWORD dwFlags,DWORD dwBufferSize, LPVOID lpvBuffer,ULONG uBufferPhysicalAddress,
        DWORD dwClientInfo= 0,LPTRANSFER_NOTIFY_ROUTINE lpStartAddress = NULL,LPVOID lpvNotifyParameter = NULL);
    virtual BOOL IssueInterruptTransfer(DWORD dwFlags,DWORD dwBufferSize, LPVOID lpvBuffer,ULONG uBufferPhysicalAddress,
        DWORD dwClientInfo= 0,LPTRANSFER_NOTIFY_ROUTINE lpStartAddress = NULL,LPVOID lpvNotifyParameter = NULL);
    virtual BOOL IssueIsochTransfer(DWORD dwFlags,DWORD dwStartingFrame,DWORD dwFrames,LPCDWORD lpdwLengths,LPVOID lpvBuffer,ULONG uBufferPhysicalAddress,
        DWORD dwClientInfo= 0, LPTRANSFER_NOTIFY_ROUTINE lpStartAddress = NULL,LPVOID lpvNotifyParameter = NULL);
    DWORD GetClientInfo() { return m_dwClientInfo ; };


    // Debug Function
    LPVOID  GetAttachedBuffer() { return m_lpAttachedBuffer; };
    DWORD   GetAttachedSize() { return m_dwAttachedSize; };
    DWORD   GetAttachedPhysicalAddr() { return m_dwAttachedPhysicalAddr; };
    void    OutputTransferInfo(); 
protected:
    void Lock(); 
    void Unlock();
// For debugging purpose.
    DWORD       m_dwDebugZone;
    LONG        m_lTransferID;
    static LONG m_lCurrentID;
    LPVOID      m_lpAttachedBuffer;
    DWORD       m_dwAttachedSize;
    DWORD       m_dwAttachedPhysicalAddr;
// For Transfer;    
    HANDLE      m_hCompleteEvent;
    USB_TRANSFER m_hUsbTransfer;
    UsbClassPipe * const m_lpClassPipe;
    UsbTransfer *   m_pNextTransfer;
    DWORD           m_dwClientInfo;
    LPTRANSFER_NOTIFY_ROUTINE m_lpStartAddress;
    LPVOID                  m_lpvNotifyParameter;
    
private:
    UsbTransfer&operator=(UsbTransfer&){ASSERT(FALSE);}
    static DWORD WINAPI TransferNotifyStub(LPVOID lpvNotifyParameter) {
        ((UsbTransfer*)lpvNotifyParameter)->CompleteNotify();
        return 0;
    }
};

#pragma pack(push, 4)

class UsbClassPipe {
public:
    UsbClassPipe(USB_ENDPOINT usbEndpoint,UsbClientDevice *lpClientDevice);
    virtual ~UsbClassPipe() {;}
    virtual BOOL Init() { return m_hPipeHandle!=NULL; };
    BOOL IsPipeHalted() ;
    DWORD GetPipeType () { return (Descriptor.bmAttributes &  USB_ENDPOINT_TYPE_MASK); };
    BOOL  IsInPipe() { return (USB_ENDPOINT_DIRECTION_IN(Descriptor.bEndpointAddress)!=0); };
    USB_PIPE   GetPipeHandle() { return m_hPipeHandle; };
    UsbClassPipe * GetNextPipe();
    UsbClientDevice * GetClientDevice(){ return m_lpClientDevice; };
    CLockObject * GetTransferLock() { return &m_cTrnasferLock; };
protected:
    UsbClassPipe&operator=(UsbClassPipe&){ASSERT(FALSE);}
    USB_ENDPOINT_DESCRIPTOR Descriptor;
    UsbClientDevice * const m_lpClientDevice ;
    // For Handling Transfer;
    CLockObject m_cTrnasferLock;
    USB_PIPE    m_hPipeHandle;
};

#pragma pack(pop)

// Extention.
class UsbSyncClassPipe: public UsbClassPipe {
public:
    UsbSyncClassPipe(USB_ENDPOINT usbEndpoint,UsbClientDevice *lpClientDevice,DWORD dwZone = 0);
    virtual ~UsbSyncClassPipe();
    virtual BOOL Init();
    virtual BOOL ResetPipe(BOOL fForce = FALSE);
    virtual BOOL BulkOrIntrTransfer(DWORD dwFlags,DWORD dwBufferSize, LPVOID lpvBuffer,ULONG uBufferPhysicalAddress=0,DWORD dwTimeout = INFINITE);
    virtual BOOL VendorTransfer(DWORD dwFlags,LPCVOID lpvControlHeader,DWORD dwBufferSize, LPVOID lpvBuffer,ULONG uBufferPhysicalAddress=0,DWORD dwTimeout = INFINITE);
    virtual BOOL IsochTransfer(DWORD dwFlags,DWORD dwStartingFrame,DWORD dwFrames,LPCDWORD lpdwLengths,LPVOID lpvBuffer,ULONG uBufferPhysicalAddress=0,DWORD dwTimeout = INFINITE);
// Transfer Function.
    virtual BOOL IsTransferEmpty() {
        return (m_lpTransfer!=NULL? m_lpTransfer->IsTransferEmpty(): FALSE);        
    }
    virtual BOOL GetTransferStatus (LPDWORD lpdwBytesTransferred,  LPDWORD lpdwError) {
        return (m_lpTransfer!=NULL? m_lpTransfer->GetTransferStatus (lpdwBytesTransferred,  lpdwError): FALSE);
    }
    BOOL GetIsochResults(  DWORD cFrames, LPDWORD lpdwBytesTransferred, LPDWORD lpdwErrors){
        return (m_lpTransfer!=NULL? m_lpTransfer->GetIsochResults( cFrames, lpdwBytesTransferred, lpdwErrors): FALSE);
    }
    BOOL AbortTransfer(DWORD dwFlags=0) {
        return (m_lpTransfer!=NULL? m_lpTransfer->AbortTransfer(dwFlags): FALSE);
    }
    BOOL CloseTransfer() {
        return (m_lpTransfer!=NULL? m_lpTransfer->CloseTransfer(): FALSE);
    }
    UsbTransfer * GetTransfer () { return m_lpTransfer; };
protected:
    UsbSyncClassPipe&operator=(UsbSyncClassPipe&){ASSERT(FALSE);}
    DWORD   m_dwDebugZone;
    UsbTransfer * m_lpTransfer;    
};
#define NUM_OF_DEFAULT_TRANSFER 2
class UsbAsyncClassPipe : public UsbClassPipe,public CLockObject {
public:
    UsbAsyncClassPipe(USB_ENDPOINT usbEndpoint,UsbClientDevice *lpClientDevice,DWORD dwNumOfTransfer = NUM_OF_DEFAULT_TRANSFER,DWORD dwZone = 0);
    virtual ~UsbAsyncClassPipe();
    virtual BOOL Init();
    virtual BOOL ResetPipe(BOOL fForce = FALSE);
    virtual BOOL ResetTransferQueue();
    virtual BOOL IsEmptyTransferAvailable();
    virtual BOOL CloseAllArmedTransfer();
    virtual BOOL CloseFrontArmedTransfer();
    virtual BOOL IsFrontArmedTransferComplete() ;
    virtual BOOL GetFrontArmedTransferStatus (LPDWORD lpdwBytesTransferred,  LPDWORD lpdwError) ;
    virtual BOOL GetFrontArmedIsochResults(DWORD cFrames, LPDWORD lpdwBytesTransfered, LPDWORD lpdwErrors);
    virtual DWORD GetClientInfo();
    virtual BOOL WaitForTransferComplete(DWORD dwTicks=INFINITE, DWORD nCount = 0 , CONST HANDLE* lpHandles = NULL);
    virtual HANDLE GetCurrentWaitObjectHandle();
    virtual BOOL BulkOrIntrTransfer(DWORD dwFlags,DWORD dwBufferSize, LPVOID lpvBuffer,ULONG uBufferPhysicalAddress=0,DWORD dwClientInfo = 0);
    virtual BOOL VendorTransfer(DWORD dwFlags,LPCVOID lpvControlHeader,DWORD dwBufferSize, LPVOID lpvBuffer,ULONG uBufferPhysicalAddress=0,DWORD dwClientInfo = 0);
    virtual BOOL IsochTransfer(DWORD dwFlags,DWORD dwStartingFrame,DWORD dwFrames,LPCDWORD lpdwLengths,LPVOID lpvBuffer,ULONG uBufferPhysicalAddress=0,DWORD dwClientInfo = 0);
protected:
    UsbAsyncClassPipe&operator=(UsbAsyncClassPipe&){ASSERT(FALSE);}
    DWORD   m_dwDebugZone;
    UsbTransfer * IncTransfer(UsbTransfer * pTransfer) const {
        UsbTransfer * pRetTransfer = NULL;
        if (pTransfer)
            pRetTransfer = pTransfer->GetNextTransfer();
        if (pRetTransfer == NULL)
            pRetTransfer = m_lpTransferList;
        return pRetTransfer;
    }
    UsbTransfer * m_lpTransferList;
    UsbTransfer * m_lpEmptyTranfser;
    UsbTransfer * m_lpArmedTranfser;
    const DWORD   m_dwNumOfTransfer;
    
};

#endif


