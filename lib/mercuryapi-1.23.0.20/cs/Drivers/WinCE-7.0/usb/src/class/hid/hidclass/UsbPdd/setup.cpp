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

    setup.cpp

Abstract:  
    USB Client Driver for Human Interface Device (HID) Class.

Functions:

Notes: 

--*/


#include "usbhid.h"


// Structure for queueing multiple USB transfers
struct USB_HID_TRANSFER {
    HANDLE hev; // Event signaled when transfer is complete
    USB_TRANSFER hTransfer; // Handle to the transfer
    PBYTE rgbBuffer; // Buffer for the HID report
};
typedef USB_HID_TRANSFER *PUSB_HID_TRANSFER;


#ifdef DEBUG
#define GARBAGE_BYTE   (0xCC)
#define GARBAGE_PVOID  ( (PVOID) 0xCCCCCCCC )
#endif

// The number of simultaneous interrupt transfers to issue
static BYTE g_bQueuedTransfers = DEFAULT_QUEUED_TRANSFERS;


// Helper function to close a pipe structure
static
void
ClosePipe(
    LPCUSB_FUNCS pUsbFuncs,
    PPIPE pPipe
    )
{
    PREFAST_DEBUGCHK(pUsbFuncs != NULL);
    PREFAST_DEBUGCHK(pPipe != NULL);

    if (pPipe->hPipe) {
        pUsbFuncs->lpClosePipe(pPipe->hPipe);
    }

    if (pPipe->hEvent) {
        CloseHandle(pPipe->hEvent);
    }
}


// Frees all device handles and removes it from the global list.
void
RemoveDeviceContext(
   PUSBHID_CONTEXT pUsbHid
   )
{
    SETFNAME(_T("RemoveDeviceContext"));
  
    DEBUGCHK(pUsbHid != NULL);
    
    if (VALID_CONTEXT(pUsbHid)) {
        if (pUsbHid->Flags.MddInitialized == TRUE) {
            // Tell the MDD about disconnect.
            // Do this before freeing resources since the MDD could currently
            // be in the process of calling us.
            HidMdd_Notifications(HID_MDD_CLOSE_DEVICE, 0, pUsbHid->pvNotifyParameter);
        }        

        if (pUsbHid->InterruptIn.hPipe != NULL) {
            pUsbHid->pUsbFuncs->lpAbortPipeTransfers(pUsbHid->InterruptIn.hPipe, 0);
        }

        ClosePipe(pUsbHid->pUsbFuncs, &pUsbHid->InterruptIn);

        if (pUsbHid->hEP0Event != NULL) CloseHandle(pUsbHid->hEP0Event);
        
        DeleteCriticalSection(&pUsbHid->csLock);

        if (pUsbHid->hHidDriver) {
            DeactivateDevice(pUsbHid->hHidDriver);
        }

        HANDLE hThread = pUsbHid->hThread;

        pUsbHid->Flags.UnloadPending = TRUE;

        // If we have created thread, don't wait INFINITE for the thread to exit here since 
        // HCD call this function when holding HCD lock, and the HID thread need require HCD lock.
        // If wait INFINITE here, deadlock can happen.
        // So if we have create thread, free the resources in the thread.
        if (hThread != NULL) {
            if (WAIT_TIMEOUT == WaitForSingleObject(pUsbHid->hThread, 2000))
            {
                DEBUGMSG(ZONE_WARNING, (_T("%s: WaitForSingleObject timeout\r\n"), pszFname));
            }
            CloseHandle(hThread);
        }
        else
        {
            FreeLibrary(pUsbHid->hModSelf);
        }

        if (0 == InterlockedDecrement(&pUsbHid->dwRefCnt))
        {
            HidFree(pUsbHid);
        }
    } 
    else {
        DEBUGMSG(ZONE_ERROR, (_T("%s: Invalid Parameter\r\n"), pszFname));
        DEBUGCHK(FALSE);
    }
}


