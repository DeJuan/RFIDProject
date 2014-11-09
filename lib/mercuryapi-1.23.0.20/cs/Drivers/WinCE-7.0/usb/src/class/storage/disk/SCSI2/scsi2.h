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

    scsi2.h

Abstract:

    SCSI-2 Definitions

    bInterfaceSubClass 0x02, SFF8020i/ATAPI CD-ROM
    bInterfaceSubClass 0x04, USB Floppy Interface (UFI)
    bInterfaceSubClass 0x06, SCSI Passthrough

Notes:

    CDDA not supported

--*/

#if !defined( _SCSI2_ )
#define _SCSI2_

//*****************************************************************************
// I N C L U D E S
//*****************************************************************************

#include <windows.h>
#include <winioctl.h>
#include <diskio.h>
#include <usbclient.h>

#pragma warning(push)
#pragma warning(disable:4201) // nonstandard extension used : nameless struct/union
#pragma warning(disable:4214) // nonstandard extension used : bit field types other than int

#include "cdioctl.h"

//*****************************************************************************
// D E F I N E S
//*****************************************************************************

#define DRIVER_NAME_SZ  TEXT("USBDISK6.DLL")
#define USBSCSI_SIG     'SCSI'

// timeout values (milliseconds)
#define MEDIA_POLL_SZ               TEXT("MediaPollInterval")
#define SCSI_MEDIA_POLL_INTERVAL    1250
#define READ_SECTOR_SZ              TEXT("ReadSectorTimeout")
#define SCSI_READ_SECTOR_TIMEOUT    10000
#define WRITE_SECTOR_SZ             TEXT("WriteSectorTimeout")
#define SCSI_WRITE_SECTOR_TIMEOUT   10000
#define SCSI_COMMAND_SZ             TEXT("ScsiCommandTimeout")
#define SCSI_COMMAND_TIMEOUT        5000

// TEST UNIT READY timeout
#define UNIT_ATTENTION_SZ           TEXT("UnitAttnRepeat")
#define UNIT_ATTENTION_REPEAT       10

// sizes
#define FAT_SECTOR_SIZE             512
#define CDSECTOR_SIZE               2048
#define MAX_SECTOR_SIZE             CDSECTOR_SIZE
#define SG_BUFF_SIZE                max((MAX_SG_BUF*FAT_SECTOR_SIZE), (3*CDSECTOR_SIZE))
// #define SG_BUFF_SIZE                65536
#define SCSI_CDB_6                  6
#define SCSI_CDB_10                 10
#define UFI_CDB                     12
#define ATAPI_CDB                   12
#define MAX_CDB                     UFI_CDB

// Minimum expected bandwidth is 10 KB/s;
// This value will be used to compute conservative timeouts for I/O calls

#define USBDISK_LEASTREADBANDWIDTH  10
#define USBDISK_LEASTWRITEBANDWIDTH 10

#define VALID_CONTEXT( p ) \
    ( p != NULL && USBSCSI_SIG == p->Sig )

#define ACCEPT_IO( p ) \
   ( VALID_CONTEXT( p ) && \
     p->Flags.Open && \
    !p->Flags.DeviceRemoved  && \
    !p->Flags.PoweredDown )

// SCSI-2 device types
#define SCSI_DEVICE_DIRECT_ACCESS     0x00
#define SCSI_DEVICE_SEQUENTIAL_ACCESS 0x01
#define SCSI_DEVICE_PRINTER           0x02
#define SCSI_DEVICE_PROCESSOR         0x03
#define SCSI_DEVICE_WRITE_ONCE        0x04
#define SCSI_DEVICE_CDROM             0x05
#define SCSI_DEVICE_SCANNER           0x06
#define SCSI_DEVICE_OPTICAL           0x07
#define SCSI_DEVICE_MEDIUM_CHANGER    0x08
#define SCSI_DEVICE_COMM              0x09
#define SCSI_DEVICE_UNKNOWN           0x1F

// SCSI-2 medium types
#define SCSI_MEDIUM_UNKNOWN           0xFF

// CD-ROM medium types
#define MEDIUM_CD_ROM_UNKNOWN     0x00
#define MEDIUM_CD_ROM_120         0x01
#define MEDIUM_CD_DA_120          0x02
#define MEDIUM_CD_MIXED_120       0x03
#define MEDIUM_CD_HYBRID_120      0x04
#define MEDIUM_CD_ROM_80          0x05
#define MEDIUM_CD_DA_80           0x06
#define MEDIUM_CD_MIXED_80        0x07
#define MEDIUM_CD_HYBRID_80       0x08
#define MEDIUM_CDR_ROM_UNKNOWN    0x10
#define MEDIUM_CDR_ROM_120        0x11
#define MEDIUM_CDR_DA_120         0x12
#define MEDIUM_CDR_MIXED_120      0x13
#define MEDIUM_CDR_HYBRID_120     0x14
#define MEDIUM_CDR_ROM_80         0x15
#define MEDIUM_CDR_DA_80          0x16
#define MEDIUM_CDR_MIXED_80       0x17

