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
//
//

#include "kbdhid.h"
#include "InputLang.h"


#ifdef DEBUG

// Debug Zones
#define DBG_ERROR              0x0001
#define DBG_WARNING            0x0002
#define DBG_INIT               0x0004
#define DBG_FUNCTION           0x0008

#define DBG_USAGES             0x0010

DBGPARAM dpCurSettings = {
        TEXT("KbdHid"), {
        TEXT("Errors"), TEXT("Warnings"), TEXT("Init"), TEXT("Function"),
        TEXT("Usages"), TEXT(""), TEXT(""), TEXT(""),
        TEXT(""), TEXT(""), TEXT(""), TEXT(""),
        TEXT(""), TEXT(""), TEXT(""), TEXT("") },
        DBG_ERROR | DBG_WARNING };
        
#endif // DEBUG


// MapVirtualKey(x, 3) converts scan codes to virtual keys
#define MAP_SC_TO_VK 3

#define SC_EXTENDED_MASK 0xFFFFFF00
#define SC_EXTENDED_BITS     0xE000
#define SC_E1_BITS         0xE11400

#define SC_PRTSCRN      0xE07C
#define SC_PAUSE      0xE11477

// Zero usage and page for convenience
static const USAGE_AND_PAGE g_uapZero = { 0, 0 };

// Variables used by the Japanese key remapper. Global, so simultaneous
// use of two Japanese HID keyboards might cause certain Japanese remappings
// to produce the incorrect virtual key.
static UINT8 g_vkAlphaNumSent;
static UINT8 g_vkFullHalfSent;
static UINT8 g_vkHiraKataSent;

// AutoRepeat variables. Global since all HID keyboards share the same 
// timings. Not protected since the Layout Manager serializes change
// requests (IOCTLs).
DWORD g_dwAutoRepeatInitialDelay = KBD_AUTO_REPEAT_INITIAL_DELAY_DEFAULT;
DWORD g_dwAutoRepeatKeysPerSec   = KBD_AUTO_REPEAT_KEYS_PER_SEC_DEFAULT;

// Locale flags specify locale-specific features such as AltGr.
DWORD g_dwLocaleFlags = 0;

// Defines for checking the state of modifier keys
#define ANY_ALT_DOWN() (*pKeyStateFlags & (KeyShiftLeftAltFlag | KeyShiftRightAltFlag))
#define ANY_CTRL_DOWN() (*pKeyStateFlags & (KeyShiftLeftCtrlFlag | KeyShiftRightCtrlFlag))
#define ANY_SHIFT_DOWN() (*pKeyStateFlags & (KeyShiftLeftShiftFlag | KeyShiftRightShiftFlag))
#define IS_NUMLOCK_ENABLED() ((*pKeyStateFlags & KeyShiftNumLockFlag) != 0)


// Two bits represent extended bytes to prepend to the scan codes.
enum ExtendedBytes {
    EB_NONE = 0,
    EB_E0,
    EB_E114,
    EB_COUNT,
};

// Six bits represent the type of processing that needs to be performed.
enum ProcessingType {
    PT_STANDARD = 0,
    PT_MODIFIER,
    PT_NUMPAD,
    PT_SPECIAL,
    PT_JPN,
    PT_NO_BREAK,
    PT_COUNT,
};


// Describes a Scan Code that is associated with a Usage
struct USAGE_TO_SCANCODE {
    UINT8 uiFlags; // Top six bits = ProcessingType. Bottom two = ExtendedBytes
    UINT8 uiSc;
};


// Describes an association from a Usage to an AT Scan Code
struct USAGE_TO_SC_ASSOCIATION {
    USAGE  usage;
    UINT16 uiSc;
};

// Helper macros to get the right bits out of and into the uiFlags field.
#define SET_EB(x)     ((x) & 0x3)
#define GET_EB(flags) ( (ExtendedBytes) ((flags) & 0x3) )

#define SET_PT(x)     ((x) << 2)
#define GET_PT(flags) ( (ProcessingType) ((UINT8) ((flags) >> 2)) )

#define MAKE_FLAGS(eb, pt) (SET_EB(eb) | SET_PT(pt))