// Returns the interface to use.
LPCUSB_INTERFACE
ParseUsbDescriptors(
    LPCUSB_INTERFACE pCurrInterface
    )
{
    SETFNAME(_T("ParseUsbDescriptors"));

    LPCUSB_INTERFACE pHidInterface = NULL;

    DEBUGMSG(ZONE_INIT, (_T("+%s"), pszFname));

    DEBUGCHK(pCurrInterface != NULL);
    
    if (pCurrInterface != NULL) 
    {        
        // If loaded as an interface driver, ignore non HID interfaces
        if (pCurrInterface->Descriptor.bInterfaceClass != USB_DEVICE_CLASS_HUMAN_INTERFACE) {
            DEBUGMSG(ZONE_INIT, (_T("%s: DeviceAttach, ignoring non HID interface class %u\r\n"),
                pszFname, pCurrInterface->Descriptor.bInterfaceClass));
        }
        else {
            pHidInterface = pCurrInterface;
        }
    }

    DEBUGMSG(ZONE_INIT, (_T("-%s"), pszFname));
    return pHidInterface;
}


// Get the report descriptor from the device.
static
BOOL
GetReportDescriptor(
    PUSBHID_CONTEXT pUsbHid,
    PBYTE pbBuffer,
    WORD cbBuffer // This should be the exact size of the descriptor
    )
{
    SETFNAME(_T("GetReportDescriptor"));

    BOOL fRet = FALSE;
    USB_DEVICE_REQUEST udr;
    DWORD dwBytesTransferred;
    DWORD dwUsbErr;
    DWORD dwErr;

    PREFAST_DEBUGCHK(pUsbHid != NULL);
    DEBUGCHK(cbBuffer > 0);

    // Do we send this to the endpoint or the interface?
    DetermineDestination(pUsbHid, &udr.bmRequestType, &udr.wIndex);
    udr.bmRequestType |= USB_REQUEST_DEVICE_TO_HOST;

    udr.bRequest = USB_REQUEST_GET_DESCRIPTOR;
    udr.wValue   = USB_DESCRIPTOR_MAKE_TYPE_AND_INDEX(HID_REPORT_DESCRIPTOR_TYPE, 0);
    udr.wLength  = cbBuffer;

    dwErr = IssueVendorTransfer(
        pUsbHid->pUsbFuncs,
        pUsbHid->hUsbDevice,
        NULL, 
        NULL,
        (USB_IN_TRANSFER | USB_SHORT_TRANSFER_OK),
        &udr,
        pbBuffer,
        0,
        &dwBytesTransferred,
        0,
        &dwUsbErr);

    if ( (ERROR_SUCCESS != dwErr) || (USB_NO_ERROR != dwUsbErr) || 
         (dwBytesTransferred != cbBuffer) ) 
    {
        DEBUGMSG(ZONE_ERROR, (_T("%s: IssueVendorTransfer ERROR:%d 0x%x\n"), 
            pszFname, dwErr, dwUsbErr));
        goto EXIT;
    }
  
    fRet = TRUE;
    
EXIT:
    return fRet;
}


