// 25 january 2017
#include "barspy.hpp"

// all of this is documented; we just need to be careful about using the right HMODULE

static const uint8_t call386[] = {
	0x55, 0x8B, 0xEC, 0x83, 0xEC, 0x18, 0x8B, 0x45,
	0x08, 0x89, 0x45, 0xFC, 0x33, 0xC9, 0x89, 0x4D,
	0xE8, 0x89, 0x4D, 0xEC, 0x89, 0x4D, 0xF0, 0x89,
	0x4D, 0xF4, 0x89, 0x4D, 0xF8, 0xC7, 0x45, 0xE8,
	0x14, 0x00, 0x00, 0x00, 0x8D, 0x55, 0xE8, 0x52,
	0x8B, 0x45, 0xFC, 0x8B, 0x08, 0xFF, 0xD1, 0x8B,
	0x55, 0xFC, 0x89, 0x42, 0x0C, 0x8B, 0x45, 0xFC,
	0x8B, 0x4D, 0xEC, 0x89, 0x48, 0x04, 0x8B, 0x55,
	0xFC, 0x8B, 0x45, 0xF0, 0x89, 0x42, 0x08, 0x33,
	0xC0, 0x8B, 0xE5, 0x5D, 0xC2, 0x04, 0x00,
};
static const size_t nCall386 = 79;

static const uint8_t callAMD64[] = {
	0x48, 0x89, 0x4C, 0x24, 0x08, 0x57, 0x48, 0x83,
	0xEC, 0x40, 0x48, 0x8B, 0x44, 0x24, 0x50, 0x48,
	0x89, 0x44, 0x24, 0x20, 0x48, 0x8D, 0x44, 0x24,
	0x28, 0x48, 0x8B, 0xF8, 0x33, 0xC0, 0xB9, 0x14,
	0x00, 0x00, 0x00, 0xF3, 0xAA, 0xC7, 0x44, 0x24,
	0x28, 0x14, 0x00, 0x00, 0x00, 0x48, 0x8D, 0x4C,
	0x24, 0x28, 0x48, 0x8B, 0x44, 0x24, 0x20, 0xFF,
	0x10, 0x48, 0x8B, 0x4C, 0x24, 0x20, 0x89, 0x41,
	0x10, 0x48, 0x8B, 0x44, 0x24, 0x20, 0x8B, 0x4C,
	0x24, 0x2C, 0x89, 0x48, 0x08, 0x48, 0x8B, 0x44,
	0x24, 0x20, 0x8B, 0x4C, 0x24, 0x30, 0x89, 0x48,
	0x0C, 0x33, 0xC0, 0x48, 0x83, 0xC4, 0x40, 0x5F,
	0xC3,
};
static const size_t nCallAMD64 = 97;

struct archInfo {
	const uint8_t *call;
	size_t nCall;
	size_t structSize;
	size_t offDGV;
	size_t sizeDGV;
	size_t offMajor;
	size_t sizeMajor;
	size_t offMinor;
	size_t sizeMinor;
	size_t offHRESULT;
	size_t sizeHRESULT;
};

const struct archInfo archInfo[] = {
#define arch386 0
	{
		call386, nCall386,
		16,
		0, 4,
		4, 4,
		8, 4,
		12, 4,
	},
#define archAMD64 1
	{
		callAMD64, nCallAMD64,
		20,
		0, 8,
		8, 4,
		12, 4,
		16, 4,
	},
};

static WCHAR *runThread(Process *p, const struct archInfo *ai, void *pCode, void *pData)
{
	HANDLE hThread;
	WCHAR *out;
	DWORD major, minor;
	HRESULT hr;

	hThread = p->CreateThread(pCode, pData);
	// TODO switch to MsgWaitForMultipleObjectsEx()? this code assumes it is atomic with regards to the UI
	if (WaitForSingleObject(hThread, INFINITE) == WAIT_FAILED)
		panic(L"error waiting for process string thread to run: %I32d", GetLastError());
	if (CloseHandle(hThread) == 0)
		panic(L"error closing thread: %I32d", GetLastError());

	p->Read(pData, ai->offMajor, &major, ai->sizeMajor);
	p->Read(pData, ai->offMinor, &minor, ai->sizeMinor);
	p->Read(pData, ai->offHRESULT, &hr, ai->sizeHRESULT);

	out = new WCHAR[32];
	// TODO error checks from StringCchPrintfW()
	if (hr != S_OK) {
		StringCchPrintfW(out, 32, L"error 0x%08I32X", hr);
		return out;
	}
	StringCchPrintfW(out, 32, L"%I32d.%I32d", major, minor);
	return out;
}

WCHAR *getDLLVersion(HWND hwnd, Process *p)
{
	int arch;
	const struct archInfo *ai;
	LPVOID pCode, pData;
	void *pcomctl32;
	uint32_t off32;
	uint64_t off64;
	void *off;
	WCHAR *ret;

	arch = arch386;
	if (p->Is64Bit())
		arch = archAMD64;
	ai = &(archInfo[arch]);

	pCode = p->AllocBlock(ai->nCall);
	p->Write(pCode, 0, ai->call, ai->nCall);
	p->MakeExecutable(pCode, ai->nCall);

	pData = p->AllocBlock(ai->structSize);

	// use the comctl32.dll that was actually used for this control
	// SxS and isolation awareness means we can load multiple versions of comctl32.dll into a process at once
	pcomctl32 = (void *) GetClassLongPtrW(hwnd, GCLP_HMODULE);

	off = p->GetProcAddress(pcomctl32, "DllGetVersion");
	switch (arch) {
	case arch386:
		off32 = (uint32_t) off;
		off = &off32;
		break;
	case archAMD64:
		off64 = (uint64_t) off;
		off = &off64;
		break;
	}
	p->Write(pData, ai->offDGV, off, ai->sizeDGV);

	ret = runThread(p, ai, pCode, pData);

	p->FreeBlock(pData);
	p->FreeBlock(pCode);
	return ret;
}
