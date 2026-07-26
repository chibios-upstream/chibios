PROJECT := ls
CHIBIOS := ../../../..

CONFDIR  := ./cfg/ls-rambox-deploy
BUILDDIR := ./build/ls-rambox-deploy
DEPDIR   := ./.dep/ls-rambox-deploy

SBAPP_EXCEPTIONS_STACKSIZE := 0x400
SBAPP_UADEFS := -DCRT0_INIT_DATA=0 -DCRT0_RESERVE_HEAP=4096

include ../common/make/app-arm.mk