// Setup the pipes for this interface
static
BOOL
SetUsbInterface(
   PUSBHID_CONTEXT pUsbHid
   )
{
    SETFNAME(_T("SetUsbInterface"));

    BOOL fRet = FALSE;
    DWORD dwIndex;
    BYTE bNumEndpoints;

    PREFAST_DEBUGCHK(pUsbHid != NULL);
    DEBUGCHK(pUsbHid->pUsbInterface != NULL);

    // Parse the endpoints
    bNumEndpoints = pUsbHid->pUsbInterface->Descriptor.bNumEndpoints;
    for (dwIndex = 0; dwIndex < bNumEndpoints; ++dwIndex) 
    {
        LPCUSB_ENDPOINT pEndpoint;
        pEndpoint = pUsbHid->pUsbInterface->lpEndpoints + dwIndex;
        PREFAST_DEBUGCHK(pEndpoint != NULL);
        USB_ENDPOINT_DESCRIPTOR const * pEPDesc = &pEndpoint->Descriptor;

        DUMP_USB_ENDPOINT_DESCRIPTOR((*pEPDesc));

        // 
        // HID Class supports 1 mandatory Interrupt IN, and 1 optional Interrupt OUT
        // 
        if (USB_ENDPOINT_DIRECTION_IN(pEndpoint->Descriptor.bEndpointAddress))
        {            
            if ( (NULL == pUsbHid->InterruptIn.hPipe) &&
               ( (pEPDesc->bmAttributes & USB_ENDPOINT_TYPE_MASK) != 0) )
            {
                // create the Interrupt In pipe
                pUsbHid->InterruptIn.hPipe = 
                    pUsbHid->pUsbFuncs->lpOpenPipe(pUsbHid->hUsbDevice, pEPDesc);
                if (pUsbHid->InterruptIn.hPipe == NULL) {
                    DEBUGMSG(ZONE_ERROR, (_T("%s: OpenPipe error:%d\r\n"), 
                        pszFname, GetLastError()));
                    goto EXIT;
                }

                // setup any endpoint specific timers, buffers, context, etc.
                pUsbHid->InterruptIn.hEvent = CreateEvent(NULL, 
                    MANUAL_RESET_EVENT, FALSE, NULL);
                if (pUsbHid->InterruptIn.hEvent == NULL) {
                    DEBUGMSG(ZONE_ERROR, (_T("%s: CreateEvent error:%d\r\n"), 
                        pszFname, GetLastError()));
                    goto EXIT;
                }

                pUsbHid->InterruptIn.bIndex         = pEPDesc->bEndpointAddress;
                pUsbHid->InterruptIn.wMaxPacketSize = pEPDesc->wMaxPacketSize;
            }
            else {
                // The HID spec does not allow for this condition. 
                // You should not get here!
                DEBUGCHK(FALSE); 
            }
        } 
        else if (USB_ENDPOINT_DIRECTION_OUT(pEPDesc->bEndpointAddress)) 
        {
            DEBUGMSG(ZONE_WARNING, 
                (_T("%s: USB HID Driver does not support optional Interrupt Out\r\n"),
                pszFname));      
        }
        else {
            // The HID spec does not allow for this condition. 
            // You should not get here!
            DEBUGCHK(FALSE); 
        }
    }

    // Did we find our endpoint?
    if (pUsbHid->InterruptIn.hPipe == NULL) {
        DEBUGMSG(ZONE_ERROR, (_T("%s: No Interrupt In endpoint\r\n"), pszFname));
        goto EXIT;
    }

    // If we failed to find all of our endpoints then cleanup will occur later

    fRet = TRUE;

EXIT:
    return fRet;
}


// Set the device protocol to report (not boot)
static
BOOL
SetReportProtocol(
    PUSBHID_CONTEXT pUsbHid
    )
{
    SETFNAME(_T("SetReportProtocol"));
    
    DWORD dwErr, dwBytesTransferred;
    USB_ERROR usbErr;
    USB_DEVICE_REQUEST udr;
    
    PREFAST_DEBUGCHK(pUsbHid != NULL);
    ValidateUsbHidContext(pUsbHid);
    DEBUGCHK(pUsbHid->pUsbInterface->Descriptor.bInterfaceSubClass == 
        HID_INTERFACE_BOOT_SUBCLASS);

    // Do we send this to the endpoint or the interface?
    DetermineDestination(pUsbHid, &udr.bmRequestType, &udr.wIndex);
    udr.bmRequestType |= USB_REQUEST_HOST_TO_DEVICE | USB_REQUEST_CLASS;

    udr.bRequest = USB_REQUEST_HID_SET_PROTOCOL;
    udr.wValue   = HID_REPORT_PROTOCOL;
    udr.wLength  = 0;

    dwErr = IssueVendorTransfer(
        pUsbHid->pUsbFuncs,
        pUsbHid->hUsbDevice,
        NULL, 
        NULL,
        USB_OUT_TRANSFER,
        &udr,
        NULL,
        0,
        &dwBytesTransferred,
        0,
        &usbErr);

    if ((dwErr != ERROR_SUCCESS) || (usbErr != USB_NO_ERROR) ) {
        DEBUGMSG(ZONE_ERROR, (_T("%s: Set report protocol error: 0x%x 0x%x\r\n"), 
            pszFname, dwErr, usbErr));
    }

    return (dwErr == ERROR_SUCCESS);
}


