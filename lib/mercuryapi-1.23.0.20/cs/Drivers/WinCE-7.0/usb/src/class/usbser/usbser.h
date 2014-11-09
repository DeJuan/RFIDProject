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
    usbser.h

Abstract:
    USB Client Driver for serial (active sync and modem) Class.

Functions:

Notes:

--*/
#include <UsbInstl.h>
#include <CMthread.h>
#include <usbtypes.h>
#include <UsbServ.h>
#include <CeDDK.h>
#include <Serdbg.h>

#ifndef __USBSER_H_
#define __USBSER_H_

class SerialUsbClientDevice : public UsbClientDevice {
public:
    SerialUsbClientDevice(USB_HANDLE hUsb, LPCUSB_FUNCS UsbFuncsPtr,LPCUSB_INTERFACE lpInputInterface, LPCWSTR szUniqueDriverId,LPCUSB_DRIVER_SETTINGS lpDriverSettings) 
    :   UsbClientDevice(hUsb, UsbFuncsPtr,lpInputInterface, szUniqueDriverId,lpDriverSettings,ZONE_INIT) 
    {
        m_lpTargetInterface = NULL;
    };
    virtual BOOL IsThisInterfaceSupported(LPCUSB_INTERFACE lpUsbInterface) ;
    virtual BOOL IsClientDriverSatisfied()  { return m_lpTargetInterface!=NULL; };
    LPCUSB_INTERFACE GetTargetInterface () { return m_lpTargetInterface; };
private:
    LPCUSB_INTERFACE m_lpTargetInterface;
};


// Function Implementation.
#define NUM_OF_IN_TRANSFER 4
#define SERIAL_DATAIN_COOKIE 0xfedd0000
#define SERIAL_DATAIN_COOKIE_MASK 0xffff0000

class SerialDataIn : UsbAsyncClassPipe,public CMiniThread  {
public:
    SerialDataIn(USB_ENDPOINT usbEndpoint,UsbClientDevice *lpClientDevice,LPTHREAD_START_ROUTINE lpCallbackAddr, LPVOID lpvCallbackParam) ;
    ~SerialDataIn();
    BOOL Init();
    BOOL InitReceive(BOOL bInit);
    BOOL ReceiveInterruptHandler(__out_bcount(*pBufflen) PUCHAR pRxBuffer,ULONG *pBufflen) ;
    ULONG   CancelReceive();
protected:
    // Callback.
    LPTHREAD_START_ROUTINE  m_lpCallbackAddr;
    LPVOID                  m_lpvCallbackParam;
    HANDLE                  m_hThreadRun;
    // Buffer For this transfer.
    static DMA_ADAPTER_OBJECT m_Adapter;
    PBYTE m_VirtualAddress;
    PHYSICAL_ADDRESS m_PhysicalAddress;
    DWORD m_dwSegDataOffset[NUM_OF_IN_TRANSFER];
    DWORD m_dwSegDataDataLen[NUM_OF_IN_TRANSFER];
    DWORD m_dwNumOfSegment;
    DWORD m_dwSegmentSize;
    DWORD m_dwTotalSize;
    DWORD m_dwCurIndex;  
private:
    DWORD       ThreadRun() ;
    
};
// Abstract Control Model Notification defines
#define USB_COMM_NETWORK_CONNECTION             0x0000
#define USB_COMM_RESPONSE_AVAILABLE             0x0001
#define USB_COMM_SERIAL_STATE                   0x0020

// Serial State Notification bits
#define USB_COMM_DCD                            0x0001
#define USB_COMM_DSR                            0x0002
#define USB_COMM_BREAK                          0x0004
#define USB_COMM_RING                           0x0008
#define USB_COMM_FRAMING_ERROR                  0x0010
#define USB_COMM_PARITY_ERROR                   0x0020
#define USB_COMM_OVERRUN                        0x0040
typedef struct _USB_COMM_SERIAL_STATUS
{
    UCHAR       RequestType;
    UCHAR       Notification;
    USHORT      Value;
    USHORT      Index;
    USHORT      Length;
    USHORT      SerialState;
} USB_COMM_SERIAL_STATUS, *PUSB_COMM_SERIAL_STATUS;

#define NUM_OF_OUT_TRANSFER     2

#define TXCHUNKSIZE_REG_VAL_NAME TEXT("TxTransferChunkSize")

// Default size of data chunk that will be transferred in one IssueTransfer call for TX.
// This should be less than the maximum physical memory that host controller driver 
// sets aside for a transfer, which is typically 64K or above
//
#define DEFAULT_TX_CHUNK_SIZE   16384

class SerialDataOut: UsbAsyncClassPipe,public CMiniThread  {
public:
    SerialDataOut(USB_ENDPOINT usbEndpoint,UsbClientDevice *lpClientDevice,LPTHREAD_START_ROUTINE lpCallbackAddr, LPVOID lpvCallbackParam);
    ~SerialDataOut();
    BOOL Init(DWORD dwTxChunkSize);
    BOOL    InitXmit(BOOL bInit);
    void    XmitInterruptHandler(PUCHAR pTxBuffer, ULONG *pBuffLen); 
    void    XmitComChar(UCHAR ComChar) ;
    BOOL    CancelXmit() ;
protected:
    LPTHREAD_START_ROUTINE  m_lpCallbackAddr;
    LPVOID                  m_lpvCallbackParam;
    HANDLE                  m_hThreadRun;
private:
    BOOL    m_fZeroLengthNeeded;
    DWORD   m_dwTxChunkSize;
    virtual DWORD  ThreadRun() ;
};
//D2      DSR state  (1=Active, 0=Inactive)
//D1      CTS state  (1=Active, 0=Inactive)
//D0      Data Available  - (1=Host should read IN endpoint, 0=No data currently available)
#define USBFN_SERIAL_DSR_SET 0x4
#define USBFN_SERIAL_CTS_SET 0x2
#define USBFN_SERIAL_DATA_AVAILABLE 0x1

class UsbSerDataStatus :public UsbSyncClassPipe, public CMiniThread, public CLockObject {
public:
    UsbSerDataStatus(USB_ENDPOINT usbEndpoint,UsbClientDevice *lpClientDevice,LPTHREAD_START_ROUTINE lpCallbackAddr, LPVOID lpvCallbackParam) ;
    ~UsbSerDataStatus();
    BOOL Init() { return m_hThreadRun!=NULL && UsbSyncClassPipe::Init(); };
    BOOL InitModem(BOOL bInit) ;
    ULONG GetModemStatus(ULONG& ulEvent) ;
protected:
    LPTHREAD_START_ROUTINE  m_lpCallbackAddr;
    LPVOID                  m_lpvCallbackParam;
    HANDLE                  m_hThreadRun;
    DWORD   m_dwPrevModemData;
    DWORD   m_dwCurModemData;
private:
    virtual DWORD  ThreadRun();
};

class SerialDataStatus : public UsbSerDataStatus {
public:
    SerialDataStatus(USB_ENDPOINT usbEndpoint,UsbClientDevice *lpClientDevice,LPTHREAD_START_ROUTINE lpCallbackAddr, LPVOID lpvCallbackParam) 
    :   UsbSerDataStatus(usbEndpoint,lpClientDevice,lpCallbackAddr, lpvCallbackParam) 
        {;};
    ULONG GetModemStatus(ULONG& ulEvent) ;
private:
    virtual DWORD  ThreadRun();
};

#endif
// EOF
