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
   standardmouse.cpp   Created Aug 28 12003
--*/

#include "mouhidlib.h"

#ifdef __cplusplus
extern "C"{
#endif 


// These functions are designed to be overridden by other libraries that get linked in
//  (such as TraMouse.lib).  The library that gets linked in has an opportunity to
//  examine the mouse move, decide if it wants the point, and to return TRUE
//  if it wants the point.  If the library's version of this function does return TRUE,
//  then the mouse driver will ignore the point.
// The overloading gives us the cabability to link in a mouse hook without
//  having to pay the code size penalty if we don't use the mouse hook.


// The basic version in this file always does nothing and returns FALSE
//  to indicate that it does not need to hook any points.
BOOL DriverMouseHookInitialize(HANDLE /*hInstDll*/)
{
    return FALSE;
}

//
// The basic version in this file will always return false.
//  If the caller checks the return value of DriverMouseHookInitialize, they need never even call this
BOOL DriverMouseHook(DWORD /*dwFlags*/, DWORD /*dx*/, DWORD /*dy*/, DWORD /*dwData*/)
{
    return FALSE;
}

#ifdef __cplusplus
}
#endif

