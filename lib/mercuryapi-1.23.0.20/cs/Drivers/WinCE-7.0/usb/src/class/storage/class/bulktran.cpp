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

        BulkTran.Cpp

Abstract:

        Bulk Transfer Handler

--*/

#include <windows.h>
#include <ceddk.h>
#include <usbdi.h>
#include <usb100.h>

#include "BulkTran.hpp"

extern "C" PVOID CreateBulkTransferMgr(
    LPCUSB_FUNCS  lpUsbFuncs, USB_PIPE hPipe, LPCUSB_ENDPOINT_DESCRIPTOR lpEndpointDesc,LPCTSTR szUniqueDriverId
    )
{
    BulkTransfer * pBulkTransfer = NULL;
    HKEY DevKey = NULL;
    if (lpUsbFuncs && lpEndpointDesc && szUniqueDriverId && 
            (DevKey = lpUsbFuncs->lpOpenClientRegistyKey(szUniqueDriverId))!=NULL ){
        pBulkTransfer = new BulkTransfer(DevKey, *(LPUSB_FUNCS)lpUsbFuncs, hPipe, *(LPUSB_ENDPOINT_DESCRIPTOR)lpEndpointDesc);
        if (pBulkTransfer && !pBulkTransfer->Init()) {
            delete pBulkTransfer;
            pBulkTransfer = NULL;
        }
    }
    if (DevKey)
        RegCloseKey(DevKey);
    return pBulkTransfer;
}

extern "C" VOID DeleteBulkTransferMgr(LPVOID lpContent)
{
    if (lpContent) 
        delete (BulkTransfer *) lpContent;
}
extern "C" BOOL BulkTransferMgrTransfer(LPVOID lpContent, LPTRANSFER_NOTIFY_ROUTINE NotifyRoutine,PVOID NotifyContext,DWORD Flags,
        LPVOID pBuffer,DWORD BufferLength)
{
    BOOL fRet = FALSE;
    if (lpContent) {
        fRet = ((BulkTransfer *)lpContent)->IssueTransfer(NotifyRoutine,NotifyContext,Flags,pBuffer,BufferLength);
    }
    return fRet;
}
extern "C" BOOL BulkTransferClose(LPVOID lpContent)
{
    BOOL fRet = FALSE;
    if (lpContent) {
        fRet = ((BulkTransfer *)lpContent)->CloseTransfer();
    }
    return fRet;
}
extern "C" BOOL BulkTransferMgrGetStatus(LPVOID lpContent, LPDWORD lpByteTransferred , LPDWORD lpdErrors)
{
    BOOL fRet = FALSE;
    if (lpContent) {
        fRet = ((BulkTransfer *)lpContent)->GetTransferStatus(lpByteTransferred,lpdErrors);
    }
    return fRet;
}
extern "C" BOOL BulkTransferWait(LPVOID lpContent,DWORD dwTicks)
{
    BOOL fRet = FALSE;
    if (lpContent) {
        fRet = ((BulkTransfer *)lpContent)->WaitForTransfer(dwTicks);
    }
    return fRet;
}

