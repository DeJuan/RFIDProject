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

    disk.c

Abstract:

    DSK_Xxx Streams Interface for USB Disk Class:
        bInterfaceSubClass = 0x02 : SFF8020i (ATAPI) CD-ROM
        bInterfaceSubClass = 0x04 : USB Floppy Interface (UFI)
        bInterfaceSubClass = 0x06 : SCSI Passthrough

Notes:

--*/

#include <windows.h>
#include <pkfuncs.h>
#include <devload.h>
#include <storemgr.h>
#include <ntddscsi.h>
#include "usbmsc.h"
#include "scsi2.h"

BOOL
WaitForUnitReady(
    IN PSCSI_DEVICE pDevice
    );


UCHAR
DiscoverLuns(
    IN PSCSI_DEVICE pDevice
    );

DWORD
GetDiskInfo(
    IN PSCSI_DEVICE     pDevice,
    IN OUT PDISK_INFO   pDiskInfo,
    IN UCHAR            Lun
    );

DWORD
GetMediumInfo(
    IN PSCSI_DEVICE pDevice,
    IN UCHAR        Lun
    );

VOID
RemoveDeviceContext(
   PSCSI_DEVICE pDevice
   );

DWORD WINAPI
MediaChangeThread(
    LPVOID Context
    );

DWORD
MountUpperDriver(
    IN PSCSI_DEVICE pDevice,
    IN PPOST_INIT_BUF pInBuf
    );

DWORD
DismountUpperDriver(
    IN PSCSI_DEVICE pDevice
    );

DWORD
GetDeviceInfo(
    IN PSCSI_DEVICE pDevice,
    IN OUT STORAGEDEVICEINFO *psdi
    );

#ifdef DEBUG
DBGPARAM dpCurSettings = {
    TEXT("USBDISK6"), {
    TEXT("Errors"),    TEXT("Warnings"),    TEXT("Init"),       TEXT("Trace"),
    TEXT("SCSI"),      TEXT("CDROM"),       TEXT("CDDA"),       TEXT("IOCTL"),
    TEXT("Read"),      TEXT("Write"),       TEXT("Thread"),     TEXT("Event"),
    TEXT("Undefined"), TEXT("Undefined"),   TEXT("Undefined"),  TEXT("Undefined"),
    },
    ZONEMASK_WARN | ZONEMASK_ERR
};

#endif  // DEBUG

static
BOOL
SterilizeIoRequest(
    PSG_REQ pSafeIoRequest,
    PSG_REQ pUnsafeIoRequest,
    DWORD   cbUnsafeIoRequest
    )
{
    DWORD i = 0;

    if (NULL == pSafeIoRequest) {
        return FALSE;
    }
    if (NULL == pUnsafeIoRequest) {
        return FALSE;
    }
    if (cbUnsafeIoRequest < sizeof(SG_REQ)) {
        return FALSE;
    }
    if (cbUnsafeIoRequest > (sizeof(SG_REQ) + ((MAX_SG_BUF - 1) * sizeof(SG_BUF)))) {
        return FALSE;
    }
    if (0 == CeSafeCopyMemory((LPVOID)pSafeIoRequest, (LPVOID)pUnsafeIoRequest, cbUnsafeIoRequest)) {
        return FALSE;
    }
    for (i = 0; i < pSafeIoRequest->sr_num_sg; i += 1) {
        if (
            NULL == pSafeIoRequest->sr_sglist[i].sb_buf ||
            0 == pSafeIoRequest->sr_sglist[i].sb_len
        ) {
            return FALSE;
        }
        // NOTENOTE: Embedded Scatter/Gather buffer ptrs are validated before
        // use in the ScsiRWSG function.
    }
    return TRUE;
}

BOOL
DllEntry(
   HANDLE hDllHandle,
   DWORD  dwReason,
   LPVOID lpreserved
   )
{
    BOOL bRc = TRUE;

    UNREFERENCED_PARAMETER(hDllHandle);
    UNREFERENCED_PARAMETER(lpreserved);

    switch (dwReason) {
        case DLL_PROCESS_ATTACH:
            DEBUGREGISTER((HINSTANCE)hDllHandle);
            DisableThreadLibraryCalls((HMODULE) hDllHandle);
            break;

        case DLL_PROCESS_DETACH:
            break;

        default:
            break;
    }

    return bRc;
}

//*****************************************************************************
//  S T R E A M S    I N T E R F A C E
//*****************************************************************************

/*++

DSK_Init:
    Called by Device Manager to initialize our streams interface in response to our call to
    ActivateDevice. We passed ActivateDevice a pointer to our device context, but must read
    it out of the registry as "ClientInfo".

Returns:
    Context pointer used in Xxx_Open, Xxx_PowerDown, Xxx_PowerUp, and Xxx_Deinit

--*/
PSCSI_DEVICE
DSK_Init(
   PVOID Context
   )
{
    LPCTSTR ActivePath = (LPCTSTR)Context;
    PSCSI_DEVICE pDevice = NULL;
    BOOL  bRc = TRUE;

    DEBUGMSG(ZONE_INIT, (TEXT("USBDISK6>DSK_Init(%p)\n"), Context));

    //
    // Get and verify our Context from the Registry
    //
    pDevice = UsbsGetContextFromReg( ActivePath );

    if ( VALID_CONTEXT( pDevice ) ) {
        //
        // Acquire the remove lock. We release it in DSK_Deinit
        //
        if (ERROR_SUCCESS == AcquireRemoveLock(&pDevice->RemoveLock,NULL)) {
            //
            // Initialize state
            //
            EnterCriticalSection(&pDevice->Lock);

            pDevice->DeviceType = SCSI_DEVICE_UNKNOWN;
            pDevice->MediumType = SCSI_MEDIUM_UNKNOWN;

            pDevice->Flags.MediumPresent = FALSE;
            pDevice->Flags.Open          = FALSE;
            pDevice->Flags.MediumChanged = TRUE;

            LeaveCriticalSection(&pDevice->Lock);

            //
            // Try to get Medium information
            //
            GetMediumInfo(pDevice, pDevice->Lun);

        } else {
            bRc = FALSE;
            TEST_TRAP();
        }

    } else {
        bRc = FALSE;
        TEST_TRAP();
    }

    DEBUGMSG(ZONE_INIT, (TEXT("USBDISK6<DSK_Init:0x%x\n"), pDevice ));

    return (bRc ? pDevice : NULL);
}


//
// Called by Device Manager when we Deregister our DSK interface.
//
BOOL
DSK_Deinit(
   PSCSI_DEVICE pDevice
   )
{
    BOOL  bRc = FALSE;

    DEBUGMSG(ZONE_INIT, (TEXT("USBDISK6>DSK_Deinit\n")));

    if ( VALID_CONTEXT( pDevice ) ) {

        //EnterCriticalSection( &pDevice->Lock );
        // ...
        //LeaveCriticalSection(&pDevice->Lock);

        //
        // Release the remove lock acquired in DSK_Init
        //
        ReleaseRemoveLock(&pDevice->RemoveLock,NULL);
    }

    DEBUGMSG(ZONE_INIT, (TEXT("USBDISK6<DSK_Deinit:%d\n"), bRc));

    return bRc;
}


/*++

DSK_Open:
    Called by device mgr after DSK_Init if you requested post-init ioctl.
    Called by FSD when it mounts on top of this disk driver.
    If successful then you can expect the DSK_Close when it unmounts.
    If not successful then you should not expect the call to DSK_Close.

Returns:
    Open context to be used in the Xxx_Read, Xxx_Write, Xxx_Seek, and Xxx_IOControl functions.
    If the device cannot be opened, this function returns NULL.

--*/
PSCSI_DEVICE
DSK_Open(
   PSCSI_DEVICE Context,      // context returned by DSK_Init.
   DWORD        AccessCode,   // @parm access code
   DWORD        ShareMode     // @parm share mode
   )
{
    PSCSI_DEVICE pDevice = Context;
    BOOL bRc = TRUE;

    UNREFERENCED_PARAMETER(AccessCode);
    UNREFERENCED_PARAMETER(ShareMode);

    DEBUGMSG(ZONE_INIT,(TEXT("USBDISK6>DSK_Open(0x%x, 0x%x, 0x%x)\n"),pDevice, AccessCode, ShareMode));

    //
    // acquire the remove lock; release it in DSK_Close
    //
    if ( VALID_CONTEXT( pDevice ) ) {

        EnterCriticalSection(&pDevice->Lock);

        if ( !pDevice->Flags.DeviceRemoved && !pDevice->Flags.PoweredDown) {

            pDevice->OpenCount++;

            pDevice->Flags.Open = TRUE;

        } else {
            DEBUGMSG( ZONE_ERR,(TEXT("DSK_Open: ERROR_ACCESS_DENIED\n")));
            SetLastError(ERROR_ACCESS_DENIED);
            bRc = FALSE;
        }

        LeaveCriticalSection(&pDevice->Lock);

    } else {
        DEBUGMSG( ZONE_ERR,(TEXT("DSK_Open: ERROR_FILE_NOT_FOUND\n")));
        SetLastError(ERROR_FILE_NOT_FOUND);
        bRc = FALSE;
    }

    DEBUGMSG(ZONE_INIT,(TEXT("USBDISK6<DSK_Open:%d\n"), bRc ));

    return (bRc ? pDevice : NULL);
}


BOOL
DSK_Close(
   PSCSI_DEVICE Context
   )
{
    PSCSI_DEVICE pDevice = Context;
    BOOL bRc = TRUE;

    DEBUGMSG(ZONE_INIT,(TEXT("USBDISK6>DSK_Close(0x%x)\n"),pDevice));

    if ( VALID_CONTEXT( pDevice ) ) {

        EnterCriticalSection(&pDevice->Lock);

        if ( 0 == --pDevice->OpenCount ) {

            pDevice->Flags.Open = FALSE;

        }

        ASSERT( pDevice->OpenCount >= 0);

        LeaveCriticalSection(&pDevice->Lock);

    } else {
        DEBUGMSG( ZONE_ERR,(TEXT("DSK_Close: ERROR_INVALID_HANDLE\n")));
        SetLastError(ERROR_INVALID_HANDLE);
        bRc = FALSE;
    }

    DEBUGMSG( ZONE_INIT,(TEXT("USBDISK6<DSK_Close:%d\n"), bRc));
    return bRc;
}


ULONG
DSK_Read(
   PSCSI_DEVICE pDevice,
   UCHAR const*const pBuffer,
   ULONG  BufferLength
   )
{
    UNREFERENCED_PARAMETER(pDevice);
    UNREFERENCED_PARAMETER(pBuffer);
    UNREFERENCED_PARAMETER(BufferLength);

    DEBUGMSG(ZONE_ERR,(TEXT("DSK_Read\n")));
    SetLastError(ERROR_INVALID_FUNCTION);
    return  0;
}


