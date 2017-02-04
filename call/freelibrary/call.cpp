// 25 january 2017
#define UNICODE
#define _UNICODE
#define STRICT
#define STRICT_TYPED_ITEMIDS

// get Windows version right; right now Windows Vista
// unless otherwise stated, all values from Microsoft's sdkddkver.h
// TODO is all of this necessary? how is NTDDI_VERSION used?
// TODO plaform update sp2
#define WINVER			0x0600	/* from Microsoft's winnls.h */
#define _WIN32_WINNT		0x0600
#define _WIN32_WINDOWS	0x0600	/* from Microsoft's pdh.h */
#define _WIN32_IE			0x0700
#define NTDDI_VERSION		0x06000000

#include <windows.h>

// This isn't meant to be compiled; rather, its assembly output is copied into the main program.

struct tpargs {
	BOOL (WINAPI *FreeLibraryPtr)(HMODULE hModule);
	DWORD (WINAPI *GetLastErrorPtr)(void);
	HMODULE module;
	BOOL ret;
	DWORD lastError;
};

DWORD WINAPI ThreadProc(LPVOID lpParameter)
{
	struct tpargs *a = (struct tpargs *) lpParameter;

	a->ret = (*(a->FreeLibraryPtr))(a->module);
	a->lastError = (*(a->GetLastErrorPtr))();
	return 0;
}
