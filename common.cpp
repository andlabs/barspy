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

#define swtpszSubAppName 0
#define swtpszSubIdList 1
#define stylesStyles 0
#define stylesExStyles 1

// TODO rename idoff to itemid
// TODO find a sensible layout for the colon or switch to a pointer
Common::Common(HWND parent, int idoff) :
	version(parent),
	setWindowTheme(parent),
	styles(parent)
{
	this->version.SetID(idoff);
	this->version.SetMinEditWidth(35);
	this->version.Add(L"comctl32.dll Version");
	idoff = this->version.ID();

	this->labelUnicode = mklabel(L"Unicode", parent, &idoff);
	this->iconUnicode = newCheckmark(parent, (HMENU) idoff);
	idoff++;

	this->setWindowTheme.SetID(idoff);
	this->setWindowTheme.SetMinEditWidth(50);
	this->setWindowTheme.SetPadded(false);
	this->setWindowTheme.Add(L"SetWindowTheme(");
	this->setWindowTheme.Add(L", ");
	idoff = this->setWindowTheme.ID();
	this->labelSWTRightParen = mklabel(L")", parent, &idoff);

	this->styles.SetMinEditWidth(100);
	this->styles.SetID(idoff);
	this->styles.Add(L"Styles");
	this->styles.Add(L"Extended Styles");
	idoff = this->styles.ID();
}

void Common::Reset(void)
{
	this->version.SetText(0, L"N/A");
	setCheckmarkIcon(this->iconUnicode, hIconUnknown);
	this->setWindowTheme.SetText(swtpszSubAppName, L"N/A");
	this->setWindowTheme.SetText(swtpszSubIdList, L"N/A");
	this->styles.SetText(stylesStyles, L"N/A");
	this->styles.SetText(stylesExStyles, L"N/A");
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

static std::wstring toolbarStyleString(HWND toolbar)
{
	uint16_t relevant;
	const WCHAR *prefix = L"";
	std::wostringstream ss;

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
	return ss.str();
}

static std::wstring toolbarExStyleString(HWND toolbar)
{
	DWORD relevant;
	const WCHAR *prefix = L"";
	std::wostringstream ss;

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
	return ss.str();
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
		std::wstring s;

		dgv = getDLLVersion(hwnd, p);
		this->version.SetText(0, dgv);
		delete[] dgv;

		iconToUse = hIconNo;
		if (SendMessageW(hwnd, CCM_GETUNICODEFORMAT, 0, 0) != 0)
			iconToUse = hIconYes;
		setCheckmarkIcon(this->iconUnicode, iconToUse);

		switch (wc) {
		case DESIREDTOOLBAR:
			s = toolbarStyleString(hwnd);
			this->styles.SetText(stylesStyles, s.c_str());
			s = toolbarExStyleString(hwnd);
			this->styles.SetText(stylesExStyles, s.c_str());
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
	this->setWindowTheme.SetText(swtpszSubAppName, pszSubAppName);
	this->setWindowTheme.SetText(swtpszSubIdList, pszSubIdList);
	delete[] pszSubIdList;
	delete[] pszSubAppName;
}

SIZE Common::MinimumSize(Layouter *dparent)
{
	SIZE ret;
	Layouter *d;
	SIZE checkSize;
	SIZE otherSize;

	ret.cx = 0;
	ret.cy = 0;

	ret = this->version.MinimumSize(dparent);

	ret.cx += dparent->PaddingX();
	d = new Layouter(this->labelUnicode);
	ret.cx += d->TextWidth() + dparent->PaddingX();
	delete d;
	checkSize = checkmarkSize(this->iconUnicode);
	ret.cx += checkSize.cx;
	if (ret.cy < checkSize.cy)
		ret.cy = checkSize.cy;
	ret.cx += dparent->PaddingX();

	otherSize = this->setWindowTheme.MinimumSize(dparent);
	ret.cx += otherSize.cx;
	if (ret.cy < otherSize.cy)
		ret.cy = otherSize.cy;
	d = new Layouter(this->labelSWTRightParen);
	ret.cx += d->TextWidth();
	delete d;

	// TODO don't assume the label's bottom will be above the edit's bottom
	ret.cy += dparent->PaddingY();
	// TODO merge this variable somehow
	otherSize = this->styles.MinimumSize(dparent);
	if (ret.cx < otherSize.cx)
		ret.cx = otherSize.cx;
	ret.cy += otherSize.cy;

	return ret;
}

void Common::Relayout(RECT *fill, Layouter *dparent)
{
	Layouter *d, *dlabel, *dedit;
	SIZE checkSize;
	SIZE otherSize;			// TODO clean this up
	int height;
	int yLabel = 0;
	int yEdit = 0;
	int yIcon = 0;
	HDWP dwp;
	int curx, oldx;
	int centerWidth;
	int cury;

	checkSize = checkmarkSize(this->iconUnicode);
	otherSize = this->setWindowTheme.MinimumSize(dparent);
	height = otherSize.cy;
	if (height < checkSize.cy) {
		height = checkSize.cy;
		// icon is largest; make it 0 and vertically center edit
		yIcon = 0;
		yEdit = (height - dparent/*TODO this is wrong*/->EditHeight()) / 2;
	} else {
		// edit is largest; make it 0 and vertically center icon
		yEdit = 0;
		yIcon = (height - checkSize.cy) / 2;
	}
	yLabel = dparent->LabelYForSiblingY(yEdit, dparent/*TODO this is wrong*/);

	dwp = BeginDeferWindowPos(10);
	if (dwp == NULL)
		panic(L"BeginDeferWindowProc() failed: %I32d\n", GetLastError());

	dwp = this->version.Relayout(dwp,
		fill->left, fill->top,
		dparent);
	oldx = fill->left + this->version.MinimumSize(dparent).cx;

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
	curx -= otherSize.cx;
	dwp = this->setWindowTheme.Relayout(dwp,
		curx, fill->top,
		dparent);

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
	dwp = this->styles.RelayoutWidth(dwp,
		fill->left, cury,
		fill->right - fill->left,
		dparent);

	if (EndDeferWindowPos(dwp) == 0)
		panic(L"error committing new common section control positions: %I32d", GetLastError());
}

// TODO find a better place for this; it's only here because it uses the TRY() macro
std::wstring drawTextFlagsString(UINT relevant)
{
	const WCHAR *prefix = L"";
	std::wostringstream ss;

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
	return ss.str();
}
