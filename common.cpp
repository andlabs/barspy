// 26 january 2017
#include "barspy.hpp"

// hooray, MAKEINTRESOURCE(iconYes) can't be used with SS_ICON (it crashes)
#define _MKSSICON(id) L"#" L ## #id
#define MKSSICON(id) _MKSSICON(id)

static HWND mklabel(const WCHAR *text, HWND parent, int *idoff)
{
	HWND hwnd;

	hwnd = CreateWindowExW(0,
		L"STATIC", text,
		WS_CHILD | SS_LEFTNOWORDWRAP | SS_NOPREFIX,
		0, 0, 100, 100,
		parent, (HMENU) (*idoff), hInstance, NULL);
	if (hwnd == NULL)
		panic(L"error creating label for common properties view: %I32d", GetLastError());
	SendMessageW(hwnd, WM_SETFONT, (WPARAM) hMessageFont, TRUE);
	(*idoff)++;
	return hwnd;
}

static HWND mkedit(int width, HWND parent, int *idoff)
{
	HWND hwnd;

	hwnd = CreateWindowExW(WS_EX_CLIENTEDGE,
		L"EDIT", L"",
		// TODO remove READONLY if this ever becomes generic
		WS_CHILD | ES_AUTOHSCROLL | ES_LEFT | ES_NOHIDESEL | ES_READONLY,
		0, 0, width, 100,
		parent, (HMENU) (*idoff), hInstance, NULL);
	if (hwnd == NULL)
		panic(L"error creating label for common properties view: %I32d", GetLastError());
	SendMessageW(hwnd, WM_SETFONT, (WPARAM) hMessageFont, TRUE);
	(*idoff)++;
	return hwnd;
}

// TODO rename idoff to itemid
Common::Common(HWND parent, int idoff)
{
	this->labelVersion = mklabel(L"comctl32.dll Version", parent, &idoff);
	this->editVersionWidth = 30;
	this->editVersion = mkedit(this->editVersionWidth, parent, &idoff);

	this->labelUnicode = mklabel(L"Unicode", parent, &idoff);
	this->iconUnicode = CreateWindowExW(0,
		L"STATIC", MKSSICON(iconUnknown),
		WS_CHILD | SS_ICON | SS_REALSIZEIMAGE,
		0, 0, 100, 100,
		parent, (HMENU) idoff, hInstance, NULL);
	if (this->iconUnicode == NULL)
		panic(L"error creating Unicode icon static control: %I32d", GetLastError());
	idoff++;

	this->labelSetWindowTheme = mklabel(L"SetWindowTheme(", parent, &idoff);
	this->editSWTpszSubAppNameWidth = 50;
	this->editSWTpszSubAppName = mkedit(this->editSWTpszSubAppNameWidth, parent, &idoff);
	this->labelSWTComma = mklabel(L", ", parent, &idoff);
	this->editSWTpszSubIdListWidth = 50;
	this->editSWTpszSubIdList = mkedit(this->editSWTpszSubIdListWidth, parent, &idoff);
	this->labelSWTRightParen = mklabel(L")", parent, &idoff);
}

void Common::Show(void)
{
	ShowWindow(this->labelVersion, SW_SHOW);
	ShowWindow(this->editVersion, SW_SHOW);
	ShowWindow(this->labelUnicode, SW_SHOW);
	ShowWindow(this->iconUnicode, SW_SHOW);
	ShowWindow(this->labelSetWindowTheme, SW_SHOW);
	ShowWindow(this->editSWTpszSubAppName, SW_SHOW);
	ShowWindow(this->labelSWTComma, SW_SHOW);
	ShowWindow(this->editSWTpszSubIdList, SW_SHOW);
	ShowWindow(this->labelSWTRightParen, SW_SHOW);
}

