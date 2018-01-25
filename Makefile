#--------------------- Basic Settings -----------------------------------------#
PROGRAM_NAME   := palloc
BINARY_BASE    := bin
BUILD_BASE     := bld
INCLUDE_BASE   := inc
SOURCE_BASE    := src

#--------------------- External Libraries -------------------------------------#
HEADER_DIRS    := \
	../libprim/inc \
	../libbits/inc \
	../libex/inc

STATIC_LIBS    := \
	../libprim/bld/libprim.a \
	../libbits/bld/libbits.a \
	../libex/bld/libex.a

#--------------------- Cpp Lint -----------------------------------------------#
LINT           := $(HOME)/.makeccpp/cpplint/cpplint.py
LINT_FLAGS     :=

#--------------------- Unit Tests ---------------------------------------------#
TEST_SUFFIX    := _TEST
GTEST_BASE     := $(HOME)/.makeccpp/gtest
GTEST_PARALLEL := $(HOME)/.makeccpp/gtest-parallel

#--------------------- Compilation and Linking --------------------------------#
CXX            := g++
AR             := gcc-ar
SRC_EXTS       := .cc
HDR_EXTS       := .h .tcc
CXX_FLAGS      := -Wall -Wextra -pedantic -Wfatal-errors -std=c++11
CXX_FLAGS      += -march=native -g -O3 -flto
LINK_FLAGS     :=

#--------------------- Auto Makefile ------------------------------------------#
include $(HOME)/.makeccpp/auto_lib.mk
