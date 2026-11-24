#!/bin/bash
rm ../../manuals/html/sb/*
doxygen Doxyfile_html
rm ../../manuals/html/sb/*.md5
rm ../../manuals/html/sb/*.map
