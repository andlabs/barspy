// 25 january 2017
#include "barspy.hpp"

struct enumTree {
	HWND treeview;
	HTREEITEM (*f)(HWND treeview, HWND window, HTREEITEM parent);
	// EnumChildWindows() recurses
	// fortunately it seems to always recurse in depth-first order
	// and since it never revisits a node, we can just do this
	// TODO I guess: make sure it never visits a new child of an old parent
	std::vector<HWND> *curHWNDPath;
	std::vector<HTREEITEM> *curNodePath;
};

static BOOL CALLBACK ewtEach(HWND hwnd, LPARAM lParam)
{
	struct enumTree *t = (struct enumTree *) lParam;
	HWND parent;
	HTREEITEM parentNode, newNode;

	// this is what Catch-22's WinSpy does
	// TODO I forget if it did anything else
	parent = GetAncestor(hwnd, GA_PARENT);
	if (parent == GetDesktopWindow())
		parent = NULL;

	if (parent == NULL) {
		t->curHWNDPath->clear();
		t->curNodePath->clear();
		parentNode = TVI_ROOT;
	} else {
		while (t->curHWNDPath->size() != 0) {
			if (t->curHWNDPath->back() == parent)
				break;
			t->curHWNDPath->pop_back();
			t->curNodePath->pop_back();
		}
		if (t->curNodePath->size() == 0)
			// back to root somehow??? TODO
			parentNode = TVI_ROOT;
		else
			parentNode = t->curNodePath->back();
	}

	newNode = (*(t->f))(t->treeview, hwnd, parentNode);
	t->curHWNDPath->push_back(hwnd);
	t->curNodePath->push_back(newNode);

	return TRUE;
}

void enumWindowTree(HWND treeview, HTREEITEM (*f)(HWND treeview, HWND window, HTREEITEM parent))
{
	struct enumTree t;

	t.treeview = treeview;
	t.f = f;
	t.curHWNDPath = new std::vector<HWND>;
	if (t.curHWNDPath->capacity() < 16)
		t.curHWNDPath->reserve(16);
	t.curNodePath = new std::vector<HTREEITEM>;
	if (t.curNodePath->capacity() < 16)
		t.curNodePath->reserve(16);
	// passing NULL only does EnuMWindows()
	EnumChildWindows(GetDesktopWindow(), ewtEach, (LPARAM) (&t));
	delete t.curNodePath;
	delete t.curHWNDPath;
}
