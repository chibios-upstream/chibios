PROJECT := wc
CHIBIOS := ../../../..

CONFDIR  := ./cfg/wc-posix-x86-debug
BUILDDIR := ./build/wc-posix-x86-debug
DEPDIR   := ./.dep/wc-posix-x86-debug

SBAPP_CSRC := main.c
SBAPP_TESTS := ./test/wc.sh

include ../common/make/cmdutil.mk
include ../common/make/app-posix-x86.mk
