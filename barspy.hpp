// 13 december 2016
#include "winapi.hpp"
#include "resources.h"

#define APPTITLE L"BarSpy"

// main.cpp
extern HINSTANCE hInstance;
extern int nCmdShow;
extern HICON hDefIcon;
extern HCURSOR hDefCursor;
extern HBRUSH blackBrush;
extern HFONT hMessageFont;
// TODO rename or delete this
extern void initCommon(HINSTANCE hInst, int nCS);

// mainwin.cpp
extern HWND mainwin;
extern void openMainWindow(void);

// panic.cpp
extern void panic(const WCHAR *msg, ...);

// util.cpp
extern int classNameOf(WCHAR *classname, ...);
extern WCHAR *windowClass(HWND hwnd);
extern int windowClassOf(HWND hwnd, ...);
#define DESIREDCLASSES TOOLBARCLASSNAMEW, REBARCLASSNAMEW
#define DESIREDTOOLBAR 0
#define DESIREDREBAR 1
extern HDWP deferWindowPos(HDWP dwp, HWND hwnd, int x, int y, int width, int height, UINT flags);

// enum.cpp
extern void enumWindowTree(HWND treeview, HTREEITEM (*f)(HWND treeview, HWND window, HTREEITEM parent));

// checkmark.cpp
extern HICON hIconYes;
extern HICON hIconNo;
extern HICON hIconUnknown;
extern void initCheckmarks(void);
extern HWND newCheckmark(HWND parent, HMENU id);
extern void setCheckmarkIcon(HWND hwnd, HICON icon);
extern SIZE checkmarkSize(HWND hwnd);

// layout.cpp
class Layouter {
	int baseX, baseY;
	TEXTMETRICW tm;
	SIZE textSize;
public:
	Layouter(HWND hwnd);

	int X(int x);
	int Y(int y);
	LONG InternalLeading(void);
	LONG TextWidth(void);

	// specifics
	int PaddingX(void);
	int PaddingY(void);
	int WindowMarginX(void);
	int WindowMarginY(void);
	int EditHeight(void);
	int LabelYForSiblingY(int siblingY, Layouter *label);
	int LabelHeight(void);
};
extern LONG longestTextWidth(HWND hwnd, ...);
// TODO allow icons
class Form {
	HWND parent;
	int id;
	int minEditWidth;
	bool padded;
	std::vector<HWND> labels;
	std::vector<HWND> edits;
	void padding(Layouter *dparent, LONG *x, LONG *y);
	HDWP relayout(HDWP dwp, LONG x, LONG y, bool useWidth, LONG width, bool widthIsEditOnly, Layouter *dparent);
public:
	Form(HWND parent, int id = 100, int minEditWidth = 0);
	int ID(void);
	void SetID(int id);
	void SetMinEditWidth(int editMinWidth);
	void SetPadded(bool padded);
	void Add(const WCHAR *label);
	void AddTrailingLabel(const WCHAR *label);
	void SetText(int id, const WCHAR *text);
	SIZE MinimumSize(Layouter *dparent);
	HDWP Relayout(HDWP dwp, LONG x, LONG y, Layouter *dparent);
	HDWP RelayoutWidth(HDWP dwp, LONG x, LONG y, LONG width, Layouter *dparent);
	HDWP RelayoutEditWidth(HDWP dwp, LONG x, LONG y, LONG width, Layouter *dparent);
};

// process.cpp
class Process {
	DWORD pid;
	HANDLE hProc;
public:
	Process(DWORD pid);
	~Process(void);

	bool Is64Bit(void);

	void *AllocBlock(size_t size);
	void FreeBlock(void *block);
	void MakeExecutable(void *block, size_t size);

	void Read(void *base, size_t off, void *buf, size_t len);
	void Write(void *base, size_t off, const void *buf, size_t len);

	void *GetModuleBase(const WCHAR *modname);
	void *GetProcAddress(void *modbase, const char *procname);

	HANDLE CreateThread(void *threadProc, void *param);
};
extern void initProcess(void);
extern Process *processFromHWND(HWND hwnd);

// common.cpp
class Common {
	Form version;

	HWND labelUnicode;
	HWND iconUnicode;

	Form setWindowTheme;

	Form styles;
public:
	Common(HWND parent, int idoff);

	void Reset(void);
	void Reflect(HWND hwnd, Process *p);

	SIZE MinimumSize(Layouter *dparent);
	void Relayout(RECT *fill, Layouter *dparent);
};
extern std::wstring drawTextFlagsString(UINT relevant);

// gettheme.cpp
extern void getWindowTheme(HWND hwnd, Process *p, WCHAR **pszSubAppName, WCHAR **pszSubIdList);

// dllgetver.cpp
extern WCHAR *getDLLVersion(HWND hwnd, Process *p);
