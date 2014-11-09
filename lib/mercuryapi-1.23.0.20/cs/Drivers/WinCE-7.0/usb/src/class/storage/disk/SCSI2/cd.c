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

    cd.c

Abstract:

    bInterfaceSubClass 0x02, SFF8020i/ATAPI CD-ROM

Notes:

    CDDA not supported

--*/

#include <windows.h>
#include <pkfuncs.h>
#include "usbmsc.h"
#include "scsi2.h"

DWORD
ScsiCDRead(
    PSCSI_DEVICE pDevice,
    PSG_REQ      pSgReq,
    PULONG       pdwBytesTransferred
    )
{
    DWORD dwErr = ERROR_SUCCESS;

    DEBUGMSG(ZONE_CDROM,(TEXT("USBMSC>ScsiCDRead\r\n")));

    // pSgReq and pdwBytesTransferred are sterile copies provided by
    // DSK_IOControl

    *pdwBytesTransferred = ScsiRWSG(pDevice, pSgReq, pDevice->Lun, TRUE);
    dwErr = pSgReq->sr_status;

    DEBUGMSG(ZONE_CDROM,(TEXT("USBMSC<ScsiCDRead:%d\r\n"),dwErr));

    return dwErr;
}

DWORD 
ScsiCDAudio(
    PSCSI_DEVICE pDevice,
    DWORD        Ioctl,
    PUCHAR       pInBuf,
    DWORD        InBufLen,
    PUCHAR       pOutBuf,
    DWORD        OutBufLen,
    PDWORD       pdwBytesTransferred
    )
{
    TRANSPORT_COMMAND tCommand = {0};
    TRANSPORT_DATA    tData = {0};
    UCHAR             bCDB[MAX_CDB];
    DWORD dwErr;

    DEBUGMSG(ZONE_CDROM,(TEXT("USBDISK6>ScsiCDAudio\r\n")));
    dwErr = AcquireRemoveLock(&pDevice->RemoveLock, NULL);
    if (ERROR_SUCCESS != dwErr) {
        return dwErr;
    }
    tCommand.Flags = DATA_OUT;
    tCommand.Timeout = pDevice->Timeouts.ScsiCommandTimeout;
    tCommand.Length = USBMSC_SUBCLASS_SCSI == pDevice->DiskSubClass ?
                      SCSI_CDB_6 : UFI_CDB;
    tCommand.CommandBlock = bCDB;
    tCommand.dwLun=pDevice->Lun;

    memset( bCDB, 0, sizeof(bCDB));
    ASSERT(pDevice->Lun <= 0x7);
    bCDB[1] = ((pDevice->Lun & 0x7) << 5);

    switch(Ioctl) {
        case IOCTL_CDROM_READ_TOC:
        {
            DEBUGMSG(ZONE_CDROM, (TEXT("IOCTL_CDROM_READ_TOC\r\n")));
            if ( !pOutBuf || OutBufLen < sizeof(CDROM_TOC) ) {
                dwErr = ERROR_INVALID_PARAMETER;
            }
            else {
                tCommand.Flags = DATA_IN;
                tData.TransferLength = 0;
                tData.RequestLength = OutBufLen;
                tData.DataBlock = pOutBuf;
                bCDB[0] = SCSI_CD_READ_TOC;
                bCDB[1] |= 0x2; // use MSF format
                // bCDB[6] = 0; // starting track
                bCDB[7] = (BYTE)((sizeof(CDROM_TOC)>> 8) &0x0FF);
                bCDB[8] = (BYTE)(sizeof(CDROM_TOC) &0x0FF); // 24
            }
        }
        break;
        case IOCTL_CDROM_PLAY_AUDIO:
            {
                PCDROM_READ pCdRead;
                DEBUGMSG(ZONE_CDROM, (TEXT("IOCTL_CDROM_PLAY_AUDIO\r\n")));
                if (!pInBuf || InBufLen < sizeof(CDROM_READ)) {
                    dwErr = ERROR_INVALID_PARAMETER;
                }
                else {
                    pCdRead = (PCDROM_READ)pInBuf;
                    bCDB[0] = SCSI_CD_PLAY10;
                    ASSERT(pCdRead->StartAddr.Mode == CDROM_ADDR_LBA);
                    // Logical Block Address
                    SetDWORD(&bCDB[2], pCdRead->StartAddr.Address.lba);
                    // TransferLength (in sectors)
                    SetWORD(&bCDB[7], (WORD)pCdRead->TransferLength);
                }
            }
            break;
        case IOCTL_CDROM_PLAY_AUDIO_MSF:
            {
                PCDROM_PLAY_AUDIO_MSF pPlayMSF;
                DEBUGMSG(ZONE_CDROM, (TEXT("IOCTL_CDROM_PLAY_AUDIO_MSF\r\n")));
                if (!pInBuf || InBufLen < sizeof(CDROM_PLAY_AUDIO_MSF)) {
                    dwErr = ERROR_INVALID_PARAMETER;
                }
                else {
                    pPlayMSF = (PCDROM_PLAY_AUDIO_MSF)pInBuf;
                    bCDB[0] = SCSI_CD_PLAY_MSF;
                    bCDB[3] = pPlayMSF->StartingM;
                    bCDB[4] = pPlayMSF->StartingS;
                    bCDB[5] = pPlayMSF->StartingF;
                    bCDB[6] = pPlayMSF->EndingM;
                    bCDB[7] = pPlayMSF->EndingS;
                    bCDB[8] = pPlayMSF->EndingF;
                }
            }
            break;
        case IOCTL_CDROM_SEEK_AUDIO_MSF:
            DEBUGMSG(ZONE_ERR,(TEXT("IOCTL_CDROM_SEEK_AUDIO_MSF\r\n")));
            dwErr = ERROR_NOT_SUPPORTED;
            break;
        case IOCTL_CDROM_STOP_AUDIO:
            DEBUGMSG(ZONE_CDROM,(TEXT("IOCTL_CDROM_STOP_AUDIO\r\n")));
            bCDB[0] = SCSI_CD_STOP;
            break;
        case IOCTL_CDROM_PAUSE_AUDIO:
            DEBUGMSG(ZONE_CDROM,(TEXT("IOCTL_CDROM_PAUSE_AUDIO\r\n")));
            bCDB[0] = SCSI_CD_PAUSE_RESUME;
            bCDB[8] = 0;
            break;
        case IOCTL_CDROM_RESUME_AUDIO:
            DEBUGMSG(ZONE_CDROM,(TEXT("IOCTL_CDROM_RESUME_AUDIO\r\n")));
            bCDB[0] = SCSI_CD_PAUSE_RESUME;
            bCDB[8] = 1;
            break;
        default:
            DEBUGMSG(ZONE_ERR,(TEXT("Unsupported CDDA command(0x%x)\r\n"), Ioctl));
            dwErr = ERROR_NOT_SUPPORTED;
            break;
    }
    if (ERROR_SUCCESS == dwErr) {
        dwErr = UsbsDataTransfer(pDevice->hUsbTransport,
                                 &tCommand,
                                 &tData);
        if ( dwErr != ERROR_SUCCESS ) {
            dwErr = ScsiGetSenseData( pDevice, pDevice->Lun );
            if (ERROR_SUCCESS == dwErr) {
                dwErr = ERROR_GEN_FAILURE;
            }
            DEBUGMSG(ZONE_ERR,(TEXT("USBDISK6!ScsiCDAudio: CD-ROM command failed (%d)\r\n"), dwErr));
            SetLastError(dwErr);
        }
        if (tData.RequestLength != tData.TransferLength) {
            DEBUGMSG(ZONE_ERR,(TEXT("USBDISK6!ScsiCDAudio: Transfer length (%d) request length (%d) mismatch\r\n"), tData.TransferLength, tData.RequestLength));
            dwErr = ERROR_GEN_FAILURE;
        }
        __try {
            if (pdwBytesTransferred) {
                *pdwBytesTransferred = tData.TransferLength;
            }
        }
        __except(EXCEPTION_EXECUTE_HANDLER) {
            dwErr = ERROR_INVALID_PARAMETER;
        }
    }

    ReleaseRemoveLock(&pDevice->RemoveLock, NULL);
    DEBUGMSG(ZONE_CDROM,(TEXT("USBDISK6<ScsiCDAudio:%d\r\n"), dwErr));
    return dwErr;
}
