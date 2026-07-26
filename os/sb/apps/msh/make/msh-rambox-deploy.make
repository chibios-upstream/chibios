PROJECT := msh
CHIBIOS := ../../../..

CONFDIR  := ./cfg/msh-rambox-deploy
BUILDDIR := ../build/sb/msh-rambox-deploy
DEPDIR   := ../build/sb/.dep/msh-rambox-deploy

UTILSSELECT := paths sglob

include ../common/make/elfexec.mk
include ../common/make/app-arm.mk
