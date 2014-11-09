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

    usbmsc.h (public)

Abstract:

    USB Mass Storage Class
        Bulk-Only Transport 1.0
        Control/Bulk/Interrupt (CBI) Transport 1.0

Functions:

Notes: 

--*/

#if !defined( _USBMSC_ )
#define _USBMSC_


//*****************************************************************************
// I N C L U D E S
//*****************************************************************************

#include <windows.h>
#include <diskio.h>

//*****************************************************************************
// D E F I N E S
//*****************************************************************************

#define USBMSC_INTERFACE_CLASS      0x08

#define USBMSC_SUBCLASS_RBC         0x01
#define USBMSC_SUBCLASS_SFF8020i    0x02
#define USBMSC_SUBCLASS_QIC157      0x03
#define USBMSC_SUBCLASS_UFI         0x04
#define USBMSC_SUBCLASS_SFF8070i    0x05
#define USBMSC_SUBCLASS_SCSI        0x06
#define USBMSC_SUBCLASS_RESERVED    0xff

#define USBMSC_INTERFACE_PROTOCOL_CBIT  0x00
#define USBMSC_INTERFACE_PROTOCOL_CBT   0x01
#define USBMSC_INTERFACE_PROTOCOL_BOT   0x50

#define DLL_SZ             TEXT("Dll")
#define DEFAULT_DISK_SZ    TEXT("USBDISK6.DLL")
#define PREFIX_SZ          TEXT("Prefix")
#define DEFAULT_PREFIX_SZ  TEXT("DSK")
#define FSD_SZ             TEXT("FSD")
#define DEFAULT_FSD_SZ     TEXT("FATFS.DLL")
#define FOLDER_SZ          TEXT("Folder")
#define DEFAULT_FOLDER_SZ  TEXT("USB Disk")
#define IOCTL_SZ           TEXT("IOCTL")
#define DEFAULT_IOCTL      DISK_IOCTL_INITIALIZED

#define SET_FLAG(Flags, Bit)    ((Flags) |= (Bit))
#define CLEAR_FLAG(Flags, Bit)  ((Flags) &= ~(Bit))
#define TEST_FLAG(Flags, Bit)   ((Flags) & (Bit))

//
// Event types
//
#define MANUAL_RESET_EVENT TRUE
#define AUTO_RESET_EVENT   FALSE

#define MAX_DLL_LEN     64  // Max length of device driver DLL name

//
// Error codes
//
#define ERROR_PERSISTANT   PERSIST_E_SIZEDEFINITE
//
// Max LUN index
//
#define MAX_LUN     0x7


//*****************************************************************************
// T Y P E D E F S
//*****************************************************************************

//
// Direction Flags
//
#define DATA_OUT 0x00000000
#define DATA_IN  0x00000001

//
// Command Block
//
typedef struct _TRANSPORT_COMMAND {
    DWORD Flags;        // IN - DATA_IN or DATA_OUT
    DWORD Timeout;      // IN - Timeout for this command Block
    DWORD Length;       // IN - Length of the command block buffer
    DWORD dwLun;        // IN - Logical Number for Logic Device.
    PVOID CommandBlock; // IN - Pointer to the command block buffer.
} TRANSPORT_COMMAND, *PTRANSPORT_COMMAND;

//
// Data Block
//
typedef struct _TRANSPORT_DATA_BUFFER {
    DWORD RequestLength;  // IN  - Requested Length
    DWORD TransferLength; // OUT - Returns number of bytes actually transferred 
    PVOID DataBlock;      // IN  - Pointer to the data buffer. May be NULL.
} TRANSPORT_DATA, *PTRANSPORT_DATA;


//*****************************************************************************
//
// F U N C T I O N    P R O T O T Y P E S
//
//*****************************************************************************

/*++

DiskAttach:
    Must be exported by the USB Disk driver.
    This routine is called by the USB Mass Storage Class driver (i.e., Transport)
    when a USB Disk is enumerated according to the device's bInterfaceSubClass.

 hTransport:
    An opaque handle to the USB Transport.

 pHardwareKey:
    Pointer to the hardware registry key.

 bInterfaceSubClass: 
    USB Disk SubClass as reported in the USB descriptor.

 Returns:
    If successful the USB Disk driver returns a pointer to it's
    context used in subsequent UsbDiskXxx calls. Else, NULL

--*/
typedef 
PVOID
(*PUSBDISK_ATTACH)(
   IN HANDLE  hTransport,
   IN LPCWSTR pHardwareKey,
   IN DWORD   dwLun,
   IN UCHAR   bInterfaceSubClass
   );

/*++

DiskDetach: 
   Must be exported by the USB Disk driver.
   This routine is called by the USB Mass Storage Class driver (i.e., Transport)
   when the transport receives the remove DeviceNotify from USBD.

 Context:
   Pointer retruned from DiskAttach.

--*/
typedef 
BOOL
(*PUSBDISK_DETACH)(
   IN PVOID Context
   );


/*++

UsbsDataTransfer:
   Called by the USB Disk driver to place the block command on the USB.
   Used for reads, writes, commands, etc.

 hTransport:
   The Transport handle passed to DiskAttach.

 pCommand:
    Pointer to Command Block for this transaction.

 pData:
    Pointer to Data Block for this transaction. May be NULL.

 Returns:
   Win32 error code.

--*/
DWORD
UsbsDataTransfer(
    HANDLE             hTransport,
    PTRANSPORT_COMMAND pCommand,
    PTRANSPORT_DATA    pData
    );


//
// UsbsGetContextFromReg
//
PVOID
UsbsGetContextFromReg(
    LPCTSTR  ActivePath
   );


//
// Get a DWORD Value from an array of chars pointed to by pbArray
// (Little Endian)
//
DWORD 
GetDWORD(
    __in_ecount(4) BYTE const* pbArray
    );

//
// Set a DWORD Value into an array of chars pointed to by pbArray
// (Little Endian)
//
VOID
SetDWORD(
    __out_ecount(4) PBYTE pbArray,
    IN DWORD dwValue
    );

VOID 
SetWORD(
    __out_ecount(2) PBYTE pBytes, 
    WORD wValue 
    );

#endif // _USBMSC_

// EOF
