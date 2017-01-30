// 13 december 2016
#include "barspy.hpp"

HWND mainwin = NULL;

class mainwinClass {
	HWND hwnd;
	HWND winlist;

	Common *common;

	int current;
	HWND instructions;
	ToolbarTab *toolbarTab;
public:
	mainwinClass(HWND h)
	{
		this->hwnd = h;
	}

	~mainwinClass()
	{
		delete this->common;
	}

	LRESULT WndProc(UINT uMsg, WPARAM wParam, LPARAM lParam);
	void relayout(void);
	void setCurrent(int c);
	void load(NMTREEVIEWW *nm);
	BOOL onNotify(NMHDR *nm, LRESULT *lResult);
	void createEverything(void);
};

void mainwinClass::relayout(void)
{
	RECT client;
	Layouter *d;
	LONG tableWidth;
	SIZE commonSize;
	HDWP dwp;

	if (GetClientRect(this->hwnd, &client) == 0)
		panic(L"error getting window client rect: %I32d", GetLastError());
	d = new Layouter(this->hwnd);
	dwp = beginDeferWindowPos(64);

	tableWidth = (client.right - client.left - 2 * d->WindowMarginX() - d->PaddingX()) / 3;

	dwp = deferWindowPos(dwp, this->winlist,
		d->WindowMarginX(), d->WindowMarginY(),
		tableWidth,
		client.bottom - client.top - 2 * d->WindowMarginY(),
		0);

	commonSize = this->common->MinimumSize(d);
	client.left += d->WindowMarginX() + tableWidth + d->PaddingX();
	client.top += d->WindowMarginY();
	client.right -= d->WindowMarginX();
	client.bottom -= d->WindowMarginY();
	dwp = this->common->Relayout(dwp, &client, d);
	client.top += commonSize.cy + d->PaddingY();

	switch (this->current) {
	case 0:		// instructions
		dwp = deferWindowPos(dwp, this->instructions,
			client.left, client.top,
			client.right - client.left, client.bottom - client.top,
			0);
		// this seems to be necessary to get the control to redraw correctly
		if (InvalidateRect(this->instructions, NULL, TRUE) == 0)
			panic(L"error queueing redraw of instructions label: %I32d", GetLastError());
		break;
	case 1:		// toolbar tab
		dwp = this->toolbarTab->Relayout(dwp, &client, d);
		break;
	case 2:
		;	// TODO
	}

	endDeferWindowPos(dwp);
	delete d;
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

void mainwinClass::setCurrent(int c)
{
	if (this->current == c)
		return;
	switch (this->current) {
	case 0:		// instructions
		ShowWindow(this->instructions, SW_HIDE);
		break;
	case 1:		// toolbar tab
		this->toolbarTab->Show(SW_HIDE);
		break;
	case 2:		// rebar tab
		;	// TODO
	}
	this->current = c;
	switch (this->current) {
	case 0:		// instructions
		ShowWindow(this->instructions, SW_SHOW);
		break;
	case 1:		// toolbar tab
		this->toolbarTab->Show(SW_SHOW);
		break;
	case 2:		// rebar tab
		;	// TODO
	}
	this->relayout();
}

void mainwinClass::load(NMTREEVIEWW *nm)
{
	HWND hwnd;
	Process *p;
	int which;

	// TODO is there a better way?
	if (nm->itemNew.lParam == 0)return;

	hwnd = (HWND) (nm->itemNew.lParam);
	which = windowClassOf(hwnd, DESIREDCLASSES, NULL);

	p = processFromHWND(hwnd);
	this->common->Reflect(hwnd, p);
	switch (which) {
	case DESIREDTOOLBAR:
		this->setCurrent(1);
		this->toolbarTab->Reflect(hwnd, p);
		break;
	case DESIREDREBAR:
		// TODO
		break;
	default:
		this->setCurrent(0);
		break;
	}
	delete p;
}

BOOL mainwinClass::onNotify(NMHDR *nm, LRESULT *lResult)
{
	if (windowClassOf(nm->hwndFrom, WC_TABCONTROLW, NULL) != -1)
		switch (this->current) {
		case 1:		// toolbar tab
			if (this->toolbarTab->HandleNotify(nm, lResult))
				return TRUE;
			break;	// fall through to the next if (which will just return FALSE for us)
		case 2:		// rebar tab
			;	// TODO
		}
	if (nm->hwndFrom != this->winlist)
		return FALSE;
	switch (nm->code) {
	case TVN_GETDISPINFOW:
		return setWinListLabel((NMTVDISPINFOW *) nm, lResult);
	case TVN_SELCHANGEDW:
		this->load((NMTREEVIEWW *) nm);
		break;		// just do the default thing; the return value doesn't matter anyway
	}
	return FALSE;
}

void mainwinClass::createEverything(void)
{
	// window list
	this->winlist = CreateWindowExW(WS_EX_CLIENTEDGE,
		WC_TREEVIEWW, L"",
		WS_CHILD | WS_VISIBLE | WS_HSCROLL | WS_VSCROLL | TVS_DISABLEDRAGDROP | TVS_HASBUTTONS | TVS_HASLINES | TVS_LINESATROOT | TVS_NONEVENHEIGHT | TVS_SHOWSELALWAYS,
		0, 0, 100, 100,
		this->hwnd, (HMENU) 101, hInstance, NULL);
	if (this->winlist == NULL)
		panic(L"error creating window list: %I32d", GetLastError());

	// common items
	this->common = new Common(this->hwnd, 200);
	this->common->Reset();

	// content
	this->current = 0;		// instructions

	// instructions (current == 0)
	this->instructions = CreateWindowExW(WS_EX_CLIENTEDGE,//TODO 0,
		L"STATIC", L"Click on a boldface item at left to begin.",
		WS_CHILD | WS_VISIBLE | SS_CENTER | SS_CENTERIMAGE | SS_NOPREFIX,
		0, 0, 100, 100,
		this->hwnd, (HMENU) 102, hInstance, NULL);
	if (this->instructions == NULL)
		panic(L"error creating instructions label: %I32d", GetLastError());
	SendMessageW(this->instructions, WM_SETFONT, (WPARAM) hMessageFont, (LPARAM) TRUE);

	// toolbar tab (current == 1)
	this->toolbarTab = new ToolbarTab(this->hwnd, 200);
	this->toolbarTab->Show(SW_HIDE);

	// rebar tab (current == 2)
	// TODO

	// and set up the initial state
	enumWindowTree(this->winlist, addWindow);
	this->relayout();
	// and start using the scroll wheel properly
	SetFocus(this->winlist);
}

LRESULT mainwinClass::WndProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	WINDOWPOS *wp = (WINDOWPOS *) lParam;
	LRESULT lResult;

	switch (uMsg) {
	case WM_CREATE:
		this->createEverything();
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
