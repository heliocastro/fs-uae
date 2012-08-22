#include "sysconfig.h"
#include "sysdeps.h"

#include <windows.h>
#include <sys/timeb.h>

#include "options.h"
#include "custom.h"
#include "events.h"
#include "debug.h"
#include "debug_win32.h"
#include "win32.h"
#include "registry.h"
#include "uae.h"

static void premsg (void)
{
#if 0
	static int done;
	char *as;
	char ast[32];
	TCHAR *ws;

	if (done)
		return;
	done = 1;

	ast[0] = 'A';
	ast[1] = 0x7f;
	ast[2] = 0x80;
	ast[3] = 0x81;
	ast[4] = 0x9f;
	ast[5] = 0;
	ws = au_fs (ast);

	MessageBoxA(NULL, "espa�ol", "ANSI", MB_OK);
	MessageBoxW(NULL, _T("espa�ol"), _T("UTF-16"), MB_OK);

	as = ua (_T("espa�ol"));
	MessageBoxA(NULL, as, "ANSI:2", MB_OK);
	ws = au (as);
	MessageBoxW(NULL, ws, _T("UTF-16:2"), MB_OK);
	xfree (ws);
	xfree (as);

	ws = au ("espa�ol");
	MessageBoxW(NULL, ws, _T("UTF-16:3"), MB_OK);
	as = ua (ws);
	MessageBoxA(NULL, as, "ANSI:3", MB_OK);
	xfree (ws);
	xfree (as);
#endif
}

#define SHOW_CONSOLE 0

static int nodatestamps = 0;

int consoleopen = 0;
static int realconsole;
static HANDLE stdinput,stdoutput;
static int bootlogmode;
static CRITICAL_SECTION cs;
static int cs_init;

FILE *debugfile = NULL;
int console_logging = 0;
static int debugger_type = -1;
extern BOOL debuggerinitializing;
extern int lof_store;
int always_flush_log = 1;

#define WRITE_LOG_BUF_SIZE 4096

/* console functions for debugger */

static HWND myGetConsoleWindow (void)
{
	return GetConsoleWindow ();
}

static void getconsole (void)
{
	CONSOLE_SCREEN_BUFFER_INFO csbi;

	stdinput = GetStdHandle (STD_INPUT_HANDLE);
	stdoutput = GetStdHandle (STD_OUTPUT_HANDLE);
	SetConsoleMode (stdinput, ENABLE_PROCESSED_INPUT|ENABLE_LINE_INPUT|ENABLE_ECHO_INPUT|ENABLE_PROCESSED_OUTPUT);
	SetConsoleCP (65001);
	SetConsoleOutputCP (65001);
	if (GetConsoleScreenBufferInfo (stdoutput, &csbi)) {
		if (csbi.dwMaximumWindowSize.Y < 900) {
			csbi.dwMaximumWindowSize.Y = 900;
			SetConsoleScreenBufferSize (stdoutput, csbi.dwMaximumWindowSize);
		}
	}
}

static void open_console_window (void)
{
	AllocConsole ();
	getconsole ();
	consoleopen = -1;
	reopen_console ();
}

static void openconsole (void)
{
	if (realconsole) {
		if (debugger_type == 2) {
			open_debug_window ();
			consoleopen = 1;
		} else {
			close_debug_window ();
			consoleopen = -1;
		}
		return;
	}
	if (debugger_active && (debugger_type < 0 || debugger_type == 2)) {
		if (consoleopen > 0 || debuggerinitializing)
			return;
		if (debugger_type < 0) {
			regqueryint (NULL, _T("DebuggerType"), &debugger_type);
			if (debugger_type <= 0)
				debugger_type = 2;
			openconsole();
			return;
		}
		close_console ();
		if (open_debug_window ()) {
			consoleopen = 1;
			return;
		}
		open_console_window ();
	} else {
		if (consoleopen < 0)
			return;
		close_console ();
		open_console_window ();
	}
}

void debugger_change (int mode)
{
	if (mode < 0)
		debugger_type = debugger_type == 2 ? 1 : 2;
	else
		debugger_type = mode;
	if (debugger_type != 1 && debugger_type != 2)
		debugger_type = 2;
	regsetint (NULL, _T("DebuggerType"), debugger_type);
	openconsole ();
}

