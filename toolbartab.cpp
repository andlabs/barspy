// 29 january 2017
#include "barspy.hpp"

// first column of General
#define gen1AnchorHighlights 0
#define gen1BitmapFlags 1
#define gen1ButtonHighlight 2
#define gen1ButtonShadow 3
#define gen1InsertionColor 4
#define gen1MaxSize 5
#define gen1PaddingSize 6
#define gen1BarPadSize 7

// second column of General
#define gen2SpacingSize 0
#define gen2Padding 1
#define gen2MaxTextRows 2
#define gen2HasTooltips 3
#define gen2FirstIndent 4
#define gen2DrawTextFlags 5

ToolbarTab::ToolbarTab(HWND parent, int id) :
	Tab(parent, id)
{
	this->generalTab = this->Add(L"General");
	this->buttonsTab = this->Add(L"Buttons");
	this->imagelistTab = this->Add(L"Image Lists");

	this->generalCol1 = new Form(this->generalTab);
	this->generalCol1->SetID(100);
	// TODO TB_BUTTONSTRUCTSIZE — first, there does not seem to be a way to get this; second, the docs are interesting...
	this->generalCol1->AddCheckmark(L"Anchor Highlights");
	this->generalCol1->Add(L"Bitmap Flags");
	this->generalCol1->Add(L"Button Highlight");
	this->generalCol1->Add(L"Button Shadow");
	this->generalCol1->Add(L"Insertion Pt Color");
	this->generalCol1->Add(L"Maximum Size");
	this->generalCol1->Add(L"Padding Size");
	this->generalCol1->Add(L"Bar Pad Size");

	this->generalCol2 = new Form(this->generalTab);
	this->generalCol2->SetID(this->generalCol1->ID());
	this->generalCol2->Add(L"Spacing Size");
	this->generalCol2->Add(L"Control Padding");
	this->generalCol2->Add(L"Maximum Text Rows");
	// TODO TB_GETSTYLE — is this different from what we do in Common?
	this->generalCol2->AddCheckmark(L"Has Tooltips");
	this->generalCol2->Add(L"First Button Indent");
	this->generalCol2->Add(L"DrawText() Flags");
	// TODO TB_SETBOUNDINGSIZE

	this->buttonHighlightBrush = NULL;
	this->buttonShadowBrush = NULL;
	this->insertionPointBrush = NULL;
}

