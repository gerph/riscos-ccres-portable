/* Error.h

   Copyright (c) 2004-2007 John Tytgat

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

#ifndef ERROR_HEADER_INCLUDED
#define ERROR_HEADER_INCLUDED

#include "ccres.h"

/*******************************************************************
 Function:      report
 Description:   Report an error in the input text, with the line
                number it is on. The conversion will exit with a
                failure status.
 Parameters:    data = conversion session
                ptrP = position in the input text of the error
                pszFmt = printf format of the message, and its values
 Returns:       none
 ******************************************************************/
void report(DATA *data, const char *ptrP, char *pszFmt, ...);

/*******************************************************************
 Function:      report_warning
 Description:   Report a problem in the input text that the conversion
                has worked around, with the line number it is on and
                'Warning: ' before the message. The exit status is not
                changed.
 Parameters:    data = conversion session
                ptrP = position in the input text of the problem
                pszFmt = printf format of the message, and its values
 Returns:       none
 ******************************************************************/
void report_warning(DATA *data, const char *ptrP, char *pszFmt, ...);

void report_end(DATA *data);
void error(DATA *data, char *pszFmt, ...);

#endif
