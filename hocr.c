#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libxml/HTMLparser.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>

#include "common.h"
#include "hocr.h"


struct bbox hocr_parse_bbox(const xmlChar *buf)
{
    size_t x1, y1, x2, y2;
    struct bbox bbox = { 0.0, 0.0, 0.0, 0.0 };
    char *src = strstr((char *)buf, "bbox ");
    if (src == NULL) {
        fprintf(stderr, "hOCR: bbox coordinates not found: %s.\n", buf);
    } else if (sscanf(src, "bbox %u %u %u %u", &x1, &y1, &x2, &y2) != 4) {
        fprintf(stderr, "hOCR: couldn't parse out bbox coordinates: %s.\n", src);
    } else {
        bbox.x1 = x1;
        bbox.x2 = x2;
        bbox.y1 = y1;
        bbox.y2 = y2;
    }
    return bbox;
}


struct node *hocr_get_children(struct node *parent,
                               const xmlNodePtr xpath_root,
                               const xmlXPathContextPtr xpath_ctx,
                               const xmlChar *xpath_expr)
{
    xmlXPathObjectPtr children =
        xmlXPathNodeEval(xpath_root, xpath_expr, xpath_ctx);
    assert(children != NULL);

    if (children->nodesetval->nodeNr == 0) {
        xmlXPathFreeObject(children);
        return NULL;
    }

    struct node *first_child = NULL, *child = NULL;
    xmlNodePtr child_node = NULL;
    char *child_node_class = NULL;

    for (size_t i = 0; i < children->nodesetval->nodeNr; i++) {
        child_node = children->nodesetval->nodeTab[i];
        child_node_class = (char *)xmlGetProp(child_node, BAD_CAST "class");

        if (child != NULL) {
            child->sibling = tree_new_node(parent);
            child = child->sibling;
        } else {
            child = tree_new_node(parent);
        }
        assert(child != NULL);

        if (first_child == NULL) {
            first_child = child;
        }

        child->bbox = hocr_parse_bbox(xmlGetProp(child_node, BAD_CAST "title"));
        if (!strcmp(child_node_class, "ocr_page")) {
            child->content = hocr_get_children(child, child_node, xpath_ctx,
                                               BAD_CAST HOCR_XPATH_COLUMN);
        } else if (!strcmp(child_node_class, "ocr_carea")) {
            child->content = hocr_get_children(child, child_node, xpath_ctx,
                                               BAD_CAST HOCR_XPATH_PARA);
        } else if (!strcmp(child_node_class, "ocr_par")) {
            child->content = hocr_get_children(child, child_node, xpath_ctx,
                                               BAD_CAST HOCR_XPATH_LINE);
        } else if (!strcmp(child_node_class, "ocr_line")) {
            child->content = hocr_get_children(child, child_node, xpath_ctx,
                                               BAD_CAST HOCR_XPATH_WORD);
        } else {
            child->content = (char *)xmlNodeGetContent(child_node);
        }
    }

    xmlXPathFreeObject(children);
    return first_child;
}


void hocr_flip_bbox(struct node *node, float page_height)
{
    float y1 = page_height - node->bbox.y2;
    node->bbox.y2 = page_height - node->bbox.y1;
    node->bbox.y1 = y1;
}


struct node *hocr_read(FILE *f)
{
    htmlParserCtxtPtr doc_ctx = htmlNewParserCtxt();
    assert(doc_ctx != NULL);

    htmlDocPtr doc =
        htmlCtxtReadFd(doc_ctx, fileno(f), NULL, NULL,
                       HTML_PARSE_NONET | HTML_PARSE_NOBLANKS);
    if (doc == NULL) {
        fprintf(stderr, "hOCR: couldn't read the document.\n");
        htmlFreeParserCtxt(doc_ctx);
        return NULL;
    }

    xmlNodePtr root = xmlDocGetRootElement(doc);
    assert(root != NULL);

    xmlXPathContextPtr xpath_ctx = xmlXPathNewContext(doc); 
    assert(xpath_ctx != NULL);

    struct node *page = hocr_get_children(NULL, root, xpath_ctx,
                                          BAD_CAST HOCR_XPATH_PAGE);

    float page_height = page->bbox.y2;
    struct node *column, *para, *line, *word;
    column = page->content;
    while (column != NULL) {
        hocr_flip_bbox(column, page_height);
        para = column->content;
        while (para != NULL) {
            hocr_flip_bbox(para, page_height);
            line = para->content;
            while (line != NULL) {
                hocr_flip_bbox(line, page_height);
                word = line->content;
                while (word != NULL) {
                    hocr_flip_bbox(word, page_height);
                    word = word->sibling;
                }
                line = line->sibling;
            }
            para = para->sibling;
        }
        column = column->sibling;
    }

