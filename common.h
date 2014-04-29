#ifndef _PDFSED_COMMON_H_
#define _PDFSED_COMMON_H_

enum pdfsed_text_fmt
{
    PDFSED_FMT_UNKNOWN,
    PDFSED_FMT_HOCR,
    PDFSED_FMT_DJVUSED,
    PDFSED_FMT_TXT
};


struct bbox
{
    float x1, y1, x2, y2;
};


struct node
{
    struct bbox bbox;
    struct node *parent;
    struct node *sibling;
    int is_leaf;
    void *content;
};


struct node *tree_new_node(struct node *parent);
void tree_free_node(struct node *node);

void recalculate_bbox(struct node *page, float scale);

#endif  /* _PDFSED_COMMON_H_ */
