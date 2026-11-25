/* Convert.c

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

#include <string.h>
#include <strings.h>

#include <oslib/colourdbox.h>
#include <oslib/colourmenu.h>
#include <oslib/dcs.h>
#include <oslib/fileinfo.h>
#include <oslib/fontdbox.h>
#include <oslib/fontmenu.h>
#include <oslib/iconbar.h>
#include <oslib/menu.h>
#include <oslib/printdbox.h>
#include <oslib/proginfo.h>
#include <oslib/quit.h>
#include <oslib/saveas.h>
#include <oslib/scale.h>
#include <oslib/window.h>
#include <oslib/optionswindow.h>
#include <oslib/updatecheck.h>

#include "ccres.h"
#include "Convert.h"
#include "Error.h"
#include "Library.h"
#include "Utils.h"
#include "Template.h"
#include "Skeleton.h"

#include "_ColourDbox.h"
#include "_ColourMenu.h"
#include "_DCS.h"
#include "_FileInfo.h"
#include "_FontDbox.h"
#include "_FontMenu.h"
#include "_Iconbar.h"
#include "_Menu.h"
#include "_Object.h"
#include "_PrintDbox.h"
#include "_ProgInfo.h"
#include "_Quit.h"
#include "_SaveAs.h"
#include "_Scale.h"
#include "_Window.h"
#include "_OptionsWindow.h"
#include "_UpdateCheck.h"

static const CLASSES Classes[] = {
	{ColourDbox_ClassSWI, colourdbox_g2t, colourdbox_t2g, "colourdbox_object", sizeof(colourdbox_object), 100},
	{ColourMenu_ClassSWI, colourmenu_g2t, colourmenu_t2g, "colourmenu_object", sizeof(colourmenu_object), 100},
	{DCS_ClassSWI       , dcs_g2t       , dcs_t2g       , "dcs_object", sizeof(dcs_object), 100},
	{FileInfo_ClassSWI  , fileinfo_g2t  , fileinfo_t2g  , "fileinfo_object", sizeof(fileinfo_object), 100},
	{FontDbox_ClassSWI  , fontdbox_g2t  , fontdbox_t2g  , "fontdbox_object", sizeof(fontdbox_object), 100},
	{FontMenu_ClassSWI  , fontmenu_g2t  , fontmenu_t2g  , "fontmenu_object", sizeof(fontmenu_object), 100},
	{Iconbar_ClassSWI   , iconbar_g2t   , iconbar_t2g   , "iconbar_object", sizeof(iconbar_object), 100},
	{Menu_ClassSWI      , menu_g2t      , menu_t2g      , "menu_object", sizeof(menu_object_base), 102},
	{PrintDbox_ClassSWI , printdbox_g2t , printdbox_t2g , "printdbox_object", sizeof(printdbox_object), 100},
	{ProgInfo_ClassSWI  , proginfo_g2t  , proginfo_t2g  , "proginfo_object", sizeof(proginfo_object), 101},
	{Quit_ClassSWI      , quit_g2t      , quit_t2g      , "quit_object", sizeof(quit_object), 100},
	{SaveAs_ClassSWI    , saveas_g2t    , saveas_t2g    , "saveas_object", sizeof(saveas_object), 100},
	{Scale_ClassSWI     , scale_g2t     , scale_t2g     , "scale_object", sizeof(scale_object), 100},
	{Window_ClassSWI    , window_g2t    , window_t2g    , "window_object", sizeof(window_object_base), 102},
	{OptionsWindow_ClassSWI, optionswindow_g2t, optionswindow_t2g, "optionswindow_object", sizeof(optionswindow_object), 100},
	{UpdateCheck_ClassSWI  , updatecheck_g2t  , updatecheck_t2g  , "updatecheck_object", sizeof(updatecheck_object), 100}
};


static bool alloc_string_table(STRINGTABLE *pTable)
{
	int cb;

	pTable->ref = pTable->max = 0;
	cb = 2 * 256 * 256;		// 256 gadgets, 256 chars each, 2 strings per gadget
	if ((pTable->pstr = MyAlloc(cb)) == NULL) {
		return false;
	}
	pTable->max = cb;
	return true;
}


static bool alloc_reloc_table(RELOCTABLE *pTable)
{
	int nReloc;

	pTable->ref = pTable->max = 0;
	nReloc = 2 * 256;		// 256 gadgets, 2 strings per gadget
	if ((pTable->pReloc = MyAlloc(nReloc * sizeof(RELOC))) == NULL) {
		return false;
	}
	pTable->max = nReloc;
	return true;
}


static void free_string_table(STRINGTABLE *pTable)
{
	if (pTable->pstr != NULL) {
		MyFree(pTable->pstr);
	}
	pTable->pstr = NULL;
	pTable->ref = pTable->max = 0;
}


static void free_reloc_table(RELOCTABLE *pTable)
{
	if (pTable->pReloc != NULL) {
		MyFree(pTable->pReloc);
	}
	pTable->pReloc = NULL;
	pTable->ref = pTable->max = 0;
}


static bool text2res(DATA *data, char *pszOutFile)
{
	toolbox_resource_file_base Hdr;
	FILE * hf;
	char *pszIn, *pszOut, *pszEnd, *pszObject;
	int m;
	bool fHeader;

	pszIn = data->pszIn;
	pszEnd = data->pszIn + data->cbIn;
	if (memcmp(pszIn, "RESF:1.01", sizeof("RESF:1.01")-1) != 0) {
		error(data, "File is not RESF v1.01");
		return false;
	}
	// approx = window with 256 gadgets, 3 strings each
	if ((pszOut = MyAlloc(296 * 268 * 3)) == NULL
			|| !alloc_string_table(&data->StringTable)
			|| !alloc_string_table(&data->MessageTable)
			|| !alloc_reloc_table(&data->RelocTable)) {
		error(data, "Unable to allocate necessary memory");
		return false;
	} else if ((hf = fopen(pszOutFile, "wb")) == NULL) {
		error(data, "Unable to create output file '%s'", pszOutFile);
		return false;
	} else {
		data->fThrowback = false;
		Hdr.file_id = RESF;
		Hdr.version = 101;
		Hdr.header_size = -1;
		fHeader = false;
		while ((pszObject = next_object(&pszIn, pszEnd)) != NULL) {
			for (m = 0; m < ELEMENTS(Classes); m++) {
				if (strcasecmp(Classes[m].name, pszObject) == 0 && Classes[m].t2o != NULL) {
					if (!fHeader) {
						Hdr.header_size = sizeof(Hdr);
						fwrite(&Hdr, sizeof(Hdr), 1, hf);
						fHeader = true;
					}
					reset_string_table(&data->StringTable);
					reset_string_table(&data->MessageTable);
					reset_reloc_table(&data->RelocTable);
					object_text2resource(data, hf, pszIn, pszOut, &Classes[m]);
					goto text2res_added;
				}
			}
			error(data, "Don't know how to handle object '%s'", pszObject);

text2res_added:

			if ((pszIn = object_end(data, pszIn, pszEnd)) == NULL) {
				break;
			}
		}
		if (!fHeader) {
			fwrite(&Hdr, sizeof(Hdr), 1, hf);
		}
		fclose(hf);
#ifdef __riscos__
		osfile_set_type(pszOutFile, osfile_TYPE_RESOURCE);
#endif
		if (data->fThrowback)
		  {
		  report_end(data);
		  data->fThrowback = false;
		  }
		free_string_table(&data->StringTable);
		free_string_table(&data->MessageTable);
		free_reloc_table(&data->RelocTable);
		MyFree(pszOut);
	}
	return true;
}


static  bool res2text(DATA *data, char *pszOutFile)
//      ==========================================
{
int * relocation_table;
toolbox_resource_file_base * file_hdr;
FILE * hf;
int cb, m;
bool fConverted;

file_hdr = (toolbox_resource_file_base *) data->pszIn;
if (file_hdr->file_id != RESF)
  {
  error(data, "Invalid Resource File Header (%x)", file_hdr->file_id);
  return false;
  }
if (file_hdr->version != 101)
  {
  error(data, "Unknown Resource File Version (%x)", file_hdr->version);
  return false;
  }

if ((hf = fopen(pszOutFile, "wb")) == NULL)
  {
  error(data, "Unable to create output file '%s'", pszOutFile);
  return false;
  }

fConverted = false;
fprintf(hf, "RESF:%d.%02d\n", file_hdr->version / 100, file_hdr->version % 100);
if (file_hdr->header_size > 0)
  {
  toolbox_relocatable_object_base * obj, * end;
  obj = (toolbox_relocatable_object_base *) (file_hdr + 1);
  end = (toolbox_relocatable_object_base *) &data->pszIn[data->cbIn];
  do
    {
    fputs("\n", hf);
    for (m = 0; m < ELEMENTS(Classes); m++)
      {
      if (Classes[m].class_no == obj->rf_obj.class_no)
        {
        fprintf(hf, "%s {\n", Classes[m].name);
        object_resource2text(data, hf, obj, Classes[m].o2t);
        fputs("}\n", hf);
        break;
        }
      }
    if (m == ELEMENTS(Classes))
      error(data, "Unknown class '%s' (%x)", obj->rf_obj.name, obj->rf_obj.class_no);

    cb = sizeof(toolbox_relocatable_object_base) - sizeof(obj->rf_obj) + obj->rf_obj.size;
    if (obj->relocation_table_offset != -1)
      {
      relocation_table = (int *) (((char *) obj) + obj->relocation_table_offset);
      cb += sizeof(int) * (1 + 2 * relocation_table[0]);
      }
    obj = (toolbox_relocatable_object_base *) (((char *) obj) + cb);
    } while (cb != 0 && obj < end);
  }
fclose(hf);
#ifdef __riscos__
osfile_set_type(pszOutFile, osfile_TYPE_TEXT);
#endif
fConverted = true;

return fConverted;
}

// Returns the number of windows and the maximum number of icons used in
// those windows (not the total number of icons !).
static int window_count(const char *pszIn, const char *pszEnd, int *pi)
{
	int d, w, i, maxi;
	char ch, ch0;

	d = w = i = maxi = 0;
	ch0 = '\0';
	while(pszIn < pszEnd) {
		if ((ch = *pszIn++) == ':' || ch == '\0') {
			ch0 = ch;
		} else if (ch0 == '\0') {
			if (ch == '{') {
				if (d == 0) {
					w++;
					i = 0;
				} else {
					i++;
					if (i > maxi)
					  maxi = i;
				}
				d++;
			} else if (ch == '}') {
				d--;
			}
		}
	}
	*pi = maxi;
	return w;
}

static int icon_count(const char *pszIn, const char *pszEnd)
{
	int d, i;
	char ch, ch0;

	d = 1;
	i = 0;
	ch0 = '\0';
	while(pszIn < pszEnd && d > 0) {
		if ((ch = *pszIn++) == ':' || ch == '\0') {
			ch0 = ch;
		} else if (ch0 == '\0') {
			if (ch == '{') {
				i++;
				d++;
			} else if (ch == '}') {
				d--;
			}
		}
	}
	return i;
}


static const OBJECTLIST TemplateFontDataList[] = {
	{iol_BITS,    "x_point_size:", offsetof(template_font_data, x_point_size), NULL,  0},
	{iol_BITS,    "y_point_size:", offsetof(template_font_data, y_point_size), NULL,  0},
	{iol_CHARPTR, "font_name:",    offsetof(template_font_data, font_name),    NULL, 40}
};


static void get_template_fonts(DATA *data, FILE *hf, template_font_data *font_data, template_font_data *end)
{
	template_font_data temp;
	char *pszFrom;

	while (font_data < end) {
		pszFrom = (char *) font_data;			// bug in memcpy? without this it does a word-aligned copy
		memcpy(&temp, pszFrom, sizeof(temp));
		fputs("\ntemplate_font_data {\n", hf);
		get_objects(data, hf, NULL, NULL, (const char *) &temp, TemplateFontDataList, ELEMENTS(TemplateFontDataList), 1, sizeof(temp));
		fputs("}\n", hf);
		font_data++;
	}
}


static const OBJECTLIST TemplateHeaderList[] = {
	{iol_CHARPTR,  "template_name:",  offsetof(template_index, name), NULL, -12}
};

/*******************************************************************
 Function:      ensure_template_space
 Description:   Make sure that the Template file buffer is at least a
                given size, enlarging it if necessary. The buffer is
                zero filled when it is enlarged. Pointers into the old
                buffer are no longer valid if it is enlarged.
 Parameters:    ppszTemplate = pointer to the buffer pointer, updated
                               if the buffer is enlarged
                pcbTemplate = pointer to the buffer size, updated if
                              the buffer is enlarged
                cbNeeded = size required, in bytes
 Returns:       true if the buffer is large enough, false if the
                memory could not be allocated
 ******************************************************************/
