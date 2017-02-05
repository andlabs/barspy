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
	LVCOLUMNW lc;

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

	this->buttonCount = new Chain(this->buttonsTab);
	this->buttonCount->SetID(100);
	this->buttonCount->SetMinEditWidth(50);
	this->buttonCount->SetPadded(true);
	this->buttonCount->Add(L"Buttons");
	this->buttonCount->Add(L"Rows");

	this->buttonList = CreateWindowExW(WS_EX_CLIENTEDGE,
		WC_LISTVIEWW, L"",
		WS_CHILD | WS_VISIBLE | WS_VSCROLL | LVS_ALIGNLEFT | LVS_NOCOLUMNHEADER | LVS_NOSORTHEADER | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_SINGLESEL,
		0, 0, 100, 100,
		this->buttonsTab, (HMENU) (this->buttonCount->ID()), hInstance, NULL);
	if (this->buttonList == NULL)
		panic(L"error creating button list: %I32d", GetLastError());
	SendMessageW(this->buttonList, LVM_SETEXTENDEDLISTVIEWSTYLE,
		LVS_EX_FULLROWSELECT,
		LVS_EX_FULLROWSELECT);
	ZeroMemory(&lc, sizeof (LVCOLUMNW));
	lc.mask = LVCF_FMT | LVCF_WIDTH;
	lc.fmt = LVCFMT_LEFT | LVCFMT_IMAGE;
	lc.cx = 100;
	if (SendMessageW(this->buttonList, LVM_INSERTCOLUMN, 0, (LPARAM) (&lc)) == (LRESULT) (-1))
		panic(L"error adding column to button list: %I32d", GetLastError());

	this->buttonHighlightBrush = NULL;
	this->buttonShadowBrush = NULL;
	this->insertionPointBrush = NULL;
	this->normalImageList = NULL;
	this->hotImageList = NULL;
	this->pressedImageList = NULL;
	this->disabledImageList = NULL;
}

void ToolbarTab::reset(void)
{
	deleteObject(this->buttonShadowBrush);
	this->buttonShadowBrush = NULL;
	deleteObject(this->buttonHighlightBrush);
	this->buttonHighlightBrush = NULL;
	deleteObject(this->insertionPointBrush);
	this->insertionPointBrush = NULL;

	if (this->normalImageList != NULL) {
		// TODO proper error check
		SendMessageW(this->buttonList, LVM_SETIMAGELIST, LVSIL_NORMAL, (LPARAM) NULL);
		if (ImageList_Destroy(this->normalImageList) == 0)
			panic(L"error destroying old normal image list: %I32d", GetLastError());
		this->normalImageList = NULL;
	}
	if (this->hotImageList != NULL) {
		if (ImageList_Destroy(this->hotImageList) == 0)
			panic(L"error destroying old hot image list: %I32d", GetLastError());
		this->hotImageList = NULL;
	}
	if (this->pressedImageList != NULL) {
		if (ImageList_Destroy(this->pressedImageList) == 0)
			panic(L"error destroying old pressed image list: %I32d", GetLastError());
		this->pressedImageList = NULL;
	}
	if (this->disabledImageList != NULL) {
		if (ImageList_Destroy(this->disabledImageList) == 0)
			panic(L"error destroying old disabled image list: %I32d", GetLastError());
		this->disabledImageList = NULL;
	}
}