void ToolbarTab::Reflect(HWND hwnd, Process *p)
{
	ProcessHelper *ph;
	HICON iconToUse;
	DWORD dw, all1s;
	COLORREF color;
	SIZE size;
	INT intval;
	int k;
	HWND hwval;
	RECT wr, cr;
	std::wostringstream *ss;
	std::wstring s;
	LRESULT lResult;

	ph = getToolbarGeneral(hwnd, p);

	lResult = SendMessageW(hwnd, TB_GETANCHORHIGHLIGHT, 0, 0);
	iconToUse = hIconNo;
	if (lResult != 0)
		iconToUse = hIconYes;
	this->generalCol1->SetCheckmark(gen1AnchorHighlights, iconToUse);

	s = toolbarBitmapFlagsString(hwnd);
	this->generalCol1->SetText(gen1BitmapFlags, s.c_str());

	deleteObject(this->buttonShadowBrush);
	this->buttonShadowBrush = NULL;
	deleteObject(this->buttonHighlightBrush);
	this->buttonHighlightBrush = NULL;
	ph->ReadField("gsResultNonzero", &dw);
	if (dw) {
		ph->ReadField("highlight", &color);
		if (color == CLR_DEFAULT)
			this->generalCol1->SetText(gen1ButtonHighlight, L"default");
		else {
			s = colorToString(color);
			this->generalCol1->SetText(gen1ButtonHighlight, s.c_str());
			// TODO make brush
		}
		ph->ReadField("shadow", &color);
		if (color == CLR_DEFAULT)
			this->generalCol1->SetText(gen1ButtonShadow, L"default");
		else {
			s = colorToString(color);
			this->generalCol1->SetText(gen1ButtonShadow, s.c_str());
			// TODO make brush
		}
	} else {
		// TODO GetLastError() (requires reworking)
		this->generalCol1->SetText(gen1ButtonHighlight, L"N/A");
		this->generalCol1->SetText(gen1ButtonShadow, L"N/A");
	}
	// TODO refresh both

	color = (COLORREF) SendMessageW(hwnd, TB_GETINSERTMARKCOLOR, 0, 0);
	s = colorToString(color);
	this->generalCol1->SetText(gen1InsertionColor, s.c_str());
	deleteObject(this->insertionPointBrush);
	this->insertionPointBrush = CreateSolidBrush(color);
	if (this->insertionPointBrush == NULL)
		panic(L"error creating new insertion point brush: %I32d", GetLastError());
	// TODO redraw the field now since its color has changed

	ph->ReadField("msResultNonzero", &dw);
	if (dw) {
		ph->ReadField("maxWidth", &(size.cx));
		ph->ReadField("maxHeight", &(size.cy));
		s = sizeToString(size);
		this->generalCol1->SetText(gen1MaxSize, s.c_str());
	} else
		// TODO we'd have to regenerate all this if we wanted to store last errors
		this->generalCol1->SetText(gen1MaxSize, L"N/A");

	ph->ReadField("gmCXPad", &k);
	size.cx = k;
	ph->ReadField("gmCYPad", &k);
	size.cy = k;
	s = sizeToString(size);
	this->generalCol1->SetText(gen1PaddingSize, s.c_str());
	ph->ReadField("gmCXBarPad", &k);
	size.cx = k;
	ph->ReadField("gmCYBarPad", &k);
	size.cy = k;
	s = sizeToString(size);
	this->generalCol1->SetText(gen1BarPadSize, s.c_str());
	ph->ReadField("gmCXButtonSpacing", &k);
	size.cx = k;
	ph->ReadField("gmCYButtonSpacing", &k);
	size.cy = k;
	s = sizeToString(size);
	this->generalCol2->SetText(gen2SpacingSize, s.c_str());

	dw = (DWORD) SendMessageW(hwnd, TB_GETPADDING, 0, 0);
	ss = new std::wostringstream;
	*ss << L"H " << LOWORD(dw);
	*ss << L" V " << HIWORD(dw);
	s = ss->str();
	this->generalCol2->SetText(gen2Padding, s.c_str());
	delete ss;

	intval = (INT) SendMessageW(hwnd, TB_GETTEXTROWS, 0, 0);
	s = std::to_wstring(intval);
	this->generalCol2->SetText(gen2MaxTextRows, s.c_str());

	hwval = (HWND) SendMessageW(hwnd, TB_GETTOOLTIPS, 0, 0);
	iconToUse = hIconNo;
	if (hwval != NULL)
		iconToUse = hIconYes;
	this->generalCol2->SetCheckmark(gen2HasTooltips, iconToUse);

	// there is no TB_GETINDENT
	// http://www.yqcomputer.com/44_3549_1.htm apparently has the method to get this info
	if (GetWindowRect(hwnd, &wr) == 0)
		panic(L"error getting toolbar window rect for first-button indent calculation: %I32d", GetLastError());
	if (GetClientRect(hwnd, &cr) == 0)
		panic(L"error getting toolbar client rect for first-button indent calculation: %I32d", GetLastError());
	// should be equivalent to calling ClientToScreen() (which the original seems to use; it's MFC) but also safer? TODO
	// TODO error check
	MapWindowRect(hwnd, NULL, &cr);
	s = std::to_wstring(cr.left - wr.left);
	this->generalCol2->SetText(gen2FirstIndent, s.c_str());

	// there is no TB_GETDRAWTEXTFLAGS, but TB_SETDRAWTEXTFLAGS returns the previous value
	// it's not documented whether passing two zeroes will clear all flags or not (as it would with other similar messages in comctl32.dll)
	// so instead, let's set it to zero and then set it back immediately afterward
	// TODO this still isn't right; does it just return 0 until initially set, at which point text drawing changes?
	all1s = ~((DWORD) 0);
	dw = (DWORD) SendMessageW(hwnd, TB_SETDRAWTEXTFLAGS, (WPARAM) all1s, 0);
	SendMessageW(hwnd, TB_SETDRAWTEXTFLAGS, (WPARAM) all1s, (LPARAM) dw);
	s = drawTextFlagsString(dw);
	this->generalCol2->SetText(gen2DrawTextFlags, s.c_str());

	delete ph;
}

HDWP ToolbarTab::RelayoutChild(HDWP dwp, HWND page, RECT *fill, Layouter *d)
{
	LONG colwid;

	colwid = (fill->right - fill->left - d->PaddingX()) / 2;
	dwp = this->generalCol1->RelayoutWidth(dwp,
		fill->left, fill->top,
		colwid,
		d);
	dwp = this->generalCol2->RelayoutWidth(dwp,
		fill->left + colwid + d->PaddingX(), fill->top,
		colwid,
		d);

	return dwp;
}

bool ToolbarTab::OnCtlColorStatic(HDC dc, HWND hwnd, HBRUSH *brush)
{
	switch (this->generalCol1->WhichRowIs(hwnd)) {
	case gen1InsertionColor:
		*brush = this->insertionPointBrush;
		return true;
	}
	return false;
}
