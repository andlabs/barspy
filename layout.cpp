// 26 january 2017
#include "barspy.hpp"

Layouter::Layouter(HWND hwnd)
{
	HDC dc;
	HFONT prevfont;
	SIZE size;
	LRESULT len;
	WCHAR *text;

	dc = GetDC(hwnd);
	if (dc == NULL)
		panic(L"error getting DC for layout calculations: %I32d", GetLastError());
	prevfont = (HFONT) SelectObject(dc, hMessageFont);
	if (prevfont == NULL)
		panic(L"error selecting font for layout calculations: %I32d", GetLastError());

	ZeroMemory(&(this->tm), sizeof (TEXTMETRICW));
	if (GetTextMetricsW(dc, &(this->tm)) == 0)
		panic(L"error getting text metrics for layout calculations: %I32d", GetLastError());
	if (GetTextExtentPoint32W(dc, L"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz", 52, &size) == 0)
		panic(L"error getting text extents for DLU calculations: %I32d", GetLastError());

	this->baseX = (int) ((size.cx / 26 + 1) / 2);
	this->baseY = (int) tm.tmHeight;

	len = SendMessageW(hwnd, WM_GETTEXTLENGTH, 0, 0);
	text = new WCHAR[len + 1];
	if (GetWindowTextW(hwnd, text, (int) (len + 1)) != len)
		panic(L"error getting window text for layout calculations: %I32d", GetLastError());
	if (GetTextExtentPoint32W(dc, text, (int) len, &(this->textSize)) == 0)
		panic(L"error getting window text extents for layout calculations: %I32d", GetLastError());
	delete[] text;

	if (SelectObject(dc, prevfont) != hMessageFont)
		panic(L"error unselecting font for layout calculations: %I32d", GetLastError());
	if (ReleaseDC(hwnd, dc) == 0)
		panic(L"error releasing DC for layout calculations: %I32d", GetLastError());
}

int Layouter::X(int x)
{
	return MulDiv(x, this->baseX, 4);
}

int Layouter::Y(int y)
{
	return MulDiv(y, this->baseY, 8);
}

LONG Layouter::InternalLeading(void)
{
	return this->tm.tmInternalLeading;
}

LONG Layouter::TextWidth(void)
{
	return this->textSize.cx;
}

// from https://msdn.microsoft.com/en-us/library/windows/desktop/dn742486.aspx#sizingandspacing and https://msdn.microsoft.com/en-us/library/windows/desktop/bb226818%28v=vs.85%29.aspx
// this X value is really only for buttons but I don't see a better one :/
#define winXPadding 4
// TODO is this too much?
#define winYPadding 4

int Layouter::PaddingX(void)
{
	return this->X(winXPadding);
}

int Layouter::PaddingY(void)
{
	return this->Y(winYPadding);
}

// from https://msdn.microsoft.com/en-us/library/windows/desktop/dn742486.aspx#sizingandspacing
#define windowMargin 7

int Layouter::WindowMarginX(void)
{
	return this->X(windowMargin);
}

int Layouter::WindowMarginY(void)
{
	return this->Y(windowMargin);
}

int Layouter::EditHeight(void)
{
	// TODO
	return this->Y(12);
}

// via http://msdn.microsoft.com/en-us/library/windows/desktop/dn742486.aspx#sizingandspacing
#define labelHeight 8
#define labelYOffset 3

int Layouter::LabelYForSiblingY(int siblingY, Layouter *label)
{
	return siblingY + this->Y(labelYOffset) - label->InternalLeading();
}

int Layouter::LabelHeight(void)
{
	return this->Y(labelHeight);
}
