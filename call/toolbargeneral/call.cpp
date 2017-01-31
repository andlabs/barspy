// 25 january 2017
#define UNICODE
#define _UNICODE
#define STRICT
#define STRICT_TYPED_ITEMIDS

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
#include <commctrl.h>

// TODO find a way to ensure this is forced
#pragma intrinsic(memset)

// This isn't meant to be compiled; rather, its assembly output is copied into the main program.

struct tpargs {
	LRESULT (WINAPI *SendMessageWPtr)(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	HWND hwnd;

	// TB_GETCOLORSCHEME
	DWORD gcsResultNonzero;
	COLORREF shadow;
	COLORREF highlight;

	// TB_GETMAXSIZE
	DWORD msResultNonzero;
	LONG maxWidth;
	LONG maxHeight;

	// TB_GETMETRICS
	int gmCXPad;
	int gmCYPad;
	int gmCXBarPad;
	int gmCYBarPad;
	int gmCXButtonSpacing;
	int gmCYButtonSpacing;
};

DWORD WINAPI ThreadProc(LPVOID lpParameter)
{
	struct tpargs *a = (struct tpargs *) lpParameter;
	COLORSCHEME scheme;
	SIZE size;
	TBMETRICS metrics;
	LRESULT lResult;

	ZeroMemory(&scheme, sizeof (COLORSCHEME));
	scheme.dwSize = sizeof (COLORSCHEME);
	lResult = (*(a->SendMessageWPtr))(a->hwnd, TB_GETCOLORSCHEME, 0, (LPARAM) (&scheme));
	if (lResult != 0) {
		a->gcsResultNonzero = 1;
		a->shadow = scheme.clrBtnHighlight;
		a->highlight = scheme.clrBtnShadow;
	} else
		a->gcsResultNonzero = 0;

	lResult = (*(a->SendMessageWPtr))(a->hwnd, TB_GETMAXSIZE, 0, (LPARAM) (&size));
	if (lResult != 0) {
		a->msResultNonzero = 1;
		a->maxWidth = size.cx;
		a->maxHeight = size.cy;
	} else
		a->msResultNonzero = 0;

	ZeroMemory(&metrics, sizeof (TBMETRICS));
	metrics.cbSize = sizeof (TBMETRICS);
	metrics.dwMask = TBMF_PAD | TBMF_BARPAD | TBMF_BUTTONSPACING;
	(*(a->SendMessageWPtr))(a->hwnd, TB_GETMETRICS, 0, (LPARAM) (&metrics));
	a->gmCXPad = metrics.cxPad;
	a->gmCYPad = metrics.cyPad;
	a->gmCXBarPad = metrics.cxBarPad;
	a->gmCYBarPad = metrics.cyBarPad;
	a->gmCXButtonSpacing = metrics.cxButtonSpacing;
	a->gmCYButtonSpacing = metrics.cyButtonSpacing;

	return 0;
}