// Sets the number of interrupt transfers to queue at one time
static
void
SetQueuedTransfers(
    LPCTSTR pszReg
    )
{
    SETFNAME(_T("SetQueuedTransfers"));
    
    DEBUGCHK(pszReg);
    DEBUGCHK(MAX_QUEUED_TRANSFERS < UCHAR_MAX);

    HKEY hkey = NULL;
    
    LONG iErr = RegOpenKeyEx(HKEY_LOCAL_MACHINE, pszReg, NULL, NULL, &hkey);
    
    if (iErr == ERROR_SUCCESS) {
        DWORD dwType;
        DWORD dwValue;
        DWORD cbValue = sizeof(dwValue);
        
        iErr = RegQueryValueEx(hkey, QUEUED_TRANSFERS_SZ, NULL, 
            &dwType, (PBYTE) &dwValue, &cbValue);

        if ( (iErr == ERROR_SUCCESS) && (dwType == REG_DWORD) ) {
            dwValue = max(MIN_QUEUED_TRANSFERS, dwValue);
            dwValue = min(MAX_QUEUED_TRANSFERS, dwValue);

            DEBUGMSG(ZONE_COMMENT, (_T("%s: Setting to %u queued USB transfers\r\n"), 
                pszFname, dwValue));
            g_bQueuedTransfers = (BYTE) dwValue;
        }
    }
    RegCloseKey(hkey);
}


