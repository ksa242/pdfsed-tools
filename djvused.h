#ifndef _PDFSED_DJVUSED_H_
#define _PDFSED_DJVUSED_H_

#include <stdio.h>

#include "common.h"


struct node *djvused_read(FILE *f);
int djvused_write(FILE *f, const struct node *page);

#endif  /* _PDFSED_DJVUSED_H_ */
