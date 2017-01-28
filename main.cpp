// 13 december 2016
#include "barspy.hpp"

HINSTANCE hInstance;
int nCmdShow;
HICON hDefIcon;
HCURSOR hDefCursor;
HBRUSH blackBrush;
HFONT hMessageFont;

void initCommon(HINSTANCE hInst, int nCS)
{
	INITCOMMONCONTROLSEX icc;
	NONCLIENTMETRICSW ncm;

	hInstance = hInst;
	nCmdShow = nCS;

	ZeroMemory(&icc, sizeof (INITCOMMONCONTROLSEX));
	icc.dwSize = sizeof (INITCOMMONCONTROLSEX);
	// TODO filter this down
	icc.dwICC = (ICC_LISTVIEW_CLASSES | ICC_TREEVIEW_CLASSES | ICC_BAR_CLASSES | ICC_TAB_CLASSES | ICC_UPDOWN_CLASS | ICC_PROGRESS_CLASS | ICC_HOTKEY_CLASS | ICC_ANIMATE_CLASS | ICC_WIN95_CLASSES | ICC_DATE_CLASSES | ICC_USEREX_CLASSES | ICC_COOL_CLASSES | ICC_INTERNET_CLASSES | ICC_PAGESCROLLER_CLASS | ICC_NATIVEFNTCTL_CLASS | ICC_STANDARD_CLASSES | ICC_LINK_CLASS);
	if (InitCommonControlsEx(&icc) == FALSE)
		panic(L"error calling InitCommonControlsEx(): %I32d", GetLastError());

	hDefIcon = LoadIconW(NULL, IDI_APPLICATION);
	if (hDefIcon == NULL)
		panic(L"error calling LoadIconW() to get default icon: %I32d", GetLastError());

	hDefCursor = LoadCursorW(NULL, IDC_ARROW);
	if (hDefCursor == NULL)
		panic(L"error calling LoadCursorW() to get default cursor: %I32d", GetLastError());

	blackBrush = (HBRUSH) GetStockObject(BLACK_BRUSH);
	if (blackBrush == NULL)
		panic(L"error calling GetStockObject() to get the stock black brush: %I32d", GetLastError());

	ZeroMemory(&ncm, sizeof (NONCLIENTMETRICSW));
	ncm.cbSize = sizeof (NONCLIENTMETRICSW);
	if (SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof (NONCLIENTMETRICSW), &ncm, sizeof (NONCLIENTMETRICSW)) == 0)
		panic(L"error calling SystemParametersInfoW() to get default fonts: %I32d", GetLastError());
	hMessageFont = CreateFontIndirectW(&(ncm.lfMessageFont));
	if (hMessageFont == NULL)
		panic(L"error calling CreateFontIndirectW() to load default messagebox font: %I32d", GetLastError());

	initProcess();
	initCheckmarks();
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
	MSG msg;
	BOOL res;

	initCommon(hInstance, nCmdShow);

	openMainWindow();
	for (;;) {
		res = GetMessageW(&msg, NULL, 0, 0);
		if (res < 0)
			panic(L"error calling GetMessageW(): %I32d", GetLastError());
		if (res == 0)		// WM_QUIT
			break;
		TranslateMessage(&msg);
		DispatchMessageW(&msg);
	}

	return 0;
}
