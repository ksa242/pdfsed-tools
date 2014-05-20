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

"""OCR layout utility functions."""

def recalculate_bbox(page, scale_factor=1.0, offset=None):
    """Recalculate bounding boxes' coordinates.

    Completely recalculates bounding boxes' coordinates for all lines,
    paragraphs and columns (areas) from words outwards.

    Optional scale_factor argument controls coordinates' scaling.
    """
    if scale_factor < 0.001:
        raise ValueError('Scale factor should be a positive, non-zero number.')

    if not hasattr(offset, '__iter__') or len(offset) != 2:
        offset = (0, 0)

    page_bbox = [round(v * scale_factor) for v in page['bbox']]
    page_width = page_bbox[2]
    page_height = page_bbox[3]

    for column in page['content']:
        column_bbox = [page_width, page_height, 0, 0]

        for para in column['content']:
            para_bbox = [page_width, page_height, 0, 0]

            for line in para['content']:
                line_bbox = [page_width, page_height, 0, 0]

                for word in line['content']:
                    word_bbox = [round(v * scale_factor) for v in word['bbox']]
                    word_bbox[0] += offset[0]
                    word_bbox[1] += offset[1]
                    word_bbox[2] += offset[0]
                    word_bbox[3] += offset[1]

                    word['bbox'] = tuple(word_bbox)

                    line_bbox[0] = min(line_bbox[0], word_bbox[0])
                    line_bbox[1] = min(line_bbox[1], word_bbox[1])
                    line_bbox[2] = max(line_bbox[2], word_bbox[2])
                    line_bbox[3] = max(line_bbox[3], word_bbox[3])

                line['bbox'] = tuple(line_bbox)
                para_bbox[0] = min(para_bbox[0], line_bbox[0])
                para_bbox[1] = min(para_bbox[1], line_bbox[1])
                para_bbox[2] = max(para_bbox[2], line_bbox[2])
                para_bbox[3] = max(para_bbox[3], line_bbox[3])

            para['bbox'] = tuple(para_bbox)
            column_bbox[0] = min(column_bbox[0], para_bbox[0])
            column_bbox[1] = min(column_bbox[1], para_bbox[1])
            column_bbox[2] = max(column_bbox[2], para_bbox[2])
            column_bbox[3] = max(column_bbox[3], para_bbox[3])

        column['bbox'] = tuple(column_bbox)

    page['bbox'] = tuple(page_bbox)
    return page


__all__ = ['djvused', 'hocr', 'txt']
