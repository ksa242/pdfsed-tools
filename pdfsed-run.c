#include <assert.h>
#include <error.h>
#include <getopt.h>
#include <libgen.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include <hpdf.h>
#include <iconv.h>

#include "common.h"
#include "djvused.h"
#include "hocr.h"
#include "pdfsed.h"
#include "pdfsed-run.h"


void libharu_error_handler(HPDF_STATUS error_no, HPDF_STATUS detail_no, void *user_data)
{
    fprintf(stderr, "HPDF: errno = %04x, details = %d\n",
            (unsigned int)error_no, (int)detail_no);
}


char *utf8_to_cp1251(char *src)
{
    static iconv_t ctx = (iconv_t)-1;

    size_t dest_len, src_len;
    char *dest, *psrc, *pdest;

    if (ctx == (iconv_t)-1) {
        ctx = iconv_open("CP1251", "UTF-8");
        assert(ctx != (iconv_t)-1);
    }

    dest_len = src_len = strlen(src) * sizeof(char);

    dest = malloc(src_len + sizeof(char));
    assert(dest != NULL);
    memset(dest, 0x00, src_len + sizeof(char));

    psrc = src;
    pdest = dest;
    assert(iconv(ctx, &psrc, &src_len, &pdest, &dest_len) >= 0);

    return dest;
}


char *extend_file_name(char *fn, const char *pwd)
{
    size_t fn_len = strlen(fn), pwd_len = strlen(pwd);

    /* Append working path to the file name. */
    fn = realloc(fn, (pwd_len + 1 + fn_len + 1) * sizeof(char));
    assert(fn != NULL);

    memmove(&(fn[pwd_len + 1]), fn, (fn_len + 1) * sizeof(char));
    memcpy(fn, pwd, pwd_len * sizeof(char));
    fn[pwd_len] = '/';

    return fn;
}


void draw_text(FILE *pdfsed_f, HPDF_Doc pdf, const char *pwd)
{
    enum pdfsed_atom prop = PDFSED_ATOM_UNKNOWN;

    HPDF_Page page = NULL;
    HPDF_REAL x, y, dpi, scale;

    char *fn = NULL, *fext = NULL;
    FILE *txt_f = NULL;
    struct node *txt = NULL,
                *column = NULL,
                *para = NULL,
                *line = NULL,
                *word = NULL;

    char *word_content = NULL;

    page = HPDF_GetCurrentPage(pdf);
    assert(page != NULL);

    fn = extend_file_name(pdfsed_read_str(pdfsed_f), pwd);

    /* Get the file extension. */
    fext = rindex(fn, '.');
    assert(fext != NULL);

    fprintf(stderr, "Laying out text from %s.\n", fn);

    txt_f = fopen(fn, "r");
    assert(txt_f != NULL);

    if (!strcasecmp(fext, ".hocr") ||
        !strcasecmp(fext, ".html")) {
        txt = hocr_read(txt_f);

    } else if (!strcasecmp(fext, ".djvused")) {
        txt = djvused_read(txt_f);

    } else {
        fprintf(stderr, "\tUnsupported text file type %s.\n", fext);
    }

    assert(fclose(txt_f) == 0);
    txt_f = NULL;

    assert(txt != NULL);

    x = 0.0;
    y = 0.0;
    dpi = round(txt->bbox.x2 / (HPDF_Page_GetWidth(page) / 72));
    scale = 1.0;

    while (1) {
        prop = pdfsed_read_atom(pdfsed_f);
        if (prop == PDFSED_ATOM_EOC) {
            break;
        }

        assert(prop == PDFSED_ATOM_PROP_DPI ||
               prop == PDFSED_ATOM_PROP_POS ||
               prop == PDFSED_ATOM_PROP_SCALE);
        if (prop == PDFSED_ATOM_PROP_DPI) {
            dpi = pdfsed_read_float(pdfsed_f);
        } else if (prop == PDFSED_ATOM_PROP_POS) {
            x = pdfsed_read_float(pdfsed_f);
            y = pdfsed_read_float(pdfsed_f);
        } else if (prop == PDFSED_ATOM_PROP_SCALE) {
            scale = pdfsed_read_float(pdfsed_f);
        }
    }

    fprintf(stderr, "\tDPI %.2f\n", dpi);
    fprintf(stderr, "\tOffset %.2f × %.2f pt\n", x, y);
    fprintf(stderr, "\tScale %.2f%%\n", scale * 100.0);

    assert(HPDF_Page_BeginText(page) == HPDF_OK);
    assert(HPDF_Page_SetTextRenderingMode(page, HPDF_INVISIBLE) == HPDF_OK);
    assert(HPDF_Page_SetFontAndSize(
        page,
        HPDF_GetFont(pdf, PDFSED_INVISIBLE_FONT, "CP1251"),
        PDFSED_INVISIBLE_FONT_SIZE
    ) == HPDF_OK);

    column = (struct node *)txt->content;
    while (column != NULL) {
        para = (struct node *)column->content;
        while (para != NULL) {
            line = (struct node *)para->content;
            while (line != NULL) {
                word = (struct node *)line->content;
                while (word != NULL) {
                    word_content = utf8_to_cp1251((char *)word->content);
                    assert(HPDF_Page_TextOut(
                        page,
                        scale * (x + 72 * (word->bbox.x1 / dpi)),
                        scale * (y + 72 * (line->bbox.y1 / dpi)),
                        word_content
                    ) == HPDF_OK);
                    free(word_content);
                    word_content = NULL;

                    word = word->sibling;
                }
                line = line->sibling;
            }
            para = para->sibling;
        }
        column = column->sibling;
    }

    assert(HPDF_Page_EndText(page) == HPDF_OK);

    tree_free_node(txt);
    txt = NULL;

    fext = NULL;
    free(fn);
    fn = NULL;

    page = NULL;
}