// The general Usage to AT Scan Code mapping.
#define FIRST_USAGE HID_USAGE_KEYBOARD_NOEVENT // 0x00
#define LAST_USAGE (FIRST_USAGE + _countof(g_rgUsageToSc) - 1)
static const USAGE_TO_SCANCODE g_rgUsageToSc[] = {
    { 0,                                  0x00 }, // 00 - No Event
    { 0,                                  0x00 }, // 01 - Overrun
    { 0,                                  0x00 }, // 02 - POST Fail
    { 0,                                  0x00 }, // 03 - ErrorUndefined
    { 0,                                  0x1C }, // 04 - a A
    { 0,                                  0x32 }, // 05 - b B
    { 0,                                  0x21 }, // 06 - c C
    { 0,                                  0x23 }, // 07 - d D
    { 0,                                  0x24 }, // 08 - e E
    { 0,                                  0x2B }, // 09 - f F
    { 0,                                  0x34 }, // 0a - g G
    { 0,                                  0x33 }, // 0b - h H
    { 0,                                  0x43 }, // 0c - i I
    { 0,                                  0x3B }, // 0d - j J
    { 0,                                  0x42 }, // 0e - k K
    { 0,                                  0x4B }, // 0f - l L
    { 0,                                  0x3A }, // 10 - m M
    { 0,                                  0x31 }, // 11 - n N
    { 0,                                  0x44 }, // 12 - o O
    { 0,                                  0x4D }, // 13 - p P
    { 0,                                  0x15 }, // 14 - q Q
    { 0,                                  0x2D }, // 15 - r R
    { 0,                                  0x1B }, // 16 - s S
    { 0,                                  0x2C }, // 17 - t T
    { 0,                                  0x3C }, // 18 - u U
    { 0,                                  0x2A }, // 19 - v V
    { 0,                                  0x1D }, // 1a - w W
    { 0,                                  0x22 }, // 1b - x X
    { 0,                                  0x35 }, // 1c - y Y
    { 0,                                  0x1A }, // 1d - z Z
    { 0,                                  0x16 }, // 1e - 1 !
    { 0,                                  0x1E }, // 1f - 2 @
    { 0,                                  0x26 }, // 20 - 3 #
    { 0,                                  0x25 }, // 21 - 4 $
    { 0,                                  0x2E }, // 22 - 5 %
    { 0,                                  0x36 }, // 23 - 6 ^
    { 0,                                  0x3D }, // 24 - 7 &
    { 0,                                  0x3E }, // 25 - 8 *
    { 0,                                  0x46 }, // 26 - 9 (
    { 0,                                  0x45 }, // 27 - 0 )
    { 0,                                  0x5A }, // 28 - Return
    { 0,                                  0x76 }, // 29 - Escape
    { 0,                                  0x66 }, // 2a - Backspace
    { 0,                                  0x0D }, // 2b - Tab
    { 0,                                  0x29 }, // 2c - Space
    { 0,                                  0x4E }, // 2d - - _
    { 0,                                  0x55 }, // 2e - = +
    { 0,                                  0x54 }, // 2f - [ {
    { 0,                                  0x5B }, // 30 - ] }
    { 0,                                  0x5D }, // 31 - \ |  
    { 0,                                  0x5D }, // 32 - Europe 1 (Note 2)
    { 0,                                  0x4C }, // 33 - ; :
    { 0,                                  0x52 }, // 34 - "' """
    { MAKE_FLAGS(0,       PT_JPN),        0x0E }, // 35 - ` ~
    { 0,                                  0x41 }, // 36 - ", <"
    { 0,                                  0x49 }, // 37 - . >
    { 0,                                  0x4A }, // 38 - / ?
    { MAKE_FLAGS(0,       PT_JPN),        0x58 }, // 39 - Caps Lock
    { 0,                                  0x05 }, // 3a - F1
    { 0,                                  0x06 }, // 3b - F2
    { 0,                                  0x04 }, // 3c - F3
    { 0,                                  0x0C }, // 3d - F4
    { 0,                                  0x03 }, // 3e - F5
    { 0,                                  0x0B }, // 3f - F6
    { 0,                                  0x83 }, // 40 - F7
    { 0,                                  0x0A }, // 41 - F8
    { 0,                                  0x01 }, // 42 - F9
    { 0,                                  0x09 }, // 43 - F10
    { 0,                                  0x78 }, // 44 - F11
    { 0,                                  0x07 }, // 45 - F12
    { MAKE_FLAGS(EB_E0,   PT_SPECIAL),    0x7C }, // 46 - Print Screen (Note 1)
    { 0,                                  0x7E }, // 47 - Scroll Lock
    { MAKE_FLAGS(EB_E114, PT_SPECIAL),    0x77 }, // 48 - Pause
    { MAKE_FLAGS(EB_E0,   0),             0x70 }, // 49 - Insert (Note 1)
    { MAKE_FLAGS(EB_E0,   0),             0x6C }, // 4a - Home (Note 1)
    { MAKE_FLAGS(EB_E0,   0),             0x7D }, // 4b - Page Up (Note 1)
    { MAKE_FLAGS(EB_E0,   0),             0x71 }, // 4c - Delete (Note 1)
    { MAKE_FLAGS(EB_E0,   0),             0x69 }, // 4d - End (Note 1)
    { MAKE_FLAGS(EB_E0,   0),             0x7A }, // 4e - Page Down (Note 1)
    { MAKE_FLAGS(EB_E0,   0),             0x74 }, // 4f - Right Arrow (Note 1)
    { MAKE_FLAGS(EB_E0,   0),             0x6B }, // 50 - Left Arrow (Note 1)
    { MAKE_FLAGS(EB_E0,   0),             0x72 }, // 51 - Down Arrow (Note 1)
    { MAKE_FLAGS(EB_E0,   0),             0x75 }, // 52 - Up Arrow (Note 1)
    { 0,                                  0x77 }, // 53 - Num Lock
    { MAKE_FLAGS(EB_E0,   0),             0x4A }, // 54 - Keypad / (Note 1)
    { 0,                                  0x7C }, // 55 - Keypad *
    { 0,                                  0x7B }, // 56 - Keypad -
    { 0,                                  0x79 }, // 57 - Keypad +
    { MAKE_FLAGS(EB_E0,   0),             0x5A }, // 58 - Keypad Enter
    { MAKE_FLAGS(0,       PT_NUMPAD),     0x69 }, // 59 - Keypad 1 End
    { MAKE_FLAGS(0,       PT_NUMPAD),     0x72 }, // 5a - Keypad 2 Down
    { MAKE_FLAGS(0,       PT_NUMPAD),     0x7A }, // 5b - Keypad 3 PageDn
    { MAKE_FLAGS(0,       PT_NUMPAD),     0x6B }, // 5c - Keypad 4 Left
    { MAKE_FLAGS(0,       PT_NUMPAD),     0x73 }, // 5d - Keypad 5
    { MAKE_FLAGS(0,       PT_NUMPAD),     0x74 }, // 5e - Keypad 6 Right
    { MAKE_FLAGS(0,       PT_NUMPAD),     0x6C }, // 5f - Keypad 7 Home
    { MAKE_FLAGS(0,       PT_NUMPAD),     0x75 }, // 60 - Keypad 8 Up
    { MAKE_FLAGS(0,       PT_NUMPAD),     0x7D }, // 61 - Keypad 9 PageUp
    { MAKE_FLAGS(0,       PT_NUMPAD),     0x70 }, // 62 - Keypad 0 Insert
    { MAKE_FLAGS(0,       PT_NUMPAD),     0x71 }, // 63 - Keypad . Delete
    { 0,                                  0x61 }, // 64 - Europe 2 (Note 2)
    { MAKE_FLAGS(EB_E0,   0),             0x2F }, // 65 - App
    { 0,                                  0x00 }, // 66 - Keyboard Power
    { 0,                                  0x0F }, // 67 - Keypad =
    { 0,                                  0x2F }, // 68 - F13
    { 0,                                  0x37 }, // 69 - F14
    { 0,                                  0x3F }, // 6a - F15
};

// Mapping from international Usages to AT Scan Codes
#define FIRST_GLOBAL_USAGE 0x85
#define LAST_GLOBAL_USAGE (FIRST_GLOBAL_USAGE + _countof(g_rgGlobalUsageToSc) - 1)
static const USAGE_TO_SCANCODE g_rgGlobalUsageToSc[] = 
{
    { 0,                                  0x6D }, // 85 - Keybad , (Brazillian)
    { 0,                                  0x00 }, // 86 - Keypad =
    { MAKE_FLAGS(0,       PT_JPN),        0x51 }, // 87 - Ro
    { MAKE_FLAGS(0,       PT_JPN),        0x13 }, // 88 - Katakana/Hiragana
    { MAKE_FLAGS(0,       PT_JPN),        0x6A }, // 89 - Yen
    { MAKE_FLAGS(0,       PT_JPN),        0x64 }, // 8a - Henkan
    { MAKE_FLAGS(0,       PT_JPN),        0x67 }, // 8b - Muhenkan
    { 0,                                  0x00 }, // 8c - Int'l 6
    { 0,                                  0x00 }, // 8d - Int'l 7
    { 0,                                  0x00 }, // 8e - Int'l 8
    { 0,                                  0x00 }, // 8f - Int'l 9
    { MAKE_FLAGS(0,       PT_NO_BREAK),   0xF2 }, // 90 - Hanguel/English
    { MAKE_FLAGS(0,       PT_NO_BREAK),   0xF1 }, // 91 - Hanja
    { 0,                                  0x63 }, // 92 - LANG Katakana
    { 0,                                  0x62 }, // 93 - LANG Hiragana
    { 0,                                  0x5F }, // 94 - LANG Zenkaku/Hankaku
};

// Mapping from modifier Usages to AT Scan Codes
#define FIRST_MODIFIER_USAGE HID_USAGE_KEYBOARD_LCTRL // 0xE0
#define LAST_MODIFIER_USAGE (FIRST_MODIFIER_USAGE + _countof(g_rgModifierUsageToSc) - 1)
static const USAGE_TO_SCANCODE g_rgModifierUsageToSc[] = {
    { MAKE_FLAGS(0,       PT_MODIFIER),   0x14 }, // E0 - Left Control
    { MAKE_FLAGS(0,       PT_MODIFIER),   0x12 }, // E1 - Left Shift
    { MAKE_FLAGS(0,       PT_MODIFIER),   0x11 }, // E2 - Left Alt
    { MAKE_FLAGS(EB_E0,   PT_MODIFIER),   0x1F }, // E3 - Left GUI
    { MAKE_FLAGS(EB_E0,   PT_MODIFIER),   0x14 }, // E4 - Right Control
    { MAKE_FLAGS(0,       PT_MODIFIER),   0x59 }, // E5 - Right Shift
    { MAKE_FLAGS(EB_E0,   PT_MODIFIER),   0x11 }, // E6 - Right Alt
    { MAKE_FLAGS(EB_E0,   PT_MODIFIER),   0x27 }, // E7 - Right GUI
};

// Describes a table listing a direct mapping from Usages to Scan Codes
struct USAGE_TO_SC_INFO {
    const USAGE_TO_SCANCODE *pUsageToSc;
    USHORT uFirstUsage;
    USHORT uLastUsage;
};

// Our list of direct Usage to Virtual Key mapping tables
static const USAGE_TO_SC_INFO g_rgUsageToScInfo[] = {
    { g_rgModifierUsageToSc, FIRST_MODIFIER_USAGE, LAST_MODIFIER_USAGE },
    { g_rgUsageToSc, FIRST_USAGE, LAST_USAGE },
    { g_rgGlobalUsageToSc, FIRST_GLOBAL_USAGE, LAST_GLOBAL_USAGE },
};


