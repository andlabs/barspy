// 13 december 2016
#include "barspy.hpp"

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