// Creates and returns the HID device context.
// Returns NULL if error.
PUSBHID_CONTEXT
CreateUsbHidDevice(
    USB_HANDLE hUsbDevice,
    PCUSB_FUNCS pUsbFuncs,
    PCUSB_INTERFACE pUsbInterface
    )
{
    SETFNAME(_T("CreateHidInterface"));
    PUSBHID_CONTEXT pUsbHid = NULL;
    BOOL fErr = TRUE;
    PCUSB_DEVICE pDeviceInfo = NULL;
    PHID_DESCRIPTOR pHidDescriptor = NULL;
    PCUSB_ENDPOINT pEndpoint = NULL;
    PBYTE pbReportDescriptor = NULL;
    WORD  cbReportDescriptor;

    DEBUGMSG(ZONE_INIT, (_T("+%s\r\n"), pszFname));

    PREFAST_DEBUGCHK(pUsbFuncs != NULL);
    PREFAST_DEBUGCHK(pUsbInterface != NULL);

    pDeviceInfo = (*pUsbFuncs->lpGetDeviceInfo)(hUsbDevice);
    if (pDeviceInfo == NULL) {
        DEBUGMSG(ZONE_ERROR, (_T("%s: Failure getting USB device info\r\n"), 
            pszFname));
        goto EXIT;
    }

    DUMP_USB_DEVICE_DESCRIPTOR(pDeviceInfo->Descriptor);
    DUMP_USB_CONFIGURATION_DESCRIPTOR(pDeviceInfo->lpActiveConfig->Descriptor);

    pUsbHid = (PUSBHID_CONTEXT)HidAlloc(sizeof(USBHID_CONTEXT));
    if (pUsbHid == NULL) {
        DEBUGMSG(ZONE_ERROR, (_T("%s: LocalAlloc error:%d\r\n"), pszFname,
            GetLastError()));
        goto EXIT;
    }
    ZeroMemory(pUsbHid, sizeof(USBHID_CONTEXT));

    pUsbHid->Sig = USB_HID_SIG;
    InitializeCriticalSection(&pUsbHid->csLock);
    pUsbHid->hUsbDevice = hUsbDevice;
    pUsbHid->pUsbInterface = pUsbInterface;
    pUsbHid->dwRefCnt = 1;

    DUMP_USB_INTERFACE_DESCRIPTOR(pUsbHid->pUsbInterface->Descriptor, 
        pUsbHid->pUsbInterface->Descriptor.bInterfaceNumber);

    pUsbHid->hHidDriver = ActivateDeviceEx(HID_REGKEY_SZ, NULL, 0, CLIENT_REGKEY_SZ);
    if (pUsbHid->hHidDriver == NULL) {
        DEBUGMSG(ZONE_ERROR,(_T("%s: Unable to activate HID stream driver\r\n"), 
            pszFname));
        goto EXIT;
    }

    if (pUsbInterface->lpEndpoints == NULL) {
        DEBUGMSG(ZONE_ERROR,(_T("%s: Missing endpoint descriptors\r\n"), 
            pszFname));
        goto EXIT;
    }
    pEndpoint = &pUsbInterface->lpEndpoints[0];
    DEBUGCHK(pEndpoint != NULL);
    
    // Regarding bSendToInterface.  The original HID spec said that the Hid
    // descriptor would come after the interface and endpoint descriptors.
    // It also said that class specific commands should be sent to the endpoint.
    // The next spec said that the HID descriptor would come after the interface
    // descriptor (not at the end) and that commands should be sent to the
    // interface, not to the endpoint.  So, I'm assuming that if I find the
    // Hid descriptor after the interface, the device is following the new spec
    // and I should send commands to the interface.  Otherwise, I'll send them
    // to the endpoint, as stated in the old spec.
    pHidDescriptor = (PHID_DESCRIPTOR) pUsbInterface->lpvExtended;
    
    if (pHidDescriptor == NULL) {
        pHidDescriptor = (PHID_DESCRIPTOR) pUsbInterface->lpEndpoints->lpvExtended;
        if (pHidDescriptor == NULL) {
            DEBUGMSG(ZONE_ERROR, (_T("%s: Missing HID descriptor\r\n"), pszFname));
            goto EXIT;
        }
        else {
            pUsbHid->fSendToInterface = FALSE;
        }
    }
    else {
        pUsbHid->fSendToInterface = TRUE;
    }

    DUMP_USB_HID_DESCRIPTOR((*pHidDescriptor));

    pUsbHid->pUsbFuncs = pUsbFuncs;

    pUsbHid->Flags.Open = FALSE;
    pUsbHid->Flags.UnloadPending = FALSE;

    // create endpoint 0 event
    pUsbHid->hEP0Event = CreateEvent(NULL, MANUAL_RESET_EVENT, FALSE, NULL);
    if (pUsbHid->hEP0Event == NULL) {
        DEBUGMSG(ZONE_ERROR, (_T("%s: CreateEvent error:%d\r\n"), pszFname, 
            GetLastError()));
        goto EXIT;
    }

    // set the USB interface/pipes
    if (SetUsbInterface(pUsbHid) == FALSE) {
        DEBUGMSG(ZONE_ERROR, (_T("%s: SetUsbInterface failed!\r\n"), pszFname));
        goto EXIT;
    }
    
    // Mouse and keyboard HID devices might have a boot protocol that can be 
    // used at boot time. We never use the boot protocol and the devices
    // should default to the standard report format, but they are not 
    // guaranteed to. Therefore, we will manually set the report protocol here.
    if (pUsbInterface->Descriptor.bInterfaceSubClass == HID_INTERFACE_BOOT_SUBCLASS) {
        SetReportProtocol(pUsbHid);
    }

    // Allocate space temporarily for the report descriptor for our interface.
    cbReportDescriptor = pHidDescriptor->wDescriptorLength;
    pbReportDescriptor = (PBYTE) HidAlloc(cbReportDescriptor);
    if (pbReportDescriptor == NULL) {
        DEBUGMSG(ZONE_ERROR, (_T("%s: LocalAlloc error:%d\r\n"), pszFname,
            GetLastError()));
        goto EXIT;
    }

    // Get the report descriptor for our interface.
    if (GetReportDescriptor(pUsbHid, pbReportDescriptor, 
            cbReportDescriptor) == FALSE) {
        goto EXIT;
    }

    // Remember if this device can wake the system.
    if (pDeviceInfo->lpActiveConfig->Descriptor.bmAttributes & USB_CONFIG_REMOTE_WAKEUP) {
        pUsbHid->Flags.RemoteWakeup = TRUE;
    }

    // Determine how many interrupt transfers to issue at once
    SetQueuedTransfers(CLIENT_REGKEY_SZ);

    // Pass the report descriptor off to the MDD.
    if (HidMdd_Attach(
            (HID_PDD_HANDLE) pUsbHid,
            pbReportDescriptor,
            cbReportDescriptor,
            pDeviceInfo->Descriptor.idVendor,
            pDeviceInfo->Descriptor.idProduct,
            pDeviceInfo->Descriptor.bcdDevice,
            pUsbHid->pUsbInterface->Descriptor.bInterfaceNumber,
            &pUsbHid->pvNotifyParameter,
            &pUsbHid->cbMaxReport) == FALSE) {
        DEBUGMSG(ZONE_ERROR, (_T("%s: Failed initializing HID Mdd\r\n"), 
            pszFname));
        goto EXIT;
    }
    pUsbHid->Flags.MddInitialized = TRUE;

    pUsbHid->hModSelf = LoadLibrary(L"USBHID.dll");
    if (pUsbHid->hModSelf == NULL)
    {
        goto EXIT;
    }

    // Create the thread that receives the data from the device.
    // Only do this, though, if there are input reports.
    if (pUsbHid->cbMaxReport && CreateInterruptThread(pUsbHid) == FALSE) {
        goto EXIT;
    }

    ValidateUsbHidContext(pUsbHid);

    fErr = FALSE;

EXIT:
    if (pbReportDescriptor != NULL) HidFree(pbReportDescriptor);
    
    if ((fErr == TRUE) && (pUsbHid != NULL)) {
        RemoveDeviceContext(pUsbHid);
        pUsbHid = NULL;
    }

    DEBUGMSG(ZONE_INIT, (_T("-%s\r\n"), pszFname));
    return pUsbHid;
}