// Consumer page (0xC) usage to virtual key.
// Note that most Consumer usages will be sent in 
// Consumer Control (0xC 0x1) top level collections 
// and will be handled in the consumer control client.
static const USAGE_TO_SC_ASSOCIATION g_rgConsumerToScAssn[] = {
    { 0x018A, 0xE048 }, // 018A - Mail
    { 0x0221, 0xE010 }, // 0221 - WWW Search
    { 0x0223, 0xE03A }, // 0223 - WWW Home
    { 0x0224, 0xE038 }, // 0224 - WWW Back
    { 0x0225, 0xE030 }, // 0225 - WWW Forward
    { 0x0226, 0xE028 }, // 0226 - WWW Stop
    { 0x0227, 0xE020 }, // 0227 - WWW Refresh
    { 0x022A, 0xE018 }, // 022A - WWW Favorites
};


// Declare all the processing functions.
typedef void (*PFN_PROCESSING_TYPE) (
    UINT uiVk,
    UINT uiSc,
    DWORD dwFlags,
    HIDP_KEYBOARD_DIRECTION hidpKeyEvent,
    KEY_STATE_FLAGS *const pKeyStateFlags
    );

#define DECLARE_PROCESSING_TYPE_FN(fn) void Process ## fn ## ( \
    UINT uiVk, UINT uiSc, DWORD dwFlags, HIDP_KEYBOARD_DIRECTION hidpKeyEvent, \
    KEY_STATE_FLAGS const*const pKeyStateFlags)

static DECLARE_PROCESSING_TYPE_FN(Standard);
static DECLARE_PROCESSING_TYPE_FN(NumPad);
static DECLARE_PROCESSING_TYPE_FN(Special);
static DECLARE_PROCESSING_TYPE_FN(Jpn);
static DECLARE_PROCESSING_TYPE_FN(NoBreak);

static void ProcessModifier( 
    UINT uiVk, UINT uiSc, DWORD dwFlags, HIDP_KEYBOARD_DIRECTION hidpKeyEvent, 
    KEY_STATE_FLAGS *const pKeyStateFlags);

// Array of processing functions. Indexed by a ProcessingType.
static const PFN_PROCESSING_TYPE g_rgpfnProcessingType[] = {
    (PFN_PROCESSING_TYPE)&ProcessStandard,
    &ProcessModifier,
    (PFN_PROCESSING_TYPE)&ProcessNumPad,
    (PFN_PROCESSING_TYPE)&ProcessSpecial,
    (PFN_PROCESSING_TYPE)&ProcessJpn,
    (PFN_PROCESSING_TYPE)&ProcessNoBreak,
};


#ifdef DEBUG

static void
ValidateUsageToSc(
    const USAGE_TO_SC_INFO *pUsageToScInfo
    );

static void
ValidateAllUsageToSc(
    );

#else

#define ValidateUsageToSc(ptr)
#define ValidateAllUsageToSc()

#endif // DEBUG


static DWORD
SendKeyboardUsages(
    PUSAGE_AND_PAGE puapUsages,
    DWORD dwMaxUsages,
    HIDP_KEYBOARD_DIRECTION keyEvent,
    KEY_STATE_FLAGS *pKeyStateFlags    
    );

static void 
KeyboardEvent(
    UINT vk,
    UINT sc,
    DWORD dwFlags
    );

static const USAGE_TO_SCANCODE *
FindUsageToSc(
    USAGE usage
    );

static void
ProcessKeyboardReport(
    PHID_KBD pHidKbd,
    PCHAR pbHidPacket,
    DWORD cbHidPacket
    );

static BOOL
AllocateUsageLists(
    PHID_KBD pHidKbd,
    size_t cbUsages
    );

static VOID
FreeHidKbd(
    PHID_KBD pHidKbd
    );

static BOOL
FlashLEDs(
    PHID_KBD pHidKbd
    );



// Dll entry function.
extern "C" 
BOOL
DllEntry(
    HANDLE hDllHandle,
    DWORD dwReason, 
    LPVOID lpReserved
    )
{
    SETFNAME(_T("KBDHID DllEntry"));
    
    UNREFERENCED_PARAMETER(lpReserved);
    
    switch (dwReason) {
        case DLL_PROCESS_ATTACH:
            DEBUGREGISTER((HINSTANCE)hDllHandle);

            DEBUGMSG(ZONE_INIT, (_T("%s: Attach\r\n"), pszFname));
            DisableThreadLibraryCalls((HMODULE) hDllHandle);
            break;
            
        case DLL_PROCESS_DETACH:
            DEBUGMSG(ZONE_INIT, (_T("%s: Detach\r\n"), pszFname));
            break;
            
        default:
            break;
    }
    
    return TRUE;
}


// Get interrupt reports from the device. Thread exits when the device has
// been removed or the close event has been signaled.
static
DWORD
WINAPI
KeyboardThreadProc(
    LPVOID lpParameter
    )
{
    SETFNAME(_T("KeyboardThreadProc"));

    PHID_KBD pHidKbd = (PHID_KBD) lpParameter;
    
    PREFAST_DEBUGCHK(pHidKbd != NULL);
    DEBUGCHK(pHidKbd->hevClosing != NULL); // Needed to signal exit

    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);
    
    const DWORD cbMaxHidPacket = pHidKbd->hidpCaps.InputReportByteLength;
    PCHAR pbHidPacket = (PCHAR) LocalAlloc(LMEM_FIXED, cbMaxHidPacket);
    if (pbHidPacket == NULL) {
        DEBUGMSG(ZONE_ERROR, (TEXT("%s: LocalAlloc error:%d\r\n"), pszFname, GetLastError()));
        goto EXIT;
    }

    for (;;) {  
        ValidateHidKbd(pHidKbd);

        DEBUGCHK(g_dwAutoRepeatInitialDelay >= KBD_AUTO_REPEAT_INITIAL_DELAY_MIN);
        DEBUGCHK(g_dwAutoRepeatInitialDelay <= KBD_AUTO_REPEAT_INITIAL_DELAY_MAX);
        DEBUGCHK(!g_dwAutoRepeatKeysPerSec || 
                 g_dwAutoRepeatKeysPerSec   >= KBD_AUTO_REPEAT_KEYS_PER_SEC_MIN);
        DEBUGCHK(g_dwAutoRepeatKeysPerSec   <= KBD_AUTO_REPEAT_KEYS_PER_SEC_MAX);

        // 0 keys per second => auto repeat disabled.
        if (g_dwAutoRepeatKeysPerSec == 0) {
            pHidKbd->ARState = AR_WAIT_FOR_ANY;
        }

        DWORD dwTimeout;

        // Set our timeout according to the current autorepeat state.
        if (pHidKbd->ARState == AR_WAIT_FOR_ANY) {
            dwTimeout = INFINITE;
        }
        else if (pHidKbd->ARState == AR_INITIAL_DELAY) {
            dwTimeout = g_dwAutoRepeatInitialDelay;
        }
        else {
            DEBUGCHK(pHidKbd->ARState == AR_AUTOREPEATING);
            DEBUGCHK(g_dwAutoRepeatKeysPerSec != 0); // Special case above
            dwTimeout = 1000 / g_dwAutoRepeatKeysPerSec;
        }

        DWORD cbHidPacket;

        // Get an interrupt report from the device.
        DWORD dwErr = pHidKbd->pHidFuncs->lpGetInterruptReport(
            pHidKbd->hDevice,
            pbHidPacket,
            cbMaxHidPacket,
            &cbHidPacket,
            pHidKbd->hevClosing,
            dwTimeout);

        if (dwErr == ERROR_TIMEOUT) {
            // Perform autorepeat
            DEBUGCHK(pHidKbd->ARState != AR_WAIT_FOR_ANY);
            VERIFY(SendKeyboardUsages(pHidKbd->puapMakeUsages, 
                pHidKbd->dwMaxUsages, HidP_Keyboard_Make, &pHidKbd->KeyStateFlags));
            pHidKbd->ARState = AR_AUTOREPEATING;
        }
        else if (dwErr != ERROR_SUCCESS) {
            if ( (dwErr != ERROR_DEVICE_REMOVED) && (dwErr != ERROR_CANCELLED) ) {
                DEBUGMSG(ZONE_ERROR, 
                    (_T("%s: GetInterruptReport returned unexpected error %u\r\n"),
                    pszFname, dwErr));
            }

            // Exit thread
            break;
        }
        else {
            DEBUGCHK(cbHidPacket <= cbMaxHidPacket);
            ProcessKeyboardReport(pHidKbd, pbHidPacket, cbHidPacket);
        }
    }