static bool ensure_template_space(char **ppszTemplate, int *pcbTemplate, int cbNeeded)
{
	char *pszNew;
	int cb;

	if (cbNeeded <= *pcbTemplate) {
		return true;
	}
	cb = *pcbTemplate * 2;
	if (cb < cbNeeded) {
		cb = cbNeeded;
	}
	if ((pszNew = MyAlloc(cb)) == NULL) {
		return false;
	}
	memcpy(pszNew, *ppszTemplate, *pcbTemplate);
	MyFree(*ppszTemplate);
	*ppszTemplate = pszNew;
	*pcbTemplate = cb;
	return true;
}


static bool text2template(DATA *data, char *pszOutFile)
{
	template_font_data font_data;
	template_index * i;
	FILE * hf;
	char *pszIn, *pszTemplate, *pszEnd, *pszObject, *pszBuff;
	int cb, cbFixed, cbBuff, cbStrings, cbTemplate, nOut, nIndex, nWindows, nIcons;
	bool fConverted;

	pszIn = data->pszIn;
	pszEnd = data->pszIn + data->cbIn;
	fConverted = false;
	if ((nWindows = window_count(pszIn, pszEnd, &nIcons)) > 0) {
		/* Start with room for the header, index, index terminator and the
		   fixed part of each window; the buffer is enlarged as each
		   window's indirected data is added. */
		cbTemplate = sizeof(template_header) + sizeof(int) +
		             nWindows * (sizeof(template_index) + sizeof(wimp_window_base) + nIcons * sizeof(wimp_icon));
		if ((pszTemplate = MyAlloc(cbTemplate)) == NULL ||
			!alloc_string_table(&data->StringTable)) {      // this is only required so that we can share put_string with Res files
			error(data, "Unable to allocate necessary memory");
		} else {
			if ((hf = fopen(pszOutFile, "wb")) == NULL) {
				error(data, "Unable to create output file '%s'", pszOutFile);
			} else {
				data->fThrowback = false;
				((template_header *) pszTemplate)->font_offset = template_NO_FONTS;
				/* The buffer may move, so the index and output are tracked as
				   offsets. The index terminator is zero from the allocation. */
				nOut = sizeof(template_header) + nWindows * sizeof(template_index) + sizeof(int);
				nIndex = 0;
				pszBuff = NULL;
				cbBuff = 0;
				while ((pszObject = next_object(&pszIn, pszEnd)) != NULL && strcasecmp(pszObject, "wimp_window") == 0) {
					nIcons = icon_count(pszIn, pszEnd);
					cbFixed = sizeof(wimp_window_base) + nIcons * sizeof(wimp_icon);
					if (cbBuff < cbFixed) {
						if (pszBuff != NULL) {
							MyFree(pszBuff);
						}
						if ((pszBuff = MyAlloc(cbFixed)) == NULL) {
							error(data, "Unable to allocate %d bytes", cbFixed);
							break;
						}
						cbBuff = cbFixed;
					}
					cb = window_text2template(data, pszIn, -cbFixed, (wimp_window_base *) pszBuff);
					if (cb != cbFixed) {
						/* The indirected data offsets assumed cbFixed */
						error(data, "Window '%s' has %d bytes of window and icon blocks, but %d were expected", pszObject, cb, cbFixed);
						break;
					}
					cbStrings = data->StringTable.ref;
					if (!ensure_template_space(&pszTemplate, &cbTemplate, nOut + cbFixed + cbStrings)) {
						error(data, "Unable to allocate %d bytes", nOut + cbFixed + cbStrings);
						break;
					}
					memcpy(pszTemplate + nOut, pszBuff, cbFixed);
					memcpy(pszTemplate + nOut + cbFixed, data->StringTable.pstr, cbStrings);
					reset_string_table(&data->StringTable);
					i = (template_index *) (pszTemplate + sizeof(template_header)) + nIndex;
					i->size = cbFixed + cbStrings;
					i->offset = nOut;
					nOut += cbFixed + cbStrings;
					i->type = template_TYPE_WINDOW;
					put_objects(data, pszIn, 0, (char *) i, TemplateHeaderList, ELEMENTS(TemplateHeaderList));
// template name is different to icon name because it always needs a terminator, which restricts it to 11 chars;
// TemplateHeaderList gives it as a terminated field (-12), so put_pstr truncates it with a warning
					nIndex++;
					if ((pszIn = object_end(data, pszIn, pszEnd)) == NULL) {
						break;
					}
				}
				pszIn = data->pszIn;
				while ((pszObject = next_object(&pszIn, pszEnd)) != NULL) {
					if (strcasecmp(pszObject, "template_font_data") == 0) {
						if (!ensure_template_space(&pszTemplate, &cbTemplate, nOut + sizeof(template_font_data))) {
							error(data, "Unable to allocate %d bytes", nOut + (int) sizeof(template_font_data));
							break;
						}
						if (((template_header *) pszTemplate)->font_offset == template_NO_FONTS) {
							((template_header *) pszTemplate)->font_offset = (bits) nOut;
						}
// make sure we're writing to a word-aligned structure...
						memset(&font_data, 0, sizeof(font_data));
						put_objects(data, pszIn, 0, (char *) &font_data, TemplateFontDataList, ELEMENTS(TemplateFontDataList));
						memcpy(pszTemplate + nOut, &font_data, sizeof(font_data));
						nOut += sizeof(template_font_data);
					}
					if ((pszIn = object_end(data, pszIn, pszEnd)) == NULL) {
						break;
					}

				}
				fwrite(pszTemplate, nOut, 1, hf);
				fclose(hf);
#ifdef __riscos__
				osfile_set_type(pszOutFile, osfile_TYPE_TEMPLATE);
#endif
				if (data->fThrowback)
				  {
				  report_end(data);
				  data->fThrowback = false;
				  }
				if (pszBuff != NULL) {
					MyFree(pszBuff);
				}
				fConverted = true;
			}
			MyFree(pszTemplate);
			free_string_table(&data->StringTable);
		}
	}
	return fConverted;
}


