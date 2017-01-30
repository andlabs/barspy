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
}

void ToolbarTab::Reflect(HWND hwnd, Process *p)
{
	HICON iconToUse;
	DWORD dw, all1s;
	INT intval;
	HWND hwval;
	RECT wr, cr;
	std::wostringstream *ss;
	std::wstring s;
	LRESULT lResult;

	lResult = SendMessageW(hwnd, TB_GETANCHORHIGHLIGHT, 0, 0);
	iconToUse = hIconNo;
	if (lResult != 0)
		iconToUse = hIconYes;
	this->generalCol1->SetCheckmark(gen1AnchorHighlights, iconToUse);

	s = toolbarBitmapFlagsString(hwnd);
	this->generalCol1->SetText(gen1BitmapFlags, s.c_str());

	// TODO TB_GETCOLORSCHEME — requires
	// - injecting to get a struct out
	// - responding to WM_CTLCOLORSTATIC in the dialog procedure
	// - letting Form tell us which row an edit handle is
	this->generalCol1->SetText(gen1ButtonHighlight, L"TODO");
	this->generalCol1->SetText(gen1ButtonShadow, L"TODO");

	// TODO TB_GETINSERTMARKCOLOR
	this->generalCol1->SetText(gen1InsertionColor, L"TODO");

	// TODO TB_GETMAXSIZE
	// requires injection
	this->generalCol1->SetText(gen1MaxSize, L"TODO");

	// TODO TB_GETMETRICS
	// injection again
	this->generalCol1->SetText(gen1PaddingSize, L"TODO");
	this->generalCol1->SetText(gen1BarPadSize, L"TODO");
	this->generalCol2->SetText(gen2SpacingSize, L"TODO");

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
	all1s = ~((DWORD) 0);
	dw = (DWORD) SendMessageW(hwnd, TB_SETDRAWTEXTFLAGS, (WPARAM) all1s, 0);
	SendMessageW(hwnd, TB_SETDRAWTEXTFLAGS, (WPARAM) all1s, (LPARAM) dw);
	s = drawTextFlagsString(dw);
	this->generalCol2->SetText(gen2DrawTextFlags, s.c_str());
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