ULONG
DSK_Write(
   PSCSI_DEVICE pDevice,
   UCHAR const*const pBuffer,
   ULONG  BufferLength
   )
{
    UNREFERENCED_PARAMETER(pDevice);
    UNREFERENCED_PARAMETER(pBuffer);
    UNREFERENCED_PARAMETER(BufferLength);

    DEBUGMSG(ZONE_ERR,(TEXT("DSK_Write\n")));
    SetLastError(ERROR_INVALID_FUNCTION);
    return  0;
}


ULONG
DSK_Seek(
   PVOID Context,
   LONG Position,
   DWORD Type
   )
{
    UNREFERENCED_PARAMETER(Context);
    UNREFERENCED_PARAMETER(Position);
    UNREFERENCED_PARAMETER(Type);
    return (ULONG)FAILURE;
}


BOOL
DSK_PowerDown(
   PVOID Context
   )
{
    UNREFERENCED_PARAMETER(Context);
    DEBUGMSG(ZONE_INIT,(TEXT("USBDISK6>DSK_PowerDown\r\n")));
    DEBUGMSG(ZONE_INIT,(TEXT("USBDISK6<DSK_PowerDown\r\n")));
    return TRUE;
}

BOOL
DSK_PowerUp(
   PVOID Context
   )
{
    UNREFERENCED_PARAMETER(Context);
    DEBUGMSG(ZONE_INIT,(TEXT("USBDISK6>DSK_PowerUp\r\n")));
    DEBUGMSG(ZONE_INIT,(TEXT("USBDISK6<DSK_PowerUp\r\n")));
    return TRUE;
}