void draw_image(FILE *pdfsed_f, HPDF_Doc pdf, const char *pwd)
{
    enum pdfsed_atom prop = PDFSED_ATOM_UNKNOWN;

    HPDF_Page page = NULL;
    HPDF_REAL x, y, w, h, dpi;
    long mask = -1;

    HPDF_Image img = NULL;
    char *fn = NULL, *fext = NULL;

    HPDF_Image smask = NULL;
    char *smask_fn = NULL, *smask_fext = NULL;

    page = HPDF_GetCurrentPage(pdf);
    assert(page != NULL);

    fn = extend_file_name(pdfsed_read_str(pdfsed_f), pwd);
    fprintf(stderr, "Drawing image from %s.\n", fn);

    fext = rindex(fn, '.');  /* Get the file extension. */
    assert(fext != NULL);
    if (!strcmp(fext, ".png")) {
        img = HPDF_LoadPngImageFromFile(pdf, fn);
    } else if (!strcasecmp(fext, ".jpg") ||
               !strcasecmp(fext, ".jpeg") ||
               !strcasecmp(fext, ".jpe")) {
        img = HPDF_LoadJpegImageFromFile(pdf, fn);
    } else {
        fprintf(stderr, "\tUnsupported image file type %s.\n", fext);
    }

    assert(img != NULL);

    x = 0.0;
    y = 0.0;
    w = 0.0;
    h = 0.0;
    dpi = round(HPDF_Image_GetWidth(img) / (HPDF_Page_GetWidth(page) / 72));

    while (1) {
        prop = pdfsed_read_atom(pdfsed_f);
        if (prop == PDFSED_ATOM_EOC) {
            break;
        }

        assert(prop == PDFSED_ATOM_PROP_DPI ||
               prop == PDFSED_ATOM_PROP_POS ||
               prop == PDFSED_ATOM_PROP_MASK ||
               prop == PDFSED_ATOM_PROP_MASK_IMAGE);

        if (prop == PDFSED_ATOM_PROP_DPI) {
            dpi = pdfsed_read_float(pdfsed_f);

        } else if (prop == PDFSED_ATOM_PROP_POS) {
            x = pdfsed_read_float(pdfsed_f);
            y = pdfsed_read_float(pdfsed_f);

        } else if (prop == PDFSED_ATOM_PROP_MASK) {
            mask = pdfsed_read_color(pdfsed_f);

        } else if (prop == PDFSED_ATOM_PROP_MASK_IMAGE) {
            smask_fn = extend_file_name(pdfsed_read_str(pdfsed_f), pwd);
            smask_fext = rindex(smask_fn, '.');
            assert(smask_fext != NULL);
            if (!strcmp(smask_fext, ".png")) {
                smask = HPDF_LoadPngImageFromFile(pdf, smask_fn);
            } else if (!strcasecmp(smask_fext, ".jpg") ||
                       !strcasecmp(smask_fext, ".jpeg") ||
                       !strcasecmp(smask_fext, ".jpe")) {
                smask = HPDF_LoadJpegImageFromFile(pdf, smask_fn);
            } else {
                fprintf(stderr, "\tUnsupported mask image file type %s.\n", smask_fext);
            }
            assert(smask != NULL);
        }
    }

    w = 72 * (HPDF_Image_GetWidth(img) / dpi);
    h = 72 * (HPDF_Image_GetHeight(img) / dpi);

    fprintf(stderr, "\tDPI %.2f\n", dpi);
    fprintf(stderr, "\tOffset %.2f × %.2f pt\n", x, y);
    fprintf(stderr, "\tSize: %.2f × %.2f pt\n", w, h);

    if (smask != NULL) {
        fprintf(stderr, "\tMasking the image with %s.\n", smask_fn);
        assert(HPDF_Image_SetMaskImage(img, smask) == HPDF_OK);

    } else if (mask >= 0) {
        HPDF_UINT r, g, b;

        fprintf(stderr, "\tMask: %06lx\n", mask);

        r = mask >> 16;
        g = (mask >> 8) & 0xff;
        b = mask & 0xff;
        assert(HPDF_Image_SetColorMask(img, r, r, g, g, b, b) == HPDF_OK);
    }

    assert(HPDF_Page_DrawImage(page, img, x, y, w, h) == HPDF_OK);

    smask = NULL;
    smask_fext = NULL;
    free(smask_fn);
    smask_fn = NULL;

    img = NULL;
    fext = NULL;
    free(fn);
    fn = NULL;

    page = NULL;
}