// Helper routine to start an Interrupt Transfer.
static
USB_TRANSFER
StartInterruptTransfer(
    PUSBHID_CONTEXT pUsbHid,
    __out_bcount(cbBuffer) PBYTE pbBuffer,
    DWORD cbBuffer,
    HANDLE hev
    )
{
    SETFNAME(_T("StartInterruptTransfer"));

    PREFAST_DEBUGCHK(pUsbHid);
    DEBUGCHK(pbBuffer);
    DEBUGCHK(hev);

#ifdef DEBUG
    FillMemory(pbBuffer, cbBuffer, GARBAGE_BYTE);
#endif

    USB_TRANSFER hTransfer = pUsbHid->pUsbFuncs->lpIssueInterruptTransfer(
        pUsbHid->InterruptIn.hPipe,
        DefaultTransferComplete,
        hev,
        USB_IN_TRANSFER | USB_SHORT_TRANSFER_OK,
        cbBuffer,
        pbBuffer,
        NULL
        );

    if (hTransfer == NULL) {
        DEBUGMSG(ZONE_ERROR, (_T("%s: IssueInterruptTransfer failed\r\n"), 
            pszFname));
    }

    return hTransfer;
}


#ifdef DEBUG
// Validates a PUSB_HID_TRANSFER
static
void
ValidateTransfer(
    const USB_HID_TRANSFER *pTransfer
    )
{
    PREFAST_DEBUGCHK(pTransfer);
    DEBUGCHK(pTransfer->hev);
    DEBUGCHK(pTransfer->hev != GARBAGE_PVOID);
    DEBUGCHK(pTransfer->hTransfer);
    DEBUGCHK(pTransfer->hTransfer != GARBAGE_PVOID);
    DEBUGCHK(pTransfer->rgbBuffer);
    DEBUGCHK(pTransfer->rgbBuffer != GARBAGE_PVOID);
}
#else
#define ValidateTransfer(ptr)
#endif // DEBUG


