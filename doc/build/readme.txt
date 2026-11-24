*** Documentation build procedure ***

The following software must be installed:
- Doxygen 1.14.0 or later.
- Graphviz. The directory containing the dot executable must be in the path
  in order to make Graphviz accessible by Doxygen.
- A LaTeX distribution providing pdflatex and makeindex, only for the PDF
  manuals.

Each directory builds one manual: rt, nil, hal, ex, xhal, vfs, sb and
full_rm, the full reference manual.

Build procedure:
- HTML manual, from the manual directory: makehtml.sh (makehtml.bat under
  Windows), the output goes into ../../manuals/html/<manual>.
- CHM manual, from the manual directory under Windows: makechm.bat.
- PDF manuals, from this directory: makepdf.sh, the output goes into
  ../manuals.
- The index of the HTML manuals is ../manuals/index.html.

Checks:
- From the repository root: python3 tools/doxygen/doxcheck.py, it reports
  groups without a parent and Doxygen warnings for all the manuals.
