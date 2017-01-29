// 26 january 2017
#include "barspy.hpp"

// TODO this positions coordinates by parents and sizes by selves
// I forget if I ever asked if the children of windows had to have the same DPI as their parents... but if so, this is wrong

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

LONG longestTextWidth(const std::vector<HWND> hwnds)
{
	LONG current, next;

	current = 0;
	for (auto hwnd : hwnds) {
		next = Layouter(hwnd).TextWidth();
		if (current < next)
			current = next;
	}
	return current;
}

LONG longestTextWidth(HWND hwnd, ...)
{
	va_list ap;
	std::vector<HWND> hwnds;

	if (hwnds.capacity() < 16)
		hwnds.reserve(16);
	hwnds.push_back(hwnd);
	va_start(ap, hwnd);
	for (;;) {
		hwnd = va_arg(ap, HWND);
		if (hwnd == NULL)
			break;
		hwnds.push_back(hwnd);
	}
	va_end(ap);
	return longestTextWidth(hwnds);
}

Form::Form(HWND parent, int id)
{
	this->parent = parent;
	this->id = id;
	if (this->labels.capacity() < 16)
		this->labels.reserve(16);
	if (this->edits.capacity() < 16)
		this->edits.reserve(16);
}

int Form::ID(void)
{
	return this->id;
}

void Form::SetID(int id)
{
	this->id = id;
}

void Form::Add(const WCHAR *msg)
{
	HWND hwnd;

	hwnd = CreateWindowExW(0,
		L"STATIC", msg,
		WS_CHILD | WS_VISIBLE | SS_LEFTNOWORDWRAP | SS_NOPREFIX,
		0, 0, 100, 100,
		this->parent, (HMENU) (this->id), hInstance, NULL);
	if (hwnd == NULL)
		panic(L"error creating label: %I32d", GetLastError());
	SendMessageW(hwnd, WM_SETFONT, (WPARAM) hMessageFont, TRUE);
	this->labels.push_back(hwnd);
	this->id++;

	hwnd = CreateWindowExW(WS_EX_CLIENTEDGE,
		L"EDIT", L"",
		// TODO remove READONLY if this ever becomes an editor instead of just a viewer
		WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_LEFT | ES_NOHIDESEL | ES_READONLY,
		0, 0, 100, 100,
		this->parent, (HMENU) (this->id), hInstance, NULL);
	if (hwnd == NULL)
		panic(L"error creating edit: %I32d", GetLastError());
	SendMessageW(hwnd, WM_SETFONT, (WPARAM) hMessageFont, TRUE);
	this->edits.push_back(hwnd);
	this->id++;
}

void Form::SetText(int id, const WCHAR *text)
{
	if (SetWindowTextW(this->edits[id], text) == 0)
		panic(L"error setting form edit text: %I32d", GetLastError());
}

// TODO optional arrange horizontally
SIZE Form::MinimumSize(LONG minEditWidth, Layouter *dparent)
{
	SIZE s;
	LONG minLabelWidth;

	minLabelWidth = longestTextWidth(this->labels);
	// TODO optional padding
	s.cx = minLabelWidth + dparent->PaddingX() + minEditWidth;
	// TODO make sure label height + offset is always < edit height
	// this intuitively seems to be so
	s.cy = (LONG) (Layouter(this->edits[0]).EditHeight() * this->edits.size());
	s.cy += (LONG) (dparent->PaddingY() * (this->edits.size() - 1));
	return s;
}

HDWP Form::relayout(HDWP dwp, LONG x, LONG y, LONG width, bool widthIsEditOnly, Layouter *dparent)
{
	LONG labelwid, labelht;
	LONG editwid, editht;
	LONG xPadding, yPadding;
	LONG yLine;
	Layouter *d;
	size_t i, n;

	xPadding = dparent->PaddingX();
	yPadding = dparent->PaddingY();
	labelwid = longestTextWidth(this->labels);
	d = new Layouter(this->labels[0]);
	labelht = d->LabelHeight();
	yLine = dparent->LabelYForSiblingY(0, d);
	delete d;
	editwid = width;
	if (!widthIsEditOnly)
		editwid -= labelwid + xPadding;
	editht = Layouter(this->edits[0]).EditHeight();

	n = this->labels.size();
	for (i = 0; i < n; i++) {
		dwp = deferWindowPos(dwp, this->labels[i],
			x, y + yLine,
			labelwid, labelht,
			0);
		dwp = deferWindowPos(dwp, this->edits[i],
			x + labelwid + xPadding, y,
			editwid, editht,
			0);
		// TODO don't assume edits are always taller than labels? see above
		y += editht + yPadding;
	}
	return dwp;
}

HDWP Form::Relayout(HDWP dwp, LONG x, LONG y, LONG width, Layouter *dparent)
{
	return this->relayout(dwp, x, y, width, false, dparent);
}

HDWP Form::RelayoutEditWidth(HDWP dwp, LONG x, LONG y, LONG width, Layouter *dparent)
{
	return this->relayout(dwp, x, y, width, true, dparent);
}
