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

Module Name:  SerDev.h

Abstract: USB Host Serial Client driver.


Notes: 
--*/
#include <windows.h>
#include <types.h>
#include <ceddk.h>
#include <serhw.h>
#include <Serdbg.h>
#include "CSerPdd.h"
#include <USerDev.h>
const USB_COMM_LINE_CODING cInitUsbCommLineCoding = {9600, USB_COMM_STOPBITS_10, USB_COMM_PARITY_NONE,8};
UsbSerClientDriver::UsbSerClientDriver(LPTSTR lpActivePath, PVOID pMdd, PHWOBJ pHwObj )
:   CSerialPDD(lpActivePath,pMdd,pHwObj)
,   m_UsbCommLineCoding(cInitUsbCommLineCoding)
{
    m_lpUsbClientDevice = NULL;
    m_lpSerialDataIn = NULL;
    m_lpSerialDataOut = NULL;
    m_lpUsbSerDataStatus = NULL;
    m_wSetModemStatus = USB_COMM_DTR|USB_COMM_RTS;
    CRegistryEdit cActiveRegistry(HKEY_LOCAL_MACHINE,lpActivePath);
    DWORD dwType;
    DWORD dwClientInfo;
    DWORD dwLen = sizeof(dwClientInfo);
    
    if (cActiveRegistry.IsKeyOpened() && 
            cActiveRegistry.RegQueryValueEx(DEVLOAD_CLIENTINFO_VALNAME,&dwType,(PBYTE)&dwClientInfo,&dwLen) &&
            dwType == DEVLOAD_CLIENTINFO_VALTYPE && dwLen == sizeof(dwClientInfo)) {
        m_lpUsbClientDevice = (SerialUsbClientDevice *)dwClientInfo;
    }
    else
        ASSERT(FALSE);

    if (!GetRegValue(TXCHUNKSIZE_REG_VAL_NAME,(LPBYTE)&m_dwTxChunkSize,sizeof(DWORD))) {
        m_dwTxChunkSize = DEFAULT_TX_CHUNK_SIZE;
    }
}
BOOL UsbSerClientDriver::Init()
{
    DEBUGMSG(ZONE_INIT,(TEXT("+UsbSerClientDriver::Init")));
    if (m_lpUsbClientDevice && CSerialPDD::Init()) {
        LPCUSB_INTERFACE lpTargetInterface = m_lpUsbClientDevice->GetTargetInterface();
        // BulkIn, BulkOut, Option Interrupt.
        if (lpTargetInterface) {
            DWORD dwEndpoints = lpTargetInterface->Descriptor.bNumEndpoints;
            LPCUSB_ENDPOINT lpEndpoint = lpTargetInterface->lpEndpoints ;
            while (dwEndpoints && lpEndpoint!=NULL) {
                if ((lpEndpoint->Descriptor.bmAttributes &  USB_ENDPOINT_TYPE_MASK) == USB_ENDPOINT_TYPE_BULK) {
                    if (USB_ENDPOINT_DIRECTION_IN(lpEndpoint->Descriptor.bEndpointAddress)) {
                        CreateBulkIn(lpEndpoint);
                    }
                    else {
                        CreateBulkOut(lpEndpoint);
                    }
                }
                else
                if ((lpEndpoint->Descriptor.bmAttributes &  USB_ENDPOINT_TYPE_MASK) == USB_ENDPOINT_TYPE_INTERRUPT &&
                        USB_ENDPOINT_DIRECTION_IN(lpEndpoint->Descriptor.bEndpointAddress)) {
                    CreateStatusIn(lpEndpoint);
                }
                lpEndpoint ++;
                dwEndpoints --;
            }
        }
        DEBUGMSG(ZONE_INIT,(TEXT("-UsbSerClientDriver::Init(m_lpSerialDataIn =%x, m_lpSerialDataOut =%x ,m_lpUsbSerDataStatus=%x )"),
            m_lpSerialDataIn,m_lpSerialDataOut,m_lpUsbSerDataStatus));
        ASSERT(m_lpSerialDataIn!=NULL && m_lpSerialDataOut!=NULL);
        return(m_lpSerialDataIn!=NULL && m_lpSerialDataOut!=NULL); 
    }
    return FALSE;
}
BOOL UsbSerClientDriver::CreateBulkIn(LPCUSB_ENDPOINT lpEndpoint)
{
    if (m_lpSerialDataIn ==NULL) {
        m_lpSerialDataIn = new SerialDataIn(*lpEndpoint,m_lpUsbClientDevice,DataInStub,this);
        if (m_lpSerialDataIn && !m_lpSerialDataIn->Init()) {
            delete m_lpSerialDataIn;
            m_lpSerialDataIn = NULL;
        }
    }
    return (m_lpSerialDataIn !=NULL) ;
}
BOOL UsbSerClientDriver::CreateBulkOut(LPCUSB_ENDPOINT lpEndpoint)
{
    if (m_lpSerialDataOut == NULL) {
        m_lpSerialDataOut = new SerialDataOut(*lpEndpoint,m_lpUsbClientDevice,DataOutStub,this);
        if (m_lpSerialDataOut && !m_lpSerialDataOut->Init(m_dwTxChunkSize)) {
            delete m_lpSerialDataOut;
            m_lpSerialDataOut = NULL;
        }
    }
    return  (m_lpSerialDataOut != NULL);
}
BOOL UsbSerClientDriver::CreateStatusIn(LPCUSB_ENDPOINT lpEndpoint)
{
    m_wSetModemStatus = 0;
    if (m_lpUsbSerDataStatus == NULL) {
        m_lpUsbSerDataStatus = new UsbSerDataStatus(*lpEndpoint,m_lpUsbClientDevice,StatusStub, this) ;
        if (m_lpUsbSerDataStatus!=NULL && !m_lpUsbSerDataStatus->Init()) { // Fail Init
            delete m_lpUsbSerDataStatus;
            m_lpUsbSerDataStatus = NULL;
        }
    }
    return (m_lpUsbSerDataStatus != NULL);
}
UsbSerClientDriver::~UsbSerClientDriver()
{
    if ( m_lpSerialDataIn != NULL)
        delete m_lpSerialDataIn;
    if (m_lpSerialDataOut != NULL)
        delete m_lpSerialDataOut;
    if (m_lpUsbSerDataStatus != NULL)
        delete m_lpUsbSerDataStatus;
    m_lpUsbClientDevice = NULL;
}
void  UsbSerClientDriver::PreDeinit()
{
    if ( m_lpSerialDataIn != NULL) {
        delete m_lpSerialDataIn;
        m_lpSerialDataIn = NULL;
    }
    if (m_lpSerialDataOut != NULL) {
        delete m_lpSerialDataOut;
        m_lpSerialDataOut = NULL; 
    }
    if (m_lpUsbSerDataStatus != NULL) {
        delete m_lpUsbSerDataStatus;
        m_lpUsbSerDataStatus = NULL;
    }
    m_lpUsbClientDevice = NULL;
}
// Xmit.
void    UsbSerClientDriver::XmitInterruptHandler(PUCHAR pTxBuffer, ULONG *pBuffLen)
{
    if (pTxBuffer && pBuffLen && *pBuffLen) { // If there is something for transmit.
        if ((m_DCB.fOutxCtsFlow && IsCTSOff()) ||(m_DCB.fOutxDsrFlow && IsDSROff())) { // We are in flow off
            DEBUGMSG(ZONE_THREAD|ZONE_WRITE,(TEXT("CPdd16550::XmitInterruptHandler! Flow Off, Data Discard.\r\n")));
            *pBuffLen = 0;
        }
    }
    if (m_lpSerialDataOut)
        m_lpSerialDataOut->XmitInterruptHandler(pTxBuffer,pBuffLen);
}
//
//  Modem
ULONG   UsbSerClientDriver::GetModemStatus() 
{
    ULONG ulModemStatus= MS_CTS_ON|MS_DSR_ON|MS_RING_ON|MS_RLSD_ON;
    if (m_lpUsbSerDataStatus) {
        ULONG ulEvent = 0;
        ulModemStatus = m_lpUsbSerDataStatus->GetModemStatus(ulEvent);
        if (ulEvent!=0)
            EventCallback(ulEvent,ulModemStatus);
    }
    return ulModemStatus;
};
void UsbSerClientDriver::SetBreak(BOOL /*bSet*/)
{
    USB_DEVICE_REQUEST usbRequest = {
        USB_REQUEST_CLASS | USB_REQUEST_FOR_INTERFACE | USB_REQUEST_HOST_TO_DEVICE,
        USB_COMM_SEND_BREAK,
        100,
        0,
        0
    };
    PREFAST_ASSERT(m_lpUsbClientDevice!=NULL)
    VERIFY(m_lpUsbClientDevice->IssueVendorTransfer(0,&usbRequest,NULL, 0 ));
    m_lpUsbClientDevice->CloseTransfer();
}

