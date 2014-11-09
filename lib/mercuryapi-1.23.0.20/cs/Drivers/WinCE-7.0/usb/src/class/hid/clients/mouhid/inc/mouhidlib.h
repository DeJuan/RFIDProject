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
//  These functions allow us to hook the mouse points for some application (like transcriber) that needs
//   to hook them at the compiled driver level.

#ifndef _MOUHIDLIB_H_
#define _MOUHIDLIB_H_

#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

BOOL DriverMouseHook(DWORD dwFlags, DWORD dx, DWORD dy, DWORD dwData);
BOOL DriverMouseHookInitialize(HANDLE hInstDll);

#ifdef __cplusplus
}
#endif

#endif // _MOUHIDLIB_H_

