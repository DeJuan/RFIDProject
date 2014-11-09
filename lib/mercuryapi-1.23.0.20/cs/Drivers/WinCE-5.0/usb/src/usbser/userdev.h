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

#ifndef __USERDEV_H_
#define __USERDEV_H_
#include <UsbSer.h>
#include <Cserpdd.h>
class UsbSerClientDriver;
// Control Line State - sent to device on default control pipe
#define USB_COMM_DTR    0x0001
#define USB_COMM_RTS    0x0002    

// Abstract Control Model defines
#define USB_COMM_SEND_ENCAPSULATED_COMMAND      0x0000
#define USB_COMM_GET_ENCAPSULATED_RESPONSE      0x0001
#define USB_COMM_SET_COMM_FEATURE               0x0002
#define USB_COMM_GET_COMM_FEATURE               0x0003
#define USB_COMM_CLEAR_COMM_FEATURE             0x0004
#define USB_COMM_SET_LINE_CODING                0x0020
#define USB_COMM_GET_LINE_CODING                0x0021
#define USB_COMM_SET_CONTROL_LINE_STATE         0x0022
#define USB_COMM_SEND_BREAK                     0x0023

// Line Coding Stop Bits
#define USB_COMM_STOPBITS_10                    0x0000
#define USB_COMM_STOPBITS_15                    0x0001
#define USB_COMM_STOPBITS_20                    0x0002

// Line Coding Parity Type
#define USB_COMM_PARITY_NONE                    0x0000
#define USB_COMM_PARITY_ODD                     0x0001
#define USB_COMM_PARITY_EVEN                    0x0002
#define USB_COMM_PARITY_MARK                    0x0003
#define USB_COMM_PARITY_SPACE                   0x0004

typedef struct _USB_COMM_LINE_CODING
{
    ULONG       DTERate;
    UCHAR       CharFormat;
    UCHAR       ParityType;
    UCHAR       DataBits;
} USB_COMM_LINE_CODING, *PUSB_COMM_LINE_CODING;