void create_page(FILE *pdfsed_f, HPDF_Doc pdf)
{
    enum pdfsed_atom prop = PDFSED_ATOM_UNKNOWN;

    HPDF_Page cur_page = NULL;
    HPDF_Page new_page = NULL;

    HPDF_REAL w = 0, h = 0, angle = 0;

    prop = pdfsed_read_atom(pdfsed_f);
    assert(prop == PDFSED_ATOM_OBJ_PAGE);

    fprintf(stderr, "\nCreating a page.\n");

    cur_page = HPDF_GetCurrentPage(pdf);
    if (cur_page != NULL) {
        w = HPDF_Page_GetWidth(cur_page);
        h = HPDF_Page_GetHeight(cur_page);
        cur_page = NULL;
    }

    new_page = HPDF_AddPage(pdf);
    assert(new_page != NULL);

    while (1) {
        prop = pdfsed_read_atom(pdfsed_f);
        if (prop == PDFSED_ATOM_EOC) {
            break;
        }

        assert(prop == PDFSED_ATOM_PROP_SIZE ||
               prop == PDFSED_ATOM_PROP_ANGLE);

        if (prop == PDFSED_ATOM_PROP_SIZE) {
            w = pdfsed_read_float(pdfsed_f);
            h = pdfsed_read_float(pdfsed_f);

        } else if (prop == PDFSED_ATOM_PROP_ANGLE) {
            angle = pdfsed_read_float(pdfsed_f);
        }
    }

    assert(w > 0 && h > 0);

    fprintf(stderr, "\tSize %.2f × %.2f pt\n", w, h);
    assert(HPDF_Page_SetWidth(new_page, w) == HPDF_OK);
    assert(HPDF_Page_SetHeight(new_page, h) == HPDF_OK);

    fprintf(stderr, "\tRotation %.2f°\n", angle);
    assert(HPDF_Page_SetRotate(new_page, angle) == HPDF_OK);
}


