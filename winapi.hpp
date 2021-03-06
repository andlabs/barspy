// 31 may 2015
#define UNICODE
#define _UNICODE
#define STRICT
#define STRICT_TYPED_ITEMIDS

// make sure psapi functions use pre-Windows 7 linkage
#define PSAPI_VERSION 1

// get Windows version right; right now Windows Vista
// unless otherwise stated, all values from Microsoft's sdkddkver.h
// TODO is all of this necessary? how is NTDDI_VERSION used?
// TODO plaform update sp2
#define WINVER			0x0600	/* from Microsoft's winnls.h */
#define _WIN32_WINNT		0x0600
#define _WIN32_WINDOWS	0x0600	/* from Microsoft's pdh.h */
#define _WIN32_IE			0x0700
#define NTDDI_VERSION		0x06000000

#include <windows.h>

// Microsoft's resource compiler will segfault if we feed it headers it was not designed to handle
#ifndef RC_INVOKED
#include <commctrl.h>
#include <uxtheme.h>
#include <windowsx.h>
#include <shobjidl.h>
#include <strsafe.h>
#include <psapi.h>
#include <shlwapi.h>
#include <commoncontrols.h>

#include <stdint.h>
#include <string.h>
#include <wchar.h>
#include <stdarg.h>
#include <stdio.h>
#include <math.h>
#include <float.h>
#include <inttypes.h>

#include <vector>
#include <map>
#include <string>
#include <sstream>
#include <functional>
#endif