// Receives the interrupt reports from the device
static
DWORD
WINAPI
InterruptThreadProc(
    LPVOID lpParameter
    )
{
    SETFNAME(_T("InterruptThreadProc"));

    PUSBHID_CONTEXT pUsbHid = (PUSBHID_CONTEXT) lpParameter;
    BYTE  cTransfers; // Count of valid transfers
    BYTE  bTransfer;
    DWORD cbBuffer;
    BOOL  fUnloadPending = FALSE;
    USB_HID_TRANSFER *pTransfers = NULL;
    HMODULE hMod;

    PREFAST_DEBUGCHK(pUsbHid != NULL);
    ValidateUsbHidContext(pUsbHid);

    InterlockedIncrement(&pUsbHid->dwRefCnt);
    hMod = pUsbHid->hModSelf;

    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);

    cbBuffer = pUsbHid->cbMaxReport;

    cTransfers = g_bQueuedTransfers;
    DEBUGCHK(cTransfers >= MIN_QUEUED_TRANSFERS);
    DEBUGCHK(cTransfers <= MAX_QUEUED_TRANSFERS);
    DEBUGMSG(ZONE_COMMENT, (_T("%s: Thread started for %u queued transfers\r\n"),
        pszFname, cTransfers));

    DWORD cbTransfer = sizeof(USB_HID_TRANSFER) + cbBuffer;
    DWORD cbTransfers ;
    if (cbTransfer >= sizeof(USB_HID_TRANSFER) && cTransfers < MAXDWORD/cbTransfer) {
        cbTransfers = cbTransfer * cTransfers;
        pTransfers = (PUSB_HID_TRANSFER) HidAlloc(cbTransfers);
    }
    else {
        pTransfers = NULL;
        ASSERT(FALSE);
        goto EXIT;
    }

    if (pTransfers == NULL) {
        DEBUGMSG(ZONE_ERROR, (_T("%s: LocalAlloc error:%d\r\n"), pszFname, 
            GetLastError()));
        goto EXIT;
    }
#ifdef DEBUG
    FillMemory(pTransfers, cbTransfers, GARBAGE_BYTE);
#endif

    for (bTransfer = 0; bTransfer < cTransfers; ++bTransfer) {
        PUSB_HID_TRANSFER pTransfer = &pTransfers[bTransfer];
        pTransfer->rgbBuffer = ((PBYTE) &pTransfers[cTransfers]) + (cbBuffer * bTransfer);
        pTransfer->hev = CreateEvent(NULL, AUTO_RESET_EVENT, FALSE, NULL);
        if (pTransfer->hev == NULL) {
            DEBUGMSG(ZONE_ERROR, (_T("%s: CreateEvent error:%d\r\n"), 
                pszFname, GetLastError()));
            cTransfers = bTransfer; // Invalidate this transfer
            break;
        }
           
        pTransfer->hTransfer = StartInterruptTransfer(pUsbHid,
            pTransfer->rgbBuffer, cbBuffer, pTransfer->hev);
        
        if (pTransfer->hTransfer == NULL) {
            CloseHandle(pTransfer->hev);
#ifdef DEBUG
            FillMemory(pTransfer, cbTransfer, GARBAGE_BYTE);
#endif
            cTransfers = bTransfer; // Invalidate this transfer
            break;
        }
    }

    if (cTransfers == 0) {
        ERRORMSG(1, (_T("USB HID: Unable to issue interrupt transfers\r\n")));
        goto EXIT;
    }

#ifdef DEBUG
    // At this point, indices 0 through cTransfers-1 are valid
    {
        for (BYTE bDebugIdx = 0; bDebugIdx < cTransfers; ++bDebugIdx) {
            ValidateTransfer(&pTransfers[bDebugIdx]);
        }
    }
