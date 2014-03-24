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

"""Plain text writer.

Use save() to save a page layout to a plain text file.
"""

def load(file_):
    """Die hard: loading a page layout from a plain text file is impossible."""
    raise NotImplemented('Plain text parsing is neither available nor feaseable.')


def save(page, file_):
    """Save a page layout to a plain text file.
    
    Tries to follow the layout by separating columns, paragraphs and lines of
    text by empty strings, and by respecting line breaks inside paragraphs.
    """
    for cn, column in enumerate(page['content']):
        if cn > 0:
            file_.write('\n')
        for pn, para in enumerate(column['content']):
            if pn > 0:
                file_.write('\n')
            for line in para['content']:
                content = ' '.join(word['content'] for word in line['content'])
                if isinstance(content, unicode):
                    content = content.encode('utf-8')
                file_.write(content)
                file_.write('\n')