void Common::Hide(void)
{
	ShowWindow(this->labelVersion, SW_HIDE);
	ShowWindow(this->editVersion, SW_HIDE);
	ShowWindow(this->labelUnicode, SW_HIDE);
	ShowWindow(this->iconUnicode, SW_HIDE);
	ShowWindow(this->labelSetWindowTheme, SW_HIDE);
	ShowWindow(this->editSWTpszSubAppName, SW_HIDE);
	ShowWindow(this->labelSWTComma, SW_HIDE);
	ShowWindow(this->editSWTpszSubIdList, SW_HIDE);
	ShowWindow(this->labelSWTRightParen, SW_HIDE);
}

SIZE Common::MinimumSize(Layouter *dparent)
{
	SIZE ret;
	Layouter *d;
	RECT r;

	ret.cx = 0;
	ret.cy = 0;

	d = new Layouter(this->labelVersion);
	ret.cx += d->TextWidth() + dparent->PaddingX();
	ret.cy = dparent->LabelYForSiblingY(0, d) + d->LabelHeight();
	delete d;

	d = new Layouter(this->editVersion);
	ret.cx += this->editVersionWidth;
	if (ret.cy < d->EditHeight())
		ret.cy = d->EditHeight();
	delete d;

	ret.cx += dparent->PaddingX();
	d = new Layouter(this->labelUnicode);
	ret.cx += d->TextWidth() + dparent->PaddingX();
	delete d;
	if (GetWindowRect(this->iconUnicode, &r) == 0)
		panic(L"error getting window rect of Unicode icon for measurement: %I32d", GetLastError());
	ret.cx += r.right - r.left;
	if (ret.cy < (r.bottom - r.top))
		ret.cy = (r.bottom - r.top);
	ret.cx += dparent->PaddingX();

	d = new Layouter(this->labelSetWindowTheme);
	ret.cx += d->TextWidth();
	delete d;
	ret.cx += this->editSWTpszSubAppNameWidth;
	d = new Layouter(this->labelSWTComma);
	ret.cx += d->TextWidth();
	delete d;
	ret.cx += this->editSWTpszSubIdListWidth;
	d = new Layouter(this->labelSWTRightParen);
	ret.cx += d->TextWidth();
	delete d;

	return ret;
}

