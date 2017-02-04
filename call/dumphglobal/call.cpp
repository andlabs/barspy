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
#include <commctrl.h>
#include <stdint.h>

// This isn't meant to be compiled; rather, its assembly output is copied into the main program.

struct tpargs {
	SIZE_T (WINAPI *GlobalSizePtr)(HGLOBAL hMem);
	LPVOID (WINAPI *GlobalLockPtr)(HGLOBAL hMem);
	BOOL (WINAPI *GlobalUnlockPtr)(HGLOBAL hMem);
	HGLOBAL (WINAPI *GlobalFreePtr)(HGLOBAL hMem);
	LPVOID (WINAPI *VirtualAllocPtr)(LPVOID lpAddress, SIZE_T dwSize, DWORD flAllocationType, DWORD flProtect);
	DWORD (WINAPI *GetLastErrorPtr)(void);
	HGLOBAL hGlobal;
	LPVOID memory;
	SIZE_T size;
	BOOL success;
	DWORD lastError;
};

DWORD WINAPI ThreadProc(LPVOID lpParameter)
{
	struct tpargs *a = (struct tpargs *) lpParameter;
	void *gmem;
	uint8_t *bdest;
	uint8_t *bsrc;
	SIZE_T i;

	a->size = (*(a->GlobalSizePtr))(a->hGlobal);
	if (a->size == 0)
		goto fail;
	gmem = (*(a->GlobalLockPtr))(a->hGlobal);
	if (gmem == NULL)
		goto fail;

	a->memory = (*(a->VirtualAllocPtr))(NULL, a->size,
		MEM_COMMIT, PAGE_READWRITE);
	if (a->memory == NULL)
		goto fail;
	// unfortunately memcpy() does not intrinsify on 32-bit if the size is determined at runtime :(
	// and memmove() cannot be made intrinsic
	// and RtlCopyMemory() and RtlMoveMemory() are just #defines to memcpy() and memmove(), even in the WDK (despite MSDN saying they're in DLLs...; http://nullprogram.com/blog/2016/02/28/ says it *is* available, but since I can't confirm the calling convention I'd rather not, at least not yet (also it says RtlCopyMemory() was not exported until Windows 7 but unlike RtlMoveMemory() there is only the Windows Drivers page on MSDN for that one, not a Windows API page), oh and then there's http://stackoverflow.com/questions/16008625/why-is-rtlfillmemory-rtlcopymemory-defined-as-macro which says we were never supposed to use these in the first place! — TODO)
	// so we have no choice...
	bdest = (uint8_t *) (a->memory);
	bsrc = (uint8_t *) gmem;
	for (i = 0; i < a->size; i++)
		*bdest++ = *bsrc++;

	if ((*(a->GlobalUnlockPtr))(a->hGlobal) == 0) {
		a->lastError = (*(a->GetLastErrorPtr))();
		if (a->lastError != NO_ERROR) {
			a->success = FALSE;
			return 0;
		}
	}
	if ((*(a->GlobalFreePtr))(a->hGlobal) != NULL)
		goto fail;

	a->success = TRUE;
	return 0;

fail:
	a->lastError = (*(a->GetLastErrorPtr))();
	a->success = FALSE;
	return 0;
}