EXIT:
    if (pbHidPacket != NULL) LocalFree(pbHidPacket);
    DEBUGMSG(ZONE_FUNCTION, (_T("%s: Exiting thread\r\n"), pszFname));
    
    return 0;
}


// Entry point for the HID driver. Initializes the structures for this 
// keyboard and starts the thread that will receive interrupt reports.
extern "C"
BOOL
HIDDeviceAttach(
    HID_HANDLE                 hDevice, 
    PCHID_FUNCS                pHidFuncs,
    const HID_DRIVER_SETTINGS * /*pDriverSettings*/,
    PHIDP_PREPARSED_DATA       phidpPreparsedData,
    PVOID                     *ppvNotifyParameter,
    DWORD                      /*dwUnused*/
    )
{
    SETFNAME(_T("HIDDeviceAttach"));

    BOOL fRet = FALSE;
    size_t cbUsages;
    PHID_KBD pHidKbd;
    const TCHAR * const pszClientKey = CLIENT_REGKEY_SZ;

    DEBUGCHK(hDevice != NULL);
    DEBUGCHK(pHidFuncs != NULL);
    DEBUGCHK(phidpPreparsedData != NULL);
    DEBUGCHK(ppvNotifyParameter != NULL);

    // Allocate this keyboard's data structure and fill it.
    pHidKbd = (PHID_KBD) LocalAlloc(LPTR, sizeof(HID_KBD));
    if (pHidKbd == NULL) {
        DEBUGMSG(ZONE_ERROR, (_T("%s: LocalAlloc error:%d\r\n"), pszFname, GetLastError()));
        goto EXIT;
    }

    pHidKbd->dwSig = HID_KBD_SIG;
    pHidKbd->hDevice = hDevice;
    pHidKbd->pHidFuncs = pHidFuncs;
    pHidKbd->phidpPreparsedData = phidpPreparsedData;
    HidP_GetCaps(pHidKbd->phidpPreparsedData, &pHidKbd->hidpCaps);
    pHidKbd->KeyStateFlags = 0;
    pHidKbd->ARState = AR_WAIT_FOR_ANY;

    // Allocate enough space for output reports.
    pHidKbd->cbOutputBuffer = pHidKbd->hidpCaps.OutputReportByteLength;
    pHidKbd->pbOutputBuffer = (PCHAR) LocalAlloc(0, pHidKbd->cbOutputBuffer);
    if (pHidKbd->pbOutputBuffer == NULL) {
        DEBUGMSG(ZONE_ERROR, (_T("%s: LocalAlloc error:%d\r\n"), pszFname, GetLastError()));
        goto EXIT;
    }
    
    pHidKbd->hevClosing = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (pHidKbd->hevClosing == NULL) {
        DEBUGMSG(ZONE_ERROR, (_T("%s: Error creating event: %d\r\n"),
            pszFname, GetLastError()));
        goto EXIT;
    }
    
    // Get the total number of usages that can be returned in an input packet.
    pHidKbd->dwMaxUsages = HidP_MaxUsageListLength(HidP_Input, 0, phidpPreparsedData);
    if (pHidKbd->dwMaxUsages == 0) {
        DEBUGMSG(ZONE_WARNING, (_T("%s: This collection does not have any ")
            _T("understood usages and will be ignored\r\n"), pszFname));
        goto EXIT;
    }
    
    cbUsages = pHidKbd->dwMaxUsages * sizeof(USAGE_AND_PAGE);

    if (AllocateUsageLists(pHidKbd, cbUsages) == FALSE) {
        goto EXIT;
    }

    FlashLEDs(pHidKbd);
    
    // Create the thread that will receive reports from this device
    pHidKbd->hThread = CreateThread(NULL, 0, KeyboardThreadProc, pHidKbd, 0, NULL);
    if (pHidKbd->hThread == NULL) {
        DEBUGMSG(ZONE_ERROR, (_T("%s: Failed creating keyboard thread\r\n"), 
            pszFname));
        goto EXIT;
    }
#ifdef DEBUG
    pHidKbd->fhThreadInited = TRUE;
#endif

    // Create the KBD device
    pHidKbd->hOSDevice = ActivateDevice(pszClientKey, (DWORD) pHidKbd);
    if (pHidKbd->hOSDevice == NULL) {
        DEBUGMSG(ZONE_ERROR, (_T("%s: Error on ActivateDevice(%s)\r\n"), 
            pszFname, pszClientKey));
        goto EXIT;
    }
#ifdef DEBUG
    pHidKbd->fhOSDeviceInited = TRUE;
#endif

    *ppvNotifyParameter = pHidKbd;
    ValidateHidKbd(pHidKbd);
    fRet = TRUE;

EXIT:
    if ((fRet == FALSE) && (pHidKbd != NULL)) {
        // Initialization failed. Clean up any resources that did allocate.
        FreeHidKbd(pHidKbd);
    }
    return fRet;
}
#ifdef DEBUG
// Match function with typedef.
static LPHID_CLIENT_ATTACH g_pfnDeviceAttach = HIDDeviceAttach;
#endif


// Entry point for the HID driver to give us notifications.
extern "C" 
BOOL 
WINAPI
HIDDeviceNotifications(
    DWORD  dwMsg,
    WPARAM wParam, // Message parameter
    PVOID  pvNotifyParameter
    )
{
    SETFNAME(_T("HIDDeviceNotifications"));

    BOOL fRet = FALSE;
    PHID_KBD pHidKbd = (PHID_KBD) pvNotifyParameter;

    UNREFERENCED_PARAMETER(wParam);

    if (VALID_HID_KBD(pHidKbd) == FALSE) {
        DEBUGMSG(ZONE_ERROR, (_T("%s: Received invalid structure pointer\r\n"), pszFname));
        goto EXIT;
    }

    ValidateHidKbd(pHidKbd);
    
    switch(dwMsg) {
        case HID_CLOSE_DEVICE:
            // Free all of our resources.
            WaitForSingleObject(pHidKbd->hThread, INFINITE);
            CloseHandle(pHidKbd->hThread);
            pHidKbd->hThread = NULL;
            
            // Key up all keys that are still down.
            SendKeyboardUsages(pHidKbd->puapPrevUsages, pHidKbd->dwMaxUsages,
                HidP_Keyboard_Break, &pHidKbd->KeyStateFlags);

            DeactivateDevice(pHidKbd->hOSDevice);

            // Wait for KBD_Deinit to complete so we can free our context.
            WaitForSingleObject(pHidKbd->hevClosing, INFINITE);
            FreeHidKbd(pHidKbd);

            fRet = TRUE;
            break;

        default:
            DEBUGMSG(ZONE_ERROR, (_T("%s: Unhandled message %u\r\n"), pszFname));
            break;
    };

EXIT:
    return fRet;
}
#ifdef DEBUG
// Match function with typedef.
static LPHID_CLIENT_NOTIFICATIONS g_pfnDeviceNotifications = HIDDeviceNotifications;
#endif


// Flashes the keyboard's LEDs.
static
BOOL
FlashLEDs(
    PHID_KBD pHidKbd
    )
{
    DEBUGCHK(pHidKbd != NULL);

    // Turn on all LEDs.
    SetLEDs(pHidKbd, (KEY_STATE_FLAGS)~0);

    // Turn off all LEDs.
    SetLEDs(pHidKbd, 0);

    return TRUE;
}


