#ifndef _PDFSED_HOCR_H_
#define _PDFSED_HOCR_H_

#include <stdio.h>

#include "common.h"

#define HOCR_XPATH_PAGE   "./body/div[@class=\"ocr_page\"]"
#define HOCR_XPATH_COLUMN "./div[@class=\"ocr_carea\"]"
#define HOCR_XPATH_PARA   "./p[@class=\"ocr_par\"]"
#define HOCR_XPATH_LINE   "./span[@class=\"ocr_line\"]"
#define HOCR_XPATH_WORD   "./span[@class=\"ocrx_word\"]"


struct node *hocr_read(FILE *f);
int hocr_write(FILE *f, const struct node *page);

#endif  /* _PDFSED_HOCR_H_ */
