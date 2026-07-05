PK3 = zz-osp-pak8be.pk3

export PATH := $(realpath ./tools):$(PATH)

# JOB_SLOTS / MAKEFLAGS: set in Makefile (after Makefile.local)

ifeq ($(PLATFORM),linux)

Q3ASM_BIN = q3asm
Q3LCC_BIN = q3lcc
7Z_BIN = 7z

endif #Linux

ifdef MINGW

Q3ASM_BIN = q3asm.exe
Q3LCC_BIN = q3lcc.exe
7Z_BIN = 7za.exe

endif #MINGW

# -v off by default (large cgame link); Q3ASM_VERBOSE=1 to enable
ifdef Q3ASM_VERBOSE
Q3ASM = $(Q3ASM_BIN) -vq3 -r -m -v
else
Q3ASM = $(Q3ASM_BIN) -vq3 -r -m
endif
Q3LCC = $(Q3LCC_BIN) -DQ3_VM -S -Wf-g -I$(QADIR)
# Same deflate settings; -mmt uses all cores for the same mx=9/pass=8/mfb=255 (override: 7Z_MT=4)
ifndef 7Z_MT
7Z_MT := $(JOB_SLOTS)
endif
ifeq ($(7Z_MT),)
7Z_MT := on
endif
7Z_EXTRA ?= -bb0 -bd
7Z = $(7Z_BIN) u -tzip -mx=9 -mpass=8 -mfb=255 -mmt=$(7Z_MT) $(7Z_EXTRA) --

basedir = ../../code

QADIR = $(basedir)/game
CGDIR = $(basedir)/cgame
UIDIR = $(basedir)/q3_ui
COMDIR = $(basedir)/qcommon


QA_CFLAGS = -DQAGAME
CG_CFLAGS = -DCGAME -I$(CGDIR) -I$(CGDIR)/chatWindow -I$(CGDIR)/superhud
CG_CHATWINDOW_CFLAGS = -DCGAME -I.. -I. -IchatWindow -I../../game -I../../qcommon
CG_SUPERHUD_CFLAGS = -DCGAME -I.. -I. -Isuperhud -I../../game -I../../qcommon
CG_SUPERHUD_ELEMENT_CFLAGS = -DCGAME -I.. -I../.. -I. -Isuperhud -Isuperhud/elements -I../../../game -I../../../qcommon
UI_CFLAGS = -DQ3UI -I$(UIDIR)

include srcs.mk
