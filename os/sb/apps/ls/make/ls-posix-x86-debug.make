PROJECT := ls
CHIBIOS := ../../../..

CONFDIR  := ./cfg/ls-posix-x86-debug
BUILDDIR := ./build/ls-posix-x86-debug
DEPDIR   := ./.dep/ls-posix-x86-debug

SBAPP_TESTS := ./test/ls.sh

include ../common/make/app-posix-x86.mk
