#!/bin/bash
# Builds the PDF reference manuals into ../manuals, run from this directory.
# Requires Doxygen, Graphviz and a LaTeX distribution providing pdflatex and
# makeindex.

export CHVERSION=27.1.0

function makeclean() {
  rm *.log 2> /dev/null
}

function makepdf() {
  mkdir -p latex
  rm latex/* 2> /dev/null
  cp ../common/tex/chibios.sty ./latex
  if ! doxygen Doxyfile_pdf
  then
    echo "Doxygen failed"
    exit 1
  fi
  cd latex
  # PDF generation.
  if ! make all 1> ../chibios_${CHVERSION}_${CHNAME}_rm_pdf1.log 2> ../chibios_${CHVERSION}_${CHNAME}_rm_pdf2.log
  then
    echo "Make failed"
    exit 1
  fi
  cd ..
  cp latex/refman.pdf ../../manuals/chibios_${CHVERSION}_${CHNAME}_rm.pdf
  rm latex/* 2> /dev/null
  echo "  - PDF generated"
}

function makearchive() {
  tar -pczf ../../manuals/chibios_${CHVERSION}_${CHNAME}_rm.tar.gz ../../manuals/chibios_${CHVERSION}_${CHNAME}_rm.pdf 2> /dev/null
}

for CHNAME in rt nil hal ex xhal vfs sb
do
  export CHNAME
  echo ${CHNAME}
  cd ${CHNAME}
  makeclean
  makepdf
  makearchive
  cd ..
done