void reopen_console (void)
{
	HWND hwnd;

	if (realconsole)
		return;
	if (consoleopen >= 0)
		return;
	hwnd = myGetConsoleWindow ();
	if (hwnd) {
		int newpos = 1;
		int x, y, w, h;
		if (!regqueryint (NULL, _T("LoggerPosX"), &x))
			newpos = 0;
		if (!regqueryint (NULL, _T("LoggerPosY"), &y))
			newpos = 0;
		if (!regqueryint (NULL, _T("LoggerPosW"), &w))
			newpos = 0;
		if (!regqueryint (NULL, _T("LoggerPosH"), &h))
			newpos = 0;
		if (newpos) {
			RECT rc;
			rc.left = x;
			rc.top = y;
			rc.right = x + w;
			rc.bottom = y + h;
			if (MonitorFromRect (&rc, MONITOR_DEFAULTTONULL) != NULL) {
				SetForegroundWindow (hwnd);
				SetWindowPos (hwnd, HWND_TOP, x, y, w, h, SWP_NOACTIVATE);

			}
		}
	}
}

void close_console (void)
{
	if (realconsole)
		return;
	if (consoleopen > 0) {
		close_debug_window ();
	} else if (consoleopen < 0) {
		HWND hwnd = myGetConsoleWindow ();
		if (hwnd && !IsIconic (hwnd)) {
			RECT r;
			if (GetWindowRect (hwnd, &r)) {
				r.bottom -= r.top;
				r.right -= r.left;
				regsetint (NULL, _T("LoggerPosX"), r.left);
				regsetint (NULL, _T("LoggerPosY"), r.top);
				regsetint (NULL, _T("LoggerPosW"), r.right);
				regsetint (NULL, _T("LoggerPosH"), r.bottom);
			}
		}
		FreeConsole ();
	}
	consoleopen = 0;
}

static void writeconsole_2 (const TCHAR *buffer)
{
	DWORD temp;

	if (!consoleopen)
		openconsole ();

	if (consoleopen > 0) {
		WriteOutput (buffer, _tcslen (buffer));
	} else if (realconsole) {
		fputws (buffer, stdout);
		fflush (stdout);
	} else if (consoleopen < 0) {
		WriteConsole (stdoutput, buffer, _tcslen (buffer), &temp, 0);
	}
}

static void writeconsole (const TCHAR *buffer)
{
	if (_tcslen (buffer) > 256) {
		TCHAR *p = my_strdup (buffer);
		TCHAR *p2 = p;
		while (_tcslen (p) > 256) {
			TCHAR tmp = p[256];
			p[256] = 0;
			writeconsole_2 (p);
			p[256] = tmp;
			p += 256;
		}
		writeconsole_2 (p);
		xfree (p2);
	} else {
		writeconsole_2 (buffer);
	}
}

static void flushconsole (void)
{
	if (consoleopen > 0) {
		fflush (stdout);
	} else if (realconsole) {
		fflush (stdout);
	} else if (consoleopen < 0) {
		FlushFileBuffers  (stdoutput);
	}
}

static TCHAR *console_buffer;
static int console_buffer_size;

TCHAR *setconsolemode (TCHAR *buffer, int maxlen)
{
	TCHAR *ret = NULL;
	if (buffer) {
		console_buffer = buffer;
		console_buffer_size = maxlen;
	} else {
		ret = console_buffer;
		console_buffer = NULL;
	}
	return ret;
}

static void console_put (const TCHAR *buffer)
{
	if (console_buffer) {
		if (_tcslen (console_buffer) + _tcslen (buffer) < console_buffer_size)
			_tcscat (console_buffer, buffer);
	} else {
		openconsole ();
		writeconsole (buffer);
	}
}

void console_out_f (const TCHAR *format,...)
{
	va_list parms;
	TCHAR buffer[WRITE_LOG_BUF_SIZE];

	va_start (parms, format);
	_vsntprintf (buffer, WRITE_LOG_BUF_SIZE - 1, format, parms);
	va_end (parms);
	console_put (buffer);
}
void console_out (const TCHAR *txt)
{
	console_put (txt);
}

