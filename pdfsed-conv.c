#include <assert.h>
#include <error.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "djvused.h"
#include "hocr.h"
#include "txt.h"
#include "pdfsed-conv.h"


void show_usage_help(const char *bin)
{
    printf("%s, v%s\n", PDFSED_CONV_NAME, PDFSED_CONV_VERSION);
    printf("hOCR <-> djvused convertion utility.\n");
    printf("Copyright (C) 2014, Sergey Kolchin <me@ksa242.name>\n\n");

    printf("Converts positioned OCR output from hOCR/djvused, to hOCR/djvused/plain text.  "
           "Conversion is complete with forced bounding boxes' adjustment with optional "
           "coordinates' scaling.\n\n");

    printf("Usage: %s [options]\n", bin);
    printf("Available options are:\n\n");

    printf("    -h, --help\n");
    printf("        Show this usage help.\n\n");
 
    printf("    -i, --input=FILE\n");
    printf("    -o, --output=FILE\n");
    printf("        Input/output file name (default: read from stdin, write to stdout).\n\n");
 
    printf("    -f, --from=FORMAT\n");
    printf("    -t, --to=FORMAT\n");
    printf("        Input/output file format: hocr, djvused, or txt (default: hocr).\n");
    printf("        Plain text format is for output only!\n\n");
 
    printf("    -s, --scale=FACTOR\n");
    printf("        Scale the coordinates uniformly by the FACTOR (default: 1.0).\n");
}


struct pdfsed_conv_cfg *parse_opts(int argc, char *argv[])
{
    struct pdfsed_conv_cfg *cfg = NULL;

    int opt;
    char shortopts[] = "?hi:o:f:t:s:";
    struct option longopts[] = {
        {"help",   no_argument,       NULL, (int)'h'},
        {"input",  required_argument, NULL, (int)'i'},
        {"output", required_argument, NULL, (int)'o'},
        {"from",   required_argument, NULL, (int)'f'},
        {"to",     required_argument, NULL, (int)'t'},
        {"scale",  required_argument, NULL, (int)'s'},
        {NULL,     0,                 NULL, 0}
    };

    cfg = malloc(sizeof(struct pdfsed_conv_cfg));
    assert(cfg != NULL);
    memset(cfg, 0x00, sizeof(struct pdfsed_conv_cfg));
    cfg->in_fn = NULL;
    cfg->out_fn = NULL;
    cfg->in_fmt = PDFSED_FMT_HOCR;
    cfg->out_fmt = PDFSED_FMT_HOCR;
    cfg->scale = 1.000;

    while ((opt = getopt_long(argc, argv, shortopts, longopts, NULL)) != -1) {
        switch (opt) {
        case '?':
        case 'h':
            if (cfg != NULL) {
                free(cfg->in_fn);
                free(cfg->out_fn);
                cfg->in_fn = cfg->out_fn = NULL;
                free(cfg);
                cfg = NULL;
            }
            return cfg;

        case 'i':
        case 'o':
            if (cfg == NULL) {
                break;
            }
            if (opt == (int)'i') {
                free(cfg->in_fn);
                cfg->in_fn = NULL;
            } else {
                free(cfg->out_fn);
                cfg->out_fn = NULL;
            }
            if (optarg != NULL && strcmp(optarg, "-") > 0) {
                size_t optarg_len = strlen(optarg);
                char *fn = malloc((optarg_len + 1) * sizeof(char));
                assert(fn != NULL);
                strncpy(fn, optarg, optarg_len * sizeof(char));
                fn[optarg_len] = '\0';
                if (opt == (int)'i') {
                    cfg->in_fn = fn;
                } else {
                    cfg->out_fn = fn;
                }
            }
            break;

        case 'f':
        case 't':
            if (cfg == NULL) {
                break;
            } else if (optarg == NULL) {
                break;
            }
            int fmt = PDFSED_FMT_UNKNOWN;
            if (optarg[0] == 'h') {
                fmt = PDFSED_FMT_HOCR;
            } else if (optarg[0] == 'd') {
                fmt = PDFSED_FMT_DJVUSED;
            } else if (optarg[0] == 't') {
                fmt = PDFSED_FMT_TXT;
            }
            if (fmt == PDFSED_FMT_UNKNOWN) {
                fprintf(stderr, "Unknown %s format %s.\n",
                                (opt == (int)'f' ? "input" : "output"),
                                optarg);
            } else if (opt == (int)'f') {
                if (fmt == PDFSED_FMT_TXT) {
                    fprintf(stderr, "Can't read plain text, need hOCR or djvused.\n");
                } else {
                    cfg->in_fmt = fmt;
                }
            } else {
                cfg->out_fmt = fmt;
            }
            break;

        case 's':
            if (cfg == NULL) {
                break;
            }
            if (sscanf(optarg, "%f", &(cfg->scale)) < 1) {
                fprintf(stderr, "Failed parsing out scaling factor %s.\n",
                                optarg);
            } else if (cfg->scale < 0.001) {
                fprintf(stderr, "Scaling factor has to be greater than zero.\n");
                cfg->scale = 1.000;
            } else if (cfg->scale > 3.000) {
                fprintf(stderr, "Scaling factor is rather big...\n");
            }
            break;
        }
    }

    return cfg;
}


int main(int argc, char *argv[])
{
    int result = 0;

    FILE *in_f = NULL, *out_f = NULL;

    struct node *page = NULL;

    struct pdfsed_conv_cfg *cfg = parse_opts(argc, argv);
    if (cfg == NULL) {
        show_usage_help(argv[0]);
        return EXIT_SUCCESS;
    }

    fprintf(stderr, "Reading %s from %s, writing %s to %s, scaling to %.1f%%.\n",
                    (cfg->in_fmt == PDFSED_FMT_DJVUSED ? "djvused" : "hOCR"),
                    (cfg->in_fn == NULL ? "stdin" : cfg->in_fn),
                    (cfg->out_fmt == PDFSED_FMT_TXT
                        ? "plain text"
                        : (cfg->out_fmt == PDFSED_FMT_DJVUSED ? "djvused" : "hOCR")),
                    (cfg->out_fn == NULL ? "stdout" : cfg->out_fn),
                    100 * cfg->scale);

    if (result == 0) {
        if (cfg->in_fn == NULL) {
            in_f = stdin;
        } else if ((in_f = fopen(cfg->in_fn, "r")) == NULL) {
            perror("fopen(in)");
            result = -1;
        }
    }

    if (result == 0) {
        if (cfg->out_fn == NULL) {
            out_f = stdout;
        } else if ((out_f = fopen(cfg->out_fn, "w")) == NULL) {
            perror("fopen(out)");
            result = -1;
        }
    }

    if (result == 0) {
        page = (cfg->in_fmt == PDFSED_FMT_HOCR)
            ? hocr_read(in_f)
            : djvused_read(in_f);
        if (page != NULL) {
            recalculate_bbox(page, cfg->scale);
            if (cfg->out_fmt == PDFSED_FMT_HOCR) {
                result = hocr_write(out_f, page);
            } else if (cfg->out_fmt == PDFSED_FMT_DJVUSED) {
                result = djvused_write(out_f, page);
            } else {
                result = txt_write(out_f, page);
            }
            tree_free_node(page);
            page = NULL;
        }
    }

    if (fclose(in_f) == EOF) {
        perror("fclose(in)");
    }
    if (fclose(out_f) == EOF) {
        perror("fclose(out)");
    }

    free(cfg->in_fn);
    free(cfg->out_fn);
    cfg->in_fn = cfg->out_fn = NULL;
    free(cfg);
    cfg = NULL;

    return (result == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