void    UsbSerClientDriver::SetDTR(BOOL bSet)
{
    m_HardwareLock.Lock();    
    WORD wOldeModemStatus = m_wSetModemStatus;
    if (bSet)
        m_wSetModemStatus |= USB_COMM_DTR;
    else
        m_wSetModemStatus &= ~USB_COMM_DTR;
    m_HardwareLock.Unlock();    
    if (wOldeModemStatus != m_wSetModemStatus) {
        USB_DEVICE_REQUEST usbRequest = {
            USB_REQUEST_CLASS | USB_REQUEST_FOR_INTERFACE | USB_REQUEST_HOST_TO_DEVICE,
            USB_COMM_SET_CONTROL_LINE_STATE,
            m_wSetModemStatus,
            0,
            0
        };
        PREFAST_ASSERT(m_lpUsbClientDevice!=NULL)
        VERIFY(m_lpUsbClientDevice->IssueVendorTransfer(0,&usbRequest,NULL, 0 ));
        m_lpUsbClientDevice->CloseTransfer();
    }
    
}
void    UsbSerClientDriver::SetRTS(BOOL bSet)
{
    m_HardwareLock.Lock();    
    WORD wOldeModemStatus = m_wSetModemStatus;
    if (bSet)
        m_wSetModemStatus |= USB_COMM_RTS;
    else
        m_wSetModemStatus &= ~USB_COMM_RTS;

    m_HardwareLock.Unlock();    
    if (wOldeModemStatus != m_wSetModemStatus) {
        USB_DEVICE_REQUEST usbRequest = {
            USB_REQUEST_CLASS | USB_REQUEST_FOR_INTERFACE | USB_REQUEST_HOST_TO_DEVICE,
            USB_COMM_SET_CONTROL_LINE_STATE,
            m_wSetModemStatus,
            0,
            0
        };
        PREFAST_ASSERT(m_lpUsbClientDevice!=NULL)
        VERIFY(m_lpUsbClientDevice->IssueVendorTransfer(0,&usbRequest,NULL, 0 ));
        m_lpUsbClientDevice->CloseTransfer();
    }
}
BOOL     UsbSerClientDriver::SetBaudRate(ULONG BaudRate,BOOL /*bIrModule*/)
{   
    m_HardwareLock.Lock();  
    m_UsbCommLineCoding.DTERate = BaudRate;
    m_DCB.BaudRate = BaudRate;
    USB_DEVICE_REQUEST usbRequest = {
        USB_REQUEST_CLASS | USB_REQUEST_FOR_INTERFACE | USB_REQUEST_HOST_TO_DEVICE,
        USB_COMM_SET_LINE_CODING,
        0,
        0,
        sizeof(m_UsbCommLineCoding)
    };
    USB_COMM_LINE_CODING UsbCommLineCoding = m_UsbCommLineCoding ;
    m_HardwareLock.Unlock();    
    BOOL bReturn = m_lpUsbClientDevice->IssueVendorTransfer(0,&usbRequest,&UsbCommLineCoding, 0  );
    ASSERT(bReturn==TRUE);
    m_lpUsbClientDevice->CloseTransfer();
    return bReturn;
}
BOOL    UsbSerClientDriver::SetByteSize(ULONG ByteSize)
{
    m_HardwareLock.Lock();  
    m_UsbCommLineCoding.DataBits = (BYTE)ByteSize;
    m_DCB.ByteSize = (BYTE)ByteSize;
    USB_DEVICE_REQUEST usbRequest = {
        USB_REQUEST_CLASS | USB_REQUEST_FOR_INTERFACE | USB_REQUEST_HOST_TO_DEVICE,
        USB_COMM_SET_LINE_CODING,
        0,
        0,
        sizeof(m_UsbCommLineCoding)
    };
    USB_COMM_LINE_CODING UsbCommLineCoding = m_UsbCommLineCoding ;
    m_HardwareLock.Unlock();    
    BOOL bReturn = m_lpUsbClientDevice->IssueVendorTransfer(0,&usbRequest,&UsbCommLineCoding, 0 );
    ASSERT(bReturn==TRUE);
    m_lpUsbClientDevice->CloseTransfer();
    return bReturn;
}
BOOL    UsbSerClientDriver::SetParity(ULONG Parity)
{
    m_HardwareLock.Lock();  
    m_UsbCommLineCoding.ParityType = (BYTE)Parity;
    m_DCB.Parity = (BYTE)Parity;
    USB_DEVICE_REQUEST usbRequest = {
        USB_REQUEST_CLASS | USB_REQUEST_FOR_INTERFACE | USB_REQUEST_HOST_TO_DEVICE,
        USB_COMM_SET_LINE_CODING,
        0,
        0,
        sizeof(m_UsbCommLineCoding)
    };
    USB_COMM_LINE_CODING UsbCommLineCoding = m_UsbCommLineCoding ;
    m_HardwareLock.Unlock();    
    BOOL bReturn = m_lpUsbClientDevice->IssueVendorTransfer(0,&usbRequest,&UsbCommLineCoding, 0 );
    m_lpUsbClientDevice->CloseTransfer();
    ASSERT(bReturn==TRUE);
    return bReturn;
}
BOOL    UsbSerClientDriver::SetStopBits(ULONG StopBits) 
{
    m_HardwareLock.Lock();  
    m_UsbCommLineCoding.CharFormat = (BYTE)StopBits;
    m_DCB.StopBits = (BYTE)StopBits;
    USB_DEVICE_REQUEST usbRequest = {
        USB_REQUEST_CLASS | USB_REQUEST_FOR_INTERFACE | USB_REQUEST_HOST_TO_DEVICE,
        USB_COMM_SET_LINE_CODING,
        0,
        0,
        sizeof(m_UsbCommLineCoding)
    };
    USB_COMM_LINE_CODING UsbCommLineCoding = m_UsbCommLineCoding ;
    m_HardwareLock.Unlock();    
    BOOL bReturn = m_lpUsbClientDevice->IssueVendorTransfer(0,&usbRequest,&UsbCommLineCoding, 0 );
    m_lpUsbClientDevice->CloseTransfer();
    ASSERT(bReturn==TRUE);
    return bReturn;
}
UsbActiveSyncSerialClientDriver::UsbActiveSyncSerialClientDriver(LPTSTR lpActivePath, PVOID pMdd, PHWOBJ pHwObj )
:   UsbSerClientDriver(lpActivePath,pMdd, pHwObj )
{
    m_lpSerialDataStatus = NULL;
}
BOOL UsbActiveSyncSerialClientDriver::CreateStatusIn(LPCUSB_ENDPOINT lpEndpoint)
{
    m_wSetModemStatus = 0;
    DEBUGMSG(ZONE_INIT, (TEXT("+UsbActiveSyncSerialClientDriver::CreateStatusIn lpEndpoint = %x "),lpEndpoint));
    if (m_lpSerialDataStatus == NULL) {
        m_lpSerialDataStatus = new SerialDataStatus(*lpEndpoint,m_lpUsbClientDevice,StatusStub, this) ;
        if (m_lpSerialDataStatus!=NULL && !m_lpSerialDataStatus->Init()) { // Fail Init
            delete m_lpSerialDataStatus;
            m_lpSerialDataStatus = NULL;
        }
    }
    DEBUGMSG(ZONE_INIT, (TEXT("-UsbActiveSyncSerialClientDriver::CreateStatusIn m_lpSerialDataStatus = %x "),m_lpSerialDataStatus));
    return (m_lpSerialDataStatus != NULL);
}