TCHAR console_getch (void)
{
	if (realconsole) {
		return getwc (stdin);
	} else if (consoleopen < 0) {
		DWORD len;
		TCHAR out[2];
		
		for (;;) {
			out[0] = 0;
			ReadConsole (stdinput, out, 1, &len, 0);
			if (len > 0)
				return out[0];
		}
	}
	return 0;
}

int console_get (TCHAR *out, int maxlen)
{
	*out = 0;

	if (consoleopen > 0) {
		return console_get_gui (out, maxlen);
	} else if (realconsole) {
		DWORD totallen;

		*out = 0;
		totallen = 0;
		while (maxlen > 0) {
			*out = getwc (stdin);
			if (*out == 13)
				break;
			out++;
			maxlen--;
			totallen++;
		}
		*out = 0;
		return totallen;
	} else if (consoleopen < 0) {
		DWORD len, totallen;

		*out = 0;
		totallen = 0;
		while(maxlen > 0) {
			ReadConsole (stdinput, out, 1, &len, 0);
			if(*out == 13)
				break;
			out++;
			maxlen--;
			totallen++;
		}
		*out = 0;
		return totallen;
	}
	return 0;
}


void console_flush (void)
{
	flushconsole ();
}

static int lfdetected = 1;

static TCHAR *writets (void)
{
	struct tm *t;
	struct _timeb tb;
	static TCHAR out[100];
	TCHAR *p;
	static TCHAR lastts[100];
	TCHAR curts[100];

	if (bootlogmode)
		return NULL;
	if (nodatestamps)
		return NULL;
	_ftime (&tb);
	t = localtime (&tb.time);
	_tcsftime (curts, sizeof curts / sizeof (TCHAR), _T("%Y-%m-%d %H:%M:%S\n"), t);
	p = out;
	*p = 0;
	if (_tcsncmp (curts, lastts, _tcslen (curts) - 3)) { // "xx\n"
		_tcscat (p, curts);
		p += _tcslen (p);
		_tcscpy (lastts, curts);
	}
	_tcsftime (p, sizeof out / sizeof (TCHAR) - (p - out) , _T("%S-"), t);
	p += _tcslen (p);
	_stprintf (p, _T("%03d"), tb.millitm);
	p += _tcslen (p);
	if (vsync_counter != 0xffffffff)
		_stprintf (p, _T(" [%d %03d%s%03d]"), vsync_counter, current_hpos (), lof_store ? _T("-") : _T("="), vpos);
	_tcscat (p, _T(": "));
	return out;
}

void write_dlog (const TCHAR *format, ...)
{
	int count;
	TCHAR buffer[WRITE_LOG_BUF_SIZE];
	int bufsize = WRITE_LOG_BUF_SIZE;
	TCHAR *bufp;
	va_list parms;

	if (!cs_init)
		return;

	premsg ();

	EnterCriticalSection (&cs);
	va_start (parms, format);
	bufp = buffer;
	for (;;) {
		count = _vsntprintf (bufp, bufsize - 1, format, parms);
		if (count < 0) {
			bufsize *= 10;
			if (bufp != buffer)
				xfree (bufp);
			bufp = xmalloc (TCHAR, bufsize);
			continue;
		}
		break;
	}
	bufp[bufsize - 1] = 0;
	if (!_tcsncmp (bufp, _T("write "), 6))
		bufsize--;
	if (bufp[0] == '*')
		count++;
	if (SHOW_CONSOLE || console_logging) {
		writeconsole (bufp);
	}
	if (debugfile) {
		_ftprintf (debugfile, _T("%s"), bufp);
	}
	lfdetected = 0;
	if (_tcslen (bufp) > 0 && bufp[_tcslen (bufp) - 1] == '\n')
		lfdetected = 1;
	va_end (parms);
	if (bufp != buffer)
		xfree (bufp);
	if (always_flush_log)
		flush_log ();
	LeaveCriticalSection (&cs);
}