void set_doc_prop(FILE *pdfsed_f, HPDF_Doc pdf)
{
    enum pdfsed_atom obj = PDFSED_ATOM_UNKNOWN;

    HPDF_InfoType type;
    char *value = NULL, *value_conv = NULL;

    fprintf(stderr, "\nSetting document properties.\n");

    while (1) {
        type = HPDF_INFO_TITLE;

        obj = pdfsed_read_atom(pdfsed_f);
        if (obj == PDFSED_ATOM_EOC) {
            break;
        }

        assert(obj == PDFSED_ATOM_OBJ_TITLE ||
               obj == PDFSED_ATOM_OBJ_AUTHOR ||
               obj == PDFSED_ATOM_OBJ_CREATOR);

        value = pdfsed_read_str(pdfsed_f);

        if (obj == PDFSED_ATOM_OBJ_TITLE) {
            fprintf(stderr, "\tTitle: %s\n", value);
            type = HPDF_INFO_TITLE;
        } else if (obj == PDFSED_ATOM_OBJ_AUTHOR) {
            fprintf(stderr, "\tAuthor: %s\n", value);
            type = HPDF_INFO_AUTHOR;
        } else if (obj == PDFSED_ATOM_OBJ_CREATOR) {
            fprintf(stderr, "\tCreator: %s\n", value);
            type = HPDF_INFO_CREATOR;
        }

        value_conv = utf8_to_cp1251(value);
        assert(value_conv != NULL);

        assert(HPDF_SetInfoAttr(pdf, type, value_conv) == HPDF_OK);

        free(value_conv);
        value_conv = NULL;

        free(value);
        value = NULL;
    }
}


void show_usage_help(const char *bin)
{
    printf("%s, v%s\n", PDFSED_RUN_NAME, PDFSED_RUN_VERSION);
    printf("Simple PDF generator from OCR images and hidden text.\n");
    printf("Copyright (C) 2014, Sergey Kolchin <me@ksa242.name>\n\n");

    printf("Composes a multi-page PDF layer by layer from OCR images (PNG, JPEG) with "
           "optional hidden text (hOCR, djvused).\n\n");

    printf("Usage: %s [options] [pdfsed script] [PDF file]\n\n", bin);

    printf("Available options are:\n\n");

    printf("    -h, --help\n");
    printf("        Show this usage help.\n\n");

    printf("By default, the utility reads pdfsed script from standard input, and "
           "dumps generated PDF to standard output.\n");
}


struct pdfsed_run_cfg *parse_opts(int argc, char *argv[])
{
    struct pdfsed_run_cfg *cfg = NULL;

    int opt;
    char shortopts[] = "?h";
    struct option longopts[] = {
        {"help", no_argument, NULL, (int)'h'},
        {NULL,   0,           NULL, 0}
    };

    cfg = malloc(sizeof(struct pdfsed_run_cfg));
    assert(cfg != NULL);
    cfg->pdfsed_fn = NULL;
    cfg->pdfsed_f = NULL;
    cfg->pdf_fn = NULL;
    cfg->pdf_f = NULL;

    while ((opt = getopt_long(argc, argv, shortopts, longopts, NULL)) != -1) {
        switch (opt) {
        case '?':
        case 'h':
            if (cfg != NULL) {
                cfg->pdfsed_fn = cfg->pdf_fn = NULL;
                free(cfg);
                cfg = NULL;
            }
        }
    }

    if (cfg != NULL) {
        if (optind < argc) {
            if (strlen(argv[optind]) == 0) {
                fprintf(stderr, "Empty pdfsed script file name.\n");
            } else {
                cfg->pdfsed_fn = argv[optind];
            }
            optind++;
        }
        if (optind < argc) {
            if (strlen(argv[optind]) == 0) {
                fprintf(stderr, "Empty PDF file name.\n");
            } else {
                cfg->pdf_fn = argv[optind];
            }
            optind++;
        }
    }

    return cfg;
}



