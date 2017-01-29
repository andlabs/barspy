// 26 january 2017
#include "barspy.hpp"

// create a Layouter at the top level and pass it down for all children
// this is safe; see http://stackoverflow.com/questions/41917279/do-child-windows-have-the-same-dpi-as-their-parents-in-a-per-monitor-aware-appli

Layouter::Layouter(HWND hwnd)
{
	SIZE size;

	this->hwnd = hwnd;
	this->dc = GetDC(this->hwnd);
	if (dc == NULL)
		panic(L"error getting DC for layout calculations: %I32d", GetLastError());
	this->prevfont = (HFONT) SelectObject(this->dc, hMessageFont);
	if (prevfont == NULL)
		panic(L"error selecting font for layout calculations: %I32d", GetLastError());

	ZeroMemory(&(this->tm), sizeof (TEXTMETRICW));
	if (GetTextMetricsW(this->dc, &(this->tm)) == 0)
		panic(L"error getting text metrics for layout calculations: %I32d", GetLastError());
	if (GetTextExtentPoint32W(this->dc, L"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz", 52, &size) == 0)
		panic(L"error getting text extents for DLU calculations: %I32d", GetLastError());

	this->baseX = (int) ((size.cx / 26 + 1) / 2);
	this->baseY = (int) tm.tmHeight;
}

