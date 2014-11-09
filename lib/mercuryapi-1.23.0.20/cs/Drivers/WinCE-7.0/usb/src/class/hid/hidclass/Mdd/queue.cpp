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

#include "queue.h"
#include "hidmdd.h"
#include <intsafe.h>

// Constructor. Set all members to unitialized states.
HidTLCQueue::HidTLCQueue(
    ) 
{
    m_fInitialized = FALSE;
    m_fAccepting = FALSE;
    m_pQueue = NULL;
    m_pbReportBuffer = NULL;
    m_dwCapacity = 0;
    m_hFilled = NULL;
    m_hClosing = NULL;
    m_cbMaxReport = 0;

    InitializeCriticalSection(&m_csLock);

    EmptyQueue();
}


// Initialize all members. Returns TRUE if successful.
BOOL
HidTLCQueue::Initialize(
    DWORD dwQueueCapacity,
    DWORD cbMaxReport
    )
{
    SETFNAME(_T("HidTLCQueue::Initialize"));

    DEBUGCHK(m_fInitialized == FALSE);
    DEBUGCHK(cbMaxReport > 0);

    Lock();

    m_cbMaxReport = cbMaxReport;

    if (ChangeQueueCapacity(dwQueueCapacity) != ERROR_SUCCESS) {
        goto EXIT;
    }

    m_hFilled = CreateEvent(NULL, MANUAL_RESET_EVENT, FALSE, NULL);
    if (m_hFilled == NULL) {
        DEBUGMSG(ZONE_ERROR, (_T("%s: CreateEvent error:%d\r\n"), pszFname, GetLastError()));
        goto EXIT;
    }

    m_hClosing = CreateEvent(NULL, MANUAL_RESET_EVENT, FALSE, NULL);
    if (m_hClosing == NULL) {
        DEBUGMSG(ZONE_ERROR, (_T("%s: CreateEvent error:%d\r\n"), pszFname, GetLastError()));
        goto EXIT;
    }

    m_fInitialized = TRUE;

    Validate();

EXIT:
    Unlock();
    return m_fInitialized;
}


// Desctructor. Unlock all objects.
HidTLCQueue::~HidTLCQueue(
    )
{
    if (m_pQueue != NULL) HidFree(m_pQueue);
    if (m_pbReportBuffer != NULL) HidFree(m_pbReportBuffer);
    if (m_hFilled != NULL) CloseHandle(m_hFilled);
    if (m_hClosing != NULL) CloseHandle(m_hClosing);
    DeleteCriticalSection(&m_csLock);
}


// Adds a new report to the queue. Verify that the queue IsAccepting() and 
// !IsFull() before calling. Sets the m_hFilled event.
// This method is only to be called by the HID class driver.
BOOL 
HidTLCQueue::Enqueue(
    CHAR const*const pbNewReport, 
    DWORD cbNewReport,
    DWORD dwReportID
    )
{
    PHID_TLC_QUEUE_NODE pNode;
    PCHAR pbDest; 

    PREFAST_DEBUGCHK(pbNewReport != NULL);
    DEBUGCHK(cbNewReport <= m_cbMaxReport);

    Lock();
    Validate();
    
    DEBUGCHK(IsAccepting() == TRUE); // Only call when the queue is accepting
    DEBUGCHK(IsFull() == FALSE); // Do not call when the queue is full

    // Insert the report information at the back of the queue
    pNode = &m_pQueue[m_dwBack];
    
    DEBUGCHK(pNode->fValid == FALSE);
    DEBUGCHK(pNode->pbReport != NULL);

    pbDest = pNode->pbReport;
    if (dwReportID == 0) {
        // Prepend the ID of 0 onto the report.
        *pbDest = 0;
        ++pbDest;
    }
    
    memcpy(pbDest, pbNewReport, cbNewReport);
    pNode->cbReport = cbNewReport;

    if (dwReportID == 0) {
        // Add 1 to the report length to account for the prepended ID
        ++pNode->cbReport;
    }
    
#ifdef DEBUG
    pNode->fValid = TRUE;
#endif

    // Increment to next queue node
    if (++m_dwBack == m_dwCapacity) {
        m_dwBack = 0;
    }
    ++m_dwSize;

    SetEvent(m_hFilled);

    Validate();
    Unlock();
    
    return TRUE;
}

