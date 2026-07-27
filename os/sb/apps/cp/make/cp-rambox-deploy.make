PROJECT := cp
CHIBIOS := ../../../..

CONFDIR  := ./cfg/cp-rambox-deploy
BUILDDIR := ./build/cp-rambox-deploy
DEPDIR   := ./.dep/cp-rambox-deploy

SBAPP_CSRC := main.c
SBAPP_EXCEPTIONS_STACKSIZE := 0x400
SBAPP_UADEFS := -DCRT0_INIT_DATA=0 -DCRT0_RESERVE_HEAP=4096

include ../common/make/cmdutil.mk
include ../common/make/app-arm.mk