ULONG   UsbActiveSyncSerialClientDriver::GetModemStatus()
{
    ULONG ulModemStatus= MS_CTS_ON|MS_DSR_ON|MS_RING_ON|MS_RLSD_ON;
    if (m_lpSerialDataStatus) {
        ULONG ulEvent = 0;
        ulModemStatus = m_lpSerialDataStatus->GetModemStatus(ulEvent);
        if (ulEvent!=0)
            EventCallback(ulEvent,ulModemStatus);
    }
    return ulModemStatus;
    
}
void    UsbActiveSyncSerialClientDriver::SetDTR(BOOL bSet)
{
    m_HardwareLock.Lock();  
    WORD wOldeModemStatus = m_wSetModemStatus;
    if (bSet)
        m_wSetModemStatus |= USB_COMM_DTR;
    else
        m_wSetModemStatus &= ~USB_COMM_DTR;
    m_HardwareLock.Unlock();    
    if (wOldeModemStatus!=m_wSetModemStatus) {
        USB_DEVICE_REQUEST usbRequest = {
            USB_REQUEST_CLASS | USB_REQUEST_FOR_INTERFACE | USB_REQUEST_HOST_TO_DEVICE,
            SET_CONTROL_LINE_STATE,
            m_wSetModemStatus,
            0,
            0
        };
        PREFAST_ASSERT(m_lpUsbClientDevice!=NULL)
        VERIFY(m_lpUsbClientDevice->IssueVendorTransfer(0,&usbRequest,NULL, 0 ));
        m_lpUsbClientDevice->CloseTransfer();
    }
}
void    UsbActiveSyncSerialClientDriver::SetRTS(BOOL bSet)
{
    m_HardwareLock.Lock();  
    WORD wOldeModemStatus = m_wSetModemStatus;
    if (bSet)
        m_wSetModemStatus |= USB_COMM_RTS;
    else
        m_wSetModemStatus &= ~USB_COMM_RTS;
    m_HardwareLock.Unlock();    
    if (wOldeModemStatus!=m_wSetModemStatus) {
        USB_DEVICE_REQUEST usbRequest = {
            USB_REQUEST_CLASS | USB_REQUEST_FOR_INTERFACE | USB_REQUEST_HOST_TO_DEVICE,
            SET_CONTROL_LINE_STATE,
            m_wSetModemStatus,
            0,
            0
        };
        PREFAST_ASSERT(m_lpUsbClientDevice!=NULL)
        VERIFY(m_lpUsbClientDevice->IssueVendorTransfer(0,&usbRequest,NULL, 0 ));
        m_lpUsbClientDevice->CloseTransfer();
    }
}

