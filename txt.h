#ifndef _PDFSED_TXT_H_
#define _PDFSED_TXT_H_

#include <stdio.h>

#include "common.h"


struct node *txt_read(FILE *f);
int txt_write(FILE *f, const struct node *page);

#endif  /* _PDFSED_TXT_H_ */
