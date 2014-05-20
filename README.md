pdfsed-run
==========

pdfsed-run builds a PDF file from a stack of images with optional hidden
text, acting according to a simple script.  Run it like this:

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

    SET-TITLE "title"

Sets the document's title.  Expects the title as single argument.
 

    SET-AUTHOR "author"

Sets the document's author.  Expects the author's name as single argument.

 
    CREATE-PAGE [width height] [rotation-angle]

Creates a new page.  Expects up to three int/float arguments: width and
height pair (in points), and rotation angle (degrees, counter-clockwise).
First page must have width/height pair present; all consecutive pages will
inherit the size until told overwise.  Rotation angle is optional and *IS NOT
inheritable*.
 

    DRAW-IMAGE "file-name" [DPI dpi] [POS x y] [MASK color]

Draws an image on the current page.  First argument is the image file name,
and is mandatory.  Optional arguments are:
 
    DPI dpi

Image DPI (overrides automatic detection), defaults to 72 if automatic
detection fails and not overriden.
 
    POS x y

Image offset in points, defaults to (0, 0).
 
    MASK color

Transparent color (mask) for the image (default: none).  Use 0xRRGGBB hex
number for readability, although a simple int will do.
 

    DRAW-TEXT "file-name" [DPI dpi] [POS x y] [SCALE factor]

Draws a hidden text on the current page.  First argument is the text layout
script file name, and is mandatory.  Both hOCR (.hocr/.htm/.html) and
djvused (.djvused) file types are supported.  Optional arguments are:
 
    DPI dpi

Layout file's DPI, i. e. the DPI of the image file you've OCRed.  The script
will try to calculate that for you (after the scaling, see below), which
should be all right for the typical case of the text covering the whole
page.
 
    POS x y

Text offset in points, to match the OCRed image.  Defaults to (0, 0).
 
    SCALE factor

Layout file's scaling factor, defaults to 1.0 (no scaling).
        

If everything went well, you'll get a searchable PDF file out of your stack
of OCRed images and text layouts.  It is recommended to run the PDF file
through an optimizer like [QPDF](http://qpdf.sourceforge.net/), as
[reportlab](http://www.reportlab.com/)'s compression support is not that
good.


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

Input/output file format: hocr, djvused, or txt.  If not set, will be
guessed from input/output file name.  Plain text format is for output only!

    -s, --scale=FACTOR

Scale the coordinates uniformly by the FACTOR (default: 1.0).  The factor
will be rounded to three decimal digits.

    -x OFFSET
    -y OFFSET

Offset the bounding boxes' coordinates, horizontally or vertically, by a
number of pixels.  May be negative.  Remember: the coordinates start in the
bottom left corner.


Requirements
============

Python v2.7 with the following modules:

*   [ReportLab](https://pypi.python.org/pypi/reportlab) (tested with version 2.7);

*   [PIL](https://pypi.python.org/pypi/PIL) or [Pillow](https://pypi.python.org/pypi/Pillow) imaging module (tested with Pillow v2.0.0).

The future versions will probably be rewritten to Python 3.