BOOL
DSK_PreDeinit(
    PVOID Context
    )
{
    UNREFERENCED_PARAMETER(Context);
    DEBUGMSG(ZONE_INIT,(TEXT("USBDISK6>DSK_PreDeInit\r\n")));
    DEBUGMSG(ZONE_INIT,(TEXT("USBDISK6<DSK_PreDeInit\r\n")));
    return TRUE;
}
#pragma warning(push)
#pragma warning(disable:4701) // goto could cause these to be uninitialized
BOOL
DSK_IOControl(
    PSCSI_DEVICE pDevice,
    DWORD        Ioctl,
    PUCHAR       pInBuf,
    DWORD        InBufLen,
    PUCHAR       pOutBuf,
    DWORD        OutBufLen,
    PDWORD       pdwBytesTransferred
   )
{
    BOOL   bRead = TRUE;
    BOOL   bRc = FALSE;
    DWORD  dwErr = ERROR_SUCCESS;
    PUCHAR pBuf = NULL;
    DWORD  dwBufLen = 0;
    DWORD  SafeBytesReturned = 0;            

    TRANSPORT_DATA    SafeTransportData = {0};
    SCSI_PASS_THROUGH* pSPTBuffer = NULL;
    PSENSE_DATA pSenseData = NULL;
    DWORD dwSenseSize = 0;
    BOOL fDataIn = FALSE;
    BOOL fCloseCallerBuffer = FALSE;


    DEBUGMSG(ZONE_DSK_IOCTL, (TEXT("USBDISK6>DSK_IOControl(IOCTL:0x%x, InBuf:0x%x, InBufLen:%d, OutBuf:0x%x, OutBufLen:0x%x)\r\n"),
        Ioctl,
        pInBuf,
        InBufLen,
        pOutBuf,
        OutBufLen));

    if (!ACCEPT_IO(pDevice)) {
        DEBUGMSG(ZONE_ERR, (TEXT("USBDISK6>DSK_IOControl: The device is not available\r\n")));
        SetLastError(ERROR_DEVICE_NOT_AVAILABLE);
        return FALSE;
    }

    EnterCriticalSection(&pDevice->Lock);

    switch (Ioctl)
    {
        case IOCTL_DISK_GETINFO:
            pBuf = pOutBuf;
            dwBufLen = OutBufLen;
            break;

        case DISK_IOCTL_GETINFO:
            pBuf = pInBuf;
            dwBufLen = InBufLen;
            break;

        case IOCTL_DISK_READ:
        case IOCTL_DISK_WRITE:
            bRead = IOCTL_DISK_READ == Ioctl ? TRUE : FALSE;
            break;

        case DISK_IOCTL_READ:
        case DISK_IOCTL_WRITE:
            bRead = DISK_IOCTL_READ == Ioctl ? TRUE : FALSE;
            break;

        default:
            break;
    }

    switch (Ioctl)
    {
        // This IOCTL is deprecated.
        case IOCTL_DISK_INITIALIZED:
        case DISK_IOCTL_INITIALIZED:
            DEBUGMSG(ZONE_DSK_IOCTL,(TEXT("USBDISK6>DSK_IOControl>IOCTL_DISK_INITIALIZED\r\n")));
            if (
                !IsStorageManagerRunning() &&
                pInBuf &&
                InBufLen >= sizeof(PPOST_INIT_BUF)
            ) {
                MountUpperDriver(pDevice, (PPOST_INIT_BUF)pInBuf);
                break;
            }
            DEBUGMSG(ZONE_TRACE, (L"USBDISK>DSK_IOControl> Not loading FSD; Storage Manager running\r\n"));
            break;

        case IOCTL_DISK_GET_MEDIA_CHANGE_COUNT:
        {
            DWORD dwMediaChangeCount = 0;

            if( OutBufLen < sizeof(pDevice->MediaChangeCount) )
            {
                dwErr = ERROR_INSUFFICIENT_BUFFER;
                break;
            }

            if( !pOutBuf )
            {
                dwErr = ERROR_INVALID_PARAMETER;
                break;
            }

            dwMediaChangeCount = (DWORD)InterlockedCompareExchange( (LONG*)&pDevice->MediaChangeCount,
                                                                    (LONG)pDevice->MediaChangeCount,
                                                                    (LONG)pDevice->MediaChangeCount );

            if( !CeSafeCopyMemory( pOutBuf,
                                   &dwMediaChangeCount,
                                   sizeof(dwMediaChangeCount) ) )
            {
                dwErr = ERROR_INVALID_PARAMETER;
            }
            break;
        }

        case IOCTL_DISK_DEVICE_INFO:
        {
            STORAGEDEVICEINFO SafeStorageDeviceInfo = {0};
            SafeBytesReturned = sizeof(STORAGEDEVICEINFO);
            DEBUGMSG(ZONE_DSK_IOCTL, (TEXT("USBDISK6>DSK_IOControl>IOCTL_DISK_DEVICE_INFO\r\n")));
            if ((pInBuf == NULL) || InBufLen != sizeof(STORAGEDEVICEINFO))
            {
                dwErr = ERROR_INVALID_PARAMETER;
                break;
            }
            dwErr = GetDeviceInfo(pDevice, &SafeStorageDeviceInfo);
            if (ERROR_SUCCESS != dwErr) {
                break;
            }
            if (0 == CeSafeCopyMemory((LPVOID)pInBuf, (LPVOID)&SafeStorageDeviceInfo, sizeof(STORAGEDEVICEINFO))) {
                dwErr = ERROR_INVALID_PARAMETER;
                break;
            }
            if (pdwBytesTransferred) {
                if (0 == CeSafeCopyMemory((LPVOID)pdwBytesTransferred, (LPVOID)&SafeBytesReturned, sizeof(DWORD))) {
                    dwErr = ERROR_INVALID_PARAMETER;
                    break;
                }
            }
            break;
        }

        case IOCTL_DISK_GETINFO:
        case DISK_IOCTL_GETINFO:
        {
            DISK_INFO SafeDiskInfo = {0};
            SafeBytesReturned = 0;
            DEBUGMSG(ZONE_DSK_IOCTL, (TEXT("USBDISK6>DSK_IOControl>IOCTL_DISK_GETINFO\r\n")));
            if (NULL == pBuf || dwBufLen != sizeof(DISK_INFO)) {
                dwErr = ERROR_INVALID_PARAMETER;
                break;
            }
            // If a device is attached and a medium is present, then we may
            // have already fetched its disk info.
            if (
                pDevice ->Flags.MediumPresent &&
                0 != pDevice->DiskInfo.di_total_sectors &&
                0 != pDevice->DiskInfo.di_bytes_per_sect
            ) {
                // Fetch cached copy of the device's disk info.
                memcpy((LPVOID)&SafeDiskInfo, (LPVOID)&pDevice->DiskInfo, sizeof(DISK_INFO));
                bRc = TRUE;
            }
            else {
                // Query the device blindly.
                if (0 == GetDiskInfo(pDevice, &SafeDiskInfo, pDevice->Lun)) {
                    // We failed to fetch the medium's device info.  A device
                    // may not be attached or may not contain media.
                    DEBUGMSG(ZONE_ERR, (TEXT("USBDISK6>DSK_IOControl> A device is not attached or does not contain media\r\n")));
                    // Reset our cached copy of the device's disk info.
                    memset((PVOID)&pDevice->DiskInfo, 0, sizeof(DISK_INFO));
                }
                else {
                    // The device returned disk info.
                    pDevice->Flags.FSDMounted = TRUE;
                    pDevice->Flags.MediumChanged = FALSE;
                    bRc = TRUE;
                }
            }
            if (bRc) {
                if (0 == CeSafeCopyMemory((LPVOID)pBuf, (LPVOID)&SafeDiskInfo, sizeof(DISK_INFO))) {
                    dwErr = ERROR_INVALID_PARAMETER;
                    break;
                }
                if (pdwBytesTransferred) {
                    if (0 == CeSafeCopyMemory((LPVOID)pdwBytesTransferred, (LPVOID)&SafeBytesReturned, sizeof(DWORD))) {
                        dwErr = ERROR_INVALID_PARAMETER;
                        break;
                    }
                }
            }
            break;
        }

        case IOCTL_DISK_SETINFO:
        case DISK_IOCTL_SETINFO:
        {
            DISK_INFO SafeDiskInfo = {0};
            SafeBytesReturned = sizeof(DISK_INFO);
            DEBUGMSG(ZONE_DSK_IOCTL, (TEXT("USBDISK6>DSK_IOControl>IOCTL_DISK_SETINFO\r\n")));
            if (NULL == pInBuf || InBufLen != sizeof(DISK_INFO)) {
                dwErr = ERROR_INVALID_PARAMETER;
                break;
            }
            if (0 == CeSafeCopyMemory((LPVOID)&SafeDiskInfo, (LPVOID)pInBuf, sizeof(DISK_INFO))) {
                dwErr = ERROR_INVALID_PARAMETER;
                break;
            }
            memcpy((LPVOID)&pDevice->DiskInfo, (LPVOID)&SafeDiskInfo, sizeof(DISK_INFO));
            if (pdwBytesTransferred) {
                if (0 == CeSafeCopyMemory((LPVOID)pdwBytesTransferred, (LPVOID)&SafeBytesReturned, sizeof(DWORD))) {
                    dwErr = ERROR_INVALID_PARAMETER;
                    break;
                }
            }
            break;
        }

        case IOCTL_DISK_READ:
        case IOCTL_DISK_WRITE:
        case DISK_IOCTL_READ:
        case DISK_IOCTL_WRITE:
        {
            DEBUGMSG(ZONE_DSK_IOCTL, (TEXT("USBDISK6>DSK_IOControl>%s\r\n"),
                (bRead ? TEXT("IOCTL_DISK_READ"):TEXT("IOCTL_DISK_WRITE"))));
            // Sterilize the caller's SG_REQ.
            if (!SterilizeIoRequest(
                pDevice->pSterileIoRequest,
                (PSG_REQ)pInBuf,
                InBufLen)
            ) {
                dwErr = ERROR_INVALID_PARAMETER;
                break;
            }
            // If write, ensure that the device is not write-protected.
            if (!bRead && pDevice->Flags.WriteProtect) {
                dwErr = ERROR_WRITE_PROTECT;
                break;
            }
            pDevice->Flags.MediumChanged = FALSE;
            // Initiate I/O.  Note that, to determine if ScsiRWSG succeeded,
            // we are required to inspect the sr_status.  It's safe to do so,
            // because pSterileIoRequest is our safe copy of the caller's
            // SG_REQ.
            SafeBytesReturned = ScsiRWSG(
                pDevice,
                pDevice->pSterileIoRequest,
                pDevice->Lun,
                bRead
                );
            if (pdwBytesTransferred) {
                if (0 == CeSafeCopyMemory((LPVOID)pdwBytesTransferred, (LPVOID)&SafeBytesReturned, sizeof(DWORD))) {
                    dwErr = ERROR_INVALID_PARAMETER;
                    break;
                }
            }

            dwErr = pDevice->pSterileIoRequest->sr_status;
            if (ERROR_SUCCESS != dwErr) {
                break;
            }
            break;
        }

        case IOCTL_DISK_FORMAT_MEDIA:
        case DISK_IOCTL_FORMAT_MEDIA:
            DEBUGMSG(ZONE_DSK_IOCTL, (TEXT("USBDISK6>DSK_IOControl>IOCTL_DISK_FORMAT_MEDIA\r\n")));
            break;

        case IOCTL_DISK_GETNAME:
        case DISK_IOCTL_GETNAME:
        {
            WCHAR SafeFolderName[] = DEFAULT_FOLDER_SZ;
            SafeBytesReturned = sizeof(DEFAULT_FOLDER_SZ);
            DEBUGMSG(ZONE_DSK_IOCTL, (TEXT("USBDISK6>DSK_IOControl>IOCTL_DISK_GETNAME\r\n")));
            if (pOutBuf == NULL) {
                dwErr = ERROR_INVALID_PARAMETER;
                break;
            }
            if (OutBufLen < sizeof(DEFAULT_FOLDER_SZ)) {
                dwErr = ERROR_INSUFFICIENT_BUFFER;
                break;
            }
            if (0 == CeSafeCopyMemory((LPVOID)pOutBuf, (LPVOID)SafeFolderName, sizeof(DEFAULT_FOLDER_SZ))) {
                dwErr = ERROR_INVALID_PARAMETER;
                break;
            }
            if (pdwBytesTransferred) {
                if (0 == CeSafeCopyMemory((LPVOID)pdwBytesTransferred, (LPVOID)&SafeBytesReturned, sizeof(DWORD))) {
                    dwErr = ERROR_INVALID_PARAMETER;
                    break;
                }
            }
            break;
        }

        case IOCTL_SCSI_PASSTHROUGH:
        {
            TRANSPORT_COMMAND SafeTransportCommand = {0};
            TRANSPORT_DATA    SafeTransportData = {0};
            LPVOID            pCommandBlockOrig = NULL;
            LPVOID            pDataBlockOrig = NULL;

            SafeBytesReturned = 0;
            DEBUGMSG(ZONE_DSK_IOCTL, (TEXT("USBDISK6>DSK_IOControl>IOCTL_SCSI_PASSTHROUGH\r\n")));
            if (
                NULL == pInBuf ||
                InBufLen != sizeof(TRANSPORT_COMMAND) ||
                NULL == pOutBuf ||
                OutBufLen != sizeof(TRANSPORT_DATA))
            {
                dwErr = ERROR_INVALID_PARAMETER;
                break;
            }
            // Sterilize the caller's command wrapper.
            if (0 == CeSafeCopyMemory((LPVOID)&SafeTransportCommand, (LPVOID)pInBuf, sizeof(TRANSPORT_COMMAND))) {
                dwErr = ERROR_INVALID_PARAMETER;
                break;
            }
            // Map the caller's command block.
            pCommandBlockOrig = SafeTransportCommand.CommandBlock;
            if (FAILED(CeOpenCallerBuffer(
                        &SafeTransportCommand.CommandBlock,
                        pCommandBlockOrig,
                        SafeTransportCommand.Length,
                        ARG_I_PTR,
                        FALSE))) {
                dwErr = ERROR_INVALID_PARAMETER;
                break;
            }

            if (NULL == SafeTransportCommand.CommandBlock) {
                dwErr = ERROR_INVALID_PARAMETER;
                break;
            }
            // Sterilize the caller's data wrapper.
            if (0 == CeSafeCopyMemory((LPVOID)&SafeTransportData, (LPVOID)pOutBuf, sizeof(TRANSPORT_DATA))) {
                dwErr = ERROR_INVALID_PARAMETER;
                goto CleanUp_Passthrough;
            }
            // The data block may be NULL.  Map the caller's data block as
            // appropriate.
            if (NULL != SafeTransportData.DataBlock) {
                pDataBlockOrig = SafeTransportData.DataBlock;
                if (FAILED(CeOpenCallerBuffer(
                            &SafeTransportData.DataBlock,
                            pDataBlockOrig,
                            SafeTransportData.RequestLength,
                            ARG_IO_PTR,
                            FALSE))) {
                    dwErr = ERROR_INVALID_PARAMETER;
                    goto CleanUp_Passthrough;
                }
            }

            // Issue command.
            dwErr = ScsiPassthrough(pDevice, &SafeTransportCommand, &SafeTransportData);

            CleanUp_Passthrough:
            // Cleanup mapped ptrs.
            if (FAILED(CeCloseCallerBuffer(
                        SafeTransportCommand.CommandBlock,
                        pCommandBlockOrig,
                        SafeTransportCommand.Length,
                        ARG_I_PTR))) {
                ASSERT(!"Unexpected failure in CeCloseCallerBuffer");
                dwErr = ERROR_GEN_FAILURE;
                break;
            }

            if (pDataBlockOrig) {
                if (FAILED(CeCloseCallerBuffer(
                            SafeTransportData.DataBlock,
                            pDataBlockOrig,
                            SafeTransportData.RequestLength,
                            ARG_IO_PTR))) {
                    ASSERT(!"Unexpected failure in CeCloseCallerBuffer");
                    dwErr = ERROR_GEN_FAILURE;
                    break;
                }
            }

            if (ERROR_SUCCESS != dwErr) {
                break;
            }
            if (pdwBytesTransferred) {
                if (0 == CeSafeCopyMemory((LPVOID)pdwBytesTransferred, (LPVOID)&SafeTransportData.TransferLength, sizeof(DWORD)) ||
                    0 == CeSafeCopyMemory(pOutBuf, &SafeTransportData, OutBufLen)) {
                    dwErr = ERROR_INVALID_PARAMETER;
                    break;
                }
            }
            break;
        }

        case IOCTL_SCSI_PASS_THROUGH:
        {
            //
            // This translates the parameters from the IOCTL_SCSI_PASS_THROUGH
            // command to those already used by the IOCTL_SCSI_PASSTHROUGH
            // command and issues the request in an identical manner.
            //
            TRANSPORT_COMMAND SafeTransportCommand = {0};

            SCSI_PASS_THROUGH SPTIn = { 0 };
            SCSI_PASS_THROUGH SPTOut = { 0 };


            SafeBytesReturned = 0;
            DEBUGMSG(ZONE_DSK_IOCTL, (TEXT("USBDISK6>DSK_IOControl>IOCTL_SCSI_PASS_THROUGH\r\n")));
            if (
                NULL == pInBuf ||
                InBufLen < sizeof(SCSI_PASS_THROUGH))
            {
                dwErr = ERROR_INVALID_PARAMETER;
                goto CleanUp_Pass_Through;
            }

            if (
                NULL == pOutBuf ||
                OutBufLen < sizeof(SCSI_PASS_THROUGH))
            {
                dwErr = ERROR_INVALID_PARAMETER;
                goto CleanUp_Pass_Through;
            }

            //
            // The pass through buffer could be changed at any time and we need
            // to make sure that we handle this situation gracefully.
            //
            if( !CeSafeCopyMemory( &SPTIn,
                                   pInBuf,
                                   sizeof(SCSI_PASS_THROUGH) ) )
            {
                dwErr = ERROR_INVALID_PARAMETER;
                goto CleanUp_Pass_Through;
            }

            if( !CeSafeCopyMemory( &SPTOut,
                                   pOutBuf,
                                   sizeof(SCSI_PASS_THROUGH) ) )
            {
                dwErr = ERROR_INVALID_PARAMETER;
                goto CleanUp_Pass_Through;
            }

            fDataIn = SPTIn.DataIn != 0;
            if( fDataIn )
            {
                pSPTBuffer = &SPTOut;
            }
            else
            {
                pSPTBuffer = &SPTIn;
            }

            //
            // Validate that the data buffer looks correct.
            //
            if( pSPTBuffer->DataTransferLength &&
                pSPTBuffer->DataBufferOffset < sizeof(SCSI_PASS_THROUGH) )
            {
                dwErr = ERROR_INVALID_PARAMETER;
                goto CleanUp_Pass_Through;
            }

            if( pSPTBuffer->DataTransferLength >
                (fDataIn ? OutBufLen : InBufLen) - sizeof(SCSI_PASS_THROUGH) )
            {
                dwErr = ERROR_INVALID_PARAMETER;
                goto CleanUp_Pass_Through;
            }

            if( (fDataIn ? OutBufLen : InBufLen) - pSPTBuffer->DataTransferLength <
                pSPTBuffer->DataBufferOffset )
            {
                dwErr = ERROR_INVALID_PARAMETER;
                goto CleanUp_Pass_Through;
            }

            //
            // Validate that the sense buffer and the data buffer don't
            // overlap.
            //
            if( SPTOut.DataBufferOffset < SPTOut.SenseInfoOffset &&
                SPTOut.DataBufferOffset + SPTOut.DataTransferLength > SPTOut.SenseInfoOffset )
            {
                dwErr = ERROR_INVALID_PARAMETER;
                goto CleanUp_Pass_Through;
            }

            if( SPTOut.SenseInfoOffset < SPTOut.DataBufferOffset &&
                SPTOut.SenseInfoOffset + SPTOut.SenseInfoLength > SPTOut.DataBufferOffset )
            {
                dwErr = ERROR_INVALID_PARAMETER;
                goto CleanUp_Pass_Through;
            }

            if( SPTOut.SenseInfoOffset == SPTOut.DataBufferOffset &&
                SPTOut.SenseInfoOffset != 0  &&
                SPTOut.DataTransferLength != 0 )
            {
                dwErr = ERROR_INVALID_PARAMETER;
                goto CleanUp_Pass_Through;
            }

            if( OutBufLen -
                sizeof(SCSI_PASS_THROUGH) -
                SPTOut.DataTransferLength <
                SPTOut.SenseInfoLength )
            {
                dwErr = ERROR_INVALID_PARAMETER;
                goto CleanUp_Pass_Through;
            }

            if( SPTOut.SenseInfoLength &&
                SPTOut.SenseInfoOffset < sizeof(SCSI_PASS_THROUGH) )
            {
                dwErr = ERROR_INVALID_PARAMETER;
                goto CleanUp_Pass_Through;
            }

            //
            // Determine the size of the sense info to return.
            //
            dwSenseSize = SPTOut.SenseInfoLength;
            if( OutBufLen - sizeof(SCSI_PASS_THROUGH) < dwSenseSize )
            {
                dwErr = ERROR_INVALID_PARAMETER;
                goto CleanUp_Pass_Through;
            }

            pSenseData = (PSENSE_DATA)(pOutBuf + SPTOut.SenseInfoOffset);

            SafeTransportCommand.Flags = fDataIn ? DATA_IN : DATA_OUT;
            SafeTransportCommand.Length = SPTIn.CdbLength;
            SafeTransportCommand.Timeout = SPTIn.TimeOutValue;
            SafeTransportCommand.CommandBlock = (PVOID)SPTIn.Cdb;
            SafeTransportCommand.dwLun = pDevice->Lun;

            SafeTransportData.RequestLength = pSPTBuffer->DataTransferLength;


            // The data block may be NULL.  Map the caller's data block as
            // appropriate.
            if (0 != pSPTBuffer->DataBufferOffset &&
                0 != pSPTBuffer->DataTransferLength) {
                if (FAILED(CeOpenCallerBuffer(
                            &SafeTransportData.DataBlock,
                            (fDataIn ? pOutBuf : pInBuf) + pSPTBuffer->DataBufferOffset,
                            SafeTransportData.RequestLength,
                            ARG_IO_PTR,
                            FALSE))) {
                    dwErr = ERROR_INVALID_PARAMETER;
                    goto CleanUp_Pass_Through;
                }

                fCloseCallerBuffer = TRUE;
            }

            // Issue command.
            dwErr = ScsiPassthrough(pDevice, &SafeTransportCommand, &SafeTransportData);

CleanUp_Pass_Through:
            if (fCloseCallerBuffer) {
                if (FAILED(CeCloseCallerBuffer(
                            SafeTransportData.DataBlock,
                            (fDataIn ? pOutBuf : pInBuf) + pSPTBuffer->DataBufferOffset,
                            SafeTransportData.RequestLength,
                            ARG_IO_PTR))) {
                    ASSERT(!"Unexpected failure in CeCloseCallerBuffer");
                    dwErr = ERROR_GEN_FAILURE;
                    break;
                }
            }

            if (ERROR_SUCCESS != dwErr) {
                DWORD dwError = ERROR_SUCCESS;
                SENSE_DATA SenseData= { 0 };

                dwError = ScsiRequestSenseSimple( pDevice, &SenseData );
                if( dwError == ERROR_SUCCESS )
                {
                    if( dwSenseSize )
                    {
                        CeSafeCopyMemory( pSenseData, &SenseData, min(sizeof(SenseData),dwSenseSize) );
                        SafeTransportData.TransferLength = min(sizeof(SenseData),dwSenseSize);
                    }
                    DEBUGMSG(ZONE_SENSEDATA, (TEXT("USBDISK6:IOCTL_SCSI_PASS_THROUGH - SenseData: %x\\%x\\%x\r\n"), SenseData.SenseKey, SenseData.AdditionalSenseCode, SenseData.AdditionalSenseCodeQualifier));
                }
                else
                {
                    DEBUGMSG(ZONE_ERR, (TEXT("USBDISK6:IOCTL_SCSI_PASS_THROUGH - Unable to retrieve sense data\r\n")));
                }

                break;
            }

            if (pdwBytesTransferred) {
                if (0 == CeSafeCopyMemory((LPVOID)pdwBytesTransferred, (LPVOID)&SafeTransportData.TransferLength, sizeof(DWORD))) {
                    dwErr = ERROR_INVALID_PARAMETER;
                    break;
                }
            }
            break;
        }

        case IOCTL_SCSI_PASS_THROUGH_DIRECT:
        {
            //
            // This translates the parameters from the
            // IOCTL_SCSI_PASS_THROUGH_DIRECT command to those already used by
            // the IOCTL_SCSI_PASSTHROUGH command and issues the request in an
            // identical manner.
            //
            TRANSPORT_COMMAND SafeTransportCommand = {0};
            TRANSPORT_DATA    SafeTransportData = {0};

            SCSI_PASS_THROUGH_DIRECT SPTIn = { 0 };
            SCSI_PASS_THROUGH_DIRECT SPTOut = { 0 };
            SCSI_PASS_THROUGH_DIRECT* pSPTBuffer = NULL;

            PSENSE_DATA pSenseData = NULL;
            DWORD dwSenseSize = 0;
            BOOL fDataIn = FALSE;
            BOOL fUserMode = GetDirectCallerProcessId() != GetCurrentProcessId();
            BOOL fCloseCallerBuffer = FALSE;

            SafeBytesReturned = 0;
            DEBUGMSG(ZONE_DSK_IOCTL, (TEXT("USBDISK6>DSK_IOControl>IOCTL_SCSI_PASS_THROUGH_DIRECT\r\n")));
            if (
                NULL == pInBuf ||
                InBufLen < sizeof(SCSI_PASS_THROUGH_DIRECT))
            {
                dwErr = ERROR_INVALID_PARAMETER;
                goto CleanUp_Pass_Through;
            }

            if (
                NULL == pOutBuf ||
                OutBufLen < sizeof(SCSI_PASS_THROUGH_DIRECT))
            {
                dwErr = ERROR_INVALID_PARAMETER;
                goto CleanUp_Pass_Through;
            }

            //
            // The pass through buffer could be changed at any time and we need
            // to make sure that we handle this situation gracefully.
            //
            if( !CeSafeCopyMemory( &SPTIn,
                                   pInBuf,
                                   sizeof(SCSI_PASS_THROUGH_DIRECT) ) )
            {
                dwErr = ERROR_INVALID_PARAMETER;
                goto CleanUp_Pass_Through_Direct;
            }

            if( !CeSafeCopyMemory( &SPTOut,
                                   pOutBuf,
                                   sizeof(SCSI_PASS_THROUGH_DIRECT) ) )
            {
                dwErr = ERROR_INVALID_PARAMETER;
                goto CleanUp_Pass_Through_Direct;
            }

            fDataIn = SPTIn.DataIn != 0;
            if( fDataIn )
            {
                pSPTBuffer = &SPTOut;
            }
            else
            {
                pSPTBuffer = &SPTIn;
            }

            if( (!pSPTBuffer->DataBuffer && pSPTBuffer->DataTransferLength) ||
                (pSPTBuffer->DataBuffer && !pSPTBuffer->DataTransferLength) )
            {
                dwErr = ERROR_INVALID_PARAMETER;
                goto CleanUp_Pass_Through_Direct;
            }

            if( fUserMode && (!IsKModeAddr( (DWORD)pInBuf ) || !IsKModeAddr( (DWORD)pOutBuf )) )
            {
                if( fUserMode &&
                    pSPTBuffer->DataBuffer &&
                    !IsValidUsrPtr( pSPTBuffer->DataBuffer,
                                    pSPTBuffer->DataTransferLength,
                                    fDataIn ? TRUE : FALSE ) )
                {
                    dwErr = ERROR_INVALID_PARAMETER;
                    goto CleanUp_Pass_Through_Direct;
                }
            }

            //
            // Determine the size of the sense info to return.
            //
            dwSenseSize = SPTOut.SenseInfoLength;
            if( OutBufLen - sizeof(SCSI_PASS_THROUGH) < dwSenseSize )
            {
                dwErr = ERROR_INVALID_PARAMETER;
                goto CleanUp_Pass_Through_Direct;
            }

            pSenseData = (PSENSE_DATA)(pOutBuf + pSPTBuffer->SenseInfoOffset);

            SafeTransportCommand.Flags = fDataIn ? DATA_IN : DATA_OUT;
            SafeTransportCommand.Length = SPTIn.CdbLength;
            SafeTransportCommand.Timeout = SPTIn.TimeOutValue;
            SafeTransportCommand.CommandBlock = (PVOID)SPTIn.Cdb;
            SafeTransportCommand.dwLun = pDevice->Lun;

            SafeTransportData.RequestLength = pSPTBuffer->DataTransferLength;


            // The data block may be NULL.  Map the caller's data block as
            // appropriate.
            if (0 != pSPTBuffer->DataBuffer &&
                0 != SafeTransportData.RequestLength) {
                if (FAILED(CeOpenCallerBuffer(
                            &SafeTransportData.DataBlock,
                            pSPTBuffer->DataBuffer,
                            SafeTransportData.RequestLength,
                            ARG_IO_PTR,
                            FALSE))) {
                    dwErr = ERROR_INVALID_PARAMETER;
                    goto CleanUp_Pass_Through_Direct;
                }

                fCloseCallerBuffer = TRUE;
            }
            // Issue command.
            dwErr = ScsiPassthrough(pDevice, &SafeTransportCommand, &SafeTransportData);

            CleanUp_Pass_Through_Direct:
            if (fCloseCallerBuffer) {
                if (FAILED(CeCloseCallerBuffer(
                            SafeTransportData.DataBlock,
                            pSPTBuffer->DataBuffer,
                            SafeTransportData.RequestLength,
                            ARG_IO_PTR))) {
                    ASSERT(!"Unexpected failure in CeCloseCallerBuffer");
                    dwErr = ERROR_GEN_FAILURE;
                    break;
                }
            }

            if (ERROR_SUCCESS != dwErr) {
                DWORD dwError = ERROR_SUCCESS;
                SENSE_DATA SenseData= { 0 };

                dwError = ScsiRequestSenseSimple( pDevice, &SenseData );

                if( dwError == ERROR_SUCCESS )
                {
                    if( dwSenseSize )
                    {
                        CeSafeCopyMemory( pSenseData, &SenseData, min(sizeof(SenseData), dwSenseSize) );
                        SafeTransportData.TransferLength = min(sizeof(SENSE_DATA), dwSenseSize);
                    }

                    DEBUGMSG(ZONE_ERR, (TEXT("USBDISK6:IOCTL_SCSI_PASS_THROUGH - SenseData: %x\\%x\\%x\r\n"), SenseData.SenseKey, SenseData.AdditionalSenseCode, SenseData.AdditionalSenseCodeQualifier));
                }
                else
                {
                    DEBUGMSG(ZONE_ERR, (TEXT("USBDISK6:IOCTL_SCSI_PASS_THROUGH - Unable to retrieve sense data\r\n")));
                }

                break;
            }

            if (pdwBytesTransferred) {
                if (0 == CeSafeCopyMemory((LPVOID)pdwBytesTransferred, (LPVOID)&SafeTransportData.TransferLength, sizeof(DWORD))) {
                    dwErr = ERROR_INVALID_PARAMETER;
                    break;
                }
            }
            break;
        }

        case IOCTL_CDROM_READ_SG:
        {
            DWORD SgCount = 0;
            PCDROM_READ pUnsafeCDROMRead = (PCDROM_READ)pInBuf;
            BYTE       rgbBuffer[sizeof(SG_REQ) + ((MAX_SG_BUF - 1) * sizeof(SG_BUF))] = {0};
            PSG_REQ    pPseudoUnsafeIoRequest = (PSG_REQ)rgbBuffer;
            DWORD      i = 0;
            SafeBytesReturned = 0;
            DEBUGMSG(ZONE_DSK_IOCTL, (TEXT("USBDISK6>DSK_IOControl>IOCTL_CDROM_READ_SG\r\n")));
            if (NULL == pInBuf) {
                dwErr = ERROR_INVALID_PARAMETER;
                break;
            }
            if (InBufLen < sizeof(CDROM_READ)) {
                dwErr = ERROR_INVALID_PARAMETER;
                break;
            }
            if (SCSI_DEVICE_CDROM != pDevice->DeviceType) {
                dwErr = ERROR_NOT_SUPPORTED;
                break;
            }
            if (pDevice->Flags.Busy) {
                dwErr = ERROR_BUSY;
                break;
            }
            // Mark device as busy.
            pDevice->Flags.Busy = TRUE;
            // Prepare device for I/O.
            dwErr = ScsiUnitAttention(pDevice, pDevice->Lun);
            if (ERROR_SUCCESS != dwErr) {
                if (!pDevice->Flags.MediumPresent) {
                    dwErr = ERROR_MEDIA_NOT_AVAILABLE;
                }
                pDevice->Flags.Busy = FALSE;
                break;
            }
            // Convert the supplied CDROM_READ to a SG_REQ.  Note that, we
            // don't support CD-DA formatted CDs or raw reads.
            // TrackMode is TBD
            if (
                CDDA == pUnsafeCDROMRead->TrackMode ||
                CDROM_ADDR_LBA != pUnsafeCDROMRead->StartAddr.Mode ||
                pUnsafeCDROMRead->bRawMode
            ) {
                dwErr = ERROR_NOT_SUPPORTED;
                pDevice->Flags.Busy = FALSE;
                break;
            }

            SgCount = pUnsafeCDROMRead->sgcount;
            if (SgCount > MAX_SG_BUF) {
                dwErr = ERROR_INVALID_PARAMETER;
                pDevice->Flags.Busy = FALSE;
                break;
            }

            // Convert CDROM_READ to SG_REQ
            pPseudoUnsafeIoRequest->sr_start = pUnsafeCDROMRead->StartAddr.Address.lba;
            pPseudoUnsafeIoRequest->sr_num_sec = pUnsafeCDROMRead->TransferLength;
            pPseudoUnsafeIoRequest->sr_num_sg = SgCount;
            for (i = 0; i < SgCount; i++) {
                pPseudoUnsafeIoRequest->sr_sglist[i].sb_buf = pUnsafeCDROMRead->sglist[i].sb_buf;
                pPseudoUnsafeIoRequest->sr_sglist[i].sb_len = pUnsafeCDROMRead->sglist[i].sb_len;
            }
            // Sterilize the caller's SG_REQ.
            if (!SterilizeIoRequest(
                pDevice->pSterileIoRequest,
                pPseudoUnsafeIoRequest,
                sizeof(SG_REQ) + ((pPseudoUnsafeIoRequest->sr_num_sg - 1) * sizeof(SG_BUF)))
            ) {
                dwErr = ERROR_INVALID_PARAMETER;
                pDevice->Flags.Busy = FALSE;
                break;
            }
            // Initiate I/O.  Note that to determine if ScsiRWSG succeeded,
            // we are required to inspect the sr_status.  It's safe to do so,
            // because pSterileIoRequest is our safe copy of the caller's
            // SG_REQ.  The following is a workaround for UDFS not refreshing
            // disk info.
            if (
                0 == pDevice->DiskInfo.di_bytes_per_sect ||
                0 == pDevice->DiskInfo.di_total_sectors
            ) {
                DISK_INFO di = {0};
                dwErr = ScsiReadCapacity(pDevice, &di, pDevice->Lun);
                if (ERROR_SUCCESS != dwErr) {
                    pDevice->Flags.Busy = FALSE;
                    break;
                }
            }
            dwErr = ScsiCDRead(
                pDevice,
                pDevice->pSterileIoRequest,
                &SafeBytesReturned
                );
            if (pdwBytesTransferred) {
                if (0 == CeSafeCopyMemory((LPVOID)pdwBytesTransferred, (LPVOID)&SafeBytesReturned, sizeof(DWORD))) {
                    dwErr = ERROR_INVALID_PARAMETER;
                    break;
                }
            }
            pDevice->Flags.Busy = FALSE;
            break;
        }

        case IOCTL_CDROM_TEST_UNIT_READY:
        {
            PCDROM_TESTUNITREADY pCDROMTestUnitReady = (PCDROM_TESTUNITREADY)pOutBuf;
            ULONG                ulMediumPresentSnapshot = 0;
            ULONG                ulMediumChanged = 0;
            DEBUGMSG(ZONE_DSK_IOCTL, (TEXT("USBDISK6>DSK_IOControl>IOCTL_CDROM_TEST_UNIT_READY\r\n")));
            if (NULL == pOutBuf || OutBufLen < sizeof(CDROM_TESTUNITREADY)) {
                dwErr = ERROR_INVALID_PARAMETER;
                break;
            }
            if (pDevice->Flags.Busy) {
                dwErr = ERROR_BUSY;
                break;
            }
            // Note that UDFS uses this IOCTL to poll the device to test for
            // the presence of media.  If the device's medium has changed, then
            // update the medium info.
            pDevice->Flags.Busy = TRUE;
            // Take a snapshot of whether a medium is present.
            ulMediumPresentSnapshot = pDevice->Flags.MediumPresent;
            dwErr = ScsiUnitAttention(pDevice, pDevice->Lun);
            if (ERROR_SUCCESS == dwErr) {
                ulMediumChanged = ulMediumPresentSnapshot ^ pDevice->Flags.MediumPresent;
                // The device's medium has changed, update the medium info.
                if (ulMediumChanged && pDevice->Flags.MediumPresent) {
                    DISK_INFO di = {0};
                    dwErr = ScsiReadCapacity(pDevice, &di, pDevice->Lun);
                }
                dwErr = ERROR_SUCCESS;
            }
            pDevice->Flags.Busy = FALSE;
            __try {
                pCDROMTestUnitReady->bUnitReady = (ERROR_SUCCESS == dwErr);
            }
            __except(EXCEPTION_EXECUTE_HANDLER) {
                dwErr = ERROR_INVALID_PARAMETER;
            }
            break;
        }

        case IOCTL_CDROM_DISC_INFO:
            DEBUGMSG(ZONE_DSK_IOCTL, (TEXT("USBDISK6>DSK_IOControl>IOCTL_CDROM_DISC_INFO\r\n")));
            // This IOCTL is not implemented.
            break;

        case IOCTL_CDROM_LOAD_MEDIA:
            DEBUGMSG(ZONE_DSK_IOCTL, (TEXT("USBDISK6>DSK_IOControl>IOCTL_CDROM_LOAD_MEDIA\r\n")));
            dwErr = ScsiStartStopUnit(pDevice, START, TRUE, pDevice->Lun);
            break;

        case IOCTL_CDROM_EJECT_MEDIA:
            DEBUGMSG(ZONE_DSK_IOCTL, (TEXT("USBDISK6>DSK_IOControl>IOCTL_CDROM_EJECT_MEDIA\r\n")));
            dwErr = ScsiStartStopUnit(pDevice, STOP, TRUE, pDevice->Lun);
            break;

        // CD-DA IOCTLs
        case IOCTL_CDROM_READ_TOC:
        case IOCTL_CDROM_GET_CONTROL:
        case IOCTL_CDROM_PLAY_AUDIO:
        case IOCTL_CDROM_PLAY_AUDIO_MSF:
        case IOCTL_CDROM_SEEK_AUDIO_MSF:
        case IOCTL_CDROM_STOP_AUDIO:
        case IOCTL_CDROM_PAUSE_AUDIO:
        case IOCTL_CDROM_RESUME_AUDIO:
        case IOCTL_CDROM_GET_VOLUME:
        case IOCTL_CDROM_SET_VOLUME:
        case IOCTL_CDROM_READ_Q_CHANNEL:
        case IOCTL_CDROM_GET_LAST_SESSION:
        case IOCTL_CDROM_RAW_READ:
        case IOCTL_CDROM_DISK_TYPE:
        case IOCTL_CDROM_SCAN_AUDIO:
        {
            dwErr = ScsiCDAudio(
                pDevice,
                Ioctl,
                pInBuf,
                InBufLen,
                pOutBuf,
                OutBufLen,
                pdwBytesTransferred
                );
            break;
        }

        default:
            DEBUGMSG(ZONE_DSK_IOCTL, (TEXT("USBDISK6>DSK_IOControl>Unsupported IOCTL %d\r\n"), Ioctl));
            dwErr = ERROR_NOT_SUPPORTED;
            break;
    }

    LeaveCriticalSection(&pDevice->Lock);

    if (ERROR_SUCCESS != dwErr) {
        SetLastError(dwErr);
    }
    else {
        bRc = TRUE;
    }

    DEBUGMSG(ZONE_DSK_IOCTL,(TEXT("USBDISK6<DSK_IOControl(dwErr:%d, bRc:%d)\r\n"), dwErr, bRc));

    return bRc;
}
#pragma warning(pop)
// returns # of Luns supported
UCHAR
DiscoverLuns(
    IN PSCSI_DEVICE pDevice
    )
{
    DWORD       dwErr, dwRepeat;
    UCHAR       Lun = 0;

    DEBUGMSG(ZONE_INIT,(TEXT("USBDISK6>DiscoverLuns\n")));

    // need to wait a bit for some units to init, spin up, etc.
    //
    dwRepeat = 10;

    do {

        dwErr = ScsiUnitAttention(pDevice, 0);
        if (ERROR_ACCESS_DENIED == dwErr || ERROR_INVALID_HANDLE == dwErr)
            break;
        else if ( ERROR_SUCCESS != dwErr )
        {
            Sleep(100);
        }

    } while (ERROR_NOT_READY == dwErr && --dwRepeat != 0 );

    if (ERROR_SUCCESS != dwErr) {
        return 0;
    }

    // Discover Max Luns
    //
    for (Lun = 0; Lun <= MAX_LUN; Lun++)
    {
        dwErr = ScsiInquiry(pDevice, Lun);
        if (ERROR_SUCCESS != dwErr) {
            break;
        }
    }

    DEBUGMSG(ZONE_INIT,(TEXT("USBDISK6<DiscoverLuns:%d\n"), Lun));

    return Lun;
}

