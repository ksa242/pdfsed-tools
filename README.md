pdfsed-run
==========

pdfsed-run builds a PDF file from a stack of images with optional hidden
text, acting according to a simple script. Run it like this:

    pdfsed-run [script-file] [pdf-file]

Use "-" for stdin (script file, default) and stdout (pdf file, default).


The script syntax is simple:

    command [argument ...];


Arguments can be int/float numbers, hex numbers (`0xHH`), strings, and atoms:

*   strings must be enclosed in double quotes; escape with back-slashes;
    
*   all lengths and coordinates must be in points, int or float;
    
*   `(0, 0)` coordinates are at the bottom-left corner of the page/image;
    
*   angle arguments must be in degrees (counter-clockwise);
    
*   colors (RGB only) should be specified as hex numbers: `0xRRGGBB`;
    
*   atoms must match `/^[a-z][a-z\d\-]*[a-z]$/i` regexp (commands are atoms!),
    and are case-insensitive; from here on atoms will be typed in upper-case
    for distinction.


Available commands are:

    SET [TITLE "title"] [AUTHOR "author"] [CREATOR "creator"]

Sets the document's properties: title, author, creator -- at least one has
to present.
 

    CREATE PAGE [SIZE width height] [ANGLE rotation-angle]

Creates a new page. Accepts page size as width and height pair (in points),
and rotation angle (in degrees, counter-clockwise). First page must have the
size present, all consecutive pages will inherit the size until told
overwise. Rotation angle is purely optional and is not inheritable.
 

    DRAW IMAGE "file-name" DPI dpi [POS x y] [MASK color]
        [MASK-IMAGE "file-name"]

Draws an image on the current page. First argument is the image file name,
and is mandatory. Arguments are:
 
    DPI dpi

Image DPI; pdfsed-run can't detect the actual image DPI yet, so this
argument is mandatory.
 
    POS x y

Image offset in points, defaults to (0, 0).
 
    MASK color

Transparent color of the image (default: none). Use 0xRRGGBB hex number.
 
    MASK-IMAGE "file-name"

Monochrome mask image's file name (default: none).
 

    DRAW TEXT "file-name" [DPI dpi] [POS x y] [SCALE factor]

Draws a hidden text on the current page. First argument is the text layout
script file name, and is mandatory. Both hOCR (.hocr/.htm/.html) and djvused
(.djvused) file types are supported.

**PDFSED-RUN EXPECTS UTF-8 TEXT INPUT! RUSSIAN IS THE ONLY LANGUAGE
SUPPORTED UNTIL THERE IS A WAY TO PUT PROPER UTF-8 INTO PDF USING LIBHARU!**

Optional arguments are:
 
    DPI dpi

Layout file's DPI, i. e. the DPI of the image file you've OCRed. The script
will try to calculate that for you (by simply fitting the text to the page
width), which should work for the typical case of OCRing the whole page.
 
    POS x y

Text offset in points to match the OCRed image. Defaults to (0, 0).
 
    SCALE factor

Layout file's scaling factor, defaults to 1.0 (no scaling).


If everything went well, you'll get a searchable PDF file out of your stack
of OCRed images and text layouts. There is no need to run the PDF file
through an optimizer since [libHaru](http://www.libharu.org/)'s compression
support is quite good.


pdfsed-conv
===========

Converts positioned OCR output from/to hOCR/djvused, or to plain text.
Conversion is done with forced recalculation of bounding boxes with optional
scaling of the coordinates.

Available options are:

    -h, --help

Show this usage help.

    -v, --verbose

Be extra verbose when parsing errors occur.

    -i, --input=FILE
    -o, --output=FILE

Input/output file name (default: read from stdin, write to stdout).

    -f, --from=FORMAT
    -t, --to=FORMAT

Input/output file format: hocr, djvused, or txt. If not set, will be guessed
from input/output file name. Plain text format is for output only!

    -s, --scale=FACTOR

Scale the coordinates uniformly by the FACTOR (default: 1.0).  The factor
will be rounded to three decimal digits.


Dependencies
============

The utilities require:

*   [libHaru](http://www.libharu.org/) (tested with v2.2.1-r1);

*   [libxml2](http://www.xmlsoft.org/) (tested with v2.9.1-r1).
