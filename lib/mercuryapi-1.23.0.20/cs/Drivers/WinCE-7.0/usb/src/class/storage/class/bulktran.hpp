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

        BUlTran.hpp

Abstract:

        Bulk Pipe Transfer Manager

--*/


#pragma once

#include <CReg.hxx>
#include <CMThread.h>
#include <CSync.h>

#define MAX_OUTSTANDING_TRANSFER 0x10
#define USBMASS_REG_NUMOF_TRANFER TEXT("NumOfOutstandingTransfer")
#define DEFAULT_NUM_OF_TRANFER 4
#define USBMASS_REG_DISABLE_BUFFERING TEXT("DisableBuffering")
#define USBMASS_REG_PACKETS_TRANFER TEXT("PacketsPerTransfer")
#define DEFAULT_NUM_OF_PACKET_PER_TRANFER 0x20


class BulkTransfer : protected CReg, protected CLockObject {
public:
    BulkTransfer(HKEY DevKey, USB_FUNCS& UsbFuncs, HANDLE hPipe,USB_ENDPOINT_DESCRIPTOR& USBEndpoint);
    ~BulkTransfer();
    BOOL    Init();
    BOOL    IssueTransfer(LPTRANSFER_NOTIFY_ROUTINE NotifyRoutine,PVOID NotifyContext,DWORD Flags,
        LPVOID pBuffer,DWORD BufferLength);
    BOOL    WaitForTransfer(DWORD dwTicks);
    BOOL    CloseTransfer();
    BOOL    IsTransferCompleted() { return m_fTransferComplete; };
    BOOL    GetTransferStatus(LPDWORD lpByteTransferred , LPDWORD lpdErrors);
private:
    USB_FUNCS       m_UsbFuncs;
    USB_PIPE        m_hPipe;
    USB_ENDPOINT_DESCRIPTOR    m_EndpointDesc;
    DWORD           m_dwDisableBuffering;
    BOOL            m_fTransferComplete;
    
    LPTRANSFER_NOTIFY_ROUTINE m_lpNotification;
    PVOID               m_NotifyContext;
    DWORD               m_Flags;
    PBYTE               m_pArgUserBufferPtr;
    DWORD               m_dwArgUserBufferSize;
    
    DWORD               m_dwLastError;
    DWORD               m_dwTransferedByte;
    
    PBYTE               m_pCurUserBufferPtr;
    DWORD               m_dwCurUserBufferSize;
    DWORD               m_dwCurWaitIndex;

    DWORD               m_numOfTransfer;
    USB_TRANSFER        m_hUsbTransfer[MAX_OUTSTANDING_TRANSFER];
    HANDLE              m_hUsbEvent[MAX_OUTSTANDING_TRANSFER];
    PBYTE               m_pUserBuffer[MAX_OUTSTANDING_TRANSFER];
    DWORD               m_dwUserBufferSize[MAX_OUTSTANDING_TRANSFER];

    DWORD               m_physPageSize;
    DWORD               m_dwTransferPhysAddr[MAX_OUTSTANDING_TRANSFER];
    PBYTE               m_pTransferUnCached[MAX_OUTSTANDING_TRANSFER];
    PBYTE               m_pTransferCached[MAX_OUTSTANDING_TRANSFER];
    
    void CompleteNotification(DWORD dwError);
    void CancelTransfer();
    static DWORD TransferCallback( LPVOID lpvNotifyParam);

    BOOL        SendAnotherTransfer(DWORD dwIndex);
    // Callback
};

