PROJECT := sbsh
CHIBIOS := ../../../..

CONFDIR  := ./cfg/sbsh-rambox-debug
BUILDDIR := ./build/sbsh-rambox-debug
DEPDIR   := ./.dep/sbsh-rambox-debug

UTILSSELECT := paths

SBAPP_CSRC := main.c parser.c glob.c execute.c
SBAPP_OPT := -O0 -ggdb -fomit-frame-pointer --specs=nano.specs

include ../common/make/elfexec.mk
include ../common/make/app-arm.mk
