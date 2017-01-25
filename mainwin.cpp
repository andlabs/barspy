// 13 december 2016
#include "barspy.hpp"

HWND mainwin = NULL;

class mainwinClass {
	HWND hwnd;
	HWND winlist;
public:
	mainwinClass(HWND h)
	{
		this->hwnd = h;
	}

	~mainwinClass()
	{
		// do nothing
	}

	LRESULT WndProc(UINT uMsg, WPARAM wParam, LPARAM lParam);
	void relayout(void);
	void getDLUBase(int *dluBaseX, int *dluBaseY);
	BOOL onNotify(NMHDR *hdr, LRESULT *lResult);
};

void mainwinClass::getDLUBase(int *dluBaseX, int *dluBaseY)
{
	HDC dc;
	HFONT prevfont;
	TEXTMETRICW tm;
	SIZE size;

	dc = GetDC(this->hwnd);
	if (dc == NULL)
		panic(L"error getting DC in windowClass::getDLUBase(): %I32d", GetLastError());
	prevfont = (HFONT) SelectObject(dc, hMessageFont);
	if (prevfont == NULL)
		panic(L"error selecting font in windowClass::getDLUBase(): %I32d", GetLastError());

	ZeroMemory(&tm, sizeof (TEXTMETRICW));
	if (GetTextMetricsW(dc, &tm) == 0)
		panic(L"error getting text metrics in windowClass::getDLUBase(): %I32d", GetLastError());
	if (GetTextExtentPoint32W(dc, L"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz", 52, &size) == 0)
		panic(L"error getting text extents in windowClass::getDLUBase(): %I32d", GetLastError());

	*dluBaseX = (int) ((size.cx / 26 + 1) / 2);
	*dluBaseY = (int) tm.tmHeight;

	SelectObject(dc, prevfont);
	ReleaseDC(hwnd, dc);
}

void mainwinClass::relayout(void)
{
	RECT client;
	int dluBaseX, dluBaseY;
	int marginsX, marginsY, paddingX;
	LONG tableWidth;

	if (GetClientRect(this->hwnd, &client) == 0)
		panic(L"error getting window client rect: %I32d", GetLastError());

	this->getDLUBase(&dluBaseX, &dluBaseY);
	// TODO sort this out
	marginsX = MulDiv(7, dluBaseX, 4);
	marginsY = MulDiv(7, dluBaseY, 8);
	paddingX = MulDiv(4, dluBaseX, 4);

	tableWidth = (client.right - client.left - 2 * marginsX - paddingX) / 3;

	if (SetWindowPos(this->winlist, NULL,
		marginsX, marginsY,
		tableWidth,
		(client.bottom - client.top - 2 * marginsY),
		SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER) == 0)
		panic(L"error positioning window list: %I32d", GetLastError());
}

static HTREEITEM addWindow(HWND treeview, HWND window, HTREEITEM parent)
{
	TVINSERTSTRUCTW tis;
	HTREEITEM item;

	ZeroMemory(&tis, sizeof (TVINSERTSTRUCTW));
	tis.hParent = parent;
	tis.hInsertAfter = TVI_LAST;
	tis.itemex.mask = TVIF_PARAM | TVIF_TEXT;
	tis.itemex.pszText = LPSTR_TEXTCALLBACK;
	tis.itemex.lParam = (LPARAM) window;
	if (windowClassOf(window, DESIREDCLASSES, NULL) != -1) {
		tis.itemex.mask |= TVIF_STATE;
		tis.itemex.state = TVIS_BOLD;
		tis.itemex.stateMask = TVIS_BOLD;
	}
	item = (HTREEITEM) SendMessageW(treeview, TVM_INSERTITEMW, 0, (LPARAM) (&tis));
	if (item == NULL)
		panic(L"error adding window to window list: %I32d", GetLastError());
	return item;
}

