// 13 december 2016
#include "barspy.hpp"

#define ourbufsiz 1024

static WCHAR buf[ourbufsiz];

// if any of these functions fail, we're screwed anyway, so don't bother with errors
// StringCchVPrintfW() at least always null-terminates the string
void panic(const WCHAR *msg, ...)
{
	va_list ap;

	va_start(ap, msg);
	StringCchVPrintfW(buf, ourbufsiz, msg, ap);
	va_end(ap);
	MessageBoxW(mainwin,
		buf,
		APPTITLE,
		MB_OK | MB_ICONERROR);
	exit(EXIT_FAILURE);
}
