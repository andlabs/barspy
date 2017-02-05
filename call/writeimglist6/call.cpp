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

// This isn't meant to be compiled; rather, its assembly output is copied into the main program.

struct tpargs {
	HRESULT (STDAPICALLTYPE *CreateStreamOnHGlobalPtr)(HGLOBAL hGlobal, BOOL fDeleteOnRelease, LPSTREAM *ppstm);
	HRESULT (STDAPICALLTYPE *GetHGlobalFromStreamPtr)(IStream *pstm, HGLOBAL *phglobal);
	HRESULT (WINAPI *ImageList_WriteExPtr)(HIMAGELIST himl, DWORD dwFlags, LPSTREAM pstm);
	HIMAGELIST imglist;
	HGLOBAL hGlobal;
	HRESULT hr;
};

DWORD WINAPI ThreadProc(LPVOID lpParameter)
{
	struct tpargs *a = (struct tpargs *) lpParameter;
	IStream *stream = NULL;
	DWORD lastError;

	a->hr = (*(a->CreateStreamOnHGlobalPtr))(NULL, FALSE, &stream);
	if (a->hr != S_OK)
		goto out;
	a->hr = (*(a->ImageList_WriteExPtr))(a->imglist, ILP_NORMAL, stream);
	if (a->hr != S_OK)
		goto out;
	a->hr = (*(a->GetHGlobalFromStreamPtr))(stream, &(a->hGlobal));

out:
	if (stream != NULL)
		stream->Release();
	return 0;
}
