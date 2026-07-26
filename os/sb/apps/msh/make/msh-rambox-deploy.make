PROJECT := msh
CHIBIOS := ../../../..

CONFDIR  := ./cfg/msh-rambox-deploy
BUILDDIR := ./build/msh-rambox-deploy
DEPDIR   := ./.dep/msh-rambox-deploy

UTILSSELECT := paths sglob

include ../common/make/elfexec.mk
include ../common/make/app-arm.mk