// mandatory SCSI-2 commands
#define SCSI_TEST_UNIT_READY      0x00
#define SCSI_REQUEST_SENSE        0x03
#define SCSI_INQUIRY              0x12
#define SCSI_SEND_DIAGNOSTIC      0x1D

// mandatory device-specific SCSI-2 commands
#define SCSI_READ10               0x28
#define SCSI_READ_CAPACITY        0x25

// optional device-specific SCSI-2 commands
#define SCSI_MODE_SELECT6         0x15
#define SCSI_MODE_SENSE6          0x1A
#define SCSI_START_STOP           0x1B
#define SCSI_WRITE10              0x2A
#define SCSI_MODE_SELECT10        0x55
#define SCSI_MODE_SENSE10         0x5A

// ATAPI (CD-ROM) commands
#define SCSI_CD_READ_TOC          0x43
#define SCSI_CD_PLAY10            0x45
#define SCSI_CD_PLAY_MSF          0x47
#define SCSI_CD_PAUSE_RESUME      0x4B
#define SCSI_CD_STOP              0x4E

// mode pages
#define MODE_PAGE_FLEXIBLE_DISK   0x05
#define MODE_PAGE_CDROM           0x0D
#define MODE_PAGE_CDROM_AUDIO     0x0E
#define MODE_PAGE_CDROM_CAPS      0x2A

// SCSI-2 sense keys
#define SENSE_NONE                0x00
#define SENSE_RECOVERED_ERROR     0x01
#define SENSE_NOT_READY           0x02
#define SENSE_MEDIUM_ERROR        0x03
#define SENSE_HARDWARE_ERROR      0x04
#define SENSE_ILLEGAL_REQUEST     0x05
#define SENSE_UNIT_ATTENTION      0x06
#define SENSE_DATA_PROTECT        0x07
#define SENSE_BLANK_CHECK         0x08

// SCSI-2 ASC
#define ASC_LUN                   0x04
#define ASC_INVALID_COMMAND_FIELD 0x24
#define ASC_MEDIA_CHANGED         0x28
#define ASC_RESET                 0x29
#define ASC_COMMANDS_CLEARED      0x2F
#define ASC_MEDIUM_NOT_PRESENT    0x3A

#define START                     TRUE
#define STOP                      FALSE

#define FAILURE (-1)

// SCSI passthrough IOCTL-s
#define IOCTL_SCSI_BASE_USB FILE_DEVICE_MASS_STORAGE

