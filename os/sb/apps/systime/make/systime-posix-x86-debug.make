PROJECT := systime
CHIBIOS := ../../../..

CONFDIR  := ./cfg/systime-posix-x86-debug
BUILDDIR := ./build/systime-posix-x86-debug
DEPDIR   := ./.dep/systime-posix-x86-debug

SBAPP_CSRC := main.c
SBAPP_UDEFS := -DSBAPP_NATIVE
SBAPP_TESTS := ./test/systime.sh

include ../common/make/cmdutil.mk
include ../common/make/app-posix-x86.mk