BulkTransfer::BulkTransfer(HKEY DevKey, USB_FUNCS& UsbFuncs, USB_PIPE hPipe,USB_ENDPOINT_DESCRIPTOR& USBEndpoint)
:   m_UsbFuncs(UsbFuncs)
,   m_hPipe(hPipe)
,   m_EndpointDesc(USBEndpoint)
,   CReg(DevKey,TEXT(""))
{
    m_fTransferComplete = TRUE;
    for(DWORD dwIndex = 0 ; dwIndex < MAX_OUTSTANDING_TRANSFER; dwIndex ++) {
        m_hUsbTransfer[dwIndex] = NULL;
        m_hUsbEvent[dwIndex] = NULL;
        m_dwTransferPhysAddr[dwIndex] = 0;
        m_pTransferUnCached[dwIndex] = NULL;
        m_pTransferCached[dwIndex] = NULL;
    }
    m_numOfTransfer = ValueDW(USBMASS_REG_NUMOF_TRANFER, DEFAULT_NUM_OF_TRANFER);
    if (m_numOfTransfer>MAX_OUTSTANDING_TRANSFER) {
        m_numOfTransfer = MAX_OUTSTANDING_TRANSFER;
    }
    m_dwDisableBuffering = ValueDW(USBMASS_REG_DISABLE_BUFFERING, 0);
    DWORD dwNumPacketPerTrans = ValueDW(USBMASS_REG_PACKETS_TRANFER, DEFAULT_NUM_OF_PACKET_PER_TRANFER);
    
    m_physPageSize = dwNumPacketPerTrans *m_EndpointDesc.wMaxPacketSize ;
    m_physPageSize = ((m_physPageSize + PAGE_SIZE -1 )/PAGE_SIZE)* PAGE_SIZE ; // Align with Page size.
    
    //Allocate Resource.
    for(dwIndex = 0 ; dwIndex < m_numOfTransfer; dwIndex ++) {
        
        m_hUsbTransfer[dwIndex] = NULL;
        m_pUserBuffer[dwIndex] = NULL;
        m_dwUserBufferSize[dwIndex] =0 ;
        m_hUsbEvent[dwIndex] =  CreateEvent(NULL, TRUE, FALSE, NULL) ; // Manual Reset.

        if (m_dwDisableBuffering) {
            m_pTransferUnCached[dwIndex] = NULL;
            m_dwTransferPhysAddr[dwIndex] = 0 ;
        }
        else
            m_pTransferUnCached[dwIndex] = (LPBYTE)AllocPhysMem(m_physPageSize,PAGE_READWRITE|PAGE_NOCACHE,
                0,0,&m_dwTransferPhysAddr[dwIndex]);
        
        if (m_dwTransferPhysAddr[dwIndex]) {            
            m_pTransferCached[dwIndex] =(LPBYTE) VirtualAlloc(0, m_physPageSize, MEM_RESERVE, PAGE_NOACCESS);
            if (m_pTransferCached[dwIndex]) {
                BOOL bSuccess = VirtualCopy(m_pTransferCached[dwIndex],m_pTransferUnCached[dwIndex],
                    m_physPageSize,PAGE_READWRITE ); // Cached Address.
                if (!bSuccess) {
                    VirtualFree(m_pTransferCached[dwIndex], 0, MEM_RELEASE);
                    m_pTransferCached[dwIndex] = NULL;
                }
            }

        }
    }
    
    m_lpNotification = NULL;
    m_NotifyContext = 0 ;
    m_pCurUserBufferPtr = m_pArgUserBufferPtr = NULL ;
    m_dwCurUserBufferSize = m_dwArgUserBufferSize = 0 ;
    m_dwCurWaitIndex = 0;
    m_dwLastError = USB_NO_ERROR;
    m_dwTransferedByte = 0;
}
BulkTransfer::~BulkTransfer()
{
    Lock();
    CloseTransfer();
    Unlock();
    
    for(DWORD dwIndex = 0 ; dwIndex < m_numOfTransfer; dwIndex ++) {
        if (m_hUsbEvent[dwIndex] )
            CloseHandle(m_hUsbEvent[dwIndex]);
        if (m_pTransferCached[dwIndex]) {
            VirtualFree(m_pTransferCached[dwIndex], 0, MEM_RELEASE);
        }
        if (m_dwTransferPhysAddr[dwIndex]) {
            FreePhysMem(m_pTransferUnCached[dwIndex]);
        }
    }
}
BOOL BulkTransfer::Init()
{
    //Allocate Resource.
    BOOL fReturn = TRUE;
    for(DWORD dwIndex = 0 ; dwIndex < m_numOfTransfer; dwIndex ++) {
        if (m_hUsbEvent[dwIndex]==NULL) {
            fReturn = FALSE;
            break;
        }
    }
    if ( m_hPipe == NULL)
        fReturn = FALSE;
        
    ASSERT(fReturn);
    return fReturn;
}
BOOL   BulkTransfer::SendAnotherTransfer(DWORD dwIndex)
{
    BOOL fReturn = FALSE;
    if  ( dwIndex<m_numOfTransfer && m_dwCurUserBufferSize ) {
        ASSERT(m_hUsbTransfer[dwIndex] == NULL);
        ResetEvent(m_hUsbEvent[dwIndex]);
        m_pUserBuffer[dwIndex]= m_pCurUserBufferPtr ;
        m_dwUserBufferSize[dwIndex] = min ( m_dwCurUserBufferSize, m_physPageSize);;
        if (m_pTransferCached[dwIndex]) { // Cached OUT transfer, we need copy.
            if ((m_Flags & USB_IN_TRANSFER)==0) {
                CeSafeCopyMemory(m_pTransferCached[dwIndex], m_pUserBuffer[dwIndex], m_dwUserBufferSize[dwIndex]);
                CacheRangeFlush(m_pTransferCached[dwIndex],m_dwUserBufferSize[dwIndex], CACHE_SYNC_WRITEBACK );
            }
            m_hUsbTransfer[dwIndex] = m_UsbFuncs.lpIssueBulkTransfer(m_hPipe,TransferCallback,(LPVOID)m_hUsbEvent[dwIndex],
                m_Flags,m_dwUserBufferSize[dwIndex],m_pTransferUnCached[dwIndex],m_dwTransferPhysAddr[dwIndex]);
        }
        else 
            m_hUsbTransfer[dwIndex] = m_UsbFuncs.lpIssueBulkTransfer(m_hPipe,TransferCallback,(LPVOID)m_hUsbEvent[dwIndex],
                m_Flags,m_dwUserBufferSize[dwIndex],m_pUserBuffer[dwIndex],0);
        if (m_hUsbTransfer[dwIndex] != NULL ) {
            m_pCurUserBufferPtr += m_dwUserBufferSize[dwIndex];
            if (m_dwCurUserBufferSize > m_dwUserBufferSize[dwIndex]) {
                m_dwCurUserBufferSize -= m_dwUserBufferSize[dwIndex];
            }
            else 
                m_dwCurUserBufferSize = 0; 
            fReturn = TRUE;
        }
        else { // Just in case.
            ASSERT(FALSE);
            SetEvent(m_hUsbEvent[dwIndex]);
        }
    }
    ASSERT(fReturn);
    return fReturn;
}

