#ifndef _PDFSED_CONV_H_
#define _PDFSED_CONV_H_

#define PDFSED_CONV_NAME    "pdfsed-conv"
#define PDFSED_CONV_VERSION "0.1.0"


struct pdfsed_conv_cfg
{
    char *in_fn;
    char *out_fn;
    enum pdfsed_text_fmt in_fmt;
    enum pdfsed_text_fmt out_fmt;
    float scale;
};


int main(int argc, char *argv[]);

#endif  /* _PDFSED_CONV_H_ */
