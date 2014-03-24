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

"""djvused reader/writer.

Use load() to read a page layout from a djvused script.

Use save() to save a page layout as a djvused script.
"""

WHITESPACES = ' \t\r\n'

DEFAULT_PAGE_NUM   = 1           # Default page no. 1
DEFAULT_PAGE_IMAGE = "page.png"  # Default page image file name


def _read_token(file_):
    """Read a djvused token: an atom, a string, or a number."""
    while True:
        c = file_.read(1)
        if c not in WHITESPACES:
            break

    token = ''
    if c.isdigit():
        token += c
        while True:
            c = file_.read(1)
            if c.isdigit():
                token += c
            else:
                break
        token = int(token, 10)
    elif c != '"':
        token += c
        while True:
            c = file_.read(1)
            if c not in WHITESPACES:
                token += c
            else:
                break
    else:
        while True:
            c = file_.read(1)
            if c == '\\':
                c = file_.read(1)
                if c.isdigit():
                    token += chr(int(c + file_.read(2), 8), 10)
                elif c == 't':
                    token += '\t'
                elif c == 'r':
                    token += '\r'
                elif c == 'n':
                    token += '\n'
                else:
                    token += c
            elif c == '"':
                break
            else:
                token += c
        return token.decode('utf-8')
    return token


def _read_node(file_):
    """Read a djvused node (a list in terms of S-expressions)."""
    while True:
        c = file_.read(1)
        if c == '(':
            break

    node_type = _read_token(file_)
    node = {'bbox': (_read_token(file_),   # x1
                     _read_token(file_),   # y1
                     _read_token(file_),   # x2
                     _read_token(file_))}  # y2

    if node_type == 'word':
        node['content'] = _read_token(file_)
        while True:
            c = file_.read(1)
            if c == ')':
                return node
            elif c not in WHITESPACES:
                break
    else:
        children = []
        while True:
            children.append(_read_node(file_))
            if file_.read(1) == ')':
                break
        node['content'] = children
        return node

    raise ValueError('Malformed djvused node.')


def _escape(s):
    """Escape an arbitrary string for djvused output."""
    if isinstance(s, unicode):
        s = s.encode('utf-8')
    out = ''
    for c in s:
        if c == '"':
            out += '\\"'
        elif c == '\\':
            out += '\\\\'
        elif c == '\t':
            out += '\\t'
        elif c == '\r':
            out += '\\r'
        elif c == '\n':
            out += '\\n'
        else:
            n = ord(c)
            if n < 0x20:
                out += '\\%03o' % n
            else:
                out += c
    return out


def load(file_):
    """Load a page layout from a djvused script file."""
    page = _read_node(file_)
    page['num'] = DEFAULT_PAGE_NUM      # Default page no. 1
    page['image'] = DEFAULT_PAGE_IMAGE  # Default page image file name
    return page
        

def save(page, file_):
    """Save a page layout to a djvused script file."""
    page_bbox = page['bbox']
    file_.write('(page %d %d %d %d\n' % page_bbox)

    ncolumn = len(page['content'])
    for column in page['content']:
        file_.write('  (column %d %d %d %d\n' % column['bbox'])

        npara = len(column['content'])
        for para in column['content']:
            file_.write('    (para %d %d %d %d\n' % para['bbox'])

            nline = len(para['content'])
            for line in para['content']:
                file_.write('      (line %d %d %d %d\n' % line['bbox'])

                nword = len(line['content'])
                for word in line['content']:
                    file_.write('        (word %d %d %d %d\n' % word['bbox'])
                    file_.write('          "%s")' % _escape(word['content']))

                    nword -= 1
                    if nword > 0:
                        file_.write('\n')

                file_.write(')')

                nline -= 1
                if nline > 0:
                    file_.write('\n')

            file_.write(')')

            npara -= 1
            if npara > 0:
                file_.write('\n')

        file_.write(')')

        ncolumn -= 1
        if ncolumn > 0:
            file_.write('\n')

    file_.write(')\n')