BOOL
WaitForUnitReady(
    IN PSCSI_DEVICE pDevice
    )
{
    DWORD       dwErr, dwRepeat;

    DEBUGMSG(ZONE_INIT,(TEXT("USBDISK6>WaitForDeviceReady\n")));

    // need to wait a bit for some units to init, spin up, etc.
    //
    dwRepeat = 10;

    do {
        dwErr = ScsiUnitAttention(pDevice, pDevice->Lun);
        if (ERROR_ACCESS_DENIED == dwErr || ERROR_INVALID_HANDLE == dwErr)
            break;
        else if ( ERROR_SUCCESS != dwErr )
        {
            Sleep(5);
        }

    } while (ERROR_NOT_READY == dwErr && --dwRepeat != 0 );

    if (ERROR_SUCCESS != dwErr) {
        return 0;
    }

    DEBUGMSG(ZONE_INIT,(TEXT("USBDISK6<WaitForDeviceReady:dwErr =%d\n"),dwErr));

    return (dwErr == ERROR_SUCCESS);
}


DWORD
GetMediumInfo(
    IN PSCSI_DEVICE pDevice,
    IN UCHAR        Lun
    )
{
    DWORD dwErr = ERROR_SUCCESS;

    DEBUGMSG(ZONE_TRACE,(TEXT("Usbdisk6!GetMediumInfo++\r\n")));

    EnterCriticalSection(&pDevice->Lock);

    do {

        if (SCSI_DEVICE_UNKNOWN == pDevice->DeviceType || SCSI_MEDIUM_UNKNOWN == pDevice->MediumType ) {
            dwErr = ScsiUnitAttention(pDevice, Lun);
            if (ERROR_SUCCESS != dwErr)
                break;
        }

        // determine device type

        if (SCSI_DEVICE_UNKNOWN == pDevice->DeviceType) {
            dwErr = ScsiInquiry(pDevice, Lun);
            if (ERROR_SUCCESS != dwErr)
                break;
        }

        // determine medium type

        if (SCSI_DEVICE_UNKNOWN != pDevice->DeviceType && SCSI_MEDIUM_UNKNOWN == pDevice->MediumType ) {
            dwErr = ScsiModeSense10(pDevice, Lun);
            if (ERROR_SUCCESS != dwErr) {

                TRANSPORT_DATA tData;
                UCHAR senseData[18];

                tData.TransferLength = 0;
                tData.RequestLength = sizeof(senseData);
                tData.DataBlock = senseData;
                memset(senseData,0,sizeof(senseData));

                dwErr = ScsiRequestSense(pDevice, &tData, Lun);
                if (ERROR_SUCCESS != dwErr) {
                    break;
                }
            }
        }

        // determine disk information

        if (!pDevice->Flags.MediumPresent) {
            DISK_INFO di = {0};
            dwErr = ScsiReadCapacity(pDevice, &di, Lun);
            if (ERROR_SUCCESS != dwErr)
                break;
        }
#pragma warning(suppress:4127)
    } while (0);

    dwErr = ERROR_SUCCESS;

    LeaveCriticalSection(&pDevice->Lock);

    DEBUGMSG(ZONE_TRACE,(TEXT("Usbdisk6!GetMediumInfo-- Error(%d)\r\n"), dwErr));

    return dwErr;
}