void write_log (const TCHAR *format, ...)
{
	int count;
	TCHAR buffer[WRITE_LOG_BUF_SIZE], *ts;
	int bufsize = WRITE_LOG_BUF_SIZE;
	TCHAR *bufp;
	va_list parms;

	if (!cs_init)
		return;

	premsg ();

	EnterCriticalSection (&cs);
	va_start (parms, format);
	bufp = buffer;
	for (;;) {
		count = _vsntprintf (bufp, bufsize - 1, format, parms);
		if (count < 0) {
			bufsize *= 10;
			if (bufp != buffer)
				xfree (bufp);
			bufp = xmalloc (TCHAR, bufsize);
			continue;
		}
		break;
	}
	bufp[bufsize - 1] = 0;
	if (!_tcsncmp (bufp, _T("write "), 6))
		bufsize--;
	ts = writets ();
	if (bufp[0] == '*')
		count++;
	if (SHOW_CONSOLE || console_logging) {
		if (lfdetected && ts)
			writeconsole (ts);
		writeconsole (bufp);
	}
	if (debugfile) {
		if (lfdetected && ts)
			_ftprintf (debugfile, _T("%s"), ts);
		_ftprintf (debugfile, _T("%s"), bufp);
	}
	lfdetected = 0;
	if (_tcslen (bufp) > 0 && bufp[_tcslen (bufp) - 1] == '\n')
		lfdetected = 1;
	va_end (parms);
	if (bufp != buffer)
		xfree (bufp);
	if (always_flush_log)
		flush_log ();
	LeaveCriticalSection (&cs);
}

void flush_log (void)
{
	if (debugfile)
		fflush (debugfile);
	flushconsole ();
}

void f_out (void *f, const TCHAR *format, ...)
{
	int count;
	TCHAR buffer[WRITE_LOG_BUF_SIZE];
	va_list parms;
	va_start (parms, format);

	if (f == NULL)
		return;
	count = _vsntprintf (buffer, WRITE_LOG_BUF_SIZE - 1, format, parms);
	openconsole ();
	writeconsole (buffer);
	va_end (parms);
}

TCHAR* buf_out (TCHAR *buffer, int *bufsize, const TCHAR *format, ...)
{
	int count;
	va_list parms;
	va_start (parms, format);

	if (buffer == NULL)
		return 0;
	count = _vsntprintf (buffer, (*bufsize) - 1, format, parms);
	va_end (parms);
	*bufsize -= _tcslen (buffer);
	return buffer + _tcslen (buffer);
}

FILE *log_open (const TCHAR *name, int append, int bootlog)
{
	FILE *f = NULL;

	if (!cs_init)
		InitializeCriticalSection (&cs);
	cs_init = 1;

	if (name != NULL) {
		if (bootlog >= 0) {
			f = _tfopen (name, append ? _T("a, ccs=UTF-8") : _T("wt, ccs=UTF-8"));
			if (!f && bootlog) {
				TCHAR tmp[MAX_DPATH];
				tmp[0] = 0;
				if (GetTempPath (MAX_DPATH, tmp) > 0) {
					_tcscat (tmp, _T("glog.txt"));
					f = _tfopen (tmp, append ? _T("a, ccs=UTF-8") : _T("wt, ccs=UTF-8"));
				}
			}
		}
		bootlogmode = bootlog;
	} else if (1) {
		TCHAR *c = GetCommandLine ();
		if (_tcsstr (c, _T(" -console"))) {
			if (GetStdHandle (STD_INPUT_HANDLE) && GetStdHandle (STD_OUTPUT_HANDLE)) {
				consoleopen = -1;
				realconsole = 1;
				getconsole ();
				_setmode(_fileno (stdout), _O_U16TEXT);
				_setmode(_fileno (stdin), _O_U16TEXT);
			}
		}
	}

	return f;
}

void log_close (FILE *f)
{
	if (f)
		fclose (f);
}

void jit_abort (const TCHAR *format,...)
{
	static int happened;
	int count;
	TCHAR buffer[WRITE_LOG_BUF_SIZE];
	va_list parms;
	va_start (parms, format);

	count = _vsntprintf (buffer, WRITE_LOG_BUF_SIZE - 1, format, parms);
	writeconsole (buffer);
	va_end (parms);
	if (!happened)
		gui_message (_T("JIT: Serious error:\n%s"), buffer);
	happened = 1;
	uae_reset (1);
}