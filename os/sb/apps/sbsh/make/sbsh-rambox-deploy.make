PROJECT := sbsh
CHIBIOS := ../../../..

CONFDIR  := ./cfg/sbsh-rambox-deploy
BUILDDIR := ./build/sbsh-rambox-deploy
DEPDIR   := ./.dep/sbsh-rambox-deploy

UTILSSELECT := paths

SBAPP_CSRC := main.c parser.c glob.c execute.c

include ../common/make/elfexec.mk
include ../common/make/app-arm.mk
