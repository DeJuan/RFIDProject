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


Module Name:

    remlock.c

Abstract:

    Common RemoveLock

Notes:
    
    Simple RemoveLock definitions made to mimic the Win2k IoXxxRemoveLock functions.
    See the Win2k DDK for descriptions.

Revision History:

--*/

#include "usbclient.h"

#define ZONE_ERROR              DEBUGZONE(0)
#define ZONE_FUNCTION           DEBUGZONE(3)
// since we don't have a standard verbose zone in usb, we will use ZONE_FUNCTION
#define ZONE_VERBOSE            ZONE_FUNCTION

BOOL
InitializeRemoveLock(
    IN  PREMOVE_LOCK Lock
    )
{
    BOOL bRc = FALSE;

    if (Lock) {
        Lock->Removed = FALSE;
        Lock->IoCount = 1;
        Lock->RemoveEvent = CreateEvent( NULL, FALSE, FALSE, NULL);
        if ( Lock->RemoveEvent ) {
            bRc = TRUE;
        } else {
            DEBUGMSG(ZONE_ERROR, (TEXT("CreateEvent error:%d\n"), GetLastError() ));
            ASSERT(0);
        }
    } else {
        DEBUGMSG(ZONE_ERROR, (TEXT("InitializeRemoveLock: Invalid Parameter\n")));
        ASSERT(0);
    }
    return bRc;
}


DWORD
AcquireRemoveLock(
    IN PREMOVE_LOCK Lock,
    IN OPTIONAL PVOID Tag
    )
{
    LONG        ioCount;
    DWORD    status;

    UNREFERENCED_PARAMETER(Tag);

    if (!Lock) {
        status = (DWORD)STATUS_INVALID_PARAMETER;
        DEBUGMSG(ZONE_ERROR, (TEXT("AcquireRemoveLock error: 0x%x\n"), status ));
        ASSERT(0);
        return status;
    }

    //
    // bump the lock's count
    //
    ioCount = InterlockedIncrement( &Lock->IoCount );

    ASSERTMSG(TEXT("AcquireRemoveLock - lock negative \n"), (ioCount > 0));

    if ( !Lock->Removed ) {

        status = ERROR_SUCCESS;

    } else {

        if (0 == InterlockedDecrement( &Lock->IoCount ) ) {
            SetEvent( Lock->RemoveEvent );
        }
        status = (DWORD)STATUS_DELETE_PENDING;

    }

    return status;
}


VOID
ReleaseRemoveLock(
    IN PREMOVE_LOCK Lock,
    IN OPTIONAL PVOID Tag
    )
{
    LONG    ioCount;

    UNREFERENCED_PARAMETER(Tag);

    if (!Lock) {
        DEBUGMSG(ZONE_ERROR, (TEXT("ReleaseRemoveLock: Invalid Parameter\n")));
        ASSERT(0);
        return;
    }

    ioCount = InterlockedDecrement( &Lock->IoCount );

    ASSERT(0 <= ioCount);

    if (0 == ioCount) {

        ASSERT(Lock->Removed);

        //
        // The device needs to be removed.  Signal the remove event
        // that it's safe to go ahead.
        //
        SetEvent( Lock->RemoveEvent );
    }

    return;
}

//
// This is the final exit point on the Remove Lock.
// It also closes the event handle, so do not reenter on the same lock.
//
VOID
ReleaseRemoveLockAndWait(
    IN PREMOVE_LOCK Lock,
    IN OPTIONAL PVOID Tag
    )
{
    LONG    ioCount;

    UNREFERENCED_PARAMETER(Tag);

    if (!Lock) {
        DEBUGMSG(ZONE_ERROR, (TEXT("ReleaseRemoveLockAndWait: Invalid Parameter\n")));
        ASSERT(0);
        return;
    }

    ASSERT( !Lock->Removed ); // should only get called once
    Lock->Removed = TRUE;

    ioCount = InterlockedDecrement( &Lock->IoCount );
    ASSERT (0 < ioCount);
    // ASSERT (ioCount >= 1);

    if (0 < InterlockedDecrement( &Lock->IoCount ) ) {
_retry:
        DEBUGMSG(ZONE_VERBOSE, (TEXT("ReleaseRemoveLockAndWait: waiting for %d IoCount...\n"), Lock->IoCount));
        switch (WaitForSingleObject( Lock->RemoveEvent, 1000 ) ) 
        {
           case WAIT_OBJECT_0:
              DEBUGMSG(ZONE_VERBOSE, (TEXT("...CloseEvent signalled\n")));
              break;

           case WAIT_TIMEOUT:
              DEBUGMSG(ZONE_VERBOSE, (TEXT("WAIT_TIMEOUT\n")));
              goto _retry;
              break;
   
           default:
              DEBUGMSG(ZONE_ERROR, (TEXT("Unhandled WaitReason\n")));
              ASSERT(0);
              break;
        }
        DEBUGMSG(ZONE_VERBOSE, (TEXT("....ReleaseRemoveLockAndWait: done!\n")));
    
    } else {
        DEBUGMSG(ZONE_VERBOSE, (TEXT("ReleaseRemoveLockAndWait IoCount:%d\n"), Lock->IoCount ));
    }

    //
    // close the event handle
    //
    ASSERT(0 == Lock->IoCount);
    if (Lock->RemoveEvent) {
        CloseHandle(Lock->RemoveEvent);
        Lock->RemoveEvent = NULL;
    }

    return;
}

// EOF