// Allocate the usage lists for this keyboard. The lists are allocated in 
// a single block and then divided up.
static
BOOL
AllocateUsageLists(
    PHID_KBD pHidKbd,
    size_t cbUsages
    )
{
    SETFNAME(_T("AllocateUsageLists"));

    BOOL fRet = FALSE;
    size_t cbTotal;
    PBYTE pbStart;
    DWORD dwIdx;
    PUSAGE_AND_PAGE *rgppUsageAndPage[] = {
        &pHidKbd->puapPrevUsages,
        &pHidKbd->puapCurrUsages,
        &pHidKbd->puapBreakUsages,
        &pHidKbd->puapMakeUsages,
        &pHidKbd->puapOldMakeUsages
    };
    
    DEBUGCHK(pHidKbd != NULL);
    DEBUGCHK(cbUsages > 0);

    // Allocate a block of memory for all the usage lists
    if (cbUsages < MAXDWORD/_countof(rgppUsageAndPage)) {
        cbTotal = cbUsages * _countof(rgppUsageAndPage);
        pbStart = (PBYTE) LocalAlloc(LPTR, cbTotal);
    }
    else {
        ASSERT(FALSE);
        pbStart = NULL; // Overflow
    }
    
    if (pbStart == NULL) {
       DEBUGMSG(ZONE_ERROR, (TEXT("%s: LocalAlloc error:%d\r\n"), pszFname, GetLastError()));
       goto EXIT;
    }

    // Divide the block.
    for (dwIdx = 0; dwIdx < _countof(rgppUsageAndPage); ++dwIdx) {
        *rgppUsageAndPage[dwIdx] = (PUSAGE_AND_PAGE) (pbStart + (cbUsages * dwIdx));
    }

    fRet = TRUE;

EXIT:
    return fRet;
}


// Free the memory used by pHidKbd
void
FreeHidKbd(
    PHID_KBD pHidKbd
    )
{
    PREFAST_DEBUGCHK(pHidKbd != NULL);

    // Close thread if it has been created.
    if (pHidKbd->hThread != NULL) {
        DEBUGCHK(pHidKbd->hevClosing != NULL);
        SetEvent(pHidKbd->hevClosing);
        WaitForSingleObject(pHidKbd->hThread, INFINITE);
        CloseHandle(pHidKbd->hThread);
    }

    if (pHidKbd->hevClosing != NULL) CloseHandle(pHidKbd->hevClosing);
    if (pHidKbd->puapPrevUsages != NULL) LocalFree(pHidKbd->puapPrevUsages);
    if (pHidKbd->pbOutputBuffer != NULL) LocalFree(pHidKbd->pbOutputBuffer);
    LocalFree(pHidKbd);    
}


// Determine what usages have been set/unset and send the key events off to
// the system.
//
// On entry, pHidKbd->puapPrevUsages contains the previous usage list and
// pHidKbd->puapMakeUsages contains the most recent set of usages that
// went down (usually a single usage).
static
void
ProcessKeyboardReport(
    PHID_KBD pHidKbd,
    PCHAR pbHidPacket,
    DWORD cbHidPacket
    )
{
    SETFNAME(_T("ProcessKeyboardReport"));

    BOOL fRollover = FALSE;
    ULONG uCurrUsages;
    DWORD dwUsageIdx;
    DWORD cbUsageList;
    DWORD cKeysSent;
    NTSTATUS status;
    
    PREFAST_DEBUGCHK(pHidKbd != NULL);
    DEBUGCHK(pbHidPacket != NULL);

    uCurrUsages = pHidKbd->dwMaxUsages;
    cbUsageList = uCurrUsages * sizeof(USAGE_AND_PAGE);

    status = HidP_GetUsagesEx(
        HidP_Input,
        0,
        pHidKbd->puapCurrUsages,
        &uCurrUsages, // IN OUT parameter
        pHidKbd->phidpPreparsedData,
        pbHidPacket,
        cbHidPacket
        );
    DEBUGCHK(NT_SUCCESS(status));
    DEBUGCHK(uCurrUsages <= pHidKbd->dwMaxUsages);

    // Check usages returned for keyboard rollover.
    for (dwUsageIdx = 0; dwUsageIdx < uCurrUsages; ++dwUsageIdx) 
    {
        PUSAGE_AND_PAGE puapCurr = &pHidKbd->puapCurrUsages[dwUsageIdx];

        if (puapCurr->UsagePage == HID_USAGE_PAGE_KEYBOARD) 
        {
            if (puapCurr->Usage == HID_USAGE_KEYBOARD_ROLLOVER) {
                DEBUGMSG(ZONE_WARNING, (_T("%s: HID keyboard rollover\r\n"), pszFname));
                fRollover = TRUE;
            }

            // At this point either we got the rollover usage or there will
            // not be one. Break either way.
            break;
        }
    }

    if (fRollover == FALSE) 
    {
        // Save our most recent down list.
        memcpy(pHidKbd->puapOldMakeUsages, pHidKbd->puapMakeUsages, cbUsageList);

        // Determine what keys went down and up.
        status = HidP_UsageAndPageListDifference(
            pHidKbd->puapPrevUsages,
            pHidKbd->puapCurrUsages,
            pHidKbd->puapBreakUsages,
            pHidKbd->puapMakeUsages,
            pHidKbd->dwMaxUsages
            );
        DEBUGCHK(NT_SUCCESS(status));

        if (HidP_IsSameUsageAndPage(pHidKbd->puapMakeUsages[0], g_uapZero) &&
            HidP_IsSameUsageAndPage(pHidKbd->puapBreakUsages[0], g_uapZero))
        {
            // No new keys
            DEBUGMSG(ZONE_USAGES, (_T("%s: No new keys. Hardware autorepeat?\r\n"), pszFname));

            // Replace the list of recent downs.
            memcpy(pHidKbd->puapMakeUsages, pHidKbd->puapOldMakeUsages, cbUsageList);
        }
        else
        {
            // Move the current usages to the previous usage list.
            memcpy(pHidKbd->puapPrevUsages, pHidKbd->puapCurrUsages, cbUsageList);

            // Convert the key ups into scan codes and send them
            cKeysSent = SendKeyboardUsages(pHidKbd->puapBreakUsages, pHidKbd->dwMaxUsages,
                HidP_Keyboard_Break, &pHidKbd->KeyStateFlags);

            // If there were ups, then turn off repeat.
            if (cKeysSent > 0) {
                pHidKbd->ARState = AR_WAIT_FOR_ANY;
            }

            if (HidP_IsSameUsageAndPage(pHidKbd->puapCurrUsages[0], g_uapZero) == FALSE &&
                HidP_IsSameUsageAndPage(pHidKbd->puapMakeUsages[0], g_uapZero) == TRUE)
            {
                // There are not any new downs, but there may be some old ones
                // that are repeating in hardware. We'll update our make usages
                // to include any keys that have not come up.
                status = HidP_UsageAndPageListDifference(
                    pHidKbd->puapBreakUsages,
                    pHidKbd->puapOldMakeUsages,
                    pHidKbd->puapCurrUsages, // We can scrap the current usages now
                    pHidKbd->puapMakeUsages,
                    pHidKbd->dwMaxUsages
                    );
                DEBUGCHK(NT_SUCCESS(status));
            }
            else {
                // Convert the key downs to scan codes and send them
                cKeysSent = SendKeyboardUsages(pHidKbd->puapMakeUsages, pHidKbd->dwMaxUsages,
                    HidP_Keyboard_Make, &pHidKbd->KeyStateFlags);

                // If there were downs, then set for repeat.
                if (cKeysSent > 0) {
                    pHidKbd->ARState = AR_INITIAL_DELAY;
                }
            }
        }
    }
}