#endif

    bTransfer = 0;
    BOOL bDeviceRemoved = FALSE;
    while (fUnloadPending == FALSE) {
        DEBUGCHK(bTransfer < cTransfers);
        PUSB_HID_TRANSFER pTransfer = &pTransfers[bTransfer];
        ValidateTransfer(pTransfer);
        DWORD dwWait = WaitForSingleObject(pTransfer->hev, INFINITE);

        if (dwWait == WAIT_OBJECT_0) {
            DWORD dwBytesTransferred;
            USB_ERROR usbError;
            
            ValidateTransfer(pTransfer);
            
            GetTransferStatus(pUsbHid->pUsbFuncs, pTransfer->hTransfer, 
                &dwBytesTransferred, &usbError);
            CloseTransferHandle(pUsbHid->pUsbFuncs, pTransfer->hTransfer);

            if (usbError != USB_NO_ERROR) {
                // Device has probably been unplugged. Allow time for 
                // USBDeviceNotifications to get the news.

                // Since this could be a real error, we'll clean up the
                // hardware, just in case.
                ResetPipe(pUsbHid->pUsbFuncs, pUsbHid->InterruptIn.hPipe, 0);

                if (usbError == USB_STALL_ERROR) {
                    ClearOrSetFeature( 
                        pUsbHid->pUsbFuncs,
                        pUsbHid->hUsbDevice,
                        NULL,
                        NULL,
                        USB_SEND_TO_ENDPOINT,
                        USB_FEATURE_ENDPOINT_STALL,
                        pUsbHid->InterruptIn.bIndex,
                        0,
                        FALSE);
                }
                
                const DWORD dwSleepTime = 25;
                Sleep(dwSleepTime);
            }
            else {
                VERIFY(SUCCEEDED(HidMdd_ProcessInterruptReport(pTransfer->rgbBuffer, 
                    dwBytesTransferred, pUsbHid->pvNotifyParameter)));
            }

            // Restart this transfer. If it fails, the device has been unplugged.
            pTransfer->hTransfer = StartInterruptTransfer(pUsbHid,
                pTransfer->rgbBuffer, cbBuffer, pTransfer->hev);
            if (pTransfer->hTransfer == NULL) {
                DEBUGMSG(ZONE_COMMENT, (_T("%s: Failed starting interrupt transfer. ")
                    _T("Assuming device removed\r\n"), pszFname));
                bDeviceRemoved = TRUE;
                break;
            }

            // Wait on the next transfer
            if (++bTransfer == cTransfers) {
                bTransfer = 0;
            }
        }
        else {
            ERRORMSG(1, (_T("USB HID: Failure in WaitForSingleObject--returned %u\r\n"),
                dwWait));
            break;
        }            

        fUnloadPending = pUsbHid->Flags.UnloadPending;
    }

    // Clean up the valid transfers
    for (bTransfer = 0; bTransfer < cTransfers; ++bTransfer) {
        PUSB_HID_TRANSFER pTransfer = &pTransfers[bTransfer];
        DEBUGCHK(pTransfer->hev && (pTransfer->hev != GARBAGE_PVOID));
        
        // The final time through the interrupt loop might get a NULL transfer.
        if (!bDeviceRemoved && pTransfer->hTransfer) {
            CloseTransferHandle(pUsbHid->pUsbFuncs, pTransfer->hTransfer);
            WaitForSingleObject(pTransfer->hev, INFINITE);
        }

        CloseHandle(pTransfer->hev);
    }

EXIT:
    if (pTransfers) HidFree(pTransfers);

    if (0 == InterlockedDecrement(&pUsbHid->dwRefCnt))
    {
        HidFree(pUsbHid);
    }

    FreeLibraryAndExitThread(hMod, 0);

    return 0;
}


// Create the thread that will receive the device's interrupt data
BOOL
CreateInterruptThread(
    PUSBHID_CONTEXT pUsbHid
    )
{
    SETFNAME(_T("CreateInterruptThread"));

    BOOL fRet = FALSE;
    
    PREFAST_DEBUGCHK(pUsbHid != NULL);
    DEBUGCHK(VALID_CONTEXT(pUsbHid));

    pUsbHid->hThread = CreateThread(NULL, 0, InterruptThreadProc, pUsbHid, 
        0, NULL);
    if (pUsbHid->hThread == NULL) {
        DEBUGMSG(ZONE_ERROR, (_T("%s: CreateThread error:%d\r\n"), 
            pszFname, GetLastError()));
        goto EXIT;
    }
#ifdef DEBUG
    pUsbHid->fhThreadInited = TRUE;
#endif

    fRet = TRUE;

EXIT:
    return fRet;
}


