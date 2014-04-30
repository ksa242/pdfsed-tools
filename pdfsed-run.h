#ifndef _PDFSED_RUN_H_
#define _PDFSED_RUN_H_

#include <stdio.h>


#define PDFSED_RUN_NAME    "pdfsed-run"
#define PDFSED_RUN_VERSION "0.1.0"

#define PDFSED_DEFAULT_FONT_SIZE 8


struct pdfsed_run_cfg
{
    char *pdfsed_fn;
    FILE *pdfsed_f;
    char *pdf_fn;
    FILE *pdf_f;
};


int main(int argc, char *argv[]);

#endif  /* _PDFSED_RUN_H_ */