static BOOL setWinListLabel(NMTVDISPINFOW *nm, LRESULT *lResult)
{
	HWND hwnd;
	WCHAR *cls;
	WCHAR text[64];

	if ((nm->item.mask & TVIF_TEXT) == 0)
		return FALSE;

	hwnd = (HWND) (nm->item.lParam);
	cls = windowClass(hwnd);
	// TODO erorr check
	// TODO dynamically allocate?
	GetWindowTextW(hwnd, text, 63);

	// TODO error check?
	// TODO does cchText include the null terminator?
	StringCchPrintfW(nm->item.pszText, nm->item.cchTextMax,
		L"%p %s %s",
		hwnd, cls, text);

	delete[] cls;
	*lResult = 0;
	return TRUE;
}

BOOL mainwinClass::onNotify(NMHDR *hdr, LRESULT *lResult)
{
	if (hdr->hwndFrom != this->winlist)
		return FALSE;
	switch (hdr->code) {
	case TVN_GETDISPINFOW:
		return setWinListLabel((NMTVDISPINFOW *) hdr, lResult);
	}
	return FALSE;
}

LRESULT mainwinClass::WndProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	WINDOWPOS *wp = (WINDOWPOS *) lParam;
	LRESULT lResult;

	switch (uMsg) {
	case WM_CREATE:
		this->winlist = CreateWindowExW(WS_EX_CLIENTEDGE,
			WC_TREEVIEWW, L"",
			WS_CHILD | WS_VISIBLE | WS_HSCROLL | WS_VSCROLL | TVS_DISABLEDRAGDROP | TVS_HASBUTTONS | TVS_HASLINES | TVS_LINESATROOT | TVS_NONEVENHEIGHT | TVS_SHOWSELALWAYS,
			0, 0,
			200, 200,
			this->hwnd, (HMENU) 101, hInstance, NULL);
		if (this->winlist == NULL)
			panic(L"error creating window list: %I32d", GetLastError());
		enumWindowTree(this->winlist, addWindow);
		this->relayout();
		break;
	case WM_WINDOWPOSCHANGED:
		if ((wp->flags & SWP_NOSIZE) != 0)
			break;
		this->relayout();
		return 0;
	case WM_NOTIFY:
		if (this->onNotify((NMHDR *) lParam, &lResult))
			return lResult;
		break;
	case WM_DESTROY:
		SetWindowLongPtrW(this->hwnd, GWLP_USERDATA, (LONG_PTR) NULL);
		// call this now so we can safely delete this afterward
		lResult = DefWindowProcW(this->hwnd, uMsg, wParam, lParam);
		delete this;
		return lResult;
	case WM_CLOSE:
		PostQuitMessage(0);
		break;
	}
	return DefWindowProcW(this->hwnd, uMsg, wParam, lParam);
}

static LRESULT CALLBACK mainwinWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	mainwinClass *w;

	w = (mainwinClass *) GetWindowLongPtrW(hwnd, GWLP_USERDATA);
	if (w == NULL) {
		if (uMsg == WM_NCCREATE) {
			w = new mainwinClass(hwnd);
			SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR) w);
		}
		// and fall through unconditionally
		return DefWindowProcW(hwnd, uMsg, wParam, lParam);
	}
	return w->WndProc(uMsg, wParam, lParam);
}

void openMainWindow(void)
{
	WNDCLASSW wc;

	ZeroMemory(&wc, sizeof (WNDCLASSW));
	wc.lpszClassName = L"mainwin";
	wc.lpfnWndProc = mainwinWndProc;
	wc.hInstance = hInstance;
	wc.hIcon = hDefIcon;
	wc.hCursor = hDefCursor;
	wc.hbrBackground = (HBRUSH) (COLOR_BTNFACE + 1);
	if (RegisterClassW(&wc) == 0)
		panic(L"error registering main window class: %I32d", GetLastError());

	// TODO adjustwindowrect this?
	mainwin = CreateWindowExW(0,
		L"mainwin", APPTITLE,
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT,
		800, 450,
		NULL, NULL, hInstance, NULL);
	if (mainwin == NULL)
		panic(L"error creating main window: %I32d", GetLastError());

	ShowWindow(mainwin, nCmdShow);
	if (UpdateWindow(mainwin) == 0)
		panic(L"error performing the initial UpdateWindow() on the main window: %I32d", GetLastError());
}