// returns number of bytes written in the pDiskInfo
DWORD
GetDiskInfo(
    IN PSCSI_DEVICE     pDevice,
    IN OUT PDISK_INFO   pDiskInfo,
    IN UCHAR            Lun
    )
{
    DWORD dwErr;
    DWORD dwSize = sizeof(DISK_INFO);
    DWORD dwRepeat = 1;

    DEBUGMSG(ZONE_TRACE,(TEXT("USBDISK6>GetDiskInfo\n")));

    dwErr = ScsiUnitAttention(pDevice, Lun);
    if (dwErr == ERROR_GEN_FAILURE) {
        return 0;
    }

    if ( ERROR_SUCCESS == dwErr ) {

        dwRepeat = pDevice->Timeouts.UnitAttnRepeat;

        do {

            dwErr = ScsiReadCapacity(pDevice, pDiskInfo, Lun);

            dwRepeat--;

        } while (ERROR_SUCCESS != dwErr && dwRepeat != 0 );

    }

    if ( ERROR_SUCCESS != dwErr ||
         0 == pDiskInfo->di_total_sectors ||
         0 == pDiskInfo->di_bytes_per_sect )
    {
        dwSize = 0;
    }

    DEBUGMSG(ZONE_TRACE,(TEXT("USBDISK6<GetDiskInfo:%d\n"), dwSize));

    return dwSize;
}