#define IOCTL_SCSI_PASSTHROUGH                \
    CTL_CODE(IOCTL_SCSI_BASE_USB, 0x700, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_CDROM_PLAY_AUDIO                \
    CTL_CODE(IOCTL_SCSI_BASE_USB, 0x701, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_CDROM_PLAY_AUDIO_TRACK_INDEX    \
    CTL_CODE(IOCTL_SCSI_BASE_USB, 0x702, METHOD_BUFFERED, FILE_ANY_ACCESS)

//*****************************************************************************
// T Y P E D E F S
//*****************************************************************************

// registry configurable timeout/repeat values
typedef struct _TIMEOUTS {
    ULONG MediaPollInterval;
    ULONG ReadSector;
    ULONG WriteSector;
    ULONG ScsiCommandTimeout;
    ULONG UnitAttnRepeat;
} TIMEOUTS, *PTIMEOUTS;

#pragma warning(push)
#pragma warning(disable:4214) // Disable warnings for unsigned bit fields

// device flags
typedef struct _DEVICE_FLAGS {
    ULONG RMB : 1;              // removable media bit (RMB)
    ULONG MediumPresent : 1;    // a medium is present
    ULONG FSDMounted : 1;       // is FSD mounted, deprecated
    ULONG WriteProtect : 1;     // medium/device is write-protected
    ULONG Open : 1;             // device is open (DSK_Xxx)
    ULONG DeviceRemoved : 1;    // device marked for removal; only set by UsbDiskAttach/UsbDiskDetach
    ULONG PoweredDown : 1;      // device marked for suspend; only set by DSK_PowerDown
    ULONG Busy : 1;             // device marked as busy (CD-ROM)
    ULONG MediumChanged : 1;    // medium has changed
    ULONG prevMediumStatus : 1; // previous medium changed status
} DEVICE_FLAGS, *PDEVICE_FLAGS;

#pragma warning(pop)

// device abstraction
typedef struct _SCSI_DEVICE {
    ULONG            Sig;                  // signature
    UCHAR            DiskSubClass;         // bInterfaceSubClass from device descriptor (fr. UsbDiskAttach)
    LPWSTR           ActivePath;           // active registry path (fr. UsbDiskAttach)
    HANDLE           hUsbTransport;        // USB Mass Storage Transport object (BOT, CBIT) (fr. UsbDiskAttach)
    CRITICAL_SECTION Lock;                 // object lock
    HANDLE           hMediaPollThread;     // media polling thread (only used with UFI)
    HANDLE           hMediaChangedEvent;   // media change event
    HANDLE           hStreamDevice;        // associated streams interface (we call ActivateDevice on ActivePath)
    UCHAR            DeviceType;           // SCSI-2 device type
    UCHAR            Lun;                  // logical unit number
    UCHAR            MediumType;           // SCSI-2 medium type
    LONG             MediaChangeCount;     // Keeps track of the media change count
    SENSE_DATA       SenseData;            // The last sense data returned from the device
    DEVICE_FLAGS     Flags;                // device flags (characteristics and state)
    LONG             OpenCount;            // DSK_Open reference count
    TIMEOUTS         Timeouts;             // timeout/repeat values
    DISK_INFO        DiskInfo;             // disk geometry
    REMOVE_LOCK      RemoveLock;           // prevent medium from being removed
    PSG_REQ          pSterileIoRequest;    // safe copy of caller's SG_REQ
    UCHAR            SgBuff[SG_BUFF_SIZE]; // double buffer
} SCSI_DEVICE, *PSCSI_DEVICE;

//*****************************************************************************
// P R O T O T Y P E S
//*****************************************************************************

DWORD
ScsiGetSenseData(
    PSCSI_DEVICE pDevice,
    UCHAR        Lun
    );

DWORD
ScsiInquiry(
    PSCSI_DEVICE pDevice,
    UCHAR        Lun
    );

DWORD
ScsiModeSense10(
    PSCSI_DEVICE pDevice,
    UCHAR        Lun
    );

DWORD
ScsiModeSense6(
    PSCSI_DEVICE pDevice,
    UCHAR        Lun
    );

DWORD
ScsiPassthrough(
    PSCSI_DEVICE       pDevice,
    PTRANSPORT_COMMAND Command,
    PTRANSPORT_DATA    Data
    );

DWORD
ScsiReadCapacity(
    PSCSI_DEVICE pDevice,
    PDISK_INFO   pDiskInfo,
    UCHAR        Lun
    );

DWORD
ScsiReadWrite(
    IN PSCSI_DEVICE pDevice,
    IN DWORD        dwStartSector,
    IN DWORD        dwNumSectors,
    IN OUT PVOID    pvBuffer,
    IN OUT PDWORD   pdwTransferSize,
    IN UCHAR        Lun,
    IN BOOL         bRead
    );

DWORD
ScsiRWSG(
    PSCSI_DEVICE pDevice,
    PSG_REQ      pSgReq,
    UCHAR        Lun,
    BOOL         bRead
    );

DWORD
ScsiRequestSense(
    PSCSI_DEVICE    pDevice,
    PTRANSPORT_DATA pTData,
    UCHAR           Lun
    );

DWORD
ScsiRequestSenseSimple(
    PSCSI_DEVICE pDevice,
    SENSE_DATA*  pSenseData );

DWORD
UpdateStatusAndGetWin32Error(
    PSCSI_DEVICE pDevice,
    PSENSE_DATA pSenseData
    );

DWORD
ScsiSendDiagnostic(
    PSCSI_DEVICE pDevice,
    UCHAR        Lun
    );

DWORD
ScsiStartStopUnit(
    PSCSI_DEVICE pDevice,
    BOOL         Start,
    BOOL         LoEj,
    UCHAR        Lun
    );

DWORD
ScsiTestUnitReady(
    PSCSI_DEVICE pDevice,
    UCHAR        Lun
    );

DWORD
ScsiUnitAttention(
    PSCSI_DEVICE pDevice,
    UCHAR        Lun
    );

DWORD
ScsiCDRead(
    PSCSI_DEVICE pDevice,
    PSG_REQ      pSgReq,
    PULONG       pdwBytesTransferred
    );

DWORD
ScsiCDAudio(
    PSCSI_DEVICE pDevice,
    DWORD        Ioctl,
    PUCHAR       pInBuf,
    DWORD        InBufLen,
    PUCHAR       pOutBuf,
    DWORD        OutBufLen,
    PDWORD       pdwBytesTransferred
    );

//*****************************************************************************
// D E B U G
//*****************************************************************************

#if DEBUG
    #define ZONEID_ERR          0
    #define ZONEID_WARN         1
    #define ZONEID_INIT         2
    #define ZONEID_TRACE        3
    #define ZONEID_SCSI         4
    #define ZONEID_CDROM        5
    #define ZONEID_CDDA         6
    #define ZONEID_DSK_IOCTL    7
    #define ZONEID_READ         8
    #define ZONEID_WRITE        9
    #define ZONEID_THREAD       10
    #define ZONEID_EVENTS       11
    #define ZONEID_SENSEDATA    12

    #define ZONE_ERR       DEBUGZONE(ZONEID_ERR)
    #define ZONE_WARN      DEBUGZONE(ZONEID_WARN)
    #define ZONE_INIT      DEBUGZONE(ZONEID_INIT)
    #define ZONE_TRACE     DEBUGZONE(ZONEID_TRACE)

    #define ZONE_SCSI      DEBUGZONE(ZONEID_SCSI)
    #define ZONE_CDROM     DEBUGZONE(ZONEID_CDROM)
    #define ZONE_CDDA      DEBUGZONE(ZONEID_CDDA)
    #define ZONE_DSK_IOCTL DEBUGZONE(ZONEID_DSK_IOCTL)

    #define ZONE_READ      DEBUGZONE(ZONEID_READ)
    #define ZONE_WRITE     DEBUGZONE(ZONEID_WRITE)
    #define ZONE_THREAD    DEBUGZONE(ZONEID_THREAD)
    #define ZONE_EVENTS    DEBUGZONE(ZONEID_EVENTS)

    #define ZONE_SENSEDATA DEBUGZONE(ZONEID_SENSEDATA)

    #define ZONEMASK_ERR            ( 1 << ZONEID_ERR )
    #define ZONEMASK_WARN           ( 1 << ZONEID_WARN )
    #define ZONEMASK_INIT           ( 1 << ZONEID_INIT )
    #define ZONEMASK_TRACE          ( 1 << ZONEID_TRACE )
    #define ZONEMASK_SCSI           ( 1 << ZONEID_SCSI )
    #define ZONEMASK_CDROM          ( 1 << ZONEID_CDROM )
    #define ZONEMASK_CDDA           ( 1 << ZONEID_CDDA )
    #define ZONEMASK_DSK_IOCTL      ( 1 << ZONEID_DSK_IOCTL )   -----------------++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++----
    #define ZONEMASK_READ           ( 1 << ZONEID_READ )
    #define ZONEMASK_WRITE          ( 1 << ZONEID_WRITE )
    #define ZONEMASK_THREAD         ( 1 << ZONEID_THREAD )
    #define ZONEMASK_EVENTS         ( 1 << ZONEID_EVENTS )
    #define ZONEMASK_SENSEDATA      ( 1 << ZONEID_SENSEDATA )

    #define TEST_TRAP() { \
        NKDbgPrintfW( TEXT("%s: TRAP %s:%d\r\n"), DRIVER_NAME_SZ, TEXT(__FILE__), __LINE__); \
        DebugBreak(); \
    }

    #define DUMP_DISK_INFO( pDI ) { \
        if (pDI) { \
            DEBUGMSG( ZONE_DSK_IOCTL, (TEXT("DISK_INFO:\n"))); \
            DEBUGMSG( ZONE_DSK_IOCTL, (TEXT("---------------------\n"))); \
            DEBUGMSG( ZONE_DSK_IOCTL, (TEXT("di_total_sectors: %u\n"), pDI->di_total_sectors )); \
            DEBUGMSG( ZONE_DSK_IOCTL, (TEXT("di_bytes_per_sect: %u\n"), pDI->di_bytes_per_sect )); \
            DEBUGMSG( ZONE_DSK_IOCTL, (TEXT("di_cylinders: %u\n"), pDI->di_cylinders )); \
            DEBUGMSG( ZONE_DSK_IOCTL, (TEXT("di_heads: %u\n"), pDI->di_heads )); \
            DEBUGMSG( ZONE_DSK_IOCTL, (TEXT("di_sectors: %u\n"), pDI->di_sectors )); \
            DEBUGMSG( ZONE_DSK_IOCTL, (TEXT("di_flags: 0x%x\n"), pDI->di_flags )); \
            DEBUGMSG( ZONE_DSK_IOCTL, (TEXT("\n"))); \
        } \
    }
#else
    #define TEST_TRAP()
    #define DUMP_DISK_INFO(pDI)
#endif // DEBUG

#pragma warning(pop) // un-sets any local warning changes

#endif // _SCSI2_
