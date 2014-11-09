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

#ifndef _QUEUE_H_
#define _QUEUE_H_

#include <windows.h>


#define HID_TLC_QUEUE_DEFAULT_CAPACITY 10


class HidTLCQueue {
public:

    HidTLCQueue();
    BOOL Initialize(DWORD dwQueueCapacity, DWORD cbReportMax);
    
    ~HidTLCQueue();

    BOOL Enqueue(CHAR const*const pbReport, DWORD cbReport, DWORD dwReportID);
    DWORD Dequeue(__in_bcount(cbBuffer) PCHAR pbBuffer, DWORD cbBuffer, PDWORD pcbTransferred, 
        HANDLE hCancel, DWORD dwTimeout);

    BOOL AcceptNewReports(BOOL fAccept);

    // Returns Win32 error
    DWORD SetCapacity(DWORD dwNewCapacity);

    void Lock() const;
    void Unlock() const;

    void Close();

    // Constant access methods
    BOOL IsInitialized() const;
    DWORD GetSize() const;
    DWORD GetCapacity() const;
    BOOL IsFull() const;
    BOOL IsAccepting() const;

#ifdef DEBUG
    void Validate() const;
#else
    inline void Validate() const {}
#endif
    
private:

    // No copying instances of this class...
    HidTLCQueue(const HidTLCQueue&); 
    HidTLCQueue& operator=(const HidTLCQueue&);

    DWORD ChangeQueueCapacity(DWORD dwNewCapacity);
    void EmptyQueue();

    // Private structure for storing a report
    typedef struct _HidTLCQueueNode {
        PCHAR pbReport;
        DWORD cbReport;
#ifdef DEBUG
        BOOL fValid;
#endif
    } HID_TLC_QUEUE_NODE, *PHID_TLC_QUEUE_NODE;

    BOOL m_fInitialized; // TRUE if successfully initialized

    // The queue nodes are treated as a circular array.
    PHID_TLC_QUEUE_NODE m_pQueue; // Pointer to an array of queue nodes
    DWORD m_dwCapacity; // Size of the array

    // m_dwFront == m_dwBack if (m_dwSize == 0 || m_dwSize == m_dwCapacity)
    DWORD m_dwFront; // Index of the front of the array. Dequeues occur here.
    DWORD m_dwBack; // Index of the back of the array. Enqueues occur here.
    DWORD m_dwSize; // Count of valid queue nodes

    // There is a single large buffer for the reports that is 
    // m_cbMaxReport * m_dwCapacity bytes in size.
    PCHAR m_pbReportBuffer;     

    DWORD m_cbMaxReport; // Maximum size for a report

    BOOL m_fAccepting; // Can this queue accept data?

    // The queue is be accessed by one thread in the Hid class and one or more
    // threads in the Hid client. This critical section serializes access.
    mutable CRITICAL_SECTION m_csLock; 

    // This event gets set whenever a new report is Enqueued. It gets unset
    // when the queue becomes empty on Dequeue.
    HANDLE m_hFilled;

    // This event gets signals that the queue is closing.
    HANDLE m_hClosing;
};


// ***** Inline HidTLCQueue functions *****

// Returns the count of reports in the queue.
inline
DWORD 
HidTLCQueue::GetSize(
    ) const 
{ 
    DWORD dwRet;

    Lock();
    dwRet = m_dwSize; 
    Unlock();

    return dwRet;

}

// Returns the maximum number of reports that can be queued at once.
inline
DWORD 
HidTLCQueue::GetCapacity(
    ) const 
{ 
    DWORD dwRet;

    Lock();
    dwRet = m_dwCapacity;
    Unlock();

    return dwRet;
}


// Returns TRUE if the queue has no more space.
inline
BOOL 
HidTLCQueue::IsFull(
    ) const 
{
    BOOL fRet;
    
    Lock();
    fRet = (GetSize() == GetCapacity());
    Unlock();
    
    return fRet;
}


// Returns TRUE if the queue can accept reports.
inline
BOOL 
HidTLCQueue::IsAccepting(
    ) const
{
    BOOL fRet;
    
    Lock();
    fRet = m_fAccepting;
    Unlock();
    
    return fRet;
}


// Enters the critical section.
inline
void 
HidTLCQueue::Lock(
    ) const 
{ 
    EnterCriticalSection(&m_csLock); 
}


// Leaves the critical section.
inline
void 
HidTLCQueue::Unlock(
    ) const 
{ 
    LeaveCriticalSection(&m_csLock); 
}


// Returns TRUE if initialization has successfully occurred.
inline
BOOL 
HidTLCQueue::IsInitialized(
    ) const
{
    BOOL fRet;
    
    Lock();
    fRet = m_fInitialized;
    Unlock();
    
    return fRet;
}



#endif