CSerialPDD * CreateSerialObject(LPTSTR lpActivePath, PVOID pMdd,PHWOBJ pHwObj, DWORD DeviceArrayIndex)
{
    DEBUGMSG(ZONE_INIT,(TEXT("+CreateSerialObject, DeviceArrayIndex=%d"),DeviceArrayIndex));
    CSerialPDD * pSerialPDD = NULL;
    if (DeviceArrayIndex == 0 ) {
        pSerialPDD = new UsbActiveSyncSerialClientDriver(lpActivePath,pMdd, pHwObj);
        if (pSerialPDD && !pSerialPDD->Init()) {
            delete pSerialPDD;
            pSerialPDD = NULL;
         }
    }
    else if (DeviceArrayIndex == 1) {
        pSerialPDD= new UsbSerClientDriver(lpActivePath,pMdd, pHwObj);
        if (pSerialPDD && !pSerialPDD->Init()) {
            delete pSerialPDD;
            pSerialPDD = NULL;
        }
    }
    DEBUGMSG(ZONE_INIT,(TEXT("-CreateSerialObject, pSerialPDD=%x"),pSerialPDD));
    return pSerialPDD;
}
void DeleteSerialObject(CSerialPDD * pSerialPDD)
{
    if (pSerialPDD)
        delete pSerialPDD;
}


