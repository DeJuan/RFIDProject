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
#include <windows.h>
#include <pnp.h>
#include <svsutil.hxx>
#include <cedrv_guid.h>

#define QUEUE_ITEM_SIZE     (sizeof(DEVDETAIL) + 100)
#define WAIT_MESSAGE_TIMEOUT  INFINITE
#define REG_GUID_FORMAT_W   L"{%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}"

HANDLE hPnpQueue = NULL;
HANDLE hNotification = NULL;
TCHAR szDrvName[50] =  { 0 }; 

BOOL  SetupNotification();
BOOL CreateAPnpMsgQueue(PHANDLE phQueue, PHANDLE phNoti, GUID inGUID );
BOOL ListeningtoDriverNotification();
BOOL ReadPnpMsgQueue(HANDLE hQueue, PDEVDETAIL pDevDetail, DWORD dwTimeout);

int
WINAPI
WinMain (
    HINSTANCE   hInstance,
    HINSTANCE   hPrevInstance,
    LPTSTR      lpCmdLine,
    int         nCmdShow
    )
{
    BOOL fRet = FALSE;


    fRet = SetupNotification();
    if (!fRet)
    {
        RETAILMSG(1, (L"SetupNotification Failed"));
        return 1;
    }

    for(int i = 0;i<10000; i++)
    {
        fRet = ListeningtoDriverNotification();    
        if(!fRet)
        {
            RETAILMSG(1, (L"Listen to Driver notification failed"));
        }
    }
    return 0;
}

BOOL ListeningtoDriverNotification(){

    PBYTE pData = new BYTE[QUEUE_ITEM_SIZE];
    if(pData == NULL){
        RETAILMSG(1, (L"Out of memory"));
        return FALSE;
    }
    PDEVDETAIL pDevDetail =(PDEVDETAIL)pData;


   if(WaitForSingleObject(hPnpQueue, WAIT_MESSAGE_TIMEOUT) == WAIT_OBJECT_0){
        memset(pDevDetail, 0, QUEUE_ITEM_SIZE);
        if(ReadPnpMsgQueue(hPnpQueue, pDevDetail, WAIT_MESSAGE_TIMEOUT) == FALSE){
            return FALSE;
        }

        wcscpy(szDrvName, pDevDetail->szName);
        RETAILMSG(1, (L"%s %S", szDrvName, pDevDetail->fAttached ? 
                                        "ATTACHED" : "DETACHED"));
    }

    delete[] pData;
    return TRUE;
}


BOOL  SetupNotification(){

    union {
        BYTE rgbGuidBuffer[sizeof(GUID) + 4]; // +4 since scanf writes longs
        GUID guidBus;
    } u = { 0 };
    LPGUID pguidBus = &u.guidBus;
    LPCTSTR pszBusGuid = CE_DRIVER_HID_MOUSE_GUID;

    // Parse the GUID
    int iErr = _stscanf(pszBusGuid, REG_GUID_FORMAT_W, SVSUTIL_PGUID_ELEMENTS(&pguidBus));
    ASSERT(iErr != 0 && iErr != EOF);

    BOOL fRet = CreateAPnpMsgQueue(&hPnpQueue, &hNotification, *pguidBus);
    if(fRet == FALSE){
        RETAILMSG(1, (L"Can not create a Pnp message queue"));
        return FALSE;
    }
    return TRUE;
}

BOOL CreateAPnpMsgQueue(PHANDLE phQueue, PHANDLE phNoti, GUID inGUID )
{
    if(phQueue == NULL || phNoti == NULL){
        return FALSE;
    }

    MSGQUEUEOPTIONS msgqopts = {0};    
    msgqopts.dwSize = sizeof(MSGQUEUEOPTIONS);
    msgqopts.dwFlags = 0;
    msgqopts.cbMaxMessage = QUEUE_ITEM_SIZE;
    msgqopts.bReadAccess = TRUE;
    
    *phQueue = NULL;
    *phQueue = CreateMsgQueue(NULL, &msgqopts);

    if(*phQueue == NULL){
        return FALSE;
    }

    *phNoti = RequestDeviceNotifications(&inGUID, *phQueue, FALSE);
    if(*phNoti == NULL){
        CloseMsgQueue(hPnpQueue);
        return FALSE;
    }

    return TRUE;
}

// --------------------------------------------------------------------
BOOL ReadPnpMsgQueue(HANDLE hQueue, PDEVDETAIL pDevDetail, DWORD dwTimeout)
// --------------------------------------------------------------------
{
    DWORD dwRead = 0;
    DWORD dwFlags =0;
    return ReadMsgQueue(hQueue, pDevDetail, QUEUE_ITEM_SIZE, &dwRead, 
        dwTimeout, &dwFlags);
}

