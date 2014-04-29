#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"

#define MAX(X, Y) ((X > Y) ? X : Y)
#define MIN(X, Y) ((X < Y) ? X : Y)


struct node *tree_new_node(struct node *parent)
{
    struct node *node = (struct node *)malloc(sizeof(struct node));
    if (node == NULL) {
        perror("tree_new_node()");
    } else {
        node->bbox.x1 = 0;
        node->bbox.y1 = 0;
        node->bbox.x2 = 0;
        node->bbox.y2 = 0;
        node->parent = parent;
        node->sibling = NULL;
        node->is_leaf = 0;
        node->content = NULL;
    }
    return node;
}


void tree_free_node(struct node *node)
{
    if (node != NULL) {
        if (node->content != NULL) {
            if (node->is_leaf == 1) {
                free(node->content);
            } else {
                struct node *child = node->content, *next_child = NULL;
                while (child != NULL) {
                    next_child = child->sibling;
                    tree_free_node(child);
                    child = next_child;
                }
                child = NULL;
                next_child = NULL;
            }
            node->content = NULL;
        }
        node->sibling = NULL;
        node->parent = NULL;
        free(node);
        node = NULL;
    }
}


struct bbox wrap_bbox(struct bbox outer, struct bbox inner)
{
    outer.x1 = MIN(outer.x1, inner.x1);
    outer.y1 = MIN(outer.y1, inner.y1);
    outer.x2 = MAX(outer.x2, inner.x2);
    outer.y2 = MAX(outer.y2, inner.y2);
    return outer;
}


/* Rewrite recursively using is_leaf. */
void recalculate_bbox(struct node *page, float scale)
{
    size_t page_width = page->bbox.x2,
           page_height = page->bbox.y2;

    struct node *column, *para, *line, *word;

    column = page->content;
    while (column != NULL) {
        column->bbox.x1 = page_width;
        column->bbox.y1 = page_height;
        column->bbox.x2 = 0;
        column->bbox.y2 = 0;

        para = column->content;
        while (para != NULL) {
            para->bbox.x1 = page_width;
            para->bbox.y1 = page_height;
            para->bbox.x2 = 0;
            para->bbox.y2 = 0;

            line = para->content;
            while (line != NULL) {
                line->bbox.x1 = page_width;
                line->bbox.y1 = page_height;
                line->bbox.x2 = 0;
                line->bbox.y2 = 0;

                word = line->content;
                while (word != NULL) {
                    word->bbox.x1 = round(word->bbox.x1 * scale);
                    word->bbox.y1 = round(word->bbox.y1 * scale);
                    word->bbox.x2 = round(word->bbox.x2 * scale);
                    word->bbox.y2 = round(word->bbox.y2 * scale);

                    line->bbox = wrap_bbox(line->bbox, word->bbox);

                    word = word->sibling;
                }

                para->bbox = wrap_bbox(para->bbox, line->bbox);

                line = line->sibling;
            }

            column->bbox = wrap_bbox(column->bbox, para->bbox);

            para = para->sibling;
        }

        column = column->sibling;
    }
}
