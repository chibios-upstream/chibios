PROJECT := chedit
CHIBIOS := ../../../..

CONFDIR  := ./cfg/chedit-rambox-deploy
BUILDDIR := ./build/chedit-rambox-deploy
DEPDIR   := ./.dep/chedit-rambox-deploy

SBAPP_CSRC := main.c
SBAPP_EXCEPTIONS_STACKSIZE := 0x400
SBAPP_UADEFS := -DCRT0_INIT_DATA=0 -DCRT0_RESERVE_HEAP=32768

include ../common/make/cmdutil.mk
include ../common/make/app-arm.mk