// Expands a usage, scan code, and flags to a full scan code and virtual key.
static
void
GenerateKeyInfo(
    const USAGE_TO_SCANCODE *pUsageToSc,
    PUINT puiVk,
    PUINT puiSc,
    PDWORD pdwFlags,
    HIDP_KEYBOARD_DIRECTION hidpKeyEvent
    )
{
    PREFAST_DEBUGCHK(pUsageToSc);
    PREFAST_DEBUGCHK(puiVk);
    PREFAST_DEBUGCHK(puiSc);
    PREFAST_DEBUGCHK(pdwFlags);
    
    *puiSc = pUsageToSc->uiSc;
    *pdwFlags = 0;

    if (hidpKeyEvent == HidP_Keyboard_Break) {
        *pdwFlags |= KEYEVENTF_KEYUP;
    }

    // Prepend extended bits
    switch (GET_EB(pUsageToSc->uiFlags)) {
        case EB_E0:
            *puiSc |= SC_EXTENDED_BITS;
            *pdwFlags |= KEYEVENTF_EXTENDEDKEY;
            break;

        case EB_E114:
            *puiSc |= SC_E1_BITS;
            break;

        case EB_NONE:
            break;

        default:
            // Why are you here?
            DEBUGCHK(FALSE);
    };

    // Convert scan code to virtual key
    *puiVk = MapVirtualKey(*puiSc, MAP_SC_TO_VK);
}


// Converts the usages to virtual keys and sends them off.
// Returns the number of usages converted.
static
DWORD
SendKeyboardUsages(
    PUSAGE_AND_PAGE puapUsages,
    DWORD dwMaxUsages,
    HIDP_KEYBOARD_DIRECTION hidpKeyEvent,
    KEY_STATE_FLAGS *pKeyStateFlags
    )
{    
    SETFNAME(_T("SendKeyboardUsages"));
    
    PREFAST_DEBUGCHK(puapUsages != NULL);
    DEBUGCHK(pKeyStateFlags != NULL);

    for (DWORD dwIdx = 0; dwIdx < dwMaxUsages; ++dwIdx) {
        // Determine virtual key mapping
        PUSAGE_AND_PAGE puapCurr = &puapUsages[dwIdx];
        const USAGE_TO_SCANCODE *pUsageToSc = NULL;
        USAGE_TO_SCANCODE usageToSc; // For Consumer page

        if (puapCurr->Usage == 0) {
            // No more usages
            break;
        }

        // Convert the usage to a scan code with flags.
        switch (puapCurr->UsagePage) {                
            case HID_USAGE_PAGE_KEYBOARD:
                pUsageToSc = FindUsageToSc(puapCurr->Usage);
                break;
                    
            case HID_USAGE_PAGE_CONSUMER:
            {
                // Note that most consumer keys will be handled in the 0xC 0x1 
                // HID client.
                DWORD dwAssn;
                
                for (dwAssn = 0; dwAssn < _countof(g_rgConsumerToScAssn); ++dwAssn) {
                    const USAGE_TO_SC_ASSOCIATION *pUsageAssn = 
                        &g_rgConsumerToScAssn[dwAssn];

                    // Later, we assume that this scan code is extended. If
                    // it is not, we need to dynamically assign the EB_E0 enum.
                    DEBUGCHK( (pUsageAssn->uiSc & SC_EXTENDED_BITS) == SC_EXTENDED_BITS );
                    
                    if (pUsageAssn->usage == puapCurr->Usage) {
                        // Fill in the local USAGE_TO_SCANCODE
                        usageToSc.uiSc = (UINT8) pUsageAssn->uiSc;
                        usageToSc.uiFlags = MAKE_FLAGS(EB_E0, PT_STANDARD);
                        pUsageToSc = &usageToSc;
                        break;
                    }
                }
                
                break;
            }
        };

        // Was there a conversion? If so, send the event.
        if (pUsageToSc != NULL) {
            DWORD dwFlags;
            UINT uiSc;
            UINT uiVk;
            PFN_PROCESSING_TYPE pfnProcessingType;
            ProcessingType PT;
            
            GenerateKeyInfo(pUsageToSc, &uiVk, &uiSc, &dwFlags, hidpKeyEvent);

            DEBUGMSG(ZONE_USAGES, (_T("%s: Usage Page: 0x%04x Usage: 0x%04x -> SC: 0x%06x\r\n"),
                pszFname, puapCurr->UsagePage, puapCurr->Usage, uiSc));

            // Determine which processing function to call
            PT = GET_PT(pUsageToSc->uiFlags);
            DEBUGCHK(_countof(g_rgpfnProcessingType) == PT_COUNT);
            DEBUGCHK(PT < PT_COUNT);
            pfnProcessingType = g_rgpfnProcessingType[PT];
            PREFAST_DEBUGCHK(pfnProcessingType != NULL);

            // Call the processing function to send off the event
            (*pfnProcessingType)(uiVk, uiSc, dwFlags, hidpKeyEvent, pKeyStateFlags);
        }
        else {
            DEBUGMSG(ZONE_WARNING, (_T("%s: No scan code for Usage Page: 0x%04x Usage: 0x%04x\r\n"),
                pszFname, puapCurr->UsagePage, puapCurr->Usage));
        }
    }

    return dwIdx;
}


// Send the keyboard event to GWES.
static
void
KeyboardEvent(
    UINT vk,
    UINT sc,
    DWORD dwFlags
    )
{
    SETFNAME(_T("KeyboardEvent"));

    UINT8 uiVk = (UINT8) vk;
    UINT8 uiSc = (UINT8) sc;

    DEBUGCHK((uiVk != 0) || (uiSc != 0));

    DEBUGMSG(ZONE_USAGES, (_T("%s: Keybd event: vk: 0x%02x sc: 0x%02x flags: 0x%08x\r\n"),
        pszFname, uiVk, uiSc, dwFlags));

    // {3C694E10-EC9D-494d-80C7-F12D5FE47A4D}
    static const GUID HIDGuid = 
    { 0x3c694e10, 0xec9d, 0x494d, { 0x80, 0xc7, 0xf1, 0x2d, 0x5f, 0xe4, 0x7a, 0x4d } };

    keybd_eventEx(uiVk, uiSc, dwFlags, &HIDGuid);
}


// Search for usage in the g_rgUsageToScInfo tables.
static
const USAGE_TO_SCANCODE *
FindUsageToSc(
    USAGE usage
    )
{
    const USAGE_TO_SC_INFO *pUsageToScInfo;
    const USAGE_TO_SCANCODE *pUsageToSc = NULL;
    DWORD cUsageToScInfo = _countof(g_rgUsageToScInfo);
    DWORD dwNormalizedUsage;
    DWORD dwIdx;

    ValidateAllUsageToSc();

    for (dwIdx = 0; dwIdx < cUsageToScInfo; ++dwIdx) 
    {
        pUsageToScInfo = &g_rgUsageToScInfo[dwIdx];

        ValidateUsageToSc(pUsageToScInfo);

        if ( (usage >= pUsageToScInfo->uFirstUsage) &&
             (usage <= pUsageToScInfo->uLastUsage) ) 
        {
            dwNormalizedUsage = usage - pUsageToScInfo->uFirstUsage;
            DEBUGCHK(dwNormalizedUsage <= pUsageToScInfo->uLastUsage);
            pUsageToSc = &pUsageToScInfo->pUsageToSc[dwNormalizedUsage];
            break;
        }
    }

    return pUsageToSc;
}


// Standard key processing.
static
void
ProcessStandard(
    UINT uiVk,
    UINT uiSc,
    DWORD dwFlags,
    HIDP_KEYBOARD_DIRECTION /*hidpKeyEvent*/,
    KEY_STATE_FLAGS const* /*pKeyStateFlags*/
    )
{
    KeyboardEvent(uiVk, uiSc, dwFlags);
}