//*****************************************************************************
//  U S B    D I S K   I N T E R F A C E
//*****************************************************************************

//
// The USB Mass Storage Class driver found a SCSI-2 disk that we support.
// Setup our device context and kick-start our Media Polling Thread.
//
PVOID
UsbDiskAttach(
    HANDLE  UsbTransport,
    LPCWSTR pHardwareKey,
    DWORD   dwLun,
    UCHAR   bInterfaceSubClass
    )
{
    BOOL bRc = TRUE;
    DWORD dwErr = ERROR_SUCCESS;
    PSCSI_DEVICE pDevice = NULL;
    REG_VALUE_DESCR rdTimeouts[] = {
        MEDIA_POLL_SZ,     REG_DWORD, sizeof(DWORD),  (PUCHAR)(NULL),
        READ_SECTOR_SZ,    REG_DWORD, sizeof(DWORD),  (PUCHAR)(NULL),
        WRITE_SECTOR_SZ,   REG_DWORD, sizeof(DWORD),  (PUCHAR)(NULL),
        SCSI_COMMAND_SZ,   REG_DWORD, sizeof(DWORD),  (PUCHAR)(NULL),
        UNIT_ATTENTION_SZ, REG_DWORD, sizeof(DWORD),  (PUCHAR)(NULL),
        NULL, 0, 0, NULL
    };

    DEBUGMSG(ZONE_INIT, (TEXT("USBDISK6>DiskAttach\n")));

    if (!UsbTransport || !pHardwareKey) {
        SetLastError(ERROR_INVALID_PARAMETER);
        DEBUGMSG( ZONE_ERR, (TEXT("Invalid Parameter\n")));
        return NULL;
    }

    // try to accept all disks, probe the device, then unload if we can't controll it.
    if ( !((bInterfaceSubClass >= USBMSC_SUBCLASS_RBC) && (bInterfaceSubClass <= USBMSC_SUBCLASS_SCSI)) ) {
        DEBUGMSG( ZONE_ERR, (TEXT("Unsupported Disk bInterfaceSubClass:0x%x\n"), bInterfaceSubClass));
        return NULL;
    }

    do {
        const DWORD cchActivePath = wcslen(pHardwareKey) + 1;
        //
        // alloc at least 1 disk device context
        //
        pDevice = (PSCSI_DEVICE)LocalAlloc( LPTR, sizeof(SCSI_DEVICE) );
        if ( !pDevice ) {
            DEBUGMSG( ZONE_ERR, (TEXT("LocalAlloc error:%d\n"), GetLastError() ));
            bRc = FALSE;
            break;
        }

        // create sterile I/O request
        pDevice->pSterileIoRequest = (PSG_REQ)LocalAlloc(
            LPTR,
            (sizeof(SG_REQ) + ((MAX_SG_BUF - 1) * sizeof(SG_BUF)))
            );
        if (NULL == pDevice->pSterileIoRequest) {
            bRc = FALSE;
            break;
        }

        pDevice->Sig = USBSCSI_SIG;
        pDevice->MediaChangeCount = 0;

        pDevice->hUsbTransport = UsbTransport;

        pDevice->ActivePath = LocalAlloc(LPTR, cchActivePath*sizeof(WCHAR));
        if (!pDevice->ActivePath) {
            DEBUGMSG( ZONE_ERR, (TEXT("LocalAlloc ERROR:%d\n"), GetLastError()));
            bRc = FALSE;
            break;
        }
        VERIFY(SUCCEEDED(StringCchCopy(pDevice->ActivePath, cchActivePath, pHardwareKey)));
        DEBUGMSG(ZONE_INIT, (TEXT("ActivePath:%s\n"), pDevice->ActivePath));

        pDevice->DiskSubClass = bInterfaceSubClass;

        InitializeCriticalSection( &pDevice->Lock );

        if ( !InitializeRemoveLock( &pDevice->RemoveLock) ) {
            DEBUGMSG( ZONE_ERR, (TEXT("InitializeRemoveLock failed\n")));
            bRc = FALSE;
            break;
        }

        //
        // Grab the RemoveLock, which balances
        // the ReleaseRemoveLockAndWait on UsbDiskDetach
        //
        AcquireRemoveLock(&pDevice->RemoveLock,NULL);

        //
        // set state flags
        //
        pDevice->MediumType = SCSI_MEDIUM_UNKNOWN;
        pDevice->DeviceType = SCSI_DEVICE_UNKNOWN;
        pDevice->Flags.MediumPresent = FALSE;
        pDevice->Flags.MediumChanged = TRUE;
        pDevice->Flags.DeviceRemoved = FALSE;
        pDevice->Flags.PoweredDown   = FALSE;

        pDevice->Lun    = (UCHAR)dwLun;

        //
        // Check registry for timeout values
        //
        rdTimeouts[0].Data = (PUCHAR)(&pDevice->Timeouts.MediaPollInterval);
        rdTimeouts[1].Data = (PUCHAR)(&pDevice->Timeouts.ReadSector);
        rdTimeouts[2].Data = (PUCHAR)(&pDevice->Timeouts.WriteSector);
        rdTimeouts[3].Data = (PUCHAR)(&pDevice->Timeouts.ScsiCommandTimeout);
        rdTimeouts[4].Data = (PUCHAR)(&pDevice->Timeouts.UnitAttnRepeat);

        if ( !GetSetKeyValues(pHardwareKey,
                                  &rdTimeouts[0],
                                  GET,
                                  FALSE) ) {
            //
            // use defaults
            //
            pDevice->Timeouts.MediaPollInterval  = SCSI_MEDIA_POLL_INTERVAL;
            pDevice->Timeouts.ReadSector         = SCSI_READ_SECTOR_TIMEOUT;
            pDevice->Timeouts.WriteSector        = SCSI_WRITE_SECTOR_TIMEOUT;
            pDevice->Timeouts.ScsiCommandTimeout = SCSI_COMMAND_TIMEOUT;
            pDevice->Timeouts.UnitAttnRepeat     = UNIT_ATTENTION_REPEAT;

            // stuff defaults back in reg for tuning
            if ( !GetSetKeyValues(pHardwareKey,
                                  &rdTimeouts[0],
                                  SET,
                                  TRUE) ) {

                DEBUGMSG(ZONE_ERR, (TEXT("GetSetKeyValues ERROR:%d\n"), GetLastError() ));
            }
        }

        // let the user try a zero value, but complain in the debugger
        ASSERT(pDevice->Timeouts.MediaPollInterval);
        ASSERT(pDevice->Timeouts.ReadSector);
        ASSERT(pDevice->Timeouts.WriteSector);
        ASSERT(pDevice->Timeouts.ScsiCommandTimeout);
        ASSERT(pDevice->Timeouts.UnitAttnRepeat);

        // Hold the object lock until we complete ScsiInquiry.  If we don't,
        // a deadlock can occur when the MountStore thread created by
        // ActivateDevice and this thread contend for this object lock and
        // the USB HCD lock.
        EnterCriticalSection(&pDevice->Lock);
         
        //
        // Activate the Streams Interface.
        // This call invokes our DSK_Init.
        //
        pDevice->hStreamDevice = ActivateDevice(pDevice->ActivePath,
                                                (DWORD)pDevice );


        if ( !pDevice->hStreamDevice ) {
            bRc = FALSE;
            LeaveCriticalSection(&pDevice->Lock);
            DEBUGMSG(ZONE_ERR, (TEXT("ActivateDevice ERROR:%d\n"), GetLastError() ));
            TEST_TRAP();
            break;
        }


        //
        // Finally, if we discovered the device type and it has the removable media bit set
        // (via the SCSI_INQUIRY command) then setup the media polling thread.
        // Special case: CD-ROM must load UDFS, which has it's owm polling mechanism.
        //

        //
        // We call need to ScsiInquiry() explicit because if a unit is not ready we do
        //  not know the DeviceType.
        //
        ScsiInquiry(pDevice, pDevice->Lun);

        LeaveCriticalSection(&pDevice->Lock);

        Sleep(500);

        if ( SCSI_DEVICE_UNKNOWN != pDevice->DeviceType && pDevice->Flags.RMB &&
             SCSI_DEVICE_CDROM != pDevice->DeviceType )
        {
            pDevice->hMediaChangedEvent = CreateEvent( NULL, AUTO_RESET_EVENT, FALSE, NULL);
            if ( !pDevice->hMediaChangedEvent ) {
                DEBUGMSG( ZONE_ERR, (TEXT("CreateEvent error:%d\n"), GetLastError() ));
                bRc = FALSE;
                break;
            }

            __try {
                pDevice->hMediaPollThread = CreateThread( NULL, 0,
                                                          MediaChangeThread,
                                                          pDevice,
                                                          0, NULL );

                if ( !pDevice->hMediaPollThread ) {
                    DEBUGMSG( ZONE_ERR, (TEXT("CreateThread error:%d\n"), GetLastError() ));
                    bRc = FALSE;
                    break;
                }
            } __except (EXCEPTION_EXECUTE_HANDLER) {
                dwErr = GetExceptionCode();
                DEBUGMSG(ZONE_ERR,(TEXT("USBDISK6::UsbDiskAttach:EXCEPTION:0x%x\n"), dwErr));
                TEST_TRAP();
            }
        }
#pragma warning(suppress:4127)
    } while (0);

    if (!bRc) {
        //
        // If error then clean up
        //
        TEST_TRAP();

        if (pDevice) {
            ReleaseRemoveLock(&pDevice->RemoveLock,NULL);
            RemoveDeviceContext( pDevice );
        }
    }

    DEBUGMSG(ZONE_INIT, (TEXT("USBDISK6<DiskAttach:%d\n"), bRc));

    return (bRc ? pDevice : NULL);
}


