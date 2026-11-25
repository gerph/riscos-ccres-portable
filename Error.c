/* Error.c

   Copyright (c) 2003-2003 Dave Appleby
   Copyright (c) 2003-2007 John Tytgat

   This file is part of CCres.

   CCres is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   CCres is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with CCres; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdio.h>
#include <string.h>

#include <oslib/ddeutils.h>

#include "ccres.h"
#include "Error.h"

/* Prefix for messages from report_warning() */
#define WARNING_PREFIX "Warning: "

static void report_line(DATA *data, const char *ptrP, const char *pszPrefix, char *pszFmt, va_list list)
{
	const char *pP;
	int nRow, cb;
	char achError[1024];

	/* vsprintf, as vsnprintf may not be in the RISC OS C library stubs;
	   callers limit the length of any input text they include. */
	cb = sprintf(achError, "%s", pszPrefix);
	vsprintf(achError + cb, pszFmt, list);
	for (nRow = 1, pP = data->pszIn; pP < ptrP; pP++) {
		if (*pP < ' ') {
			nRow++;
		}
	}
	if (!data->fThrowback) {
#ifdef __riscos__
//FIXME:		ddeutils_throwback_start();
#endif
		data->fThrowback = true;
	}
#ifdef __riscos__
//FIXME:	ddeutils_throwback_send(ddeutils_THROWBACK_INFO_DETAILS, data->achTextFile, nRow, ddeutils_SEVERITY_ERROR, achError);
	fprintf(stderr, "Line %d: %s\n", nRow, achError);
#else
	fprintf(stderr, "Line %d: %s\n", nRow, achError);
#endif
}


void report(DATA *data, const char *ptrP, char *pszFmt, ...)
{
	va_list list;

	va_start(list, pszFmt);
	report_line(data, ptrP, "", pszFmt, list);
	va_end(list);
	data->returnStatus = EXIT_FAILURE;
}


void report_warning(DATA *data, const char *ptrP, char *pszFmt, ...)
{
	va_list list;

	va_start(list, pszFmt);
	report_line(data, ptrP, WARNING_PREFIX, pszFmt, list);
	va_end(list);
}


void report_end(DATA *data)
{
#ifdef __riscos__
//FIXME:ddeutils_throwback_end();
#endif
}


void error(DATA *sessionP, char *pszFmt, ...)
{
//FIXME:#ifdef __riscos__
#if 0
os_error err;
va_list list;
err.errnum = 0;
va_start(list, pszFmt);
vsprintf(err.errmess, pszFmt, list);
va_end(list);
wimp_report_error(&err, wimp_ERROR_BOX_OK_ICON | wimp_ERROR_BOX_NO_PROMPT, APPNAME);
#else
va_list list;
va_start(list, pszFmt);
vfprintf(stderr, pszFmt, list);
va_end(list);
fputc('\n', stderr);
#endif
sessionP->returnStatus = EXIT_FAILURE;
}