static bool template2text(DATA *data, char *pszOutFile)
{
template_header * template_hdr;
template_index * obj;
char *pszBuff;
FILE * hf;
int cbBuff;

if ((hf = fopen(pszOutFile, "wb")) == NULL)
  {
  error(data, "Unable to create output file '%s'", pszOutFile);
  return false;
  }
fputs("Template:\n", hf);

pszBuff = NULL;
cbBuff = 0;
template_hdr = (template_header *) data->pszIn;
for (obj = (template_index *) (template_hdr + 1); obj->offset != 0; ++obj)
  {
  if (cbBuff < obj->size)
    {
    if (pszBuff != NULL)
      MyFree(pszBuff);
    if ((pszBuff = MyAlloc(obj->size)) == NULL)
      {
      error(data, "Unable to allocate %d bytes", obj->size);
      break;
      }
    }
  memcpy(pszBuff, data->pszIn + obj->offset, obj->size);
  fputs("\nwimp_window {\n", hf);
  get_objects(data, hf, NULL, NULL, (const char *) obj, TemplateHeaderList, ELEMENTS(TemplateHeaderList), 1, sizeof(template_index));
  window_template2text(data, hf, pszBuff);
  fputs("}\n", hf);
  }
if (template_hdr->font_offset != template_NO_FONTS)
  {
  template_index *end = (template_index *) &data->pszIn[data->cbIn];
  get_template_fonts(data, hf, (template_font_data *) (data->pszIn + template_hdr->font_offset), (template_font_data *) end);
  }
fclose(hf);
#ifdef __riscos__
osfile_set_type(pszOutFile, osfile_TYPE_TEXT);
#endif

if (pszBuff != NULL)
  MyFree(pszBuff);

return true;
}


        int ccres_initialise(DATA *sessionP)
