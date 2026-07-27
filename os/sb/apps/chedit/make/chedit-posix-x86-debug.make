PROJECT := chedit
CHIBIOS := ../../../..

CONFDIR  := ./cfg/chedit-posix-x86-debug
BUILDDIR := ./build/chedit-posix-x86-debug
DEPDIR   := ./.dep/chedit-posix-x86-debug

SBAPP_CSRC := main.c
SBAPP_UDEFS := -DSBAPP_NATIVE
SBAPP_TESTS := ./test/chedit.sh

include ../common/make/cmdutil.mk
include ../common/make/app-posix-x86.mk
