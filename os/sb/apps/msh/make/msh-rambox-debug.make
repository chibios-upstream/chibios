PROJECT := msh
CHIBIOS := ../../../..

CONFDIR  := ./cfg/msh-rambox-debug
BUILDDIR := ../build/sb/msh-rambox-debug
DEPDIR   := ../build/sb/.dep/msh-rambox-debug

UTILSSELECT := paths sglob

SBAPP_OPT := -O0 -ggdb -fomit-frame-pointer --specs=nano.specs

include ../common/make/elfexec.mk
include ../common/make/app-arm.mk
