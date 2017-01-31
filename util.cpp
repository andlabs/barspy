// 25 january 2017
#include "barspy.hpp"

// TODO keep?
int classNameOf(WCHAR *classname, ...)
{
	va_list ap;
	WCHAR *curname;
	int i;

	va_start(ap, classname);
	i = 0;
	for (;;) {
		curname = va_arg(ap, WCHAR *);
		if (curname == NULL)
			break;
		if (_wcsicmp(classname, curname) == 0) {
			va_end(ap);
			return i;
		}
		i++;
	}
	// no match
	va_end(ap);
	return -1;
}

// MSDN says 256 is the maximum length of a class name; add a few characters just to be safe (because it doesn't say whether this includes the terminating null character)
#define maxClassName 260

WCHAR *windowClass(HWND hwnd)
{
	WCHAR *classname;

	classname = new WCHAR[maxClassName + 1];
	if (GetClassNameW(hwnd, classname, maxClassName) == 0)
		panic(L"error getting name of window class: %I32d", GetLastError());
	return classname;
}

// TODO rewrite this in terms of the above, if kept?
int windowClassOf(HWND hwnd, ...)
{
	WCHAR classname[maxClassName + 1];
	va_list ap;
	WCHAR *curname;
	int i;

	if (GetClassNameW(hwnd, classname, maxClassName) == 0) {
		panic(L"error getting name of window class: %I32d", GetLastError());
		// assume no match on error, just to be safe
		return -1;
	}
	va_start(ap, hwnd);
	i = 0;
	for (;;) {
		curname = va_arg(ap, WCHAR *);
		if (curname == NULL)
			break;
		if (_wcsicmp(classname, curname) == 0) {
			va_end(ap);
			return i;
		}
		i++;
	}
	// no match
	va_end(ap);
	return -1;
}

HDWP beginDeferWindowPos(int n)
{
	HDWP dwp;

	dwp = BeginDeferWindowPos(n);
	if (dwp == NULL)
		panic(L"BeginDeferWindowPos() failed: %I32d\n", GetLastError());
	return dwp;
}

HDWP deferWindowPos(HDWP dwp, HWND hwnd, int x, int y, int width, int height, UINT flags)
{
	flags |= SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER;
	if (dwp == NULL) {
		if (SetWindowPos(hwnd, NULL,
			x, y,
			width, height,
			flags) == 0)
			panic(L"SetWindowPos() failed: %I32d", GetLastError());
		return NULL;
	}
	dwp = DeferWindowPos(dwp,
		hwnd, NULL,
		x, y,
		width, height,
		flags);
	if (dwp == NULL)
		panic(L"DeferWindowPos() failed: %I32d", GetLastError());
	return dwp;
}

void endDeferWindowPos(HDWP dwp)
{
	if (dwp == NULL)
		return;
	if (EndDeferWindowPos(dwp) == 0)
		panic(L"EndDeferWindowPos() failed: %I32d", GetLastError());
}

std::wstring colorToString(COLORREF color)
{
	uint8_t r, g, b;
	std::wostringstream ss;

	r = GetRValue(color);
	g = GetGValue(color);
	b = GetBValue(color);
	ss << L"(" << r << L", " << g << L", " << b << L")";
	return ss.str();
}

std::wstring sizeToString(SIZE size)
{
	std::wostringstream ss;

	ss << size.cx << L"x" << size.cy;
	return ss.str();
}

void deleteObject(HGDIOBJ obj)
{
	if (obj == NULL)
		return;
	if (DeleteObject(obj) == 0)
		panic(L"error deleting GDI object: %I32d", GetLastError());
}
