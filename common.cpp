// 26 january 2017
#include "barspy.hpp"

static HWND mklabel(const WCHAR *text, HWND parent, int *idoff)
{
	HWND hwnd;

	hwnd = CreateWindowExW(0,
		L"STATIC", text,
		WS_CHILD | WS_VISIBLE | SS_LEFTNOWORDWRAP | SS_NOPREFIX,
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
		WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_LEFT | ES_NOHIDESEL | ES_READONLY,
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
	this->editVersionWidth = 35;
	this->editVersion = mkedit(this->editVersionWidth, parent, &idoff);

	this->labelUnicode = mklabel(L"Unicode", parent, &idoff);
	this->iconUnicode = newCheckmark(parent, (HMENU) idoff);
	idoff++;

	this->labelSetWindowTheme = mklabel(L"SetWindowTheme(", parent, &idoff);
	this->editSWTpszSubAppNameWidth = 50;
	this->editSWTpszSubAppName = mkedit(this->editSWTpszSubAppNameWidth, parent, &idoff);
	this->labelSWTComma = mklabel(L", ", parent, &idoff);
	this->editSWTpszSubIdListWidth = 50;
	this->editSWTpszSubIdList = mkedit(this->editSWTpszSubIdListWidth, parent, &idoff);
	this->labelSWTRightParen = mklabel(L")", parent, &idoff);

	this->labelStyles = mklabel(L"Styles", parent, &idoff);
	this->editStylesWidth = 100;
	this->editStyles = mkedit(this->editStylesWidth, parent, &idoff);
	this->labelExStyles = mklabel(L"Extended Styles", parent, &idoff);
	this->editExStylesWidth = 100;
	this->editExStyles = mkedit(this->editExStylesWidth, parent, &idoff);
}

void Common::Reset(void)
{
	if (SetWindowTextW(this->editVersion, L"N/A") == 0)
		panic(L"error resetting version text: %I32d", GetLastError());
	setCheckmarkIcon(this->iconUnicode, hIconUnknown);
	if (SetWindowTextW(this->editSWTpszSubAppName, L"N/A") == 0)
		panic(L"error resetting pszSubAppName text: %I32d", GetLastError());
	if (SetWindowTextW(this->editSWTpszSubIdList, L"N/A") == 0)
		panic(L"error resetting pszSubIdList text: %I32d", GetLastError());
	if (SetWindowTextW(this->editStyles, L"N/A") == 0)
		panic(L"error resetting styles text: %I32d", GetLastError());
	if (SetWindowTextW(this->editExStyles, L"N/A") == 0)
		panic(L"error resetting extended styles text: %I32d", GetLastError());
}

#define TRY(bit) if ((relevant & bit) != 0) { ss << prefix << #bit; prefix = L" | "; relevant &= ~(bit); }
#define TRYCC \
	TRY(CCS_TOP) \
	TRY(CCS_NOMOVEY) \
	TRY(CCS_NORESIZE) \
	TRY(CCS_NOPARENTALIGN) \
	TRY(CCS_ADJUSTABLE) \
	TRY(CCS_NODIVIDER) \
	TRY(CCS_VERT)

static void setToolbarStyleEdit(HWND edit, HWND toolbar)
{
	uint16_t relevant;
	const WCHAR *prefix = L"";
	std::wostringstream ss;
	std::wstring s;

	relevant = (uint16_t) (GetWindowLongPtrW(toolbar, GWL_STYLE) & 0xFFFF);
	TRYCC
	TRY(TBSTYLE_TOOLTIPS)
	TRY(TBSTYLE_WRAPABLE)
	TRY(TBSTYLE_ALTDRAG)
	TRY(TBSTYLE_FLAT)
	TRY(TBSTYLE_LIST)
	TRY(TBSTYLE_CUSTOMERASE)
	TRY(TBSTYLE_REGISTERDROP)
	TRY(TBSTYLE_TRANSPARENT)
	if (relevant != 0) {
		ss << prefix << L"0x";
		ss.fill(L'0');
		ss.setf(ss.hex | ss.uppercase, ss.basefield | ss.uppercase);
		ss.width(4);
		ss << relevant;
	}
	s = ss.str();		// to be safe
	if (SetWindowTextW(edit, s.c_str()) == 0)
		panic(L"error setting toolbar Style text: %I32d", GetLastError());
}

static void setToolbarExStyleEdit(HWND edit, HWND toolbar)
{
	DWORD relevant;
	const WCHAR *prefix = L"";
	std::wostringstream ss;
	std::wstring s;

	relevant = (DWORD) SendMessageW(toolbar, TB_GETEXTENDEDSTYLE, 0, 0);
	TRY(TBSTYLE_EX_DRAWDDARROWS)
	TRY(TBSTYLE_EX_MULTICOLUMN)
	TRY(TBSTYLE_EX_VERTICAL)
	TRY(TBSTYLE_EX_MIXEDBUTTONS)
	TRY(TBSTYLE_EX_HIDECLIPPEDBUTTONS)
	TRY(TBSTYLE_EX_DOUBLEBUFFER)
	if (relevant != 0) {
		ss << prefix << L"0x";
		ss.fill(L'0');
		ss.setf(ss.hex | ss.uppercase, ss.basefield | ss.uppercase);
		ss.width(8);
		ss << relevant;
	}
	s = ss.str();		// to be safe
	if (SetWindowTextW(edit, s.c_str()) == 0)
		panic(L"error setting toolbar Extended Style text: %I32d", GetLastError());
}

static WCHAR *nullCopy(void)
{
	WCHAR *s;

	s = new WCHAR[5];
	s[0] = L'N';
	s[1] = L'U';
	s[2] = L'L';
	s[3] = L'L';
	s[4] = L'\0';
	return s;
}

void Common::Reflect(HWND hwnd, Process *p)
{
	int wc;
	WCHAR *pszSubAppName, *pszSubIdList;

	this->Reset();

	// only get these if this is a known control
	wc = windowClassOf(hwnd, DESIREDCLASSES, NULL);
	if (wc != -1) {
		WCHAR *dgv;
		HICON iconToUse;

		dgv = getDLLVersion(hwnd, p);
		if (SetWindowTextW(this->editVersion, dgv) == 0)
			panic(L"error setting version text: %I32d", GetLastError());
		delete[] dgv;

		iconToUse = hIconNo;
		if (SendMessageW(hwnd, CCM_GETUNICODEFORMAT, 0, 0) != 0)
			iconToUse = hIconYes;
		setCheckmarkIcon(this->iconUnicode, iconToUse);

		switch (wc) {
		case DESIREDTOOLBAR:
			setToolbarStyleEdit(this->editStyles, hwnd);
			setToolbarExStyleEdit(this->editExStyles, hwnd);
			break;
		case DESIREDREBAR:
			// TODO
			break;
		}
	}

	// always set this
	getWindowTheme(hwnd, p, &pszSubAppName, &pszSubIdList);
	if (pszSubAppName == NULL)
		pszSubAppName = nullCopy();
	if (pszSubIdList == NULL)
		pszSubIdList = nullCopy();
	if (SetWindowTextW(this->editSWTpszSubAppName, pszSubAppName) == 0)
		panic(L"error setting pszSubAppName text: %I32d", GetLastError());
	if (SetWindowTextW(this->editSWTpszSubIdList, pszSubIdList) == 0)
		panic(L"error setting pszSubIdList text: %I32d", GetLastError());
	delete[] pszSubIdList;
	delete[] pszSubAppName;
}

SIZE Common::MinimumSize(Layouter *dparent)
{
	SIZE ret;
	Layouter *d;
	SIZE checkSize;
	LONG editHeight;

	ret.cx = 0;
	ret.cy = 0;

	d = new Layouter(this->labelVersion);
	ret.cx += d->TextWidth() + dparent->PaddingX();
	ret.cy = dparent->LabelYForSiblingY(0, d) + d->LabelHeight();
	delete d;

	d = new Layouter(this->editVersion);
	ret.cx += this->editVersionWidth;
	editHeight = d->EditHeight();
	if (ret.cy < editHeight)
		ret.cy = editHeight;
	delete d;

	ret.cx += dparent->PaddingX();
	d = new Layouter(this->labelUnicode);
	ret.cx += d->TextWidth() + dparent->PaddingX();
	delete d;
	checkSize = checkmarkSize(this->iconUnicode);
	ret.cx += checkSize.cx;
	if (ret.cy < checkSize.cy)
		ret.cy = checkSize.cy;
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

	// TODO don't assume the label's bottom will be above the edit's bottom
	ret.cy += 2 * dparent->PaddingY() + 2 * editHeight;
	d = new Layouter(this->labelStyles);
	if (ret.cx < (d->TextWidth() + dparent->PaddingX() + this->editStylesWidth))
		ret.cx = (d->TextWidth() + dparent->PaddingX() + this->editStylesWidth);
	delete d;
	d = new Layouter(this->labelExStyles);
	if (ret.cx < (d->TextWidth() + dparent->PaddingX() + this->editExStylesWidth))
		ret.cx = (d->TextWidth() + dparent->PaddingX() + this->editExStylesWidth);
	delete d;

	return ret;
}

void Common::Relayout(RECT *fill, Layouter *dparent)
{
	Layouter *d, *dlabel, *dedit;
	SIZE checkSize;
	int height;
	int yLabel = 0;
	int yEdit = 0;
	int yIcon = 0;
	HDWP dwp;
	int curx, oldx;
	int centerWidth;
	int cury;

	checkSize = checkmarkSize(this->iconUnicode);

	// TODO what if the label height is taller than the edit height?
	dlabel = new Layouter(this->labelVersion);
	height = dparent->LabelYForSiblingY(0, dlabel) + dlabel->LabelHeight();
	dedit = new Layouter(this->editVersion);
	if (height < dedit->EditHeight())
		height = dedit->EditHeight();
	if (height < checkSize.cy) {
		height = checkSize.cy;
		// icon is largest; make it 0 and vertically center edit
		yIcon = 0;
		yEdit = (height - dedit->EditHeight()) / 2;
	} else {
		// edit is largest; make it 0 and vertically center icon
		yEdit = 0;
		yIcon = (height - checkSize.cy) / 2;
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
	centerWidth += checkSize.cx;
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

	cury = fill->top + height + dparent->PaddingY();
	d = new Layouter(this->labelStyles);
	curx = fill->left + d->TextWidth() + dparent->PaddingX();
	delete d;
	d = new Layouter(this->labelExStyles);
	if (curx < (fill->left + d->TextWidth() + dparent->PaddingX()))
		curx = (fill->left + d->TextWidth() + dparent->PaddingX());
	dlabel = d;
	dedit = new Layouter(this->editStyles);
	dwp = DeferWindowPos(dwp,
		this->labelStyles, NULL,
		fill->left, cury + (yLabel - yEdit),
		(curx - dparent->PaddingX()) - fill->left, dlabel->LabelHeight(),
		SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER);
	if (dwp == NULL)
		panic(L"error moving Styles label: %I32d", GetLastError());
	dwp = DeferWindowPos(dwp,
		this->editStyles, NULL,
		curx, cury,
		fill->right - curx, dedit->EditHeight(),
		SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER);
	if (dwp == NULL)
		panic(L"error moving Styles edit: %I32d", GetLastError());
	cury += dedit->EditHeight() + dparent->PaddingY();
	dwp = DeferWindowPos(dwp,
		this->labelExStyles, NULL,
		fill->left, cury + (yLabel - yEdit),
		(curx - dparent->PaddingX()) - fill->left, dlabel->LabelHeight(),
		SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER);
	if (dwp == NULL)
		panic(L"error moving Extended Styles label: %I32d", GetLastError());
	dwp = DeferWindowPos(dwp,
		this->editExStyles, NULL,
		curx, cury,
		fill->right - curx, dedit->EditHeight(),
		SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER);
	if (dwp == NULL)
		panic(L"error moving Extended Styles edit: %I32d", GetLastError());
	delete dedit;
	delete d;

	if (EndDeferWindowPos(dwp) == 0)
		panic(L"error committing new common section control positions: %I32d", GetLastError());
}

// TODO find a better place for this; it's only here because it uses the TRY() macro
void setDrawTextFlagsEdit(HWND edit, UINT relevant)
{
	const WCHAR *prefix = L"";
	std::wostringstream ss;
	std::wstring s;

	TRY(DT_CENTER)
	TRY(DT_RIGHT)
	TRY(DT_VCENTER)
	TRY(DT_BOTTOM)
	TRY(DT_WORDBREAK)
	TRY(DT_SINGLELINE)
	TRY(DT_EXPANDTABS)
	TRY(DT_TABSTOP)
	TRY(DT_NOCLIP)
	TRY(DT_EXTERNALLEADING)
	TRY(DT_CALCRECT)
	TRY(DT_NOPREFIX)
	TRY(DT_INTERNAL)
	TRY(DT_EDITCONTROL)
	TRY(DT_PATH_ELLIPSIS)
	TRY(DT_END_ELLIPSIS)
	TRY(DT_MODIFYSTRING)
	TRY(DT_RTLREADING)
	TRY(DT_WORD_ELLIPSIS)
	TRY(DT_NOFULLWIDTHCHARBREAK)
	TRY(DT_HIDEPREFIX)
	TRY(DT_PREFIXONLY)
	if (relevant != 0) {
		ss << prefix << L"0x";
		ss.fill(L'0');
		ss.setf(ss.hex | ss.uppercase, ss.basefield | ss.uppercase);
		ss.width(8);
		ss << relevant;
	}
	s = ss.str();		// to be safe
	if (SetWindowTextW(edit, s.c_str()) == 0)
		panic(L"error setting DrawText() flag text: %I32d", GetLastError());
}
