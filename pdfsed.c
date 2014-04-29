#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pdfsed.h"


int pdfsed_is_whitespace(int c)
{
    return (c == ' ' || c == '\t' || c == '\r' || c == '\n') ? 1 : 0;
}


int pdfsed_skip_whitespace(FILE *f)
{
    int c;
    while (1) {
        c = fgetc(f);
        if (!pdfsed_is_whitespace(c) || c == EOF) {
            break;
        }
    }
    return c;
}


enum pdfsed_atom pdfsed_read_atom(FILE *f)
{
    static char atom_buf[32];
    size_t atom_buf_pos = 0;
    memset(atom_buf, 0x00, 32);

    enum pdfsed_atom atom = PDFSED_ATOM_UNKNOWN;

    int c = pdfsed_skip_whitespace(f);
    if (c == EOF) {
        /* End of file reached, treat it as end-of-command. */
        return PDFSED_ATOM_EOC;
    } else if (c == ';') {
        /* End of command; semi-colon is an atom on its own. */
        return PDFSED_ATOM_EOC;
    }

    while (!pdfsed_is_whitespace(c)) {
        if (c == ';') {
            ungetc(c, f);
            break;

        } else {
            assert(atom_buf_pos < 31);
            atom_buf[atom_buf_pos] = c;
            atom_buf_pos++;
        }

        assert((c = fgetc(f)) != EOF);
    }

    if (atom == PDFSED_ATOM_UNKNOWN) {
        if (!strcmp(atom_buf, "set")) {
            atom = PDFSED_ATOM_CMD_SET;
        } else if (!strcmp(atom_buf, "create")) {
            atom = PDFSED_ATOM_CMD_CREATE;
        } else if (!strcmp(atom_buf, "draw")) {
            atom = PDFSED_ATOM_CMD_DRAW;

        } else if (!strcmp(atom_buf, "title")) {
            atom = PDFSED_ATOM_OBJ_TITLE;
        } else if (!strcmp(atom_buf, "author")) {
            atom = PDFSED_ATOM_OBJ_AUTHOR;
        } else if (!strcmp(atom_buf, "creator")) {
            atom = PDFSED_ATOM_OBJ_AUTHOR;
        } else if (!strcmp(atom_buf, "page")) {
            atom = PDFSED_ATOM_OBJ_PAGE;
        } else if (!strcmp(atom_buf, "text")) {
            atom = PDFSED_ATOM_OBJ_TEXT;
        } else if (!strcmp(atom_buf, "image")) {
            atom = PDFSED_ATOM_OBJ_IMAGE;

        } else if (!strcmp(atom_buf, "size")) {
            atom = PDFSED_ATOM_PROP_SIZE;
        } else if (!strcmp(atom_buf, "angle")) {
            atom = PDFSED_ATOM_PROP_ANGLE;
        } else if (!strcmp(atom_buf, "pos")) {
            atom = PDFSED_ATOM_PROP_POS;
        } else if (!strcmp(atom_buf, "dpi")) {
            atom = PDFSED_ATOM_PROP_DPI;
        } else if (!strcmp(atom_buf, "scale")) {
            atom = PDFSED_ATOM_PROP_SCALE;
        } else if (!strcmp(atom_buf, "mask")) {
            atom = PDFSED_ATOM_PROP_MASK;
        } else if (!strcmp(atom_buf, "mask-image")) {
            atom = PDFSED_ATOM_PROP_MASK_IMAGE;

        } else {
            fprintf(stderr, "Unknown pdfsed atom: %s.\n", atom_buf);
        }
    }

    return atom;
}


char *pdfsed_read_str(FILE *f)
{
    char *val = NULL;
    size_t val_len = 0, val_pos = 0;

    int c = pdfsed_skip_whitespace(f);
    assert(c == '"');

    while (1) {
        assert((c = fgetc(f)) != EOF);

        if (c == '"') {
            /* End of string. */
            break;

        } else if (c == '\\') {
            /* Read and use the escaped character. */
            assert((c = fgetc(f)) != EOF);
            if (c == 't') {
                c = '\t';
            } else if (c == 'r') {
                c = '\r';
            } else if (c == 'n') {
                c = '\n';
            }
        }

        if (val_pos <= val_len) {
            val_len = val_pos + 32;
            val = realloc(val, val_len * sizeof(char));
            assert(val != NULL);
            memset(&(val[val_pos]), 0x00, (val_len - val_pos) * sizeof(char));
        }

        val[val_pos] = c;
        val_pos++;
    }

    if (val != NULL) {
        val = realloc(val, (val_pos + 1) * sizeof(char));
        assert(val != NULL);
    }

    return val;
}


float pdfsed_read_float(FILE *f)
{
    float val;
    ungetc(pdfsed_skip_whitespace(f), f);
    assert(fscanf(f, "%f", &val) == 1);
    return val;
}


int pdfsed_read_int(FILE *f)
{
    int val;
    ungetc(pdfsed_skip_whitespace(f), f);
    assert(fscanf(f, "%d", &val) == 1);
    return val;
}


unsigned long pdfsed_read_color(FILE *f)
{
    unsigned long val;
    assert(pdfsed_skip_whitespace(f) == '0');
    assert(fgetc(f) == 'x');
    assert(fscanf(f, "%lx", &val) == 1);
    return val;
}