    xmlXPathFreeContext(xpath_ctx);
    xmlFreeDoc(doc);
    xmlCleanupParser();

    htmlFreeParserCtxt(doc_ctx);
    return page;
}


char *hocr_render_bbox(struct bbox bbox, float page_height)
{
    static char s[64];
    memset(s, 0x00, 64);
    snprintf(s, 64, "bbox %u %u %u %u", (int)bbox.x1,
                                        (int)(page_height - bbox.y2),
                                        (int)bbox.x2,
                                        (int)(page_height - bbox.y1));
    return s;
}


int hocr_write(FILE *f, const struct node *page)
{
    int result = -1;

    struct node *column, *para, *line, *word;

    xmlDocPtr doc = xmlNewDoc(BAD_CAST "1.0");
    if (doc == NULL) {
        fprintf(stderr, "hOCR: couldn't create a document.\n");
        return result;
    }

    xmlNodePtr root = xmlNewNode(NULL, BAD_CAST "html");
    xmlDocSetRootElement(doc, root);

    xmlNodePtr head = xmlNewChild(root, NULL, BAD_CAST "head", NULL);

    xmlNewTextChild(head, NULL, BAD_CAST "title", BAD_CAST "");

    xmlNodePtr content_type = xmlNewChild(head, NULL, BAD_CAST "meta", NULL);
    xmlNewProp(content_type, BAD_CAST "http-equiv", BAD_CAST "Content-Type");
    xmlNewProp(content_type, BAD_CAST "content", BAD_CAST "text/html");

    xmlNodePtr capabilities = xmlNewChild(head, NULL, BAD_CAST "meta", NULL);
    xmlNewProp(capabilities, BAD_CAST "name", BAD_CAST "ocr-capabilities");
    xmlNewProp(capabilities,
               BAD_CAST "content",
               BAD_CAST "ocr_page ocr_carea ocr_par ocr_line ocrx_word");

    xmlNodePtr body = xmlNewChild(root, NULL, BAD_CAST "body", NULL);

    float page_height = page->bbox.y2;

    xmlNodePtr hocr_page = xmlNewChild(body, NULL, BAD_CAST "div", NULL);
    xmlNewProp(hocr_page, BAD_CAST "class", BAD_CAST "ocr_page");
    xmlNewProp(hocr_page,
               BAD_CAST "title",
               BAD_CAST hocr_render_bbox(page->bbox, page_height));

    column = (struct node *)page->content;
    while (column != NULL) {
        xmlNodePtr hocr_column =
            xmlNewChild(hocr_page, NULL, BAD_CAST "div", NULL);
        xmlNewProp(hocr_column, BAD_CAST "class", BAD_CAST "ocr_carea");
        xmlNewProp(hocr_column,
                   BAD_CAST "title",
                   BAD_CAST hocr_render_bbox(column->bbox, page_height));

        para = (struct node *)column->content;
        while (para != NULL) {
            xmlNodePtr hocr_para =
                xmlNewChild(hocr_column, NULL, BAD_CAST "p", NULL);
            xmlNewProp(hocr_para, BAD_CAST "class", BAD_CAST "ocr_par");
            xmlNewProp(hocr_para,
                       BAD_CAST "title",
                       BAD_CAST hocr_render_bbox(para->bbox, page_height));

            line = (struct node *)para->content;
            while (line != NULL) {
                xmlNodePtr hocr_line =
                    xmlNewChild(hocr_para, NULL, BAD_CAST "span", NULL);
                xmlNewProp(hocr_line, BAD_CAST "class", BAD_CAST "ocr_line");
                xmlNewProp(hocr_line,
                           BAD_CAST "title",
                           BAD_CAST hocr_render_bbox(line->bbox, page_height));

                word = (struct node *)line->content;
                while (word != NULL) {
                    xmlNodePtr hocr_word =
                        xmlNewTextChild(hocr_line, NULL,
                                        BAD_CAST "span",
                                        BAD_CAST (char *)word->content);
                    xmlNewProp(hocr_word, BAD_CAST "class", BAD_CAST "ocrx_word");
                    xmlNewProp(hocr_word,
                               BAD_CAST "title",
                               BAD_CAST hocr_render_bbox(word->bbox, page_height));

                    word = word->sibling;
                }

                line = line->sibling;
            }

            para = para->sibling;
        }

        column = column->sibling;
    }

    if (fputs("<!DOCTYPE html>\n", f) == 0) {
        fprintf(stderr, "hOCR: couldn't write out the DOCTYPE.\n");
    } else {
        xmlElemDump(f, doc, root);
        result = 0;
    }

    xmlFreeDoc(doc);
    xmlCleanupParser();

    return result;
}
