#ifndef _PDFSED_PDFSED_H_
#define _PDFSED_PDFSED_H_

#include <stdio.h>


enum pdfsed_atom
{
    PDFSED_ATOM_UNKNOWN,
    PDFSED_ATOM_EOC,

    PDFSED_ATOM_CMD_SET,
    PDFSED_ATOM_CMD_CREATE,
    PDFSED_ATOM_CMD_DRAW,

    PDFSED_ATOM_OBJ_TITLE,
    PDFSED_ATOM_OBJ_AUTHOR,
    PDFSED_ATOM_OBJ_CREATOR,
    PDFSED_ATOM_OBJ_PAGE,
    PDFSED_ATOM_OBJ_TEXT,
    PDFSED_ATOM_OBJ_IMAGE,

    PDFSED_ATOM_PROP_SIZE,
    PDFSED_ATOM_PROP_ANGLE,
    PDFSED_ATOM_PROP_POS,
    PDFSED_ATOM_PROP_DPI,
    PDFSED_ATOM_PROP_SCALE,
    PDFSED_ATOM_PROP_MASK,
    PDFSED_ATOM_PROP_MASK_IMAGE
};


enum pdfsed_atom pdfsed_read_atom(FILE *f);
char *pdfsed_read_str(FILE *f);
float pdfsed_read_float(FILE *f);
int pdfsed_read_int(FILE *f);
unsigned long pdfsed_read_color(FILE *f);

#endif  /* _PDFSED_PDFSED_H_ */