class UsbSerClientDriver: public CSerialPDD {
public:
    UsbSerClientDriver(LPTSTR lpActivePath, PVOID pMdd, PHWOBJ pHwObj );
    ~UsbSerClientDriver();
// For serial driver we have to do this.
    virtual BOOL Init();
    virtual void  PreDeinit();
protected:
    virtual BOOL CreateBulkIn(LPCUSB_ENDPOINT lpEndpoint);
    virtual BOOL CreateBulkOut(LPCUSB_ENDPOINT lpEndpoint);
    virtual BOOL CreateStatusIn(LPCUSB_ENDPOINT lpEndpoint);
public:
    virtual void    SerialRegisterBackup() {;};
    virtual void    SerialRegisterRestore() {;};
//  Tx Function.
    virtual BOOL    InitXmit(BOOL bInit) {
        if (m_lpSerialDataOut)
            return m_lpSerialDataOut->InitXmit(bInit);
        else
            return FALSE;
    }
    virtual void    XmitInterruptHandler(PUCHAR pTxBuffer, ULONG *pBuffLen) ;
    virtual void    XmitComChar(UCHAR ComChar)  {
        if (m_lpSerialDataOut)
            m_lpSerialDataOut->XmitComChar(ComChar);
    }
    virtual BOOL    EnableXmitInterrupt(BOOL /*bEnable*/) { return TRUE; };
    virtual BOOL    CancelXmit()  {
        if (m_lpSerialDataOut)
            m_lpSerialDataOut->CancelXmit();
        return TRUE;
    }
//
//  Rx Function.
    virtual BOOL    InitReceive(BOOL bInit) {
        if (m_lpSerialDataIn)
            return m_lpSerialDataIn->InitReceive(bInit);
        else
            return FALSE;
    }
    virtual ULONG   ReceiveInterruptHandler(PUCHAR pRxBuffer,ULONG *pBufflen) {
        DWORD dwReturn = 0;
        if (!(m_lpSerialDataIn && m_lpSerialDataIn->ReceiveInterruptHandler(pRxBuffer,pBufflen))) {
            if (pBufflen)
                dwReturn = *pBufflen;
        }
        return dwReturn;
    }
    virtual ULONG   CancelReceive() {
        if (m_lpSerialDataIn)
            m_lpSerialDataIn->CancelReceive();
        return 0;
    }
//
//  Modem
    virtual BOOL    InitModem(BOOL bInit)  {
        if (m_lpUsbSerDataStatus)
            return m_lpUsbSerDataStatus->InitModem(bInit);
        return FALSE;
    }
    virtual void    ModemInterruptHandler() { GetModemStatus();}
    virtual ULONG   GetModemStatus() ;
    virtual void    SetDTR(BOOL bSet);
    virtual void    SetRTS(BOOL bSet);
//  Line Function
    virtual BOOL    InitLine(BOOL /*bInit*/) { return TRUE; };
    virtual void    LineInterruptHandler() { };
    virtual void    SetBreak(BOOL bSet);
    virtual BOOL    SetBaudRate(ULONG BaudRate,BOOL bIrModule);
    virtual BOOL    SetByteSize(ULONG ByteSize) ;
    virtual BOOL    SetParity(ULONG Parity) ;
    virtual BOOL    SetStopBits(ULONG StopBits) ;
protected:
    virtual DWORD DataInNotify() {
        return NotifyPDDInterrupt(INTR_RX);
    }
    virtual DWORD DataOutNotify() {
        return NotifyPDDInterrupt(INTR_TX);
    }
    virtual DWORD StatusNotify() {
        return NotifyPDDInterrupt(INTR_MODEM);
    }
    SerialUsbClientDevice *   m_lpUsbClientDevice;
    SerialDataIn  *     m_lpSerialDataIn;
    SerialDataOut *     m_lpSerialDataOut;
    WORD m_wSetModemStatus;
    USB_COMM_LINE_CODING m_UsbCommLineCoding;
private:
    UsbSerDataStatus *  m_lpUsbSerDataStatus;
    BOOL GetLineCodeing;
    BOOL SetModemStatus;
    DWORD m_dwTxChunkSize;
    static DWORD WINAPI DataInStub(LPVOID lpvNotifyParameter) {
        return ((UsbSerClientDriver *)lpvNotifyParameter)->DataInNotify();
    }
    static DWORD WINAPI DataOutStub(LPVOID lpvNotifyParameter) {
        return ((UsbSerClientDriver *)lpvNotifyParameter)->DataOutNotify();
    }
    static DWORD WINAPI StatusStub(LPVOID lpvNotifyParameter) {
        return ((UsbSerClientDriver *)lpvNotifyParameter)->StatusNotify();
    }
    UsbSerClientDriver&operator=(UsbSerClientDriver&){ASSERT(FALSE);}
};


#define SET_CONTROL_LINE_STATE  0x22

class UsbActiveSyncSerialClientDriver : public UsbSerClientDriver {
public:
    UsbActiveSyncSerialClientDriver(LPTSTR lpActivePath, PVOID pMdd, PHWOBJ pHwObj );
    virtual BOOL    InitModem(BOOL bInit)  {
        if (m_lpSerialDataStatus)
            return m_lpSerialDataStatus->InitModem(bInit);
        return FALSE;
    }
    virtual void    ModemInterruptHandler() { GetModemStatus();}
    virtual ULONG   GetModemStatus() ;
    virtual void    SetDTR(BOOL bSet);
    virtual void    SetRTS(BOOL bSet);
//  Line Function
    virtual BOOL    InitLine(BOOL /*bInit*/) { return TRUE; };
    virtual void    LineInterruptHandler() { };
    virtual void    SetBreak(BOOL /*bSet*/) { ;};
    virtual BOOL    SetBaudRate(ULONG /*BaudRate*/,BOOL /*bIrModule*/) { return TRUE; };
    virtual BOOL    SetByteSize(ULONG /*ByteSize*/) { return TRUE; };
    virtual BOOL    SetParity(ULONG /*Parity*/) { return TRUE; };
    virtual BOOL    SetStopBits(ULONG /*StopBits*/) { return TRUE; };
protected:
    virtual BOOL CreateStatusIn(LPCUSB_ENDPOINT lpEndpoint);

private:
    SerialDataStatus *  m_lpSerialDataStatus;
    static DWORD WINAPI StatusStub(LPVOID lpvNotifyParameter) {
        return ((UsbActiveSyncSerialClientDriver *)lpvNotifyParameter)->StatusNotify();
    }
    UsbActiveSyncSerialClientDriver&operator=(UsbActiveSyncSerialClientDriver&){ASSERT(FALSE);}
};

#endif

