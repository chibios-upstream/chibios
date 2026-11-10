#!/bin/bash
rm ../../manuals/html/vfs/*
doxygen Doxyfile_html
rm ../../manuals/html/vfs/*.md5
rm ../../manuals/html/vfs/*.map