// Updates the given key state with the given virtual key modifier.
// Only processes L/R Ctrl, Shift and Alt. The toggled key state is
// set through IOCTL calls from the Layout Manager.
static
void
UpdateKeyState(
    KEY_STATE_FLAGS *const pKeyStateFlags,
    UINT8 vk,
    HIDP_KEYBOARD_DIRECTION hidpKeyEvent
    )
{
    struct VKeyToShiftState {
        UINT8           vk; // Virtual key
        KEY_STATE_FLAGS ksf;   // Corresponding shift state flag
    }; 
    
    static const VKeyToShiftState rgVKeyDownToShiftState[] = {
        { VK_LSHIFT,    KeyShiftLeftShiftFlag },
        { VK_RSHIFT,    KeyShiftRightShiftFlag },
        { VK_LCONTROL,  KeyShiftLeftCtrlFlag },
        { VK_RCONTROL,  KeyShiftRightCtrlFlag },
        { VK_LMENU,     KeyShiftLeftAltFlag },
        { VK_RMENU,     KeyShiftRightAltFlag },
    };

    DWORD dwIdx;
    const VKeyToShiftState *pvkshiftCurr;

    PREFAST_DEBUGCHK(pKeyStateFlags != NULL);
    DEBUGCHK(vk != 0);

    for (dwIdx = 0; dwIdx < _countof(rgVKeyDownToShiftState); ++dwIdx) {
        pvkshiftCurr = &rgVKeyDownToShiftState[dwIdx];
        if (pvkshiftCurr->vk == vk) {
            if (hidpKeyEvent == HidP_Keyboard_Make) {
                *pKeyStateFlags |= pvkshiftCurr->ksf;
            }
            else {
                *pKeyStateFlags &= ~pvkshiftCurr->ksf;
            }
            break;
        }
    }
}


// Send an LControl keyboard event
static
void
GenerateLControl(
    HIDP_KEYBOARD_DIRECTION hidpKeyEvent,
    KEY_STATE_FLAGS *const pKeyStateFlags
    )
{
    DEBUGCHK(pKeyStateFlags);

    const USAGE_TO_SCANCODE *pUsageToSc = FindUsageToSc(HID_USAGE_KEYBOARD_LCTRL);
    DEBUGCHK(pUsageToSc);
    UINT uiVk;
    UINT uiSc;
    DWORD dwFlags;
    GenerateKeyInfo(pUsageToSc, &uiVk, &uiSc, &dwFlags, hidpKeyEvent);

    if (uiVk == VK_LCONTROL) {
        UpdateKeyState(pKeyStateFlags, (UINT8) uiVk, hidpKeyEvent);
        KeyboardEvent(uiVk, uiSc, dwFlags);
    }
    else {
        RETAILMSG(1, (_T("Keyboard: AltGr processing failed. Returned vkey 0x%02X\r\n"),
            uiVk));
    }
}


// Modifier key processing.
static
void
ProcessModifier(
    UINT uiVk,
    UINT uiSc,
    DWORD dwFlags,
    HIDP_KEYBOARD_DIRECTION hidpKeyEvent,
    KEY_STATE_FLAGS *const pKeyStateFlags
    )
{    
    // Handle AltGr
    if ( (uiVk == VK_RMENU) && (g_dwLocaleFlags & KLLF_ALTGR) ) {
        // Also send a Left Control
        GenerateLControl(hidpKeyEvent, pKeyStateFlags);
    }

    UpdateKeyState(pKeyStateFlags, (UINT8) uiVk, hidpKeyEvent);
    KeyboardEvent(uiVk, uiSc, dwFlags);
}


// NumPad key processing.
static
void
ProcessNumPad(
    UINT uiVk,
    UINT uiSc,
    DWORD dwFlags,
    HIDP_KEYBOARD_DIRECTION /*hidpKeyEvent*/,
    KEY_STATE_FLAGS const*pKeyStateFlags
    )
{
    static const UINT8 rgVkNumLockOff[] = {
        VK_INSERT,      //   VK_NUMPAD0        0x60
        VK_END,         //   VK_NUMPAD1        0x61
        VK_DOWN,        //   VK_NUMPAD2        0x62
        VK_NEXT,        //   VK_NUMPAD3        0x63
        VK_LEFT,        //   VK_NUMPAD4        0x64
        VK_CLEAR,       //   VK_NUMPAD5        0x65
        VK_RIGHT,       //   VK_NUMPAD6        0x66
        VK_HOME,        //   VK_NUMPAD7        0x67
        VK_UP,          //   VK_NUMPAD8        0x68
        VK_PRIOR,       //   VK_NUMPAD9        0x69
        VK_MULTIPLY,    //   VK_MULTIPLY       0x6A
        VK_ADD,         //   VK_ADD            0x6B
        VK_SEPARATOR,   //   VK_SEPARATOR      0x6C
        VK_SUBTRACT,    //   VK_SUBTRACT       0x6D
        VK_DELETE       //   VK_DECIMAL        0x6E
    };
    
    DWORD dwNormalizedIdx;
    
    PREFAST_DEBUGCHK(pKeyStateFlags != NULL);
    DEBUGCHK(uiVk >= VK_NUMPAD0);
    DEBUGCHK(uiVk < VK_NUMPAD0 + _countof(rgVkNumLockOff));

    if (IS_NUMLOCK_ENABLED() == FALSE) {
        dwNormalizedIdx = uiVk - VK_NUMPAD0;
        DEBUGCHK(dwNormalizedIdx < _countof(rgVkNumLockOff));
        uiVk = rgVkNumLockOff[dwNormalizedIdx];
    }

    KeyboardEvent(uiVk, uiSc, dwFlags);
}


// Special key processing that does not fit into any other category.
static
void
ProcessSpecial(
    UINT uiVk,
    UINT uiSc,
    DWORD dwFlags,
    HIDP_KEYBOARD_DIRECTION /*hidpKeyEvent*/,
    KEY_STATE_FLAGS const*pKeyStateFlags
    )
{
    UINT uiScOld = uiSc;
    
    PREFAST_DEBUGCHK(pKeyStateFlags != NULL);

    if (uiSc == SC_PRTSCRN) {
        if (ANY_ALT_DOWN()) {
            uiSc = 0x84;
        }
    }
    else if (uiSc == SC_PAUSE) {
        if (ANY_CTRL_DOWN()) {
            uiSc = 0xE07E;
        }
    }

    // If the scan code changed, update the virtual key and extended flag.
    if (uiSc != uiScOld) 
    {
        uiVk = MapVirtualKey(uiSc, MAP_SC_TO_VK);

        if ((uiSc & SC_EXTENDED_MASK) == SC_EXTENDED_BITS) {
            dwFlags |= KEYEVENTF_EXTENDEDKEY;
        }
        else {
            dwFlags &= ~KEYEVENTF_EXTENDEDKEY;
        }
    }

    KeyboardEvent(uiVk, uiSc, dwFlags);
}


// Remap the Japanese keydowns.
static
UINT8
RemapJpnKeyDown(
    UINT8 vkOnly,
    KEY_STATE_FLAGS const*pKeyStateFlags
    )
{
    UINT8 vkDown = vkOnly;

    if ( vkOnly == VK_DBE_SBCSCHAR ) {
        if ( g_vkFullHalfSent ) {
            vkDown = g_vkFullHalfSent;
        }
        else if ( ANY_ALT_DOWN() && !ANY_CTRL_DOWN() && !ANY_SHIFT_DOWN() ) {
            vkDown = g_vkFullHalfSent = VK_KANJI;
        }
        else {
            //  Don't use imm function if not configured.
            //  The remapping won't work correctly but at least there won't be link errors.
            DWORD fdwConversion;
            DWORD fdwSentence;
            if ( ImmGetConversionStatus(NULL, &fdwConversion, &fdwSentence) ) {
                if ( fdwConversion & IME_CMODE_FULLSHAPE ) {
                    vkDown = g_vkFullHalfSent = VK_DBE_SBCSCHAR;
                }
                else {
                    vkDown = g_vkFullHalfSent = VK_DBE_DBCSCHAR;
                }
            }
            else {
                vkDown = g_vkFullHalfSent = VK_DBE_SBCSCHAR;
            }
        }
    }
    else if ( vkOnly == VK_DBE_ALPHANUMERIC ) {
        if ( g_vkAlphaNumSent ) {
            vkDown = g_vkAlphaNumSent;
        }
        else if ( !ANY_ALT_DOWN() && !ANY_CTRL_DOWN() && !ANY_SHIFT_DOWN() ) {
            vkDown = g_vkAlphaNumSent = VK_DBE_ALPHANUMERIC;
        }
        else {
            vkDown = g_vkAlphaNumSent = VK_CAPITAL;
        }
    }
    else if ( vkOnly == VK_DBE_HIRAGANA ) {
        if ( g_vkHiraKataSent ) {
            vkDown = g_vkHiraKataSent;
        }
        else if ( !ANY_ALT_DOWN() && !ANY_CTRL_DOWN() && ANY_SHIFT_DOWN() ) {
            vkDown = g_vkHiraKataSent = VK_DBE_KATAKANA;
        }
        else if ( !ANY_ALT_DOWN() && ANY_CTRL_DOWN() && ANY_SHIFT_DOWN() ) {
            vkDown = g_vkHiraKataSent = VK_KANA;
        }
        else if ( ANY_ALT_DOWN() && !ANY_CTRL_DOWN() && !ANY_SHIFT_DOWN() ) {
            DWORD fdwConversion;
            DWORD fdwSentence;
            if ( ImmGetConversionStatus(NULL, &fdwConversion, &fdwSentence) ) {
                if ( fdwConversion & IME_CMODE_ROMAN ) {
                    vkDown = g_vkHiraKataSent = VK_DBE_NOROMAN;
                }
                else {
                    vkDown = g_vkHiraKataSent = VK_DBE_ROMAN;
                }
            }
            else {
                vkDown = g_vkHiraKataSent = VK_DBE_NOROMAN;
            }
        }
        else {
            g_vkHiraKataSent = vkOnly;
        }
    }

    return vkDown;
}


