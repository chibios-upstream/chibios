PROJECT := cat
CHIBIOS := ../../../..

CONFDIR  := ./cfg/cat-posix-x86-debug
BUILDDIR := ./build/cat-posix-x86-debug
DEPDIR   := ./.dep/cat-posix-x86-debug

SBAPP_CSRC := main.c
SBAPP_TESTS := ./test/cat.sh

include ../common/make/cmdutil.mk
include ../common/make/app-posix-x86.mk
