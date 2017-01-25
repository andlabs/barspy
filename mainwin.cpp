// 13 december 2016
#include "barspy.hpp"

HWND mainwin = NULL;

class mainwinClass {
	HWND hwnd;
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
	LRESULT windowPosChanged(WPARAM wParam, LPARAM lParam, struct metrics *m);
	void getDLUBase(int *dluBaseX, int *dluBaseY);
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

LRESULT mainwinClass::windowPosChanged(WPARAM wParam, LPARAM lParam, struct metrics *m)
{
	WINDOWPOS *wp = (WINDOWPOS *) lParam;

	if ((wp->flags & SWP_NOSIZE) != 0)
		return DefWindowProcW(this->hwnd, WM_WINDOWPOSCHANGED, wParam, lParam);
	// TODO
	return 0;
}

LRESULT mainwinClass::WndProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LRESULT lResult;
	HRESULT hr;

	switch (uMsg) {
	case WM_WINDOWPOSCHANGED:
		return this->windowPosChanged(wParam, lParam, &m);
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

	mainwin = CreateWindowExW(0,
		L"mainwin", APPTITLE,
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT,
		400, 400,
		NULL, NULL, hInstance, NULL);
	if (mainwin == NULL)
		panic(L"error creating main window: %I32d", GetLastError());

	ShowWindow(mainwin, nCmdShow);
	if (UpdateWindow(mainwin) == 0)
		panic(L"error performing the initial UpdateWindow() on the main window: %I32d", GetLastError());
}