// Remove a report from the queue. The call will wait for dwTimeout if the
// queue is empty.
// Returns ERROR_SUCCESS, ERROR_DEVICE_REMOVED, or ERROR_TIMEOUT.
// This method is only to be called by a HID client driver.
DWORD 
HidTLCQueue::Dequeue(
    __in_bcount(cbBuffer) PCHAR pbBuffer, // The buffer to receive the report
    DWORD cbBuffer, // The size of the buffer
    PDWORD pcbTransferred, // How many bytes are in the report
    HANDLE hCancel, // Signal this to cancel the transfer
    DWORD dwTimeout // How long to wait for a report
    )
{
    SETFNAME(_T("HidTLCQueue::Dequeue"));

    PHID_TLC_QUEUE_NODE pNode;
    HANDLE rghWaits[] = { m_hFilled, m_hClosing, hCancel };
    DWORD chWaits = _countof(rghWaits);
    DWORD dwErr = ERROR_SUCCESS;
    DWORD dwWaitResult;
    
    PREFAST_DEBUGCHK(pbBuffer != NULL);
    DEBUGCHK(cbBuffer == m_cbMaxReport);
    PREFAST_DEBUGCHK(pcbTransferred != NULL);

    UNREFERENCED_PARAMETER(cbBuffer);

    if (hCancel == NULL) {
        --chWaits;
    }

    // Wait for a report (m_hFilled), a call to Close (m_hClosing), 
    // a cancel (hCancel), or timeout.
    dwWaitResult = WaitForMultipleObjects(chWaits, rghWaits, FALSE, dwTimeout);

    Lock();
    Validate();
    
    if (dwWaitResult == WAIT_TIMEOUT) {
        // GetSize() will be 0, so check for the timeout first
        dwErr = ERROR_TIMEOUT;
        goto EXIT;
    } 
    else if (dwWaitResult == WAIT_OBJECT_0 + 1) {
        // This device has been removed. We return an error so the thread
        // calling us can exit.
        dwErr = ERROR_DEVICE_REMOVED;
        goto EXIT;
    }
    else if (dwWaitResult == WAIT_OBJECT_0 + 2 || GetSize() == 0 ) {
        dwErr = ERROR_CANCELLED;
        goto EXIT;
    }

    // Remove the report at the front of the queue.
    pNode = &m_pQueue[m_dwFront];
    
    DEBUGCHK(pNode->fValid == TRUE);
    DEBUGCHK(pNode->pbReport != NULL);
    DEBUGCHK(pNode->cbReport <= m_cbMaxReport);

    __try {
        memcpy(pbBuffer, pNode->pbReport, pNode->cbReport);
        *pcbTransferred = pNode->cbReport;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        dwErr = ERROR_INVALID_PARAMETER;
    }
    if (dwErr != ERROR_SUCCESS) {
        DEBUGMSG(ZONE_ERROR, (_T("%s: Exception writing to user buffer\r\n"),
            pszFname));
        goto EXIT;
    }
    
#ifdef DEBUG
    pNode->fValid = FALSE;
    memset(pNode->pbReport, 0xCC, m_cbMaxReport);
#endif

    // Increment to next queue node
    if (++m_dwFront == m_dwCapacity) {
        m_dwFront = 0;
    }
    --m_dwSize;

    if (m_dwSize == 0) {
        // We are empty. Reset the m_hFilled event so we will wait next time.
        ResetEvent(m_hFilled);
    }

EXIT:
    Validate();
    Unlock();
    
    return dwErr;
}


// Signal that this queue will be closing, presumably because the device
// has been removed.
void
HidTLCQueue::Close(
    )
{
    Lock();
    Validate();
    SetEvent(m_hClosing);
    Unlock();
}


// Change the capacity (max number of reports) of the queue. All reports 
// currently in the queue are lost.
DWORD 
HidTLCQueue::SetCapacity(
    DWORD dwNewCapacity
    )
{
    DWORD dwErr;

    DEBUGCHK(m_fInitialized == TRUE);

    Lock();
    Validate();
    dwErr = ChangeQueueCapacity(dwNewCapacity);
    Validate();
    Unlock();

    return dwErr;
}


// Allow the queue to accept new queued reports or not.
// Returns the old state of m_fAccepting.
BOOL 
HidTLCQueue::AcceptNewReports(
    BOOL fAccept
    )
{
    BOOL fOldAccept;
    
    Lock();
    Validate();
    fOldAccept = m_fAccepting;
    m_fAccepting = fAccept;
    Unlock();

    return fOldAccept;
}


// Remove all items from the queue.
void
HidTLCQueue::EmptyQueue(
    )
{
    Lock();
    m_dwFront = 0;
    m_dwBack = 0;
    m_dwSize = 0;
    ResetEvent(m_hFilled);
    Unlock();
}