//
// The USB Mass Storage Class driver is removing this disk instance.
// Delete our context & streams interface.
//
BOOL
UsbDiskDetach(
    PSCSI_DEVICE pDevice // Context
    )
{
    BOOL bRc = TRUE;

    DEBUGMSG(ZONE_INIT, (TEXT("USBDISK6>DiskDetach:%p\n"), pDevice));

    if ( !VALID_CONTEXT( pDevice ) ) {
        DEBUGMSG( ZONE_ERR, (TEXT("Invalid Context!\n")));
        return FALSE;
    }

    //
    // set Remove Pending state
    //
    EnterCriticalSection( &pDevice->Lock );

    pDevice->Flags.DeviceRemoved = TRUE;

    LeaveCriticalSection( &pDevice->Lock );

    //
    // wait for the polling thread to terminate
    //
    if (pDevice->hMediaChangedEvent && pDevice->hMediaPollThread) {

        SetEvent(pDevice->hMediaChangedEvent);

        WaitForSingleObject(pDevice->hMediaPollThread, INFINITE);

    } else if ( pDevice->hStreamDevice ) {
        //
        // The device has no polling thread, so dismount the disk
        //
        DismountUpperDriver(pDevice);

    } else {
        TEST_TRAP();
    }

    //
    // wait for the remove lock
    //
    DEBUGMSG(ZONE_INIT, (TEXT("ReleaseRemoveLockAndWait...\n")));
    ReleaseRemoveLockAndWait( &pDevice->RemoveLock, NULL );
    DEBUGMSG(ZONE_INIT, (TEXT("...ReleaseRemoveLockAndWait\n")));

    //
    // remove this device instance
    //
    RemoveDeviceContext( pDevice );

    DEBUGMSG(ZONE_INIT, (TEXT("USBDISK6<DiskDetach:%d\n"), bRc));

    return bRc;
}

//
// Mount the File System Driver named in the registry on top of our Device.
//
DWORD
MountUpperDriver(
    IN PSCSI_DEVICE pDevice,
    IN PPOST_INIT_BUF pInBuf
    )
{
    DWORD dwErr = ERROR_SUCCESS;
    WCHAR sFsd[MAX_DLL_LEN] = DEFAULT_FSD_SZ;
    REG_VALUE_DESCR RegVal[] = {
        FSD_SZ, REG_SZ, MAX_DLL_LEN, NULL,
        NULL,   0, 0, NULL
    };
    HANDLE h;
    RegVal[0].Data = (PUCHAR)(sFsd);

    DEBUGMSG(ZONE_TRACE, (TEXT("USBDISK6>MountUpperDriver\n")));

    if ( !VALID_CONTEXT( pDevice ) ) {
        DEBUGMSG( ZONE_ERR, (TEXT("MountUpperDriver: Invalid Context!\n")));
        return ERROR_INVALID_PARAMETER;
    }

    EnterCriticalSection(&pDevice->Lock);

    do {

        if (!pInBuf)
            h = pDevice->hStreamDevice;
        else
            h = pInBuf->p_hDevice;

        if ( !pDevice->ActivePath || !h ) {
            dwErr = ERROR_INVALID_PARAMETER;
            DEBUGMSG(ZONE_ERR, (TEXT("MountUpperDriver: ERROR_INVALID_PARAMETER\n")));
            break;
        }

        if ( !GetSetKeyValues(pDevice->ActivePath,
                              RegVal,
                              GET,
                              FALSE ) ) {
            dwErr = GetLastError();
            DEBUGMSG(ZONE_ERR, (TEXT("GetSetKeyValues ERRROR:%d\n"), dwErr));
            TEST_TRAP();
            break;
        }

        //
        // Special case: CD-ROM must load UDFS
        //
        if (pDevice->DeviceType == SCSI_DEVICE_CDROM &&
            wcscmp(sFsd, TEXT("UDFS.DLL")) != 0 )
        {
            DEBUGMSG(ZONE_WARN, (TEXT("LoadFSD:%s OVERRIDE to:%s\n"), sFsd, TEXT("UDFS.DLL")));
            memset(sFsd, 0, MAX_DLL_LEN);
            VERIFY(SUCCEEDED(StringCchCopy(sFsd, MAX_DLL_LEN, TEXT("UDFS.DLL"))));
        }

        //
        // This will cause the named FSD to open our device, query via Ioctl,
        // then close our device. We can not assume that the FSD is loaded until
        // we successfully complete the IOCTL_DISK_GETINFO Ioctl.
        //
        __try {
            if ( !LoadFSDEx(h, sFsd, LOADFSD_SYNCH) ) {
                dwErr = GetLastError();
                DEBUGMSG(ZONE_ERR, (TEXT("LoadFSD error:%d\n"), dwErr ));
                TEST_TRAP();
                break;
            }
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            dwErr = GetExceptionCode();
            DEBUGMSG(ZONE_ERR,(TEXT("USBDISK6::MountUpperDriver:EXCEPTION:0x%x\n"), dwErr));
        }

        // pDevice->Flags.FSDMounted = TRUE; // Set in IOCTL_DISK_GETINFO

#pragma warning(suppress:4127)
    } while(0);

    LeaveCriticalSection(&pDevice->Lock);

    DEBUGMSG(ZONE_TRACE, (TEXT("USBDISK6<MountUpperDriver:%d\n"), dwErr));

    return dwErr;
}


/* ++

This function forces the FSD to dismount by unloading the streams interface.
This invokes our DSK_Deinit.
This deletes the active key in the registry, sends a WM_DEVICECHANGE
message, and triggers a NOTIFICATION_EVENT_DEVICE_CHANGE.

-- */
DWORD
DismountUpperDriver(
    IN PSCSI_DEVICE pDevice
    )
{
    DWORD dwErr = ERROR_SUCCESS;

    DEBUGMSG(ZONE_TRACE, (TEXT("USBDISK6>DismountUpperDriver\n")));

    if ( !pDevice || !pDevice->hStreamDevice ) {

        dwErr = ERROR_INVALID_PARAMETER;

    } else {
        EnterCriticalSection(&pDevice->Lock);

        if ( DeactivateDevice( pDevice->hStreamDevice ) ) {

            pDevice->hStreamDevice = NULL;
            pDevice->Flags.FSDMounted = FALSE;

        } else {
            dwErr = GetLastError();
            DEBUGMSG(ZONE_ERR, (TEXT("DeactivateDevice error: %d\n")));
            TEST_TRAP();
        }

        LeaveCriticalSection(&pDevice->Lock);
    }
    DEBUGMSG(ZONE_TRACE, (TEXT("USBDISK6<DismountUpperDriver:%d\n"), dwErr));

    return dwErr;
}