BOOL   BulkTransfer::IssueTransfer(LPTRANSFER_NOTIFY_ROUTINE NotifyRoutine,PVOID NotifyContext,DWORD Flags,
        LPVOID pBuffer,DWORD BufferLength)
{
    BOOL fReturn = FALSE;
    Lock();
    if (m_fTransferComplete  && pBuffer && BufferLength) {
        m_Flags = Flags|USB_NO_WAIT;
        m_fTransferComplete = FALSE;
        m_lpNotification = NotifyRoutine;
        m_NotifyContext = NotifyContext;
        m_pCurUserBufferPtr = m_pArgUserBufferPtr = (LPBYTE) pBuffer;
        m_dwCurUserBufferSize = m_dwArgUserBufferSize = BufferLength;
        m_dwCurWaitIndex = 0;
        m_dwLastError = USB_NO_ERROR;
        m_dwTransferedByte = 0 ;
        
        // Start as many as we allowed
        for (DWORD dwIndex=0; dwIndex<m_numOfTransfer && m_dwCurUserBufferSize ; dwIndex++) {
            if (!SendAnotherTransfer(dwIndex)) {
                m_dwLastError = USB_NOT_COMPLETE_ERROR;
                break;
            }
        }
        if (m_dwLastError != USB_NO_ERROR) {
            CompleteNotification(m_dwLastError);
        }
        else {
            fReturn = TRUE;
        }
    }
    Unlock();
    return fReturn;
}

