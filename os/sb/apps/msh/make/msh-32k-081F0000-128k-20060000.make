PROJECT := msh
CHIBIOS := ../../../..

CONFDIR  := ./cfg/msh-32k-081F0000-128k-20060000
BUILDDIR := ./build/msh-32k-081F0000-128k-20060000
DEPDIR   := ./.dep/msh-32k-081F0000-128k-20060000

UTILSSELECT := paths sglob

SBAPP_OPT      := -Og -ggdb -fomit-frame-pointer --specs=nano.specs
SBAPP_LDOPT    := -q -Wl,-zmax-page-size=512
SBAPP_LDSCRIPT := $(CONFDIR)/sandbox.ld

include ../common/make/elfexec.mk
include ../common/make/app-arm.mk
