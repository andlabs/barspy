// 25 january 2017
#include "barspy.hpp"

struct enumTree {
	HWND treeview;
	HTREEITEM (*f)(HWND treeview, HWND window, HTREEITEM parent);
	HTREEITEM parent;
};

static BOOL CALLBACK ewtEach(HWND hwnd, LPARAM lParam)
{
	struct enumTree *parent = (struct enumTree *) lParam;
	struct enumTree child;

	child.treeview = parent->treeview;
	child.f = parent->f;
	child.parent = (*(parent->f))(parent->treeview, hwnd, parent->parent);
	EnumChildWindows(hwnd, ewtEach, (LPARAM) (&child));
	return TRUE;
}

void enumWindowTree(HWND treeview, HTREEITEM (*f)(HWND treeview, HWND window, HTREEITEM parent))
{
	struct enumTree topmost;

	topmost.treeview = treeview;
	topmost.f = f;
	topmost.parent = TVI_ROOT;
	EnumChildWindows(NULL, ewtEach, (LPARAM) (&topmost));
}