DWORD BulkTransfer::TransferCallback( LPVOID lpvNotifyParam)
{
    if (lpvNotifyParam!=NULL) {
        SetEvent((HANDLE)lpvNotifyParam);
    }
    return 1;
}
BOOL    BulkTransfer::WaitForTransfer(DWORD dwTicks)
{
    DWORD dwStartTick = GetTickCount();
    BOOL fContinue = TRUE;
    while (fContinue &&  !IsTransferCompleted()) {
        DWORD dwTickPassed = GetTickCount() - dwStartTick;
        DWORD dwTickWait = (dwTicks == INFINITE? INFINITE: (dwTickPassed< dwTicks? dwTicks- dwTickPassed : 1));
        BOOL    fRet = (WaitForSingleObject(m_hUsbEvent[m_dwCurWaitIndex], dwTickWait) == WAIT_OBJECT_0);
        if (fRet && m_hUsbTransfer[m_dwCurWaitIndex] ) {  // Signaled.
            Lock();
            ASSERT(!m_fTransferComplete );
            DWORD dwError = USB_NOT_COMPLETE_ERROR ;
            DWORD dwBytes = m_dwUserBufferSize[m_dwCurWaitIndex];
            if (m_hUsbTransfer[m_dwCurWaitIndex]) {
                if (!m_UsbFuncs.lpGetTransferStatus(m_hUsbTransfer[m_dwCurWaitIndex], &dwBytes , &dwError )) {
                    dwError = USB_NOT_COMPLETE_ERROR ;
                }
                m_dwTransferedByte += min (dwBytes,m_dwUserBufferSize[m_dwCurWaitIndex]);  ;
                m_UsbFuncs.lpCloseTransfer( m_hUsbTransfer[m_dwCurWaitIndex]);
                m_hUsbTransfer[m_dwCurWaitIndex] = NULL ;
                    
                if (m_pTransferCached[m_dwCurWaitIndex] && (m_Flags & USB_IN_TRANSFER)!=0) { // In transfer complete.
                    CacheRangeFlush(m_pTransferCached[m_dwCurWaitIndex],m_dwUserBufferSize[m_dwCurWaitIndex], CACHE_SYNC_DISCARD );
                    CeSafeCopyMemory(m_pUserBuffer[m_dwCurWaitIndex], m_pTransferCached[m_dwCurWaitIndex],dwBytes);
                }            
            }
            if (dwError != USB_NO_ERROR || dwBytes < m_dwUserBufferSize[m_dwCurWaitIndex]) { // Error or end with short transfer.
                CompleteNotification(dwError);
                fContinue = FALSE;
            }
            else if (m_dwCurUserBufferSize!= 0 ) { // Continue to transfer.
                if (SendAnotherTransfer(m_dwCurWaitIndex)) {
                    if (++m_dwCurWaitIndex >= m_numOfTransfer)
                        m_dwCurWaitIndex = 0 ;
                }
                else {
                    CompleteNotification(USB_NOT_COMPLETE_ERROR);
                    fContinue = FALSE;
                }
            }
            else { // Check for the completion.
                if (++m_dwCurWaitIndex >= m_numOfTransfer)
                    m_dwCurWaitIndex = 0 ;
            
                if (m_hUsbTransfer[m_dwCurWaitIndex] == NULL ) {
                    CompleteNotification(m_dwLastError);
                    fContinue = FALSE;
                }
            }
            Unlock();
        }
        else
            fContinue = FALSE;
    }
    return IsTransferCompleted();
}
void BulkTransfer::CompleteNotification(DWORD dwError)
{
    for (DWORD dwIndex=0; dwIndex<m_numOfTransfer ; dwIndex++) {
        if (m_hUsbTransfer[dwIndex]!=NULL ) {
            m_UsbFuncs.lpCloseTransfer(m_hUsbTransfer[dwIndex]);
            m_hUsbTransfer[dwIndex] = NULL;
        }
        if (m_hUsbEvent[dwIndex]!=NULL) {
            SetEvent(m_hUsbEvent[dwIndex]);
        }
    }
    LPTRANSFER_NOTIFY_ROUTINE lpNotification = m_lpNotification ;
    m_lpNotification = NULL;
    m_dwCurWaitIndex = 0 ;
    m_fTransferComplete = TRUE;
    m_dwLastError = dwError ;
    if (lpNotification) {
        lpNotification(m_NotifyContext);
    }
    
}
BOOL    BulkTransfer::CloseTransfer()
{
    BOOL fReturn = TRUE;
    Lock();
    CompleteNotification(m_dwLastError);
    Unlock();
    return fReturn;
}
BOOL    BulkTransfer::GetTransferStatus(LPDWORD lpByteTransferred , LPDWORD lpdErrors)
{
    BOOL fReturn = TRUE;
    Lock();
    DWORD dwErrors = USB_NOT_COMPLETE_ERROR;
    DWORD dwByteTransfered = m_dwTransferedByte;
    if (IsTransferCompleted()) {
        dwErrors = m_dwLastError;
    }
    //ASSERT(dwErrors == USB_NO_ERROR);
    Unlock();
    if (lpByteTransferred)
        *lpByteTransferred = dwByteTransfered;
    if (lpdErrors)
        *lpdErrors = dwErrors;
    return fReturn;
    
}