void Common::Relayout(RECT *fill, Layouter *dparent)
{
	Layouter *d, *dlabel, *dedit;
	RECT r;
	int height;
	int yLabel = 0;
	int yEdit = 0;
	int yIcon = 0;
	HDWP dwp;
	int curx, oldx;
	int centerWidth;

	if (GetWindowRect(this->iconUnicode, &r) == 0)
		panic(L"error getting window rect of Unicode icon: %I32d", GetLastError());

	// TODO what if the label height is taller than the edit height?
	dlabel = new Layouter(this->labelVersion);
	height = dparent->LabelYForSiblingY(0, dlabel) + dlabel->LabelHeight();
	dedit = new Layouter(this->editVersion);
	if (height < dedit->EditHeight())
		height = dedit->EditHeight();
	if (height < (r.bottom - r.top)) {
		height = (r.bottom - r.top);
		// icon is largest; make it 0 and vertically center edit
		yIcon = 0;
		yEdit = (height - dedit->EditHeight()) / 2;
	} else {
		// edit is largest; make it 0 and vertically center icon
		yEdit = 0;
		yIcon = (height - (r.bottom - r.top)) / 2;
	}
	yLabel = dparent->LabelYForSiblingY(yEdit, dlabel);

	dwp = BeginDeferWindowPos(10);
	if (dwp == NULL)
		panic(L"BeginDeferWindowProc() failed: %I32d\n", GetLastError());

	// okay, we already have the version label and edit; do them first
	dwp = DeferWindowPos(dwp,
		this->labelVersion, NULL,
		fill->left, fill->top + yLabel,
		dlabel->TextWidth(), dlabel->LabelHeight(),
		SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER);
	if (dwp == NULL)
		panic(L"error rearranging version label: %I32d", GetLastError());
	dwp = DeferWindowPos(dwp,
		this->editVersion, NULL,
		fill->left + dlabel->TextWidth() + dparent->PaddingX(), fill->top + yEdit,
		this->editVersionWidth, dedit->EditHeight(),
		SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER);
	if (dwp == NULL)
		panic(L"error rearranging version edit: %I32d", GetLastError());
	oldx = fill->left + dlabel->TextWidth() + dparent->PaddingX() + this->editVersionWidth;

	delete dedit;
	delete dlabel;

	// now rearrange the right half
	d = new Layouter(this->labelSWTRightParen);
	curx = fill->right - d->TextWidth();
	dwp = DeferWindowPos(dwp,
		this->labelSWTRightParen, NULL,
		curx, fill->top + yLabel,
		d->TextWidth(), d->LabelHeight(),
		SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER);
	if (dwp == NULL)
		panic(L"error moving right parentheses label: %I32d", GetLastError());
	delete d;
	d = new Layouter(this->editSWTpszSubIdList);
	curx -= this->editSWTpszSubIdListWidth;
	dwp = DeferWindowPos(dwp,
		this->editSWTpszSubIdList, NULL,
		curx, fill->top + yEdit,
		this->editSWTpszSubIdListWidth, d->EditHeight(),
		SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER);
	if (dwp == NULL)
		panic(L"error moving pszSubIdList edit: %I32d", GetLastError());
	delete d;
	d = new Layouter(this->labelSWTComma);
	curx -= d->TextWidth();
	dwp = DeferWindowPos(dwp,
		this->labelSWTComma, NULL,
		curx, fill->top + yLabel,
		d->TextWidth(), d->LabelHeight(),
		SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER);
	if (dwp == NULL)
		panic(L"error moving comma label: %I32d", GetLastError());
	delete d;
	d = new Layouter(this->editSWTpszSubAppName);
	curx -= this->editSWTpszSubAppNameWidth;
	dwp = DeferWindowPos(dwp,
		this->editSWTpszSubAppName, NULL,
		curx, fill->top + yEdit,
		this->editSWTpszSubAppNameWidth, d->EditHeight(),
		SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER);
	if (dwp == NULL)
		panic(L"error moving pszSubIdList edit: %I32d", GetLastError());
	delete d;
	d = new Layouter(this->labelSetWindowTheme);
	curx -= d->TextWidth();
	dwp = DeferWindowPos(dwp,
		this->labelSetWindowTheme, NULL,
		curx, fill->top + yLabel,
		d->TextWidth(), d->LabelHeight(),
		SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER);
	if (dwp == NULL)
		panic(L"error moving SetWindowTheme() label: %I32d", GetLastError());
	delete d;

	// now lay out the center
	// we'll center it relative to the remaining space, not to the entire width of the details area
	centerWidth = dparent->PaddingX();
	dlabel = new Layouter(this->labelUnicode);
	centerWidth += dlabel->TextWidth() + dparent->PaddingX();
	centerWidth += r.right - r.left;
	curx = ((curx - oldx) - centerWidth) / 2;
	curx += oldx + dparent->PaddingX();
	dwp = DeferWindowPos(dwp,
		this->labelUnicode, NULL,
		curx, fill->top + yLabel,
		dlabel->TextWidth(), dlabel->LabelHeight(),
		SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER);
	if (dwp == NULL)
		panic(L"error moving Unicode label: %I32d", GetLastError());
	curx += dlabel->TextWidth() + dparent->PaddingX();
	delete dlabel;
	dwp = DeferWindowPos(dwp,
		this->iconUnicode, NULL,
		curx, fill->top + yIcon,
		0, 0,
		SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOSIZE | SWP_NOZORDER);
	if (dwp == NULL)
		panic(L"error moving Unicode icon: %I32d", GetLastError());

	if (EndDeferWindowPos(dwp) == 0)
		panic(L"error committing new common section control positions: %I32d", GetLastError());
}
