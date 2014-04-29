#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "djvused.h"


int djvused_is_whitespace(int c)
{
    return (c == ' ' || c == '\t' || c == '\n' || c == '\r') ? 1 : 0;
}


char *djvused_read_token(FILE *f)
{
    int c;

    while (1) {
        c = fgetc(f);

        if (c == EOF) {
            perror("djvused_read_token(): fgetc()");
            return NULL;

        } else if (c == '(' || c == ')') {
            char *token = malloc(2 * sizeof(char));
            if (token == NULL) {
                perror("djvused_read_token(): malloc()");
            } else {
                token[0] = c;
                token[1] = '\0';
            }
            return token;

        } else if (!djvused_is_whitespace(c)) {
            break;
        }
    }

    char *token = NULL;
    size_t token_len = 0, token_idx = 0;

    if (c == '"') {
        while (1) {
            if (token_idx >= token_len) {
                token_len = token_idx + 32 * sizeof(char);
                if ((token = realloc(token, token_len)) == NULL) {
                    perror("djvused_read_token(): realloc()");
                    return NULL;
                }
            }

            c = fgetc(f);

            if (c == EOF) {
                perror("djvused_read_token(): fgetc()");
                free(token);
                return NULL;

            } else if (c == '"') {
                break;

            } else if (c == '\\') {
                c = fgetc(f);
                if (c >= '0' && c <= '9') {
                    c = ((c - 48) << 6) +
                        ((fgetc(f) - 48) << 3) +
                        (fgetc(f) - 48);
                } else if (c == 't') {
                    c = '\t';
                } else if (c == 'n') {
                    c = '\n';
                } else if (c == 'r') {
                    c = '\r';
                }
            }

            token[token_idx] = c;
            token[token_idx + 1] = '\0';
            token_idx++;
        }

    } else {
        while (1) {
            if (token_idx >= token_len) {
                token_len = token_idx + 8 * sizeof(char);
                if ((token = realloc(token, token_len)) == NULL) {
                    perror("djvused_read_token(): realloc()");
                    return NULL;
                }
            }

            token[token_idx] = c;
            token[token_idx + 1] = '\0';
            token_idx++;

            c = fgetc(f);
            if (c == EOF) {
                perror("djvused_read_token(): fgetc()");
                free(token);
                return NULL;

            } else if (djvused_is_whitespace(c)) {
                break;

            } else if (c == ')') {
                ungetc(c, f);
                break;
            }
        }
    }

    return token;
}


struct node *djvused_read(FILE *f)
{
    size_t v;

    struct node *node = tree_new_node(NULL);
    assert(node != NULL);

    char *token = NULL;
    while (1) {
        free(token);

        token = djvused_read_token(f);
        if (token == NULL) {
            tree_free_node(node);
            node = NULL;
            return NULL;
        } else if (!strcmp(token, ")")) {
            tree_free_node(node);
            node = NULL;
            return NULL;
        } else if (!strcmp(token, "(")) {
            break;
        }
    }

    char *node_type = djvused_read_token(f);
    if (node_type == NULL) {
        tree_free_node(node);
        node = NULL;
        return NULL;
    }

    token = djvused_read_token(f);
    assert(sscanf(token, "%u", &v) == 1);
    node->bbox.x1 = (float)v;
    free(token);

    token = djvused_read_token(f);
    assert(sscanf(token, "%u", &v) == 1);
    node->bbox.y1 = (float)v;
    free(token);

    token = djvused_read_token(f);
    assert(sscanf(token, "%u", &v) == 1);
    node->bbox.x2 = (float)v;
    free(token);

    token = djvused_read_token(f);
    assert(sscanf(token, "%u", &v) == 1);
    node->bbox.y2 = (float)v;
    free(token);

    if (!strcmp(node_type, "word")) {
        node->is_leaf = 1;
        node->content = (void *)djvused_read_token(f);
        token = djvused_read_token(f);
        if (strcmp(token, ")")) {
            free(node->content);
            node->content = NULL;
            tree_free_node(node);
            node = NULL;
        }
        free(token);
    } else {
        struct node *child = NULL;
        while (1) {
            if (child == NULL) {
                child = djvused_read(f);
                node->content = child;
            } else {
                child->sibling = djvused_read(f);
                child = child->sibling;
            }
            if (child == NULL) {
                break;
            } else {
                child->parent = node;
            }
        }
    }

    free(node_type);

    return node;
}


int djvused_write_str(FILE *f, const char *buf)
{
    fputc('"', f);
    for (size_t l = strlen(buf), i = 0; i < l; i++) {
        if (buf[i] == '\\' || buf[i] == '"') {
            fputc('\\', f);
            fputc(buf[i], f);
        } else if (buf[i] == '\t') {
            fputs("\\t", f);
        } else if (buf[i] == '\n') {
            fputs("\\n", f);
        } else if (buf[i] == '\r') {
            fputs("\\r", f);
        } else if (buf[i] < 0x20) {
            fprintf(f, "\\%03o", buf[i]);
        } else {
            fputc(buf[i], f);
        }
    }
    fputc('"', f);
    return 0;
}


int djvused_write_bbox(FILE *f, struct bbox bbox)
{
    return fprintf(f, "%d %d %d %d", (int)bbox.x1,
                                     (int)bbox.y1,
                                     (int)bbox.x2,
                                     (int)bbox.y2);
}


int djvused_write(FILE *f, const struct node *page)
{
    struct node *column, *para, *line, *word;

    fputs("(page ", f);
    djvused_write_bbox(f, page->bbox);
    fputc('\n', f);

    column = (struct node *)page->content;
    while (column != NULL) {
        fputs("  (column ", f);
        djvused_write_bbox(f, column->bbox);
        fputc('\n', f);

        para = (struct node *)column->content;
        while (para != NULL) {
            fputs("    (para ", f);
            djvused_write_bbox(f, para->bbox);
            fputc('\n', f);

            line = (struct node *)para->content;
            while (line != NULL) {
                fputs("      (line ", f);
                djvused_write_bbox(f, line->bbox);
                fputc('\n', f);

                word = (struct node *)line->content;
                while (word != NULL) {
                    fputs("        (word ", f);
                    djvused_write_bbox(f, word->bbox);
                    fputc('\n', f);

                    fputs("          ", f);
                    djvused_write_str(f, (char *)word->content);
                    fputc(')', f);

                    if (word->sibling != NULL) {
                        fputc('\n', f);
                    }
                    word = word->sibling;
                }

                fputc(')', f);
                if (line->sibling != NULL) {
                    fputc('\n', f);
                }
                line = line->sibling;
            }

            fputc(')', f);
            if (para->sibling != NULL) {
                fputc('\n', f);
            }
            para = para->sibling;
        }

        fputc(')', f);
        if (column->sibling != NULL) {
            fputc('\n', f);
        }
        column = column->sibling;
    }

    fputs(")\n", f);

    return 0;
}
