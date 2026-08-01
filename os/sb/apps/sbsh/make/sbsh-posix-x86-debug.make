PROJECT := sbsh
CHIBIOS := ../../../..

CONFDIR  := ./cfg/sbsh-posix-x86-debug
BUILDDIR := ./build/sbsh-posix-x86-debug
DEPDIR   := ./.dep/sbsh-posix-x86-debug

SBAPP_CSRC := main.c parser.c glob.c execute.c
SBAPP_UDEFS := -DSBAPP_NATIVE
SBAPP_TESTS := ./test/sbsh.sh

include ../common/make/app-posix-x86.mk
