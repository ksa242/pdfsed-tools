#include <errno.h>
#include <stdio.h>

#include "txt.h"


struct node *txt_read(FILE *f)
{
    return NULL;
}


int txt_write(FILE *f, const struct node *page)
{
    struct node *column = NULL, *para = NULL, *line = NULL, *word = NULL;

    column = (struct node *)page->content;
    while (column != NULL) {

        para = (struct node *)column->content;
        while (para != NULL) {

            line = (struct node *)para->content;
            while (line != NULL) {

                word = (struct node *)line->content;
                while (word != NULL) {
                    if (fputs((char *)word->content, f) == EOF) {
                        return -1;
                    }
                    if (word->sibling != NULL) {
                        if (fputc(' ', f) == EOF) {
                            return -1;
                        }
                    }
                    word = word->sibling;
                }
                if (fputc('\n', f) == EOF) {
                    return -1;
                }

                line = line->sibling;
            }

            if (para->sibling != NULL) {
                if (fputc('\n', f) == EOF) {
                    return -1;
                }
            }

            para = para->sibling;
        }

        if (column->sibling != NULL) {
            if (fputc('\n', f) == EOF) {
                return -1;
            }
        }

        column = column->sibling;
    }

    return 0;
}