DWORD
GetDeviceInfo(
    IN PSCSI_DEVICE pDevice,
    IN OUT STORAGEDEVICEINFO *psdi
    )
{
    REG_VALUE_DESCR RegValHD[] = {
        L"USBHDProfile", REG_SZ, MAX_DLL_LEN, NULL,
        NULL,   0, 0, NULL
    };
    REG_VALUE_DESCR RegValCD[] = {
        L"USBCDProfile", REG_SZ, MAX_DLL_LEN, NULL,
        NULL,   0, 0, NULL
    };
    REG_VALUE_DESCR RegValFD[] = {
        L"USBFDProfile", REG_SZ, MAX_DLL_LEN, NULL,
        NULL,   0, 0, NULL
    };
    RegValHD[0].Data = (PUCHAR)(psdi->szProfile);
    RegValCD[0].Data = (PUCHAR)(psdi->szProfile);
    RegValFD[0].Data = (PUCHAR)(psdi->szProfile);

    psdi->dwDeviceType = STORAGE_DEVICE_TYPE_REMOVABLE_DRIVE | \
                        STORAGE_DEVICE_TYPE_USB;
    psdi->dwDeviceFlags = STORAGE_DEVICE_FLAG_MEDIASENSE;
    VERIFY(SUCCEEDED(StringCchCopy( psdi->szProfile, _countof(psdi->szProfile), L"Default")));

    if (pDevice->DeviceType == SCSI_DEVICE_CDROM) {
        psdi->dwDeviceClass = STORAGE_DEVICE_CLASS_MULTIMEDIA;
        psdi->dwDeviceType |= STORAGE_DEVICE_TYPE_REMOVABLE_MEDIA;
        psdi->dwDeviceFlags |= STORAGE_DEVICE_FLAG_READONLY;
        if ( !GetSetKeyValues(pDevice->ActivePath,
                              RegValCD,
                              GET,
                              FALSE ) ) {
            VERIFY(SUCCEEDED(StringCchCopy( psdi->szProfile, _countof(psdi->szProfile), L"USBCDProfile")));
        }
    }
    else
    if (pDevice->DeviceType== SCSI_DEVICE_DIRECT_ACCESS && pDevice->DiskSubClass == USBMSC_SUBCLASS_UFI) {  // USB FLOPPY
        psdi->dwDeviceClass = STORAGE_DEVICE_CLASS_BLOCK;
        psdi->dwDeviceType |= STORAGE_DEVICE_TYPE_REMOVABLE_MEDIA;
        if (!pDevice->Flags.WriteProtect)
        {
            psdi->dwDeviceFlags |= STORAGE_DEVICE_FLAG_READWRITE;
        }
        else
        {
            psdi->dwDeviceFlags |= STORAGE_DEVICE_FLAG_READONLY;
        }
        if ( !GetSetKeyValues(pDevice->ActivePath,
                              RegValFD,
                              GET,
                              FALSE ) ) {
            VERIFY(SUCCEEDED(StringCchCopy( psdi->szProfile, _countof(psdi->szProfile), L"USBFDProfile")));
        }
    }
    else
    {
        psdi->dwDeviceClass = STORAGE_DEVICE_CLASS_BLOCK;
        if (!pDevice->Flags.WriteProtect)
        {
            psdi->dwDeviceFlags |= STORAGE_DEVICE_FLAG_READWRITE;
        }
        else
        {
            psdi->dwDeviceFlags |= STORAGE_DEVICE_FLAG_READONLY;
        }

        if ( !GetSetKeyValues(pDevice->ActivePath,
                              RegValHD,
                              GET,
                              FALSE ) ) {

            VERIFY(SUCCEEDED(StringCchCopy( psdi->szProfile, _countof(psdi->szProfile), L"USBHDProfile")));
        }
    }
    return ERROR_SUCCESS;
}

DWORD ScsiPureTestUnitReady(PSCSI_DEVICE pDevice, UCHAR Lun);

//
// Media Change Polling Thread
//
DWORD WINAPI
MediaChangeThread(
    LPVOID Context
    )
{
    PSCSI_DEVICE pDevice = (PSCSI_DEVICE)Context;
    DWORD dwErr, dwReason;
    BOOL bReturn;
    ULONG  ulMediumPresent,ulMediumChanged;

    DEBUGMSG(ZONE_THREAD,(TEXT("USBDISK6>MediaChangeThread\n")));

    if ( !VALID_CONTEXT( pDevice ) ) {
        DEBUGMSG( ZONE_ERR, (TEXT("Invalid Context!\n")));
        ExitThread(ERROR_INVALID_PARAMETER);
        return ERROR_SUCCESS;
    }

    dwErr = AcquireRemoveLock(&pDevice->RemoveLock, NULL) ;
    if ( ERROR_SUCCESS != dwErr) {
        ExitThread(dwErr);
        return ERROR_SUCCESS;
    }

    EnterCriticalSection(&pDevice->Lock);
    pDevice->Flags.prevMediumStatus=pDevice->Flags.MediumPresent;
    LeaveCriticalSection(&pDevice->Lock);

    while (pDevice && !pDevice->Flags.DeviceRemoved && !pDevice->Flags.PoweredDown)
    {
        EnterCriticalSection(&pDevice->Lock);

        //
        // See if the medium changed by polling the device.
        // Some devices hard code the information returned in Inquiry,
        // so use TestUnitReady.
        //
        dwErr = ScsiTestUnitReady(pDevice, pDevice->Lun);
        if (dwErr==ERROR_SUCCESS) { // Media is available.
            pDevice->Flags.MediumPresent=TRUE;
        }
        else {
            pDevice->Flags.MediumPresent=FALSE;
        }

        ulMediumPresent = pDevice->Flags.MediumPresent;
        ulMediumChanged = pDevice->Flags.prevMediumStatus ^ ulMediumPresent;
        // RETAILMSG(1, (L"LUN=%d err=%d present=%d changed=%d\n", pDevice->Lun, dwErr,ulMediumPresent, ulMediumChanged));

        pDevice->Flags.prevMediumStatus = ulMediumPresent;


#ifdef YE_DATA
        //
        // Y-E DATA floppy: TestUnitReady will (incorrectly) not return scsi check condition
        // if the media is removed. ReadCapacity or ModeSense10 keeps the floppy drive motor constantly
        // running, but seems to be the best thing available.
        //
        // TBD: is this true of all UFI devices, or just Y-E Data? May need a dword reg flag
        // set by usbmsc.dll since the scsi driver has no knowledge if VID/PIDs.
        if (USBMSC_SUBCLASS_UFI == pDevice->DiskSubClass)
        {
            DISK_INFO   di = { 0 };
            // YE-DATA data toggle problem?
            ScsiReadCapacity(pDevice, &di, pDevice->Lun);
        }
#endif

        if (ulMediumChanged)
            pDevice->Flags.MediumChanged = TRUE;

        if (ulMediumChanged && pDevice->Flags.MediumPresent) {
            // Try to update the medium info
            dwErr = GetMediumInfo(pDevice, pDevice->Lun);
        }

        //
        // If the Medium has changed then resync the FS, which forces the FS to Dismount, then Remount
        //
        if (ulMediumChanged && pDevice->hStreamDevice) {
            DEBUGMSG(ZONE_THREAD, (TEXT("*** MediaChangeThread::CeResyncFilesys\n")));

            LeaveCriticalSection(&pDevice->Lock);
            bReturn=CeResyncFilesys(pDevice->hStreamDevice);
            EnterCriticalSection(&pDevice->Lock);

            if ( !bReturn ) {
                DEBUGMSG(ZONE_ERR, (TEXT("*** CeResyncFilesys ERROR:%d\n"), GetLastError() ));

                if ( !pDevice->Flags.FSDMounted ) {
                    //
                    // The FSD never loaded. This happens when the user
                    // plugged in the device with no media.
                    //
                    dwErr = MountUpperDriver(pDevice, NULL);
                }
            }
        }

        LeaveCriticalSection(&pDevice->Lock);

        //
        // sleep
        //
        DEBUGMSG(ZONE_EVENTS, (TEXT("*** MediaChangeThread:WaitForSingleObject...\n")));

        dwReason = WaitForSingleObject(pDevice->hMediaChangedEvent, pDevice->Timeouts.MediaPollInterval );

        DEBUGMSG(ZONE_EVENTS, (TEXT("*** ...MediaChangeThread:WaitForSingleObject:%d\n"), dwReason));
    }

    //
    // The device has been removed, Deactivate the Streams Interface & dismount the FSD
    //
    if ( pDevice && pDevice->hStreamDevice ) {
        DEBUGMSG( ZONE_THREAD, (TEXT("*** MediaChangeThread:DismountUpperDriver\n")));

        DismountUpperDriver(pDevice);
    }

    ReleaseRemoveLock(&pDevice->RemoveLock, NULL);

    DEBUGMSG(ZONE_THREAD,(TEXT("USBDISK6<MediaChangeThread\n")));
    ExitThread(ERROR_SUCCESS);

    return ERROR_SUCCESS;
}


VOID
RemoveDeviceContext(
   PSCSI_DEVICE pDevice
   )
{
   DEBUGMSG(ZONE_INIT,(TEXT("USBDISK6>RemoveDeviceContext(%p)\n"), pDevice));

   if ( VALID_CONTEXT( pDevice ) ) {

    ASSERT(!pDevice->hStreamDevice);
    ASSERT(!pDevice->Flags.FSDMounted);
    ASSERT(pDevice->Flags.DeviceRemoved);

    // If Flags.Open is set here then we were suprise removed
    // so the FSD never called DSK_Close.
    //
    //ASSERT(!pDevice->Flags.Open);
    //ASSERT(0 == pDevice->OpenCount);

    if (pDevice->hMediaChangedEvent) {
        CloseHandle(pDevice->hMediaChangedEvent);
    }

    if (pDevice->hMediaPollThread) {
        CloseHandle(pDevice->hMediaPollThread);
        pDevice->hMediaPollThread = NULL;
    }

    if (&pDevice->Lock) {
        DeleteCriticalSection( &pDevice->Lock );
    }

    if (pDevice->ActivePath) {
        LocalFree(pDevice->ActivePath);
        pDevice->ActivePath = NULL;
    }

    //pDevice->hUsbTransport = NULL;

    if (pDevice->pSterileIoRequest) {
        LocalFree(pDevice->pSterileIoRequest);
        pDevice->pSterileIoRequest = NULL;
    }

    LocalFree(pDevice);

   } else {
      DEBUGMSG(ZONE_ERR,(TEXT("Invalid Parameter\n")));
   }

   DEBUGMSG(ZONE_INIT,(TEXT("USBDISK6<RemoveDeviceContext\n")));

   return;
}

// EOF
