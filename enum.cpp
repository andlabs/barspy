// 25 january 2017
#include "barspy.hpp"

// whoops, EnumChildWindows() is already recursive
// TODO can TVM_GETNEXTITEM return an error other than "no more items"? and isn't root NULL?
static HTREEITEM locate(HWND tree, LPARAM lParam, HTREEITEM cur)
{
	HTREEITEM child;
	TVITEMEXW tvi;

	while (cur != NULL) {
		ZeroMemory(&tvi, sizeof (TVITEMEXW));
		tvi.mask = TVIF_PARAM;
		tvi.hItem = cur;
		if (SendMessageW(tree, TVM_GETITEMW, 0, (LPARAM) (&tvi)) == (LRESULT) FALSE)
			panic(L"error getting info of tree item searching for items by lParam: %I32d", GetLastError());
		if (tvi.lParam == lParam)
			return cur;
		child = (HTREEITEM) SendMessageW(tree, TVM_GETNEXTITEM, TVGN_CHILD, (LPARAM) cur);
		if (child != NULL) {
			child = locate(tree, lParam, child);
			if (child != NULL)
				return child;
		}
		cur = (HTREEITEM) SendMessageW(tree, TVM_GETNEXTITEM, TVGN_NEXT, (LPARAM) cur);
	}
	return NULL;			// make compiler happy
}

static HTREEITEM windowParentNode(HWND tree, HWND hwnd)
{
	DWORD style;
	HTREEITEM item;

	// TODO replace this with the explicit call
	style = GetWindowStyle(hwnd);
	// this function only works on child windows as their parents already have items
	// all non-child windows are at the top level of our tree, so...
	if ((style & WS_CHILD) == 0)
		return TVI_ROOT;

	item = (HTREEITEM) SendMessageW(tree, TVM_GETNEXTITEM, TVGN_ROOT, 0);
	item = (HTREEITEM) SendMessageW(tree, TVM_GETNEXTITEM, TVGN_CHILD, (LPARAM) item);
	item = locate(tree, (LPARAM) hwnd, item);
	if (item == NULL)
		panic(L"parent window not found in tree view");
	return item;
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
