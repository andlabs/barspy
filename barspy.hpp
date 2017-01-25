// 13 december 2016
#include "winapi.hpp"

#define APPTITLE L"BarSpy"

// common.cpp
extern HINSTANCE hInstance;
extern int nCmdShow;
extern HICON hDefIcon;
extern HCURSOR hDefCursor;
extern HBRUSH blackBrush;
extern HFONT hMessageFont;
extern void initCommon(HINSTANCE hInst, int nCS);

// mainwin.cpp
extern HWND mainwin;
extern void openMainWindow(void);

// panic.cpp
extern void panic(const WCHAR *msg, ...);

// util.cpp
extern WCHAR *windowClass(HWND hwnd);
extern int windowClassOf(HWND hwnd, ...);
