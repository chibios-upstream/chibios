#!/bin/bash
rm ../../manuals/html/xhal/*
doxygen Doxyfile_html
rm ../../manuals/html/xhal/*.md5
rm ../../manuals/html/xhal/*.map