Layouter::~Layouter()
{
	if (SelectObject(this->dc, this->prevfont) != hMessageFont)
		panic(L"error unselecting font for layout calculations: %I32d", GetLastError());
	if (ReleaseDC(this->hwnd, this->dc) == 0)
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

LONG Layouter::TextWidth(const WCHAR *text, size_t len)
{
	SIZE textSize;

	if (GetTextExtentPoint32W(this->dc, text, (int) len, &textSize) == 0)
		panic(L"error getting text extents for layout calculations: %I32d", GetLastError());
	return textSize.cx;
}

LONG Layouter::TextWidth(const WCHAR *text)
{
	return this->TextWidth(text, wcslen(text));
}

LONG Layouter::TextWidth(HWND hwnd)
{
	LRESULT len;
	WCHAR *text;
	LONG ret;

	len = SendMessageW(hwnd, WM_GETTEXTLENGTH, 0, 0);
	text = new WCHAR[len + 1];
	if (GetWindowTextW(hwnd, text, (int) (len + 1)) != len)
		panic(L"error getting window text for layout calculations: %I32d", GetLastError());
	ret = this->TextWidth(text, len);
	delete[] text;
	return ret;
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

int Layouter::LabelYForSiblingY(int siblingY)
{
	return siblingY + this->Y(labelYOffset) - this->InternalLeading();
}

int Layouter::LabelHeight(void)
{
	return this->Y(labelHeight);
}

LONG longestTextWidth(Layouter *d, const std::vector<HWND> &hwnds)
{
	LONG current, next;

	current = 0;
	for (auto hwnd : hwnds) {
		next = d->TextWidth(hwnd);
		if (current < next)
			current = next;
	}
	return current;
}

// note: will fail if Ts is not a pack of HWND
template<typename... Ts>
LONG longestTextWidth(Layouter *d, HWND first, Ts... hwnds)
{
	std::vector<HWND> hv {first, hwnds...};

	// gracefully handle accidentally-NULL-terminated lists
	if (hv.back() == NULL)
		hv.pop_back();
	return longestTextWidth(d, hv);
}

Form::Form(HWND parent, int id, int minEditWidth)
{
	this->parent = parent;
	this->id = id;
	this->minEditWidth = minEditWidth;
	this->padded = true;
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

void Form::SetMinEditWidth(int minEditWidth)
{
	this->minEditWidth = minEditWidth;
}

void Form::SetPadded(bool padded)
{
	this->padded = padded;
}

void Form::Add(const WCHAR *label)
{
	HWND hwnd;

	// TODO split control creation into static global functions
	hwnd = CreateWindowExW(0,
		L"STATIC", label,
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

void Form::padding(Layouter *d, LONG *x, LONG *y)
{
	if (!this->padded) {
		*x = 0;
		*y = 0;
		return;
	}
	*x = d->PaddingX();
	*y = d->PaddingY();
}

SIZE Form::MinimumSize(Layouter *d)
{
	SIZE s;
	LONG minLabelWidth;
	LONG xPadding, yPadding;

	this->padding(d, &xPadding, &yPadding);
	minLabelWidth = longestTextWidth(d, this->labels);
	s.cx = minLabelWidth + xPadding + this->minEditWidth;
	// TODO make sure label height + offset is always < edit height
	// this intuitively seems to be so
	s.cy = (LONG) (d->EditHeight() * this->labels.size());
	s.cy += (LONG) (yPadding * (this->labels.size() - 1));
	return s;
}

HDWP Form::relayout(HDWP dwp, LONG x, LONG y, bool useWidth, LONG width, bool widthIsEditOnly, Layouter *d)
{
	LONG labelwid, labelht;
	LONG editwid, editht;
	LONG xPadding, yPadding;
	LONG yLine;
	size_t i, n;

	this->padding(d, &xPadding, &yPadding);
	labelwid = longestTextWidth(d, this->labels);
	labelht = d->LabelHeight();
	yLine = d->LabelYForSiblingY(0);
	editwid = this->minEditWidth;
	if (useWidth) {
		editwid = width;
		if (!widthIsEditOnly)
			editwid -= labelwid + xPadding;
	}
	editht = d->EditHeight();

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

HDWP Form::Relayout(HDWP dwp, LONG x, LONG y, Layouter *d)
{
	return this->relayout(dwp, x, y, false, 0, false, d);
}

HDWP Form::RelayoutWidth(HDWP dwp, LONG x, LONG y, LONG width, Layouter *d)
{
	return this->relayout(dwp, x, y, true, width, false, d);
}

HDWP Form::RelayoutEditWidth(HDWP dwp, LONG x, LONG y, LONG width, Layouter *d)
{
	return this->relayout(dwp, x, y, true, width, true, d);
}

Chain::Chain(HWND parent, int id, int minEditWidth)
{
	this->parent = parent;
	this->id = id;
	this->minEditWidth = minEditWidth;
	this->padded = true;
	if (this->labels.capacity() < 16)
		this->labels.reserve(16);
	if (this->edits.capacity() < 16)
		this->edits.reserve(16);
}

int Chain::ID(void)
{
	return this->id;
}

void Chain::SetID(int id)
{
	this->id = id;
}

void Chain::SetMinEditWidth(int minEditWidth)
{
	this->minEditWidth = minEditWidth;
}

void Chain::SetPadded(bool padded)
{
	this->padded = padded;
}

void Chain::Add(const WCHAR *label)
{
	HWND hwnd;

	this->AddTrailingLabel(label);
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

void Chain::AddTrailingLabel(const WCHAR *label)
{
	HWND hwnd;

	hwnd = CreateWindowExW(0,
		L"STATIC", label,
		WS_CHILD | WS_VISIBLE | SS_LEFTNOWORDWRAP | SS_NOPREFIX,
		0, 0, 100, 100,
		this->parent, (HMENU) (this->id), hInstance, NULL);
	if (hwnd == NULL)
		panic(L"error creating label: %I32d", GetLastError());
	SendMessageW(hwnd, WM_SETFONT, (WPARAM) hMessageFont, TRUE);
	this->labels.push_back(hwnd);
	this->id++;
}

void Chain::SetText(int id, const WCHAR *text)
{
	if (SetWindowTextW(this->edits[id], text) == 0)
		panic(L"error setting form edit text: %I32d", GetLastError());
}

void Chain::padding(Layouter *d, LONG *x, LONG *y)
{
	if (!this->padded) {
		*x = 0;
		*y = 0;
		return;
	}
	*x = d->PaddingX();
	*y = d->PaddingY();
}

SIZE Chain::MinimumSize(Layouter *d)
{
	SIZE s;
	LONG xPadding, yPadding;

	this->padding(d, &xPadding, &yPadding);
	s.cx = (LONG) ((this->minEditWidth + xPadding) * this->edits.size());
	for (auto hwnd : this->labels)
		s.cx += d->TextWidth(hwnd);
	// TODO make sure label height + offset is always < edit height
	// this intuitively seems to be so
	s.cy = d->EditHeight();
	return s;
}

HDWP Chain::Relayout(HDWP dwp, LONG x, LONG y, Layouter *d)
{
	LONG labelwid, labelht;
	LONG editwid, editht;
	LONG xPadding, yPadding;
	LONG yLine;
	size_t i, n;
	bool hasTrailingLabel;

	this->padding(d, &xPadding, &yPadding);
	labelht = d->LabelHeight();
	yLine = d->LabelYForSiblingY(0);
	editwid = this->minEditWidth;
	editht = d->EditHeight();

	n = this->labels.size();
	hasTrailingLabel = n != this->edits.size();
	for (i = 0; i < n; i++) {
		labelwid = d->TextWidth(this->labels[i]);
		dwp = deferWindowPos(dwp, this->labels[i],
			x, y + yLine,
			labelwid, labelht,
			0);
		if (i == (n - 1) && hasTrailingLabel)
			break;
		x += labelwid + xPadding;
		dwp = deferWindowPos(dwp, this->edits[i],
			x, y,
			editwid, editht,
			0);
		// TODO don't assume edits are always taller than labels? see above
		x += editwid + xPadding;
	}
	return dwp;
}
