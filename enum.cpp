// 25 january 2017
#include "barspy.hpp"

// whoops, EnumChildWindows() is already recursive
// TODO can TVM_GETNEXTITEM return an error other than "no more items"? and isn't root NULL?
static HTREEITEM windowParentNode(HWND tree, HWND hwnd)
{
	DWORD style;
	HWND parent;
	HTREEITEM nparent;
	HTREEITEM cur;
	TVITEMEXW tvi;

	// TODO replace this with the explicit call
	style = GetWindowStyle(hwnd);
	// this function only works on child windows
	// all non-child windows are at the top level of our tree, so...
	if ((style & WS_CHILD) == 0)
		return TVI_ROOT;

	parent = GetAncestor(hwnd, GA_PARENT);
	nparent = windowParentNode(tree, parent);
	if (nparent == TVI_ROOT)
		nparent = (HTREEITEM) SendMessageW(tree, TVM_GETNEXTITEM, TVGN_ROOT, 0);
	cur = (HTREEITEM) SendMessageW(tree, TVM_GETNEXTITEM, TVGN_CHILD, (LPARAM) nparent);
	while (cur != NULL) {
		ZeroMemory(&tvi, sizeof (TVITEMEXW));
		tvi.mask = TVIF_PARAM;
		tvi.hItem = cur;
		if (SendMessageW(tree, TVM_GETITEMW, 0, (LPARAM) (&tvi)) == (LRESULT) FALSE)
			panic(L"error getting info of tree item searching for parents: %I32d", GetLastError());
{WCHAR msg[64];
wsprintf(msg,L"%p %p",tvi.lParam,parent);
MessageBoxW(NULL,msg,NULL,MB_OK);
}
		if ((HWND) (tvi.lParam) == parent)
			return cur;
		cur = (HTREEITEM) SendMessageW(tree, TVM_GETNEXTITEM, TVGN_NEXT, (LPARAM) cur);
	}
	panic(L"parent window not found in tree view");
	return NULL;			// make compiler happy
}

struct enumTree {
	HWND treeview;
	HTREEITEM (*f)(HWND treeview, HWND window, HTREEITEM parent);
};

static BOOL CALLBACK ewtEach(HWND hwnd, LPARAM lParam)
{
	struct enumTree *t = (struct enumTree *) lParam;

	// TODO somehow merge all of this together
	(*(t->f))(t->treeview, hwnd,
		windowParentNode(t->treeview, hwnd));
	return TRUE;
}

void enumWindowTree(HWND treeview, HTREEITEM (*f)(HWND treeview, HWND window, HTREEITEM parent))
{
	struct enumTree topmost;

	topmost.treeview = treeview;
	topmost.f = f;
	EnumChildWindows(NULL, ewtEach, (LPARAM) (&topmost));
}