static HIMAGELIST readImageList(Process *p, HWND hwnd, UINT uMsg)
{
	HIMAGELIST imglist;
	HMODULE pole32;
	HGLOBAL hGlobal;
	uint8_t *buf;
	SIZE_T size;
	IStream *stream;

	imglist = (HIMAGELIST) SendMessageW(hwnd, uMsg, 0, 0);
	if (imglist == NULL)
		return NULL;
	pole32 = loadLibraryProcess(p, L"ole32.dll");
	hGlobal = writeImageListV5(hwnd, p, imglist, (void *) pole32);
	buf = dumpHGLOBALStreamData(p, hGlobal, &size);
	freeLibraryProcess(p, pole32);

	// TODO allow larger sizes
	stream = SHCreateMemStream(buf, (UINT) size);
	if (stream == NULL)
		panic(L"error creating image list read stream: %I32d", GetLastError());
	imglist = ImageList_Read(stream);
	if (imglist == NULL)
		panic(L"error reading V5 image list: %I32d", GetLastError());
	stream->Release();
	delete[] buf;
	return imglist;
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

	this->reset();

	ph = getToolbarGeneral(hwnd, p);

	lResult = SendMessageW(hwnd, TB_GETANCHORHIGHLIGHT, 0, 0);
	iconToUse = hIconNo;
	if (lResult != 0)
		iconToUse = hIconYes;
	this->generalCol1->SetCheckmark(gen1AnchorHighlights, iconToUse);

	s = toolbarBitmapFlagsString(hwnd);
	this->generalCol1->SetText(gen1BitmapFlags, s.c_str());

	ph->ReadField("gsResultNonzero", &dw);
	if (dw) {
		ph->ReadField("highlight", &color);
		if (color == CLR_DEFAULT)
			this->generalCol1->SetText(gen1ButtonHighlight, L"default");
		else {
			s = colorToString(color);
			this->generalCol1->SetText(gen1ButtonHighlight, s.c_str());
			this->buttonHighlightBrush = createSolidBrush(color);
		}
		ph->ReadField("shadow", &color);
		if (color == CLR_DEFAULT)
			this->generalCol1->SetText(gen1ButtonShadow, L"default");
		else {
			s = colorToString(color);
			this->generalCol1->SetText(gen1ButtonShadow, s.c_str());
			this->buttonShadowBrush = createSolidBrush(color);
		}
	} else {
		// TODO GetLastError() (requires reworking)
		this->generalCol1->SetText(gen1ButtonHighlight, L"N/A");
		this->generalCol1->SetText(gen1ButtonShadow, L"N/A");
	}
	this->generalCol1->QueueRedraw(gen1ButtonHighlight);
	this->generalCol1->QueueRedraw(gen1ButtonShadow);

	color = (COLORREF) SendMessageW(hwnd, TB_GETINSERTMARKCOLOR, 0, 0);
	s = colorToString(color);
	this->generalCol1->SetText(gen1InsertionColor, s.c_str());
	this->insertionPointBrush = createSolidBrush(color);
	this->generalCol1->QueueRedraw(gen1InsertionColor);

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

	// TODO used named constants?
	lResult = SendMessageW(hwnd, TB_BUTTONCOUNT, 0, 0);
	s = std::to_wstring(lResult);
	this->buttonCount->SetText(0, s.c_str());
	lResult = SendMessageW(hwnd, TB_GETROWS, 0, 0);
	s = std::to_wstring(lResult);
	this->buttonCount->SetText(1, s.c_str());

	if (SendMessageW(this->buttonList, LVM_DELETEALLITEMS, 0, 0) == (LRESULT) FALSE)
		panic(L"error wiping button list for new toolbar: %I32d", GetLastError());

	this->normalImageList = readImageList(p, hwnd, TB_GETIMAGELIST);
	// TODO error check
	SendMessageW(this->buttonList, LVM_SETIMAGELIST, LVSIL_NORMAL, (LPARAM) (this->normalImageList));
	{ int i, n;
	n = ImageList_GetImageCount(this->normalImageList);
	for (i = 0; i < n; i++) {
	LVITEMW lvi;
	ZeroMemory(&lvi, sizeof (LVITEMW));
	lvi.mask = LVIF_IMAGE | LVIF_TEXT;
	lvi.iItem = i;
	lvi.pszText = L"image list entry";
	lvi.iImage = i;
	SendMessageW(this->buttonList, LVM_INSERTITEMW, 0, (LPARAM) (&lvi)); } }
}

HDWP ToolbarTab::RelayoutChild(HDWP dwp, HWND page, RECT *fill, Layouter *d)
{
	LONG colwid;
	SIZE size;

	if (page == this->generalTab) {
		colwid = (fill->right - fill->left - d->PaddingX()) / 2;
		dwp = this->generalCol1->RelayoutWidth(dwp,
			fill->left, fill->top,
			colwid,
			d);
		dwp = this->generalCol2->RelayoutWidth(dwp,
			fill->left + colwid + d->PaddingX(), fill->top,
			colwid,
			d);
	}

	if (page == this->buttonsTab) {
		colwid = (fill->right - fill->left - d->PaddingX()) / 3;
		size = this->buttonCount->MinimumSize(d);
		if (colwid < size.cx)
			colwid = size.cx;
		dwp = this->buttonCount->Relayout(dwp,
			fill->left, fill->top,
			d);
		dwp = deferWindowPos(dwp, this->buttonList,
			fill->left, fill->top + size.cy + d->PaddingY(),
			colwid, fill->bottom - (fill->top + size.cy + d->PaddingY()),
			0);
		if (SendMessageW(this->buttonList, LVM_SETCOLUMNWIDTH, 0, LVSCW_AUTOSIZE) == (LRESULT) FALSE)
			panic(L"error resizing button list column: %I32d", GetLastError());
	}

	return dwp;
}

bool ToolbarTab::OnCtlColorStatic(HDC dc, HWND hwnd, HBRUSH *brush)
{
	switch (this->generalCol1->WhichRowIs(hwnd)) {
	case gen1ButtonHighlight:
		*brush = this->buttonHighlightBrush;
		return *brush != NULL;
	case gen1ButtonShadow:
		*brush = this->buttonShadowBrush;
		return *brush != NULL;
	case gen1InsertionColor:
		*brush = this->insertionPointBrush;
		return true;
	}
	return false;
}
