// 29 january 2017
#include "barspy.hpp"

// first column of General
#define gen1AnchorHighlights 0
#define gen1BitmapFlags 1

ToolbarTab::ToolbarTab(HWND parent, int id) :
	Tab(parent, id)
{
	this->generalTab = this->Add(L"General");
	this->buttonsTab = this->Add(L"Buttons");

	this->generalCol1 = new Form(this->generalTab);
	this->generalCol1->SetID(100);
	this->generalCol1->AddCheckmark(L"Anchor Highlights");
	this->generalCol1->Add(L"Bitmap Flags");
}

void ToolbarTab::Reflect(HWND hwnd, Process *p)
{
	HICON iconToUse;
	LRESULT lResult;

	lResult = SendMessageW(hwnd, TB_GETANCHORHIGHLIGHT, 0, 0);
	iconToUse = hIconNo;
	if (lResult != 0)
		iconToUse = hIconYes;
	this->generalCol1->SetCheckmark(gen1AnchorHighlights, iconToUse);

	this->generalCol1->SetText(gen1BitmapFlags, L"TODO");
}

HDWP ToolbarTab::RelayoutChild(HDWP dwp, HWND page, RECT *fill, Layouter *d)
{
	// TODO switch to Width when we add multiple columns
	dwp = this->generalCol1->RelayoutWidth(dwp,
		fill->left, fill->top,
		fill->right - fill->left,
		d);

	return dwp;
}
