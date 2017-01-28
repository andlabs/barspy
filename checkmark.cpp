// 28 january 2017
#include "barspy.hpp"

HICON hIconYes;
HICON hIconNo;
HICON hIconUnknown;

void initCheckmarks(void)
{
	hIconYes = (HICON) LoadImage(hInstance, MAKEINTRESOURCE(iconYes),
		IMAGE_ICON,
		0, 0,
		0);
	if (hIconYes == NULL)
		panic(L"error loading Yes icon: %I32d", GetLastError());
	hIconNo = (HICON) LoadImage(hInstance, MAKEINTRESOURCE(iconNo),
		IMAGE_ICON,
		0, 0,
		0);
	if (hIconNo == NULL)
		panic(L"error loading No icon: %I32d", GetLastError());
	hIconUnknown = (HICON) LoadImage(hInstance, MAKEINTRESOURCE(iconUnknown),
		IMAGE_ICON,
		0, 0,
		0);
	if (hIconUnknown == NULL)
		panic(L"error loading Unknown icon: %I32d", GetLastError());
}

// hooray, MAKEINTRESOURCE(iconYes) can't be used with SS_ICON (it crashes)
// and we can't use # and ## outside of a macro definition
#define _MKSSICON(id) L"#" L ## #id
#define MKSSICON(id) _MKSSICON(id)

HWND newCheckmark(HWND parent, HMENU id)
{
	HWND hwnd;

	hwnd = CreateWindowExW(0,
		L"STATIC", MKSSICON(iconUnknown),
		WS_CHILD | WS_VISIBLE | SS_ICON | SS_REALSIZEIMAGE,
		0, 0, 100, 100,
		parent, id, hInstance, NULL);
	if (hwnd == NULL)
		panic(L"error creating checkmark static control: %I32d", GetLastError());
	return hwnd;
}

void setCheckmarkIcon(HWND hwnd, HICON icon)
{
	if (SendMessageW(hwnd, STM_SETICON, (WPARAM) icon, 0) == 0)
		panic(L"error resetting checkmark icon: %I32d", GetLastError());
}

SIZE checkmarkSize(HWND hwnd)
{
	RECT r;
	SIZE s;

	if (GetWindowRect(hwnd, &r) == 0)
		panic(L"error getting window rect of checkmark to get its size: %I32d", GetLastError());
	s.cx = r.right - r.left;
	s.cy = r.bottom - r.top;
	return s;
}