//      ====================================
{
memset(sessionP, 0, sizeof(DATA));
sessionP->returnStatus = EXIT_SUCCESS;

return true;
}


        int ccres_finish(void)
//      ======================
{

return true;
}


        bool ccres_convert(DATA *data, char *pszOutFile)
//      ===============================================
{
switch (data->nFiletypeIn)
  {
  case osfile_TYPE_TEXT:
    switch (data->nFiletypeOut)
      {
      case osfile_TYPE_RESOURCE:
        return text2res(data, pszOutFile);
      case osfile_TYPE_TEMPLATE:
        return text2template(data, pszOutFile);
      }
    break;
  case osfile_TYPE_RESOURCE:
    return res2text(data, pszOutFile);
  case osfile_TYPE_TEMPLATE:
    return template2text(data, pszOutFile);
  }

return false;
}


        bool ccres_skeleton(DATA *data, bits nFiletype, char *pszOutFile)
//      ================================================================
{
char *pImage;
int cbImage;
bool fBuilt, fConverted;

if (nFiletype == osfile_TYPE_RESOURCE)
  fBuilt = skeleton_resource(Classes, ELEMENTS(Classes), &pImage, &cbImage);
else
  fBuilt = skeleton_template(&pImage, &cbImage);
if (!fBuilt)
  {
  error(data, "Unable to allocate memory for the skeleton");
  return false;
  }

/* Decode the image as though it had been loaded */
data->pszIn = pImage;
data->cbIn = cbImage;
data->nFiletypeIn = nFiletype;
data->nFiletypeOut = osfile_TYPE_TEXT;
fConverted = ccres_convert(data, pszOutFile);
data->pszIn = NULL;
data->cbIn = 0;
MyFree(pImage);
return fConverted;
}
