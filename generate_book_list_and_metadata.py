#!/usr/bin/env python3

import json
import os
import pathlib
import sys
import xml.etree.ElementTree as ElementTree

BOOK_TYPES = ['epub', 'mobi', 'txt', 'pdf', 'cbr', 'cbz']

if len(sys.argv) != 2:
    print("Outputs books.json for consumption by index.html. Usage:  ./generate_book_list_and_metadata.py ./path/to/books")
    sys.exit(1)

# get file list with correct relative path
file_list = list(map(lambda filename: os.path.join(sys.argv[1], filename), os.listdir(sys.argv[1])))

# filter out the books
book_files = list(filter(lambda filename: filename.split('.')[-1] in BOOK_TYPES, file_list))

collated_metadata = {}

for book_file in book_files:
    book_metadata = {}
    book_metadata['file'] = book_file

    # parse OPF
    expected_opf_location = pathlib.Path(book_file).with_suffix('.opf')
    if not os.path.isfile(expected_opf_location):
        print(f"Error! Couldn't fine OPF for '{book_file}' -- expected it at '{expected_opf_location}' but it wasn't there!")
        sys.exit(1)

    book_metadata['opf'] = str(expected_opf_location)

    tree = ElementTree.parse(expected_opf_location)
    root = tree.getroot()
    ns = {
        'dc':'http://purl.org/dc/elements/1.1/',
        'opf': 'http://www.idpf.org/2007/opf'
    }

    title = root.findall('.//dc:title', namespaces=ns)
    if len(title) != 1:
        print(f"Warning! Found 0 or more than one title fields for {book_file}; using 'Unknown Title'")
        book_metadata['title'] = 'Unknown Title'
    else:
        book_metadata['title'] = title[0].text


    authors = root.findall('.//dc:creator', namespaces=ns)
    collected_authors = []
    for author in authors:
        collected_authors.append(author.text)
    book_metadata['author'] = ', '.join(collected_authors)

    description = root.findall('.//dc:description', namespaces=ns)
    if len(description) != 1:
        print(f"Warning! Found 0 or more than one description fields for {book_file}; using 'No description available.'")
        book_metadata['description'] = 'No description available.'
    else:
        book_metadata['description'] = description[0].text

    tags = root.findall('.//dc:subject', namespaces=ns)
    collected_tags = []
    for tag in tags:
        collected_tags.append(tag.text)
    book_metadata['tags'] = ', '.join(collected_tags)

    cover = root.findall('.//{http://www.idpf.org/2007/opf}reference', namespaces=ns)
    if len(cover) != 1:
        print(f"Warning! Found 0 or more than one cover fields for {book_file}; keeping empty")
        book_metadata['cover'] = ''
    else:
        book_metadata['cover'] = os.path.join(sys.argv[1], cover[0].get('href'))

    collated_metadata[book_metadata['title']] = book_metadata


with open("books.json", "w") as book_data:
    # magic happens here to make it pretty-printed
    book_data.write(
        json.dumps(collated_metadata, indent=4, sort_keys=True)
    )
