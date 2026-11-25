# Issues to report upstream

Problems found in other projects while working on CCres, to be reported to
them. Remove an entry once it has been reported, with a link to the report.

## Pyromaniac: Wimp_LoadTemplate turns a validation string of -1 into an empty string

**Component:** `riscos/pymods/core/wimp/templates.py`, `fixup_icon()`.

### Summary

In an indirected text icon in a Templates file, a validation string pointer of
-1 means that the icon has no validation string. Pyromaniac's
`Wimp_LoadTemplate` treats only 0 as "none". It takes -1 as an offset into the
file, reads an empty string from it, copies that into the indirected
workspace, and gives the loaded icon a pointer to it. The icon should keep -1.

The Programmer's Reference Manual describes the indirected data of a window's
title bar as "a pointer to a buffer containing the text, a pointer to a
validation string (-1 if none), and the length of the buffer" (Wimp chapter,
Wimp_CreateWindow). An indirected icon's data has the same layout.

Template editors write -1. In the six Templates files of the CCres test set
which were made by other tools (`TestQueue/Tem01`, from AntiWord, DeskEdit,
OvationPro and others), 220 indirected text icons have no validation string,
and every one is -1. None uses 0.

### Why it happens

`fixup_icon()` reads the validation string when the pointer is not 0:

```python
if iconflags & IconConstants.Wimp_IconText:
    if validation_offset != 0:
        self.seek(params['template_fileptr'] + validation_offset)
        validation = self.read_string()
```

The word is unpacked as unsigned (`'<LLL'`), so -1 is &FFFFFFFF. `seek()` limits
the position to the end of the file, so `read_string()` returns `b''` rather
than failing. Since `validation` is then not `None`, an empty string is placed
in the workspace and its address replaces the -1.

The plotting code already treats both values as "none"
(`ploticon.py`: `if validation_pointer in (0, 0xFFFFFFFF)`), so icons still look
right. What differs from the documented behaviour is:

* a program which reads the icon block after loading it sees a pointer to an
  empty string, not -1;
* a byte of the indirected workspace is used for each such icon, so the
  workspace size returned when `Wimp_LoadTemplate` is called with R1 = 0 is
  larger than needed.

### Reproduction

Use `TestQueue/Tem01/OvationPro,fec` from CCres, copied as `OvPro`. In the
`RenameStyle` template, icon 0 is indirected text with a validation pointer of
-1 in the file (window at file offset &106E0; the icon's validation word is at
offset 88 + 24 into the window).

```basic
DIM buf% 16384, ws% 16384, name% 12
SYS "Wimp_OpenTemplate",,"OvPro"
$name%="RenameStyle"
SYS "Wimp_LoadTemplate",,buf%,ws%,ws%+16384,-1,name%,0 TO ,,end%,,,,pos%
SYS "Wimp_CloseTemplate"
PRINT "found ";pos%<>0;" workspace used ";end%-ws%
i%=buf%+88
PRINT "icon 0 flags &";~i%!16;" text &";~i%!20;" validation &";~i%!24;" size ";i%!28
IF i%!24<>-1 THEN PRINT "validation string length ";LEN($(i%!24))
```

Run under Pyromaniac with `riscos-run`, this printed:

```
found -1 workspace used 103
icon 0 flags &C7006311 text &D0CF validation &D0D9 size 9
validation string length 0
```

The validation word should be -1, as the file has it. The same check has not
been run on RISC OS Classic.

### Suggested fix

Treat &FFFFFFFF as "none", as `ploticon.py` does, and leave the word unchanged:

```python
if iconflags & IconConstants.Wimp_IconText:
    if validation_offset not in (0, 0xFFFFFFFF):
        self.seek(params['template_fileptr'] + validation_offset)
        validation = self.read_string()
```

When `validation` stays `None`, `validation_offset` keeps its value from the file
and is written back to the icon as it was. A test would load a template with an
icon like the one above and check that the validation word is -1 and that the
workspace used does not include it.

### Found by

The CCres converter used to write an empty `text.validation:""` as a pointer to
a string holding only a terminator. CCres 1.16 writes -1, as template editors do.
That is why this case now matters for files made with CCres, and how it was
noticed.