int main(int argc, char *argv[])
{
    int result = 0;

    struct pdfsed_run_cfg *cfg = parse_opts(argc, argv);
    if (cfg == NULL) {
        show_usage_help(argv[0]);
        return EXIT_SUCCESS;
    }

    if (cfg->pdfsed_fn == NULL) {
        cfg->pdfsed_f = stdin;
    } else if ((cfg->pdfsed_f = fopen(cfg->pdfsed_fn, "r")) == NULL) {
        perror("fopen(pdfsed)");
        result = -1;
    }

    if (result == 0) {
        if (cfg->pdf_fn == NULL) {
            cfg->pdf_f = stdout;
        } else if ((cfg->pdf_f = fopen(cfg->pdf_fn, "w")) == NULL) {
            perror("fopen(pdf)");
            result = -1;
        }
    }

    if (result == 0) {
        HPDF_Doc pdf = NULL;

        char *pwd = NULL;

        enum pdfsed_atom cmd = PDFSED_ATOM_UNKNOWN;

        fprintf(stderr, "Reading pdfsed script from %s.\n",
                (cfg->pdfsed_fn != NULL) ? cfg->pdfsed_fn : "stdin");
        fprintf(stderr, "Dumping PDF to %s.\n",
                (cfg->pdf_fn != NULL) ? cfg->pdf_fn : "stdout");

        pwd = (cfg->pdfsed_fn == NULL) ? NULL : dirname(cfg->pdfsed_fn);
        if (pwd == NULL || strlen(pwd) == 0) {
            pwd = ".";
        }

        fprintf(stderr, "\nPreparing a new PDF document.\n");

        pdf = HPDF_New(libharu_error_handler, NULL);
        assert(pdf != NULL);
        assert(HPDF_SetCompressionMode(pdf, HPDF_COMP_ALL) == HPDF_OK);
        assert(HPDF_SetInfoAttr(pdf, HPDF_INFO_PRODUCER, PDFSED_RUN_NAME) == HPDF_OK);
        assert(HPDF_SetCurrentEncoder(pdf, "CP1251") == HPDF_OK);

        while (1) {
            cmd = pdfsed_read_atom(cfg->pdfsed_f);
            if (cmd == PDFSED_ATOM_EOC) {
                /* Unexpected end-of-command or end-of-file. */
                break;

            } else if (cmd == PDFSED_ATOM_CMD_SET) {
                set_doc_prop(cfg->pdfsed_f, pdf);

            } else if (cmd == PDFSED_ATOM_CMD_CREATE) {
                create_page(cfg->pdfsed_f, pdf);

            } else if (cmd == PDFSED_ATOM_CMD_DRAW) {
                enum pdfsed_atom obj = pdfsed_read_atom(cfg->pdfsed_f);
                assert(obj == PDFSED_ATOM_OBJ_IMAGE ||
                       obj == PDFSED_ATOM_OBJ_TEXT);

                if (obj == PDFSED_ATOM_OBJ_IMAGE) {
                    draw_image(cfg->pdfsed_f, pdf, pwd);
                } else {
                    draw_text(cfg->pdfsed_f, pdf, pwd);
                }

            } else {
                fprintf(stderr, "\nUnknown pdfsed command.\n");
            }
        }

        if (pdf != NULL && HPDF_GetCurrentPage(pdf) != NULL) {
            HPDF_STATUS err = HPDF_SaveToStream(pdf);
            if (err != HPDF_OK) {
                fprintf(stderr, "Failed saving the document stream: 0x%04lx.\n", err);

            } else {
                HPDF_BYTE *pdf_buf = NULL;
                HPDF_UINT32 pdf_buf_len = 0, received = 0, saved = 0, n = 0;

                pdf_buf_len = HPDF_GetStreamSize(pdf);
                pdf_buf = malloc(pdf_buf_len);
                assert(pdf_buf != NULL);
                memset(pdf_buf, 0x00, pdf_buf_len);

                while (received < pdf_buf_len) {
                    n = pdf_buf_len - received;
                    if (HPDF_ReadFromStream(pdf, pdf_buf + received, &n) != HPDF_OK) {
                        perror("HPDF_ReadFromStream()");
                        break;
                    } else {
                        received += n;
                    }
                }

                fprintf(stderr, "\nGot %d bytes from the PDF buffer, saving...\n",
                        received);

                while (saved < pdf_buf_len) {
                    n = fwrite(pdf_buf + saved, 1, pdf_buf_len - saved, cfg->pdf_f);
                    if (n < 1) {
                        perror("fwrite(pdf)");
                        break;
                    } else {
                        saved += n;
                    }
                }

                fprintf(stderr, "Saved %d bytes to the PDF file.\n", saved);

                free(pdf_buf);
                pdf_buf = NULL;
            }

            HPDF_Free(pdf);
            pdf = NULL;
        }

        fprintf(stderr, "\nDone.\n");
    }

    if (fclose(cfg->pdfsed_f) == EOF) {
        perror("fclose(pdfsed)");
    }
    if (fclose(cfg->pdf_f) == EOF) {
        perror("fclose(pdf)");
    }

    cfg->pdfsed_fn = cfg->pdf_fn = NULL;
    free(cfg);
    cfg = NULL;

    return (result == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