// Private helper function to change the queue capacity.
// Caller must verify that dwNewCapacity != 0.
// All reports currently in the queue are lost.
DWORD 
HidTLCQueue::ChangeQueueCapacity(
    DWORD dwNewCapacity
    )
{
    SETFNAME(_T("HidTLCQueue::ChangeQueueCapacity"));

    PHID_TLC_QUEUE_NODE pNewQueue;
    PCHAR pbNewReportBuffer;
    DWORD cbQueue;
    DWORD cbMaxReportBuffer;
    DWORD dwIdx;
    DWORD dwErr;
    
    DEBUGCHK(dwNewCapacity != 0);

    // Allocate the circular array of report nodes.
    if((S_OK != 
        DWordMult(dwNewCapacity, sizeof(HID_TLC_QUEUE_NODE), &cbQueue)) ||
        (S_OK != 
        DWordMult(dwNewCapacity, m_cbMaxReport, &cbMaxReportBuffer)))
    {
        dwErr = ERROR_INVALID_PARAMETER;
        DEBUGMSG(ZONE_ERROR, (_T("%s: Integer overflow. ")
                           _T("New capacity too large.:%d\r\n"), pszFname));
        cbQueue = 0;
        cbMaxReportBuffer = 0;
        goto EXIT;
    }
     
    pNewQueue = (PHID_TLC_QUEUE_NODE) HidAlloc(cbQueue);
    if (pNewQueue == NULL) {
        dwErr = GetLastError();
        DEBUGMSG(ZONE_ERROR, (_T("%s: LocalAlloc error:%d\r\n"), pszFname, dwErr));
        goto EXIT;
    }

    // Allocate the memory for a report in each node. This is done in one
    // big chunk and divided later.
    pbNewReportBuffer = (PCHAR) HidAlloc(cbMaxReportBuffer);
    if (pbNewReportBuffer == NULL) {
        dwErr = GetLastError();
        DEBUGMSG(ZONE_ERROR, (_T("%s: LocalAlloc error:%d\r\n"), pszFname, dwErr));
        HidFree(pNewQueue);
        goto EXIT;
    }

    ZeroMemory(pNewQueue, cbQueue);

    // Assign a portion of the report buffer to each node.    
    for (dwIdx = 0; dwIdx < dwNewCapacity; ++dwIdx) {
        PCHAR pbCurrReport = pbNewReportBuffer + (dwIdx * m_cbMaxReport);
        DEBUGCHK(pbCurrReport < pbNewReportBuffer + cbMaxReportBuffer);
        pNewQueue[dwIdx].pbReport = pbCurrReport;
    }
    
    if (m_pQueue != NULL) {
        // Free the old queue
        HidFree(m_pQueue);
        HidFree(m_pbReportBuffer);
    }

    // Set the updated values.
    m_pQueue = pNewQueue;
    m_pbReportBuffer = pbNewReportBuffer;
    m_dwCapacity = dwNewCapacity;
    EmptyQueue();
    dwErr = ERROR_SUCCESS;
    
EXIT:
    return dwErr;
}


#ifdef DEBUG

// Validate the state of the queue.
void
HidTLCQueue::Validate(
    ) const
{
    PHID_TLC_QUEUE_NODE pNode;
    DWORD dwCalculatedSize;
    DWORD dwIdx;
    
    Lock();
    
    DEBUGCHK(m_fInitialized == TRUE);
    DEBUGCHK(m_pQueue != NULL);
    DEBUGCHK(LocalSize(m_pQueue) >= (m_dwCapacity * sizeof(HID_TLC_QUEUE_NODE)));
    DEBUGCHK(m_pbReportBuffer != NULL);
    DEBUGCHK(LocalSize(m_pbReportBuffer) >= (m_cbMaxReport * m_dwCapacity));

    DEBUGCHK(m_dwFront < m_dwCapacity);
    DEBUGCHK(m_dwBack < m_dwCapacity);
    DEBUGCHK(m_dwSize <= m_dwCapacity);

    // Verify that the queue nodes are valid 
    for (dwIdx = 0; dwIdx < m_dwCapacity; ++dwIdx) {
        pNode = &m_pQueue[dwIdx];
        DEBUGCHK(pNode->pbReport != NULL);
        DEBUGCHK(pNode->pbReport >= m_pbReportBuffer);
        DEBUGCHK(pNode->pbReport < m_pbReportBuffer + m_cbMaxReport * m_dwCapacity);
        DEBUGCHK(pNode->cbReport <= m_cbMaxReport);

        // If filled to capacity, then all nodes must be valid
        DEBUGCHK( (m_dwSize != m_dwCapacity) || (pNode->fValid == TRUE) );
    }

    dwIdx = m_dwFront;
    while (dwIdx != m_dwBack) {
        DEBUGCHK(dwIdx < m_dwCapacity);
        pNode = &m_pQueue[dwIdx];
        DEBUGCHK(pNode->fValid == TRUE);
        PREFAST_SUPPRESS( 394, "Potential buffer overrun while writing to buffer 'm_pQueue'. The buffer pointer is being incremented inside a loop." )
        if (++dwIdx == m_dwCapacity) {
            dwIdx = 0;
        }
    }

    // Check our size
    if (m_dwFront < m_dwBack) {
        dwCalculatedSize = m_dwBack - m_dwFront; 
    }
    else if (m_dwFront > m_dwBack) {
        dwCalculatedSize = m_dwCapacity - (m_dwFront - m_dwBack);
    }
    else {
        if (m_pQueue[m_dwFront].fValid == TRUE) {
            dwCalculatedSize = m_dwCapacity;
        }
        else {
            dwCalculatedSize = 0;
        }
    }
    DEBUGCHK(dwCalculatedSize == m_dwSize);

    DEBUGCHK(m_hFilled != NULL);
    DEBUGCHK(m_hClosing != NULL);

    Unlock();
}

#endif // DEBUG