// Remap the Japanese keyup.
static
UINT8
RemapJpnKeyUp(
    UINT8 vkOnly,
    KEY_STATE_FLAGS const* /*pKeyStateFlags*/
    )
{    
    UINT8 vkUp = vkOnly;
    
    if ( vkOnly == VK_DBE_SBCSCHAR ) {
        vkUp = g_vkFullHalfSent;
        g_vkFullHalfSent = 0;
    }
    else if ( vkOnly == VK_DBE_ALPHANUMERIC ) {
        vkUp = g_vkAlphaNumSent;
        g_vkAlphaNumSent = 0;
    }
    else if ( vkOnly == VK_DBE_HIRAGANA ) {
        vkUp = g_vkHiraKataSent;
        g_vkHiraKataSent = 0;
    }

    return vkUp;
}


// Japanese key processing.
static
void
ProcessJpn(
    UINT uiVk,
    UINT uiSc,
    DWORD dwFlags,
    HIDP_KEYBOARD_DIRECTION hidpKeyEvent,
    KEY_STATE_FLAGS const*pKeyStateFlags
    )
{
    DEBUGCHK(pKeyStateFlags != NULL);
    
    if (hidpKeyEvent == HidP_Keyboard_Make) {
        uiVk = RemapJpnKeyDown((UINT8)uiVk, pKeyStateFlags);
    }
    else {
        uiVk = RemapJpnKeyUp((UINT8)uiVk, pKeyStateFlags);
    }

    KeyboardEvent(uiVk, uiSc, dwFlags);
}


// Do not send a key up for this key.
static
void
ProcessNoBreak(
    UINT uiVk,
    UINT uiSc,
    DWORD dwFlags,
    HIDP_KEYBOARD_DIRECTION hidpKeyEvent,
    KEY_STATE_FLAGS const*pKeyStateFlags
    )
{
    DEBUGCHK(pKeyStateFlags != NULL);
    UNREFERENCED_PARAMETER(pKeyStateFlags);

    if (hidpKeyEvent == HidP_Keyboard_Make) {
        KeyboardEvent(uiVk, uiSc, dwFlags);
    }
}


#ifdef DEBUG


// Validate a PHID_KBD structure
void
ValidateHidKbd(
    PHID_KBD pHidKbd
    )
{
    DWORD cbUsageList;
    
    PREFAST_DEBUGCHK(pHidKbd != NULL);
    DEBUGCHK(pHidKbd->dwSig == HID_KBD_SIG);
    DEBUGCHK(pHidKbd->hDevice != NULL);
    DEBUGCHK(pHidKbd->pHidFuncs != NULL);
    DEBUGCHK(pHidKbd->phidpPreparsedData != NULL);
    DEBUGCHK(pHidKbd->hidpCaps.UsagePage == HID_USAGE_PAGE_GENERIC);
    DEBUGCHK(pHidKbd->hidpCaps.Usage == HID_USAGE_GENERIC_KEYBOARD);
    DEBUGCHK(pHidKbd->pbOutputBuffer != NULL);
    if (pHidKbd->fhThreadInited == TRUE) {
        DEBUGCHK(pHidKbd->hThread != NULL);
    }
    DEBUGCHK(pHidKbd->ARState <= AR_AUTOREPEATING);
    if (pHidKbd->fhOSDeviceInited == TRUE) {
        DEBUGCHK(pHidKbd->hOSDevice != NULL);
    }
    DEBUGCHK(pHidKbd->hevClosing != NULL);

    cbUsageList = pHidKbd->dwMaxUsages * sizeof(USAGE_AND_PAGE);
    DEBUGCHK(pHidKbd->puapPrevUsages != NULL);
    DEBUGCHK(LocalSize(pHidKbd->puapPrevUsages) >= cbUsageList * 5);
    DEBUGCHK((DWORD) pHidKbd->puapCurrUsages == ((DWORD) pHidKbd->puapPrevUsages) + cbUsageList);
    DEBUGCHK((DWORD) pHidKbd->puapBreakUsages == ((DWORD) pHidKbd->puapCurrUsages) + cbUsageList);
    DEBUGCHK((DWORD) pHidKbd->puapMakeUsages == ((DWORD) pHidKbd->puapBreakUsages) + cbUsageList);
    DEBUGCHK((DWORD) pHidKbd->puapOldMakeUsages == ((DWORD) pHidKbd->puapMakeUsages) + cbUsageList);
}


// Validate the given USAGE_TO_SC_INFO structure.
static 
void
ValidateUsageToSc(
    const USAGE_TO_SC_INFO *pUsageToScInfo
    )
{
    DEBUGCHK(pUsageToScInfo->pUsageToSc != NULL);
    DEBUGCHK(pUsageToScInfo->uFirstUsage <= pUsageToScInfo->uLastUsage);
}


// Validate the list of USAGE_TO_SC_INFO structures.
static
void
ValidateAllUsageToSc(
    )
{
    const USAGE_TO_SC_INFO *pUsageToScInfo;
    DWORD cUsageToScInfo = _countof(g_rgUsageToScInfo);
    DWORD dwOuter, dwInner;

    // Check the definitions of EB and EP
    DEBUGCHK(SET_EB(EB_COUNT) <= SET_PT(1));

    for (dwOuter = 0; dwOuter < cUsageToScInfo; ++dwOuter) 
    {
        pUsageToScInfo = &g_rgUsageToScInfo[dwOuter];
        ValidateUsageToSc(pUsageToScInfo);
    
        // Verify that this range does not overlap with any of the later ones
        USHORT uFirstUsage = pUsageToScInfo->uFirstUsage;
        USHORT uLastUsage = pUsageToScInfo->uLastUsage;
        for (dwInner = dwOuter + 1; dwInner < cUsageToScInfo; ++dwInner) {
            DEBUGCHK( (uFirstUsage < g_rgUsageToScInfo[dwInner].uFirstUsage) ||
                      (uLastUsage  > g_rgUsageToScInfo[dwInner].uLastUsage) );
        }

        // Now make sure that each USAGE_TO_SCANCODE is valid
        for (dwInner = 0; dwInner < (USHORT)(uLastUsage - uFirstUsage + 1); ++dwInner)
        {
            const USAGE_TO_SCANCODE *pUsageToSc = pUsageToScInfo->pUsageToSc;
            DEBUGCHK(GET_EB(pUsageToSc->uiFlags) < EB_COUNT);
            DEBUGCHK(GET_PT(pUsageToSc->uiFlags) < PT_COUNT);
        }
    }
}

#endif // DEBUG


