/* CMD.c

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

/* Standalone version of ccres */

/* Std C headers :
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ccres.h"
#include "Convert.h"
#include "Error.h"
#include "Utils.h"

/* Options which write a skeleton, plain or annotated, instead of converting */
#define OPTION_SKELETON "--skeleton"
#define OPTION_DESCRIBE "--describe"

/* Kinds of skeleton */
#define SKELETON_KIND_RESOURCE "resource"
#define SKELETON_KIND_TEMPLATE "template"


/*******************************************************************
 Function:      skeleton_command
 Description:   Handle the --skeleton and --describe options.
 Parameters:    data = initialised session
                pszOption = the option given
                pszKind = "resource" or "template"
                pszOutFile = text file to write
 Returns:       none (the status is left in data->returnStatus)
 ******************************************************************/
static void skeleton_command(DATA *data, const char *pszOption, const char *pszKind, char *pszOutFile)
{
bits nFiletype;

if (strcmp(pszKind, SKELETON_KIND_RESOURCE) == 0)
  nFiletype = osfile_TYPE_RESOURCE;
else if (strcmp(pszKind, SKELETON_KIND_TEMPLATE) == 0)
  nFiletype = osfile_TYPE_TEMPLATE;
else
  {
  error(data, "Unknown kind of skeleton '%s' for %s: use '" SKELETON_KIND_RESOURCE "' or '" SKELETON_KIND_TEMPLATE "'", pszKind, pszOption);
  return;
  }
data->fDescribe = (strcmp(pszOption, OPTION_DESCRIBE) == 0);
(void) ccres_skeleton(data, nFiletype, pszOutFile);
}


        int main(int argc, char *argv[])
//      ================================
{
DATA data;

if (argc == 4 && (strcmp(argv[1], OPTION_SKELETON) == 0 || strcmp(argv[1], OPTION_DESCRIBE) == 0))
  {
  if (!ccres_initialise(&data))
    return EXIT_FAILURE;
  skeleton_command(&data, argv[1], argv[2], argv[3]);
  (void)ccres_finish();
  return data.returnStatus;
  }

if (argc != 3)
  {
  fprintf(stderr, "CCres " VERSION "\n"
                  "Convertor between RISC OS Toolbox Resource (filetype &FAE) & Wimp Template (filetype &FEC) files to and from text format.\n"
                  "Syntax: CCres <infile> <outfile>\n"
                  "  <infile>  : either Template, Resource or Text file\n"
                  "  <outfile> : output Text file (case <infile> is a Template or Resource file) or output Template/Resource file (case <infile> is a Text file)\n"
                  "Syntax: CCres " OPTION_SKELETON " " SKELETON_KIND_RESOURCE "|" SKELETON_KIND_TEMPLATE " <outfile>\n"
                  "  Writes a Text file holding every kind of object and gadget, or window and icon, with every field\n"
                  "Syntax: CCres " OPTION_DESCRIBE " " SKELETON_KIND_RESOURCE "|" SKELETON_KIND_TEMPLATE " <outfile>\n"
                  "  As " OPTION_SKELETON ", with '#@' comments describing each field's type and accepted values\n");
  return EXIT_FAILURE;
  }

if (!ccres_initialise(&data))
  return EXIT_FAILURE;

bits nFileType = get_filetype(&data, argv[1]);

switch (nFileType)
  {
  case osfile_TYPE_TEXT:
  case osfile_TYPE_RESOURCE:
  case osfile_TYPE_TEMPLATE:
    if (load_file(&data, argv[1], nFileType))
      ccres_convert(&data, argv[2]);
    break;
  default:
    error(&data, "Unsupported input filetype\n");
    break;
  }

if (data.pszIn != NULL)
  MyFree(data.pszIn);

(void)ccres_finish();

return data.returnStatus;
}
