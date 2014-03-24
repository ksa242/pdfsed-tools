#! /usr/bin/env python2
# -*- coding: utf-8 -*-
#
# Copyright (c) 2014, Sergey Kolchin <me@ksa242.name>
# All rights reserved.
# 
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met: 
# 
# 1. Redistributions of source code must retain the above copyright notice,
#    this list of conditions and the following disclaimer.
# 
# 2. Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution. 
# 
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.

"""hOCR reader/writer.

Use load() to read a page layout from an hOCR file.

Use save() to save a page layout as an hOCR file.
"""

import logging

import lxml.etree
import lxml.html
import lxml.html.builder


def _title2bbox(src, page_height=None):
    """Parse out bbox coordinates from an hOCR element's "title" attribute."""
    bbox = (src or '').split()

    try:
        assert(len(bbox) == 5)
        assert(bbox[0] == 'bbox')
        bbox = [int(v, 10) for v in bbox[1:]]
    except (AssertionError, ValueError):
        raise ValueError('Invalid hOCR bbox definition: %s' % (src, ))

    if page_height is not None:
        bbox[1], bbox[3] = \
            page_height - bbox[3], page_height - bbox[1]

    return tuple(bbox)


def _bbox2title(bbox, page_height):
    """Compose an hOCR elenent's "title" attribute from a bbox tuple."""
    return 'bbox %d %d %d %d' % (bbox[0], page_height - bbox[3],
                                 bbox[2], page_height - bbox[1])


def load(file_):
    """Load a page layout from an hOCR file."""
    try:
        doc = lxml.html.document_fromstring(file_.read())
    except lxml.etree.XMLSyntaxError as err:
        raise ValueError('Invalid or empty hOCR: %s' % (err.args[0], ))

    try:
        page = doc.xpath('/html/body/div[@class="ocr_page"]')[0]
    except IndexError:
        raise ValueError('No hOCR pages found')

    page_num = 0
    page_image = 'page.tiff'
    page_bbox = None
    for chunk in page.get('title', '').split(';'):
        chunk = chunk.strip()
        if chunk.startswith('image'):
            try:
                page_image = chunk.split(None, 1)[-1].strip('"')
            except ValueError as err:
                logging.warn('Failed parsing hOCR image file name: %s. Ignoring...' %
                             (err.args[0], ))
        elif chunk.startswith('ppageno'):
            try:
                page_num = int(chunk.split(None, 1)[-1], 10)
            except ValueError as err:
                logging.warn('Failed parsing hOCR page number: %s. Ignoring...' %
                             (err.args[0], ))
        elif chunk.startswith('bbox'):
            page_bbox = _title2bbox(chunk.strip())
        else:
            logging.warn('Unknown hOCR page info chunk: "%s". Skipping...' %
                         (chunk, ))
    if not page_bbox:
        raise ValueError('hOCR page bbox info not found')

    page_height = page_bbox[3]

    columns = []
    for column in page.xpath('./div[@class="ocr_carea"]'):
        paras = []
        for para in column.xpath('./p[@class="ocr_par"]'):
            lines = []
            for line in para.xpath('./span[@class="ocr_line"]'):
                words = []
                for word in line.xpath('./span[@class="ocrx_word"]'):
                    content = (word.text or '').strip()
                    if content:
                        words.append({'bbox': _title2bbox(word.get('title'),
                                                          page_height),
                                      'content': content})
                if len(words) > 0:
                    lines.append({'bbox': _title2bbox(line.get('title'),
                                                      page_height),
                                  'content': words})
            if len(lines) > 0:
                paras.append({'bbox': _title2bbox(para.get('title'),
                                                  page_height),
                              'content': lines})
        if len(paras) > 0:
            columns.append({'bbox': _title2bbox(column.get('title'),
                                                page_height),
                            'content': paras})
    return {'num': page_num,
            'image': page_image,
            'bbox': page_bbox,
            'content': columns}


def save(page, file_):
    """Save a page layout to an hOCR file."""
    doc = lxml.html.builder.HTML(
        lxml.html.builder.HEAD(
            lxml.html.builder.TITLE(''),
            lxml.html.builder.META({
                'http-equiv': 'Content-Type',
                'content': 'text/html; charset=utf-8'
            }),
            lxml.html.builder.META({
                'name': 'ocr-capabilities',
                'content': 'ocr_page ocr_carea ocr_par ocr_line ocrx_word'
            })
        )
    )

    body = lxml.etree.SubElement(doc, 'body')

    page_bbox = page['bbox']
    page_height = page_bbox[3]

    page_elem = lxml.etree.SubElement(body, 'div')
    page_elem.set('class', 'ocr_page')
    page_elem.set('title', 'image "%s"; %s; ppageno %d' %
                           (page['image'],
                            _bbox2title(page_bbox, page_height),
                            page['num']))

    for column in page['content']:
        column_elem = lxml.etree.SubElement(page_elem, 'div')
        column_elem.set('class', 'ocr_carea')
        column_elem.set('title', _bbox2title(column['bbox'], page_height))

        for para in column['content']:
            para_elem = lxml.etree.SubElement(column_elem, 'p')
            para_elem.set('class', 'ocr_par')
            para_elem.set('title', _bbox2title(para['bbox'], page_height))

            for line in para['content']:
                line_elem = lxml.etree.SubElement(para_elem, 'span')
                line_elem.set('class', 'ocr_line')
                line_elem.set('title', _bbox2title(line['bbox'], page_height))

                for word in line['content']:
                    word_elem = lxml.etree.SubElement(line_elem, 'span')
                    word_elem.set('class', 'ocrx_word')
                    word_elem.set('title', _bbox2title(word['bbox'],
                                                       page_height))
                    word_elem.text = word['content']

    file_.write(lxml.etree.tostring(doc,
                                    doctype='<!DOCTYPE html>',
                                    encoding='utf-8',
                                    pretty_print=True))
